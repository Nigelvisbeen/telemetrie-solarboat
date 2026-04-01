<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
# Solarboat Telemetrie (ESP32+LoRa combi-board + BMV-700)
=======
# Solarboat Telemetrie (ESP32 + LoRa SX1278 + BMV-700)
>>>>>>> main

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


## Snelle start voor beginners ("ik heb dit nog nooit gedaan")

<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
### Hardware scope (belangrijk)
Dit project is nu **alleen** bedoeld voor:
- 2x **LILYGO T3 v1.6.1 ESP32+LoRa combi-board**
- **geen** losse ESP32 + losse LoRa-module setup

Geen stress — volg dit precies op volgorde.

### Benodigd
- 2x LILYGO T3 v1.6.1 ESP32+LoRa combi-board (868/915 variant) met antenne
=======
Geen stress — volg dit precies op volgorde.

### Benodigd
- 2x ESP32 dev board
- 2x LoRa SX1278 module (433 MHz) met antenne
>>>>>>> main
- 1x Victron BMV-700 met VE.Direct kabel
- Jumper wires
- 1x laptop met Python 3

### Stap 1: Eerst labels plakken
Noem de hardware:
- **ESP32-A = zender (op de boot)**
- **ESP32-B = ontvanger (bij de laptop)**

Als je dat niet doet, flash je vaak per ongeluk de verkeerde firmware.

<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
### Stap 2: Zender aansluiten (ESP32+LoRa-A + BMV)
#### 2.1 Interne LoRa op het combi-board
Bij de LILYGO T3 v1.6.1 hoef je **geen losse LoRa bedrading** te doen.
De LoRa-radio zit al intern aan de ESP32 gekoppeld (MOSI=27, SCK=5, CS=18, DIO=26, RST=23, MISO=19).
(Volgens TTGO-LoRa-Series tabel: v1.6 gebruikt RST=23, oudere v1.0 gebruikte RST=14.)
=======
### Stap 2: Zender aansluiten (ESP32-A + LoRa + BMV)
#### 2.1 LoRa naar ESP32-A
| LoRa SX1278 pin | ESP32-A pin |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SCK | GPIO5 |
| MISO | GPIO19 |
| MOSI | GPIO27 |
| NSS / CS | GPIO18 |
| RST | GPIO14 |
| DIO0 | GPIO26 |
>>>>>>> main

#### 2.2 VE.Direct (BMV-700) naar ESP32-A
| BMV VE.Direct | ESP32-A pin |
|---|---|
<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
| TX (BMV zendt data) | GPIO34 (RX-only) |
=======
| TX (BMV zendt data) | GPIO16 (RX2) |
>>>>>>> main
| GND | GND |

> Belangrijk: TX van apparaat 1 gaat naar RX van apparaat 2.


### VE.Direct zonder knippen (belangrijk)
Je hoeft normaal **niks door te knippen** aan de BMV-700 unit zelf.

- De BMV-700 heeft een **VE.Direct poort** (kleine 4-polige aansluiting), niet losse TX/GND schroefklemmen op de achterkant.
- Gebruik daarom een **VE.Direct kabel** of een **VE.Direct -> UART breakout kabel/adapter**.
- Vanaf die kabel/adapter pak je de signalen naar je ESP32:
<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
  - VE.Direct **TX** -> ESP32 **RX2 (GPIO34)**
=======
  - VE.Direct **TX** -> ESP32 **RX2 (GPIO16)**
>>>>>>> main
  - VE.Direct **GND** -> ESP32 **GND**

Als je een kale kabel gebruikt waarbij aders niet gelabeld zijn: zoek eerst de pinout van precies dat kabeltype op en meet desnoods na met multimeter. Niet gokken.

<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
### Stap 3: Ontvanger aansluiten (ESP32+LoRa-B + USB)
Ook hier gebruik je een tweede ESP32+LoRa combi-board (zelfde type).
Daarna ESP32+LoRa-B met **micro-USB** aan laptop koppelen (voeding + seriële data).
=======
### Stap 3: Ontvanger aansluiten (ESP32-B + LoRa + USB)
Gebruik exact dezelfde LoRa pinmapping als hierboven op ESP32-B.
Daarna ESP32-B met USB aan laptop koppelen.
>>>>>>> main

### Stap 4: Firmware uploaden
1. Open `firmware/sender_esp32_lora/sender_esp32_lora.ino` en upload naar **ESP32-A**.
2. Open `firmware/receiver_esp32_lora/receiver_esp32_lora.ino` en upload naar **ESP32-B**.

### Stap 5: Controleren of ontvanger data ziet
Open serial monitor van ESP32-B op `115200 baud`.
Als alles werkt zie je regels zoals:

```
12345,17,12340,52.110,-3.400,1,-82,7.2
```

Zie je niks? Check in deze volgorde:
<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
1. Antennes op beide ESP32+LoRa combi-boards aangesloten?
2. Hebben beide kanten dezelfde LoRa instellingen (freq/SF/BW/CR/sync)?
3. Is op zender de BMV GND echt gekoppeld aan ESP32 GND?
4. VE.Direct TX op ESP32 RX (GPIO34) aangesloten?
=======
1. Antennes op beide LoRa modules aangesloten?
2. Hebben beide kanten dezelfde LoRa instellingen (freq/SF/BW/CR/sync)?
3. Is op zender de BMV GND echt gekoppeld aan ESP32 GND?
4. VE.Direct TX op ESP32 RX (GPIO16) aangesloten?
>>>>>>> main

### Stap 6: Dashboard starten op laptop
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

### Stap 7: Veilig testen
- Test eerst op tafel (binnen), daarna pas op het water.
- Bevestig kabels mechanisch (trillingen op boot).
- Bij `flags` met stale-data: data niet blind vertrouwen.


## Veiligheid (boot-omgeving)

Voor gebruik op een solarboat:
1. **Voeding en massa**
<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
   - Gebruik stabiele 3.3V voeding voor elk ESP32+LoRa combi-board.
   - Zorg voor correcte common ground tussen BMV VE.Direct GND en de ESP32 GND van de zender.
=======
   - Gebruik stabiele 3.3V voeding voor ESP32 en LoRa module.
   - Zorg voor correcte common ground tussen ESP32 en LoRa.
>>>>>>> main
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
<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
   - Mechanisch fixeren van boards/connectoren.
=======
   - Mechanisch fixeren van modules/connectoren.
>>>>>>> main
   - Gebruik afgeschermde/korte kabels waar mogelijk.

> Dit is een basisimplementatie voor telemetrie en **geen** gecertificeerde maritieme veiligheidscontroller.

<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
## 1) Zender: ESP32+LoRa combi-board + BMV-700
=======
## 1) Zender: ESP32 + SX1278 + BMV-700
>>>>>>> main

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
<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
- LoRa: `RST=23, DIO0=26` (LILYGO T3 v1.6.1)
- VE.Direct UART: `RX=34, TX=-1` (RX-only)


### Boardtype voor dit project
Dit project gaat uit van **2x ESP32+LoRa combi-board** (zelfde type/frequentie).
De LoRa-radio is intern bedraad; je sluit dus geen losse LoRa-module meer aan.

Als jouw board een afwijkende pinout heeft, pas dan deze constants in de firmware aan:
`LORA_SCK`, `LORA_MISO`, `LORA_MOSI`, `LORA_CS`, `LORA_RST`, `LORA_DIO0`.

### LoRa instellingen
- Freq: `868E6`

> Frequentie kiezen: zet in beide firmwarebestanden exact één regio-macro aan:
> - `LORA_REGION_EU868` (standaard, Nederland/Europa)
> - `LORA_REGION_US915`
> - `LORA_REGION_433`


=======
- LoRa: `RST=14, DIO0=26`
- VE.Direct UART: `RX=16, TX=17`

### LoRa instellingen
- Freq: `433E6`
>>>>>>> main
- Bandwidth: `125kHz`
- SF: `10`
- CR: `4/7`
- Sync word: `0x12`
- Preamble: `12`
- TX power (sender): `17 dBm`



<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh

## Let op: de gekochte VE.Direct TX Digital Output kabel

De **VE.Direct TX Digital Output Cable (ASS030550500)** is bedoeld voor digitale uitgangstoepassingen (bijv. MPPT load/relay/PWM),
niet als algemene VE.Direct UART datakabel voor BMV-telemetrie.

Voor deze firmware heb je nodig:
- een VE.Direct kabel/breakout waarmee je de **UART data TX + GND** van de BMV kunt uitlezen, of
- een VE.Direct -> USB interface (voor PC), of
- een VE.Direct -> TTL/UART breakout naar ESP32.


### Mapping van jouw kabel (A/B/C)
Volgens jouw schema:
- **A** = VE.Direct plug (gaat in de BMV-700)
- **B** = GND (aarding)
- **C** = TX (data-uitgang)

Voor de ESP32-zender sluit je dan aan:
- **B (GND)** -> ESP32 **GND**
- **C (TX)** -> ESP32 **GPIO34 (RX)**

Niet doen:
- C (TX) op een TX-pin van de ESP zetten (dan ontvang je niets).
- B op 3V3/VIN zetten (dat is fout; B is massa).

### Minimale bedrading voor deze code (RX-only)
- BMV VE.Direct **TX** -> ESP32 **GPIO34 (RX)**
- BMV VE.Direct **GND** -> ESP32 **GND**
- ESP32 **TX2** is niet nodig

In de sender-firmware staat daarom nu RX-only ingesteld (`VEDIRECT_TX = -1`).


## Exacte pins: BMV-700 VE.Direct naar ESP32+LoRa combi-board
=======
## Exacte pins: BMV-700 VE.Direct naar ESP32/LoRa-board
>>>>>>> main

Belangrijk: je sluit de BMV-700 **niet** direct op de LoRa-radiochip aan.
De VE.Direct data gaat naar de **ESP32 UART** op het board, en de ESP32 stuurt daarna via LoRa.

### Aansluiting VE.Direct -> ESP32 (zender-board)
Gebruik deze pins in de huidige firmware:
<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
- VE.Direct **TX** -> ESP32 **GPIO34** (RX)
- VE.Direct **GND** -> ESP32 **GND**

In code staat dit als:
- `VEDIRECT_RX = 34`
- `VEDIRECT_TX = -1` (RX-only)
=======
- VE.Direct **TX** -> ESP32 **GPIO16** (RX2)
- VE.Direct **GND** -> ESP32 **GND**

Optioneel (meestal niet nodig voor alleen uitlezen):
- ESP32 **GPIO17** (TX2) -> VE.Direct RX

In code staat dit als:
- `VEDIRECT_RX = 16`
- `VEDIRECT_TX = 17`
>>>>>>> main

### Belangrijk voor de VE.Direct kabel
- Gebruik een VE.Direct kabel of VE.Direct->UART breakout (niet knippen in de unit).
- Aderkleuren kunnen per kabel verschillen; volg altijd de pinout van jouw specifieke kabel/adapter.
- Meet bij twijfel eerst na met multimeter.

<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
=======
### Als je een geïntegreerd ESP32+LoRa board koopt
Dat is prima en vaak makkelijker. Dan blijft VE.Direct aansluiting **hetzelfde** (naar GPIO16/GND),
maar de interne LoRa-SPI pins kunnen afwijken per boardtype.
Pas in dat geval `LORA_SCK/MISO/MOSI/CS/RST/DIO0` in de firmware aan op de pinout van dat board.

>>>>>>> main

## Bereikdoel 1km+ zonder netwerk

Ja, dat is precies deze opzet: **directe LoRa point-to-point** (geen LoRaWAN gateway nodig).

Voor ~1km+ op water heb je vooral nodig:
1. Goede antennes op beide kanten (zelfde band, netjes getuned).
2. Antenne zo hoog mogelijk en vrij zichtlijn (LOS).
3. Exact gelijke LoRa instellingen op zender en ontvanger.
4. Rustige opstelling: voeding stabiel, korte RF-kabels, geen losse connectors.

In deze firmware staan daarom robuustere defaults:
<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
- 868 MHz
=======
- 433 MHz
>>>>>>> main
- BW 125 kHz
- SF10
- CR 4/7
- preamble 12
- TX 17 dBm (zender)

> Let op: toegestane frequentie/vermogen verschilt per land en event-regels. Check altijd lokale regelgeving en wedstrijdregels.


<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh
## 2) Ontvanger: ESP32+LoRa combi-board naar USB
=======
## 2) Ontvanger: ESP32 + SX1278 naar USB
>>>>>>> main

De ontvanger print CSV regels op serial (115200):

`pc_time_ms,seq,uptime_ms,battery_v,battery_i,flags,rssi,snr`

Gebruik op PC een serial monitor of direct het Python dashboard.

<<<<<<< codex/create-firmware-for-esp32-and-lora-module-tpjwuh

### OLED scherm op het board
Ja, de firmware kan nu ook waarden op het ingebouwde OLED tonen (indien de libraries aanwezig zijn):
- Sender: seq, V, I, flags
- Receiver: seq, V, I, RSSI/SNR

OLED I2C pins op dit board:
- SDA = GPIO21
- SCL = GPIO22

Voor Arduino IDE: installeer `Adafruit GFX` en `Adafruit SSD1306` als je display-output wilt.
Zonder die libraries blijft de firmware werken, alleen zonder OLED-output.


=======
>>>>>>> main
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


## PR merge-conflict snel oplossen

Als je PR op GitHub conflict geeft in `README.md` en/of de firmware files, kun je lokaal deze helper gebruiken:

```bash
./scripts/resolve_pr_conflicts.sh main origin
```

Wat dit script doet:
1. Merge `origin/main` in je huidige branch.
2. Bij conflict in bekende bestanden (`README.md`, sender/receiver `.ino`) kiest het de huidige branch-versie.
3. Als er nog andere conflicten overblijven, stopt het script zodat je die handmatig afmaakt.


## Aanbevolen teststappen
1. Test zender met alleen VE.Direct aangesloten (controleer serial logs).
2. Test LoRa link op korte afstand (binnen) met twee ESP32's.
3. Test op water met logging op PC; controleer packet loss en stale flags.
4. Bouw alarmregels in je race-UI voor onder-/overspanning en overstromen.
