# Solarboat Telemetrie (ESP32 + LoRa SX1278 + BMV-700)

Dit project bevat:
- **Zender firmware** (`firmware/sender_esp32_lora/sender_esp32_lora.ino`)
  - leest VE.Direct data van een **Victron BMV-700**
  - pakt batterijspanning/stroom in een binair pakket met CRC16
  - verstuurt elke seconde via LoRa
- **Ontvanger firmware** (`firmware/receiver_esp32_lora/receiver_esp32_lora.ino`)
  - ontvangt LoRa pakketten
  - valideert protocol + CRC
  - stuurt CSV via USB serial naar de PC
- **PC dashboard** (`pc_dashboard/dashboard.py`)
  - leest USB serial CSV
  - toont live grafieken (spanning, stroom, RSSI)

## Veiligheid (boot-omgeving)

Voor gebruik op een solarboat:
1. **Voeding en massa**
   - Gebruik stabiele 3.3V voeding voor ESP32 en LoRa module.
   - Zorg voor correcte common ground tussen ESP32 en LoRa.
2. **Galvanische aandacht**
   - VE.Direct is TTL-level; check levels en pinout van jouw BMV-kabel.
   - Vermijd ground loops; houd kabels kort en robuust.
3. **Radio betrouwbaarheid**
   - Gebruik vaste LoRa instellingen op zender/ontvanger (freq/SF/BW/CR/sync).
   - Test bereik op water, inclusief worst-case bochten/helling.
4. **Fail-safe gedrag**
   - Pakket bevat `flags` voor valid/stale data.
   - Dashboard/hostsoftware moet stale data als waarschuwing tonen en niet als realtime waarheid gebruiken.
5. **EMC en trillingen**
   - Mechanisch fixeren van modules/connectoren.
   - Gebruik afgeschermde/korte kabels waar mogelijk.

> Dit is een basisimplementatie voor telemetrie en **geen** gecertificeerde maritieme veiligheidscontroller.

## 1) Zender: ESP32 + SX1278 + BMV-700

### VE.Direct
- BMV-700 VE.Direct baudrate: **19200 8N1**.
- Uitgelezen velden:
  - `V` (mV)
  - `I` (mA)

### LoRa pakket
Binary struct:
- magic (`SB`)
- version
- sequence
- uptimeMs
- batteryMilliVolt
- batteryMilliAmp
- flags
  - bit0: data valid
  - bit1: data stale
- crc16 (CCITT)

### Standaard pins (pas aan op jouw board)
In de `.ino` bestanden staan defaults:
- SPI: `SCK=5, MISO=19, MOSI=27, CS=18`
- LoRa: `RST=14, DIO0=26`
- VE.Direct UART: `RX=16, TX=17`

### LoRa instellingen
- Freq: `433E6`
- Bandwidth: `125kHz`
- SF: `9`
- CR: `4/5`
- Sync word: `0x12`

## 2) Ontvanger: ESP32 + SX1278 naar USB

De ontvanger print CSV regels op serial (115200):

`pc_time_ms,seq,uptime_ms,battery_v,battery_i,flags,rssi,snr`

Gebruik op PC een serial monitor of direct het Python dashboard.

## 3) Python dashboard (PC)

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r pc_dashboard/requirements.txt
python pc_dashboard/dashboard.py --port /dev/ttyUSB0 --baud 115200
```

Op Windows bijvoorbeeld:
```powershell
python pc_dashboard/dashboard.py --port COM5 --baud 115200
```

## Aanbevolen teststappen
1. Test zender met alleen VE.Direct aangesloten (controleer serial logs).
2. Test LoRa link op korte afstand (binnen) met twee ESP32's.
3. Test op water met logging op PC; controleer packet loss en stale flags.
4. Bouw alarmregels in je race-UI voor onder-/overspanning en overstromen.
