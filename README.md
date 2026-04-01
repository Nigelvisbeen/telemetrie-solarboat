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


## Snelle start voor beginners ("ik heb dit nog nooit gedaan")

Geen stress — volg dit precies op volgorde.

### Benodigd
- 2x ESP32 dev board
- 2x LoRa SX1278 module (433 MHz) met antenne
- 1x Victron BMV-700 met VE.Direct kabel
- Jumper wires
- 1x laptop met Python 3

### Stap 1: Eerst labels plakken
Noem de hardware:
- **ESP32-A = zender (op de boot)**
- **ESP32-B = ontvanger (bij de laptop)**

Als je dat niet doet, flash je vaak per ongeluk de verkeerde firmware.

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

#### 2.2 VE.Direct (BMV-700) naar ESP32-A
| BMV VE.Direct | ESP32-A pin |
|---|---|
| TX (BMV zendt data) | GPIO16 (RX2) |
| GND | GND |

> Belangrijk: TX van apparaat 1 gaat naar RX van apparaat 2.


### VE.Direct zonder knippen (belangrijk)
Je hoeft normaal **niks door te knippen** aan de BMV-700 unit zelf.

- De BMV-700 heeft een **VE.Direct poort** (kleine 4-polige aansluiting), niet losse TX/GND schroefklemmen op de achterkant.
- Gebruik daarom een **VE.Direct kabel** of een **VE.Direct -> UART breakout kabel/adapter**.
- Vanaf die kabel/adapter pak je de signalen naar je ESP32:
  - VE.Direct **TX** -> ESP32 **RX2 (GPIO16)**
  - VE.Direct **GND** -> ESP32 **GND**

Als je een kale kabel gebruikt waarbij aders niet gelabeld zijn: zoek eerst de pinout van precies dat kabeltype op en meet desnoods na met multimeter. Niet gokken.

### Stap 3: Ontvanger aansluiten (ESP32-B + LoRa + USB)
Gebruik exact dezelfde LoRa pinmapping als hierboven op ESP32-B.
Daarna ESP32-B met USB aan laptop koppelen.

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
1. Antennes op beide LoRa modules aangesloten?
2. Hebben beide kanten dezelfde LoRa instellingen (freq/SF/BW/CR/sync)?
3. Is op zender de BMV GND echt gekoppeld aan ESP32 GND?
4. VE.Direct TX op ESP32 RX (GPIO16) aangesloten?

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
- VE.Direct UART: `RX=16, TX=-1` (RX-only)


### TinyTronics board (product_id=4345)
Als jouw gekochte board een ESP32+LoRa-combi is, gebruik dan eerst de huidige defaults.
Werkt LoRa niet direct, controleer de exacte pinout van jouw board en pas dan deze constants aan:
`LORA_SCK`, `LORA_MISO`, `LORA_MOSI`, `LORA_CS`, `LORA_RST`, `LORA_DIO0`.

### LoRa instellingen
- Freq: `433E6`
- Bandwidth: `125kHz`
- SF: `10`
- CR: `4/7`
- Sync word: `0x12`
- Preamble: `12`
- TX power (sender): `17 dBm`




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
- **C (TX)** -> ESP32 **GPIO16 (RX2)**

Niet doen:
- C (TX) op een TX-pin van de ESP zetten (dan ontvang je niets).
- B op 3V3/VIN zetten (dat is fout; B is massa).

### Minimale bedrading voor deze code (RX-only)
- BMV VE.Direct **TX** -> ESP32 **GPIO16 (RX2)**
- BMV VE.Direct **GND** -> ESP32 **GND**
- ESP32 **TX2** is niet nodig

In de sender-firmware staat daarom nu RX-only ingesteld (`VEDIRECT_TX = -1`).


## Exacte pins: BMV-700 VE.Direct naar ESP32/LoRa-board

Belangrijk: je sluit de BMV-700 **niet** direct op de LoRa-radiochip aan.
De VE.Direct data gaat naar de **ESP32 UART** op het board, en de ESP32 stuurt daarna via LoRa.

### Aansluiting VE.Direct -> ESP32 (zender-board)
Gebruik deze pins in de huidige firmware:
- VE.Direct **TX** -> ESP32 **GPIO16** (RX2)
- VE.Direct **GND** -> ESP32 **GND**

In code staat dit als:
- `VEDIRECT_RX = 16`
- `VEDIRECT_TX = -1` (RX-only)

### Belangrijk voor de VE.Direct kabel
- Gebruik een VE.Direct kabel of VE.Direct->UART breakout (niet knippen in de unit).
- Aderkleuren kunnen per kabel verschillen; volg altijd de pinout van jouw specifieke kabel/adapter.
- Meet bij twijfel eerst na met multimeter.

### Als je een geïntegreerd ESP32+LoRa board koopt
Dat is prima en vaak makkelijker. Dan blijft VE.Direct aansluiting **hetzelfde** (naar GPIO16/GND),
maar de interne LoRa-SPI pins kunnen afwijken per boardtype.
Pas in dat geval `LORA_SCK/MISO/MOSI/CS/RST/DIO0` in de firmware aan op de pinout van dat board.


## Bereikdoel 1km+ zonder netwerk

Ja, dat is precies deze opzet: **directe LoRa point-to-point** (geen LoRaWAN gateway nodig).

Voor ~1km+ op water heb je vooral nodig:
1. Goede antennes op beide kanten (zelfde band, netjes getuned).
2. Antenne zo hoog mogelijk en vrij zichtlijn (LOS).
3. Exact gelijke LoRa instellingen op zender en ontvanger.
4. Rustige opstelling: voeding stabiel, korte RF-kabels, geen losse connectors.

In deze firmware staan daarom robuustere defaults:
- 433 MHz
- BW 125 kHz
- SF10
- CR 4/7
- preamble 12
- TX 17 dBm (zender)

> Let op: toegestane frequentie/vermogen verschilt per land en event-regels. Check altijd lokale regelgeving en wedstrijdregels.


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
