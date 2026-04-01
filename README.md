# Solarboat Telemetrie (LILYGO T3 v1.6.1 + BMV-700)

Dit project is voor:
- **2x LILYGO T3 v1.6.1 ESP32+LoRa combi-board** (zelfde frequentievariant)
- **Victron BMV-700** via VE.Direct
- **Laptop** met Python dashboard

> Geen losse ESP32 + losse LoRa-module setup.

## Wat doet het systeem?

1. **Zender (op de boot)**
   - leest VE.Direct batterijdata (`V`, `I`)
   - verstuurt elke seconde een binair LoRa pakket met CRC16 en flags
2. **Ontvanger (aan wal / in begeleidingboot)**
   - ontvangt en valideert pakket (magic/version/CRC)
   - stuurt CSV via USB naar de laptop
3. **PC dashboard**
   - leest CSV via serial
   - toont live grafieken (spanning, stroom, RSSI)

## 1) Hardware en aansluitingen

## Zender-board (LILYGO T3) + BMV-700

### LoRa op dit board
De LoRa-radio zit al intern aangesloten op de ESP32.
Gebruikte pinmapping in de firmware:
- MOSI = GPIO27
- SCK = GPIO5
- CS = GPIO18
- DIO0 = GPIO26
- RST = GPIO23
- MISO = GPIO19

### VE.Direct naar zender
Voor deze firmware is VE.Direct **RX-only**:
- BMV VE.Direct **TX** -> ESP32 **GPIO34 (RX, input-only)**
- BMV VE.Direct **GND** -> ESP32 **GND**
- TX terug naar BMV is niet nodig

### Jouw A/B/C kabel mapping
Als jouw kabel labels heeft zoals in je afbeelding:
- **A** = VE.Direct stekker (naar BMV)
- **B** = GND -> ESP32 GND
- **C** = TX -> ESP32 GPIO34

## Ontvanger-board (LILYGO T3)
- Geen extra sensorbedrading nodig
- Sluit de ontvanger met **micro-USB** aan op laptop
  - voeding + serial data loopt via die ene kabel

## 2) Firmware-instellingen (belangrijk)

Deze instellingen moeten op zender en ontvanger overeenkomen.

### Regio/frequentie
In beide `.ino` bestanden staat:
- `LORA_REGION_EU868` (default)
- `LORA_REGION_US915`
- `LORA_REGION_433`

Laat exact **één** regio actief per build.
Voor NL/BE is meestal `EU868` juist.

### Overige LoRa defaults
- BW: 125 kHz
- SF: 10
- CR: 4/7
- Sync word: 0x12
- Preamble: 12
- Sender TX power: 17 dBm

## 3) Flashen

1. Flash `firmware/sender_esp32_lora/sender_esp32_lora.ino` op zender-board.
2. Flash `firmware/receiver_esp32_lora/receiver_esp32_lora.ino` op ontvanger-board.
3. Open serial monitor op ontvanger (`115200`) en check CSV output.

Voorbeeld regel:

```text
12345,17,12340,52.110,-3.400,1,-82,7.2
```

## 4) OLED (optioneel)

Beide firmwares kunnen data op het ingebouwde OLED tonen.

- OLED I2C pins op dit board:
  - SDA = GPIO21
  - SCL = GPIO22
- Libraries nodig voor display-output:
  - `Adafruit GFX`
  - `Adafruit SSD1306`

Als die libraries niet aanwezig zijn, blijft de firmware gewoon werken zonder OLED-output.

## 5) Dashboard op laptop

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r pc_dashboard/requirements.txt
python pc_dashboard/dashboard.py --port /dev/ttyUSB0 --baud 115200
```

Windows:

```powershell
python pc_dashboard/dashboard.py --port COM5 --baud 115200
```

## 6) Veiligheid en betrouwbaarheid

- Altijd antenne aangesloten tijdens LoRa gebruik.
- Test eerst op tafel, daarna pas op water.
- Controleer common ground (BMV GND naar zender GND).
- Gebruik stale/valid flags als kwaliteitsindicator, niet blind als absolute waarheid.
- Check lokale regelgeving / wedstrijdregels voor frequentie en zendvermogen.

## 7) Troubleshooting

- Geen data op receiver?
  - zelfde regio/frequentie op beide boards?
  - antenne op beide boards?
  - VE.Direct TX echt op GPIO34?
- Geen dashboard data?
  - juiste COM/tty poort?
  - baud 115200?
- OLED blijft zwart?
  - Adafruit libraries geïnstalleerd?

## 8) Merge conflicts (als GitHub weer moeilijk doet)

Lokaal kun je bekende conflicts automatisch oplossen met:

```bash
./scripts/resolve_pr_conflicts.sh main origin
```

Dat script pakt specifiek:
- `README.md`
- `firmware/sender_esp32_lora/sender_esp32_lora.ino`
- `firmware/receiver_esp32_lora/receiver_esp32_lora.ino`

