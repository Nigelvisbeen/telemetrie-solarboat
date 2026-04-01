#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>

#if __has_include(<Adafruit_GFX.h>) && __has_include(<Adafruit_SSD1306.h>)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define HAS_OLED 1
#else
#define HAS_OLED 0
#endif

// ---------------------------
// Hardware config (adjust to your board)
// ---------------------------
static constexpr int LORA_SCK = 5;
static constexpr int LORA_MISO = 19;
static constexpr int LORA_MOSI = 27;
static constexpr int LORA_CS = 18;
static constexpr int LORA_RST = 23; // LILYGO T3 v1.6.1
static constexpr int LORA_DIO0 = 26;

// ---------------------------
// LoRa radio config
// ---------------------------
// Must exactly match sender settings.
#define LORA_REGION_EU868 1
// #define LORA_REGION_US915 1
// #define LORA_REGION_433 1

#if defined(LORA_REGION_US915)
static constexpr long LORA_FREQ_HZ = 915E6;
#elif defined(LORA_REGION_433)
static constexpr long LORA_FREQ_HZ = 433E6;
#else
static constexpr long LORA_FREQ_HZ = 868E6; // default for NL/EU
#endif
static constexpr long LORA_BW_HZ = 125E3;
static constexpr int LORA_SPREADING_FACTOR = 10;
static constexpr int LORA_CODING_RATE = 7;
static constexpr uint8_t LORA_SYNC_WORD = 0x12;
static constexpr int LORA_PREAMBLE_LEN = 12;

#if HAS_OLED
static constexpr int OLED_WIDTH = 128;
static constexpr int OLED_HEIGHT = 64;
static constexpr int OLED_ADDR = 0x3C;
static constexpr int OLED_SDA = 21;
static constexpr int OLED_SCL = 22;
static Adafruit_SSD1306 g_display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
#endif

#pragma pack(push, 1)
struct TelemetryPacket {
  uint8_t magic[2];
  uint8_t version;
  uint32_t sequence;
  uint32_t uptimeMs;
  int32_t batteryMilliVolt;
  int32_t batteryMilliAmp;
  uint8_t flags;
  uint16_t crc16;
};
#pragma pack(pop)

static uint16_t crc16Ccitt(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t j = 0; j < 8; ++j) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Solarboat receiver booting...");
  Serial.println("CSV header: pc_time_ms,seq,uptime_ms,battery_v,battery_i,flags,rssi,snr");

#if HAS_OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (g_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    g_display.clearDisplay();
    g_display.setTextSize(1);
    g_display.setTextColor(SSD1306_WHITE);
    g_display.setCursor(0, 0);
    g_display.println("Solarboat receiver");
    g_display.display();
  }
#endif

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ_HZ)) {
    Serial.println("LoRa init failed. Check wiring.");
    while (true) {
      delay(1000);
    }
  }

  LoRa.setSignalBandwidth(LORA_BW_HZ);
  LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
  LoRa.setCodingRate4(LORA_CODING_RATE);
  LoRa.enableCrc();
  LoRa.setSyncWord(LORA_SYNC_WORD);
  LoRa.setPreambleLength(LORA_PREAMBLE_LEN);

  Serial.println("Receiver ready.");
}

void loop() {
  const int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  if (packetSize != static_cast<int>(sizeof(TelemetryPacket))) {
    while (LoRa.available()) {
      LoRa.read();
    }
    return;
  }

  TelemetryPacket packet{};
  uint8_t* raw = reinterpret_cast<uint8_t*>(&packet);
  size_t idx = 0;
  while (LoRa.available() && idx < sizeof(packet)) {
    raw[idx++] = static_cast<uint8_t>(LoRa.read());
  }

  if (idx != sizeof(packet)) {
    return;
  }

  if (packet.magic[0] != 'S' || packet.magic[1] != 'B' || packet.version != 1) {
    return;
  }

  const uint16_t expectedCrc = crc16Ccitt(raw, sizeof(packet) - sizeof(packet.crc16));
  if (expectedCrc != packet.crc16) {
    return;
  }

  const unsigned long pcTimeMs = millis();
  const float batteryV = packet.batteryMilliVolt / 1000.0f;
  const float batteryI = packet.batteryMilliAmp / 1000.0f;
  const int rssi = LoRa.packetRssi();
  const float snr = LoRa.packetSnr();

  Serial.printf(
      "%lu,%lu,%lu,%.3f,%.3f,%u,%d,%.1f\n",
      pcTimeMs,
      static_cast<unsigned long>(packet.sequence),
      static_cast<unsigned long>(packet.uptimeMs),
      batteryV,
      batteryI,
      static_cast<unsigned>(packet.flags),
      rssi,
      snr);

#if HAS_OLED
  g_display.clearDisplay();
  g_display.setCursor(0, 0);
  g_display.printf("RECV seq:%lu\n", static_cast<unsigned long>(packet.sequence));
  g_display.printf("V: %.2f V\n", batteryV);
  g_display.printf("I: %.2f A\n", batteryI);
  g_display.printf("RSSI:%d SNR:%.1f\n", rssi, snr);
  g_display.display();
#endif
}
