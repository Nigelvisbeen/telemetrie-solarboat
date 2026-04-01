# Solarboat Telemetriehandleiding (LILYGO T3 + BMV-700)

Deze handleiding is **vanaf nul** opgebouwd voor dit project.
Doel: je moet zowel begrijpen **hoe het systeem werkt** als het direct kunnen **aansluiten en gebruiken**.

---

## 1. Doel van het systeem

Je meet met een Victron BMV-700 de accuwaarden en stuurt die draadloos naar je laptop.

Het systeem bestaat uit 3 delen:

1. **Zender op de boot (LILYGO T3 #1)**
   - leest VE.Direct data van de BMV-700
   - verstuurt elke seconde LoRa-pakketten
2. **Ontvanger aan wal (LILYGO T3 #2)**
   - ontvangt LoRa-pakketten
   - zet ze door als CSV via USB naar de laptop
3. **PC dashboard (Python)**
   - leest de CSV via serial
   - toont live grafieken (spanning, stroom, RSSI)

---

## 2. Wat heb je nodig?

- 2x **LILYGO T3 v1.6.1 ESP32+LoRa** (zelfde frequentievariant)
- 2x antenne (altijd aangesloten tijdens LoRa gebruik)
- 1x Victron BMV-700 met VE.Direct kabel (TX + GND beschikbaar)
- 1x laptop met Python 3
- 2x USB kabel (ontvanger sowieso, zender voor flashen)
- Jumper wires

> Dit project is voor **combi-boards** (ESP32 + LoRa op 1 print), niet voor losse ESP32 + losse LoRa module.

---

## 3. Hoe de data loopt (conceptueel)

BMV-700 -> VE.Direct TX/GND -> **Zender ESP32** -> LoRa -> **Ontvanger ESP32** -> USB serial -> **Dashboard**

Belangrijk:
- de BMV gaat naar de **ESP32 UART** op de zender
- niet direct naar de LoRa-chip

---

## 4. Aansluiten

## 4.1 Zender (LILYGO T3 #1 op de boot)

### VE.Direct naar zender (RX-only)
| BMV VE.Direct | Zender ESP32 pin |
|---|---|
| TX (data uit BMV) | GPIO34 (RX, input-only) |
| GND | GND |

<img width="1600" height="900" alt="aansluiten lora transmitter" src="https://github.com/user-attachments/assets/01414516-20f4-48e5-a9bd-13734b91a84b" />



## 4.2 Ontvanger (LILYGO T3 #2 aan wal)

- Geen sensorbedrading nodig
- Sluit de ontvanger met **micro-USB** aan op laptop
- Via die micro-USB loopt voeding + serial data

---

## 5. Firmware configuratie (voor je flasht)

In beide firmwarebestanden (`sender` en `receiver`) kies je exact één LoRa-regio:

- `LORA_REGION_EU868` (meestal NL/BE)
- `LORA_REGION_US915`
- `LORA_REGION_433`

Zender en ontvanger moeten **exact dezelfde** regio en LoRa-instellingen hebben.

Standaard projectinstellingen:
- BW 125 kHz
- SF10
- CR 4/7
- SyncWord 0x12
- Preamble 12
- TX power zender 17 dBm

---

## 6. Flashen

1. Open `firmware/sender_esp32_lora/sender_esp32_lora.ino`
2. Selecteer juiste board/poort en flash naar **zender-board**
3. Open `firmware/receiver_esp32_lora/receiver_esp32_lora.ino`
4. Selecteer juiste board/poort en flash naar **ontvanger-board**

---

## 7. Eerste opstarttest

1. Zet beide boards aan (met antennes)
2. Open serial monitor op ontvanger @ `115200`
3. Je moet CSV-regels zien, bijvoorbeeld:

```text
12345,17,12340,52.110,-3.400,1,-82,7.2
```

Als je dit ziet, werkt de LoRa-link + parser.

---

## 8. Dashboard starten op laptop

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

---

## 9. OLED op het board (optioneel)

Beide firmwares kunnen status tonen op OLED (als libs aanwezig zijn):
- Adafruit GFX
- Adafruit SSD1306

OLED I2C op dit board:
- SDA=21
- SCL=22

Zonder deze libraries werkt de firmware nog steeds (alleen zonder OLED-output).

---

## 10. Troubleshooting

## Geen data op ontvanger
- antenne op beide boards?
- zender + ontvanger zelfde regio/frequentie?
- `LoRa.begin(...)` fout in serial?

## Geen VE.Direct data
- BMV TX echt op GPIO34?
- GND goed verbonden?
- kabel pinout geverifieerd?

## Dashboard blijft leeg
- juiste COM/tty poort?
- baud op 115200?
- draait receiver daadwerkelijk en print hij CSV?

---

## 11. Veilig in gebruik nemen op boot

- eerst op tafel testen, daarna pas op water
- kabels mechanisch fixeren (trillingen)
- stale/valid flags als kwaliteitscheck gebruiken
- lokale regelgeving/wedstrijdregels voor RF-band en vermogen volgen

---

## 12. GitHub merge conflicts (optioneel)

Als je lokaal werkt en PR-conflicts hebt in de bekende files:

```bash
./scripts/resolve_pr_conflicts.sh main origin
```

