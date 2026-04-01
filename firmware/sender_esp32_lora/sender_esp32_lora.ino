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

static constexpr int VEDIRECT_RX = 34; // ESP32 RX <- VE.Direct TX (input-only pin)
static constexpr int VEDIRECT_TX = -1; // RX-only setup: BMV TX -> ESP32 RX
static constexpr int LORA_RST = 14;
static constexpr int LORA_DIO0 = 26;

static constexpr int VEDIRECT_RX = 16; // ESP32 RX <- VE.Direct TX
static constexpr int VEDIRECT_TX = 17; // not used by BMV-700, keep for UART init

// ---------------------------
// LoRa radio config
// ---------------------------
// Tuned for longer range (~1km+ line-of-sight over water with decent antennas).
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
static constexpr long LORA_FREQ_HZ = 433E6; // SX1278 typically 433 MHz
static constexpr int LORA_TX_POWER_DBM = 17;
static constexpr long LORA_BW_HZ = 125E3;
static constexpr int LORA_SPREADING_FACTOR = 10;
static constexpr int LORA_CODING_RATE = 7; // 4/7 (more robust)
static constexpr uint8_t LORA_SYNC_WORD = 0x12;
static constexpr int LORA_PREAMBLE_LEN = 12;
static constexpr long LORA_FREQ_HZ = 433E6; // SX1278 typically 433 MHz
static constexpr int LORA_TX_POWER_DBM = 14;
static constexpr long LORA_BW_HZ = 125E3;
static constexpr int LORA_SPREADING_FACTOR = 9;
static constexpr int LORA_CODING_RATE = 5; // 4/5
static constexpr uint8_t LORA_SYNC_WORD = 0x12;

// ---------------------------
// Timing / safety
// ---------------------------
static constexpr uint32_t TX_INTERVAL_MS = 1000;
static constexpr uint32_t DATA_STALE_MS = 5000;

// ---------------------------
// VE.Direct parser
// ---------------------------
struct VeFrameData {
  int32_t batteryMilliVolt = 0;  // mV
  int32_t batteryMilliAmp = 0;   // mA (+charge, -discharge)
  bool hasBatteryV = false;
  bool hasBatteryI = false;
  uint32_t lastUpdateMs = 0;
};

static VeFrameData g_veData;

static String g_line;
static bool g_inFrame = false;
static uint8_t g_checksumAccumulator = 0;

// ---------------------------
// LoRa packet format (binary)
// ---------------------------
#pragma pack(push, 1)
struct TelemetryPacket {
  uint8_t magic[2];     // 'S','B' (solarboat)
  uint8_t version;      // protocol version
  uint32_t sequence;
  uint32_t uptimeMs;
  int32_t batteryMilliVolt;
  int32_t batteryMilliAmp;
  uint8_t flags;        // bit0=dataValid, bit1=dataStale
  uint16_t crc16;
};
#pragma pack(pop)

static uint32_t g_sequence = 0;

#if HAS_OLED
static constexpr int OLED_WIDTH = 128;
static constexpr int OLED_HEIGHT = 64;
static constexpr int OLED_ADDR = 0x3C;
static constexpr int OLED_SDA = 21;
static constexpr int OLED_SCL = 22;
static Adafruit_SSD1306 g_display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
#endif

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

static bool parseVeLine(const String& line) {
  // VE.Direct frame lines are "KEY<TAB>VALUE" and frame ends with "Checksum\tX"
  const int tabPos = line.indexOf('\t');
  if (tabPos <= 0) {
    return false;
  }

  const String key = line.substring(0, tabPos);
  const String value = line.substring(tabPos + 1);

  if (key == "V") {
    g_veData.batteryMilliVolt = value.toInt();
    g_veData.hasBatteryV = true;
    return true;
  }

  if (key == "I") {
    g_veData.batteryMilliAmp = value.toInt();
    g_veData.hasBatteryI = true;
    return true;
  }

  return false;
}

static void resetVeFrameState() {
  g_inFrame = true;
  g_checksumAccumulator = 0;
}

static void processVeChar(char c) {
  // Accumulate checksum over the whole frame including checksum byte,
  // resulting sum should be 0 (mod 256).
  g_checksumAccumulator = static_cast<uint8_t>(g_checksumAccumulator + static_cast<uint8_t>(c));

  if (c == '\n') {
    // Strip CR/LF
    String line = g_line;
    line.trim();

    if (line.startsWith("PID\t")) {
      // Heuristic: a new frame often starts at PID
      resetVeFrameState();
    }

    if (line.startsWith("Checksum\t")) {
      // At this point checksum accumulator should be 0 on valid frame.
      if (g_checksumAccumulator == 0 && g_veData.hasBatteryV && g_veData.hasBatteryI) {
        g_veData.lastUpdateMs = millis();
      }
      // Start collecting next frame.
      g_veData.hasBatteryV = false;
      g_veData.hasBatteryI = false;
      g_checksumAccumulator = 0;
      g_inFrame = false;
    } else {
      parseVeLine(line);
    }

    g_line = "";
  } else {
    g_line += c;
  }
}

static void readVeDirect() {
  while (Serial2.available() > 0) {
    const char c = static_cast<char>(Serial2.read());
    processVeChar(c);
  }
}

static void sendTelemetry() {
  TelemetryPacket packet{};
  packet.magic[0] = 'S';
  packet.magic[1] = 'B';
  packet.version = 1;
  packet.sequence = g_sequence++;
  packet.uptimeMs = millis();

  const uint32_t ageMs = millis() - g_veData.lastUpdateMs;
  const bool dataValid = (g_veData.lastUpdateMs > 0);
  const bool dataStale = dataValid && (ageMs > DATA_STALE_MS);

  packet.batteryMilliVolt = g_veData.batteryMilliVolt;
  packet.batteryMilliAmp = g_veData.batteryMilliAmp;
  packet.flags = 0;
  if (dataValid) {
    packet.flags |= 0x01;
  }
  if (dataStale) {
    packet.flags |= 0x02;
  }

  packet.crc16 = crc16Ccitt(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet) - sizeof(packet.crc16));

  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
  LoRa.endPacket();

  Serial.printf(
      "TX seq=%lu V=%.3fV I=%.3fA flags=0x%02X\n",
      static_cast<unsigned long>(packet.sequence),
      packet.batteryMilliVolt / 1000.0,
      packet.batteryMilliAmp / 1000.0,
      packet.flags);

#if HAS_OLED
  g_display.clearDisplay();
  g_display.setCursor(0, 0);
  g_display.printf("SENDER seq:%lu\n", static_cast<unsigned long>(packet.sequence));
  g_display.printf("V: %.2f V\n", packet.batteryMilliVolt / 1000.0);
  g_display.printf("I: %.2f A\n", packet.batteryMilliAmp / 1000.0);
  g_display.printf("flags: 0x%02X\n", packet.flags);
  g_display.display();
#endif
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Solarboat sender booting...");

#if HAS_OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (g_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    g_display.clearDisplay();
    g_display.setTextSize(1);
    g_display.setTextColor(SSD1306_WHITE);
    g_display.setCursor(0, 0);
    g_display.println("Solarboat sender");
    g_display.display();
  }
#endif

  // RX-only VE.Direct readout (no TX back to BMV required).
  Serial2.begin(19200, SERIAL_8N1, VEDIRECT_RX, VEDIRECT_TX);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ_HZ)) {
    Serial.println("LoRa init failed. Check wiring.");
    while (true) {
      delay(1000);
    }
  }

  LoRa.setTxPower(LORA_TX_POWER_DBM);
  LoRa.setSignalBandwidth(LORA_BW_HZ);
  LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
  LoRa.setCodingRate4(LORA_CODING_RATE);
  LoRa.enableCrc();
  LoRa.setSyncWord(LORA_SYNC_WORD);
  LoRa.setPreambleLength(LORA_PREAMBLE_LEN);

  Serial.println("Sender ready.");
}

void loop() {
  static uint32_t lastTxMs = 0;

  readVeDirect();

  const uint32_t now = millis();
  if (now - lastTxMs >= TX_INTERVAL_MS) {
    lastTxMs = now;
    sendTelemetry();
  }
}
