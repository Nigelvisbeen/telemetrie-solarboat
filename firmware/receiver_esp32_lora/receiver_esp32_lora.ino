#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// ---------------------------
// Hardware config (adjust to your board)
// ---------------------------
static constexpr int LORA_SCK = 5;
static constexpr int LORA_MISO = 19;
static constexpr int LORA_MOSI = 27;
static constexpr int LORA_CS = 18;
static constexpr int LORA_RST = 14;
static constexpr int LORA_DIO0 = 26;

// ---------------------------
// LoRa radio config
// ---------------------------
static constexpr long LORA_FREQ_HZ = 433E6;
static constexpr long LORA_BW_HZ = 125E3;
static constexpr int LORA_SPREADING_FACTOR = 9;
static constexpr int LORA_CODING_RATE = 5;
static constexpr uint8_t LORA_SYNC_WORD = 0x12;

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
}
