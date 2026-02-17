# EG-SOS-v6 - Descrizione Firmware

## Panoramica

EG-SOS-v6 è un modulo firmware per Meshtastic che aggiunge funzionalità di **emergenza SOS** ai dispositivi Heltec Mesh Node T114. Il firmware estende Meshtastic 2.7.20 mantenendo piena compatibilità con app standard e web client.

**Versione**: EG-SOS-v6  
**Hardware**: Heltec Mesh Node T114 V2 (nRF52840 + SX1262)  
**Base**: Meshtastic firmware 2.7.20 (develop branch)  
**Licenza**: GPL-3.0 (come Meshtastic)

---

## Caratteristiche Principali

### 🆘 Invio SOS
- **Triplo-click** sul pulsante PRG
- **Pulsante esterno** su GPIO8 (long-press 4 secondi)
- Rate limit: massimo 1 SOS ogni 30 secondi
- Retry automatico fino a 3 volte se nessun ACK ricevuto
- Priorità alta sulla rete mesh

### 📡 Messaggi SOS
- **Formato**: `[SOS] id:abcd seq:001 t:14:32 lat:45.123456 lon:9.123456`
- **Canale**: configurabile (default: canale 1)
- **Encoding**: UTF-8 text message, compatibile con firmware standard
- **Posizione**: GPS fix, BLE phone, o ultima posizione nota da NVM
- **Orari**: tutti in formato locale (timezone configurato nel dispositivo)

#### Posizione Stimata
Quando viene usata l'ultima posizione conosciuta (salvata in NVM):
```
[SOS] id:abcd seq:001 t:14:32 lat:45.123 lon:9.123 Epos@12:15
```
Dove `Epos@12:15` indica che la posizione è stata rilevata alle ore 12:15 locali.

### 📲 Ricezione SOS
- **Display**: accensione automatica + schermata lampeggiante per 30 secondi
- **Buzzer**: suono continuo per 10 secondi (piezo passivo 3400Hz o attivo)
- **ACK automatico**: risposta diretta al sender
- **Forward BLE**: link Google Maps inviato allo smartphone quando connesso
- **Deduplica**: ignora messaggi SOS duplicati per sender+sequence

### 🔄 Sistema ACK
- ACK inviati come messaggio diretto al sender
- Tracking fino a 10 ACK ricevuti per ogni SOS
- Display mostra i primi 3 ID che hanno risposto
- Deduplica automatica ACK per nodo

### 💾 Persistenza NVM
- **Posizione**: salvata ogni minuto (se cambiata)
- **Messaggi SOS pending**: salvati per forward BLE successivo
- Filesystem: LittleFS/InternalFS (nRF52)

---

## Architettura Software

### Struttura File

```
src/modules/SOS/
├── SOSConfig.h          # Configurazione (UNICO FILE DA MODIFICARE)
├── SOSModule.h          # Header modulo principale
└── SOSModule.cpp        # Implementazione

src/input/
├── InputBroker.cpp      # Patch: triple-click con display
└── ButtonThread.cpp     # Patch: gestione triple-click SOS

src/modules/
└── Modules.cpp          # Patch: istanziazione SOSModule

src/graphics/draw/
└── UIRenderer.cpp       # Patch: versione EG-SOS-v6 in boot screen

variants/nrf52840/heltec_mesh_node_t114/
└── platformio.ini       # Environment heltec-mesh-node-t114-sos
```

### Classi e Moduli

**`SOSModule`** (eredita da `SinglePortModule` e `OSThread`)
- Gestisce invio/ricezione messaggi SOS
- Polling GPIO8 per pulsante esterno
- Timer display e buzzer
- Storage NVM posizione e pending

**Integrazione InputBroker**
- Fix triple-click: `setClickMs(250)` invece di default 20ms con display
- Configurazione `triplePress` anche per path con schermo

**Integrazione ButtonThread**
- Case 3 del multipress: chiama `sosModule->triggerSOS()` con priorità

---

## Protocollo Messaggi

### Messaggio SOS

**Portnum**: `TEXT_MESSAGE_APP`  
**Formato**: testo UTF-8 con prefisso `[SOS]` + bell char (`\x07`)

```
\x07[SOS] id:abcd seq:042 t:14:32 lat:45.123456 lon:9.234567
```

**Campi**:
- `id`: ultimi 4 hex del node ID (es: `abcd`)
- `seq`: numero sequenza 000-999 (per deduplica)
- `t:HH:MM`: orario invio SOS (locale)
- `lat`/`lon`: coordinate WGS84 (6 decimali)
- `Epos@HH:MM`: (opzionale) orario rilevamento posizione se stimata

### Messaggio ACK

```
[SOSACK] id:wxyz seq:042
```

**Portnum**: `TEXT_MESSAGE_APP`  
**Destinazione**: unicast al sender dell'SOS  
**Priorità**: `RELIABLE` con `want_ack=true`

---

## Configurazione

### File SOSConfig.h

Tutti i parametri sono centralizzati in `src/modules/SOS/SOSConfig.h`:

```cpp
// Timing
#define SOS_RATE_LIMIT_MS           30000   // Rate limit invio
#define SOS_ACK_TIMEOUT_MS          120000  // Timeout ACK (2 min)
#define SOS_MAX_RETRIES             3       // Retry massimi

// Canale
#define SOS_CHANNEL_INDEX           1       // Canale mesh (0-7)

// GPIO
#define SOS_EXT_BUTTON_PIN          8       // Pulsante esterno
#define SOS_EXT_BUTTON_HOLD_MS      4000    // Long-press (4s)
#define SOS_BUZZER_PASSIVE_PIN_A    7       // Piezo pin A
#define SOS_BUZZER_PASSIVE_PIN_B    44      // Piezo pin B
#define SOS_BUZZER_FREQ_HZ          3400    // Frequenza tono

// Buzzer
#define SOS_BUZZER_PASSIVE                  // o SOS_BUZZER_ACTIVE

// Display
#define SOS_DISPLAY_SENT_MS         5000    // "SOS INVIATO"
#define SOS_DISPLAY_RECEIVED_MS     30000   // Ricezione SOS
#define SOS_DISPLAY_ACK_MS          10000   // ACK ricevuti
```

### Compilazione

```bash
pio run -e heltec-mesh-node-t114-sos
```

Output: `.pio/build/heltec-mesh-node-t114-sos/firmware.hex`

Conversione in UF2 per DFU:
```bash
python bin/uf2conv.py firmware.hex -c -f 0xADA52840 -o firmware-sos.uf2
```

---

## Comportamento Display

### Con Display (T114 full)

**Boot**: mostra "EG-SOS-v6" in basso a sinistra per 4 secondi

**SOS Inviato** (5s, lampeggiante):
```
     SOS INVIATO
```

**SOS Ricevuto** (30s, lampeggiante):
```
Rx SOS da:abcd
t:14:32 pos@12:15
lat:45.123456
lon:9.234567
```

**ACK Ricevuti** (10s, lampeggiante):
```
SOS ACK da 3 nodi
da: abcd wxyz 1234
```

**Pulsante GPIO8**: countdown durante pressione
```
SOS in 3s...
SOS in 2s...
SOS in 1s...
```

### Senza Display

Funziona identicamente per invio/ricezione SOS. Il triple-click usa timing ottimizzato (`setClickMs(250)`).

---

## Posizione e Fonti Dati

### Priorità Acquisizione Posizione

1. **GPS fix** (se disponibile e valido)
2. **Phone position** via BLE (se connesso)
3. **Ultima posizione NVM** (con indicazione età)
4. **Fallback**: `lat:0.000000 lon:0.000000`

### Storage NVM

**File**: `/prefs/sos_position.bin`

```cpp
struct SOSPosition {
    double latitude;
    double longitude;
    uint32_t timestamp;     // Unix time locale
    bool valid;
    bool estimated;
    uint8_t age_minutes;    // non usato (legacy)
};
```

Aggiornamento: massimo 1 volta al minuto se posizione cambiata.

---

## Forward BLE Smartphone

Quando un nodo riceve un SOS e ha uno smartphone connesso via BLE:

**Messaggio inviato**:
```
SOS da abcd lat:45.123456 lon:9.234567
https://maps.google.com/?q=45.123456,9.234567
```

Se posizione stimata:
```
SOS da abcd lat:45.123456 lon:9.234567 pos ore 12:15
https://maps.google.com/?q=45.123456,9.234567
```

**File NVM**: `/prefs/sos_pending.bin` - salva SOS in attesa di BLE connect.

---

## Buzzer

### Piezo Passivo (default)
- **Drive differenziale**: GPIO7 e GPIO44 in push-pull opposto
- Frequenza: 3400Hz (risonanza tipica piezo 27mm)
- Durata: 10 secondi continui
- Raddoppia la tensione sul piezo → volume massimo

### Buzzer Attivo
- Singolo GPIO: HIGH per 10 secondi
- Configurazione: `#define SOS_BUZZER_ACTIVE` in `SOSConfig.h`

### Feedback Pulsante GPIO8
- Beep breve (80ms, 2000Hz) ogni secondo durante pressione
- Countdown visivo sul display

---

## Compatibilità

### ✅ Compatibile con:
- Meshtastic Android/iOS app (via BLE/USB)
- Meshtastic Web Client
- Firmware standard (messaggi visibili come TEXT_MESSAGE)
- Nodi senza modulo SOS (vedono il messaggio come testo)

### ❌ Incompatibile con:
- Versioni Meshtastic < 2.5 (API cambiate)
- Hardware diverso da nRF52840 (code path specifici)

---

## Note Tecniche

### Triple-Click Fix
Il T114 con display usa `setClickMs(20)` di default, rendendo impossibile il triple-click. La patch forza `setClickMs(250)` quando `USE_SOS_MODULE` è definito.

### Thread Timing
`runOnce()` esegue ogni 100ms per:
- Polling GPIO8
- Aggiornamento lampeggio display (500ms on/off)
- Verifica scadenza timer display
- Stop buzzer

### Deduplica
- **SOS ricevuti**: array `rx_seen[8]` con sender+seq
- **ACK ricevuti**: array `ack_tracker.node_nums[10]` con node ID

### Display Wake
Quando arriva un SOS, il display viene acceso con `screen->setOn(true)` prima di mostrare l'alert.

---

## Limitazioni Note

1. **Canale fisso per sessione**: cambio canale richiede ricompilazione
2. **Deduplica limitata**: max 8 SOS visti, max 10 ACK per SOS
3. **NVM wear**: salvataggio posizione ogni minuto (flash lifetime ~100k cicli)
4. **Timestamp solo HH:MM**: nessuna informazione sul giorno (assumiamo stesso giorno)
5. **Forward BLE single-shot**: se pending non consegnato e device riavviato, perso

---

## Debug e Log

### Monitor Seriale
```bash
pio device monitor --baud 115200
```

### Log Chiave
```
SOS GPIO8: premuto, tieni 4000ms per SOS
SOS GPIO8: tenuto 1s / 4s
SOS: long-long-press GPIO8, triggerSOS()
SOS ricevuto da abcd seq=001 lat=45.123 lon=9.234
ACK SOS inviato a 0x12345678
SOS: messaggio inviato allo smartphone via BLE
```

---

## Changelog

**v6** (2025-02-17)
- Orari assoluti locali (invece di età in minuti)
- Display wake automatico su ricezione SOS
- Triple-click fix per nodi senza display
- Buzzer drive differenziale (volume +100%)
- Feedback visivo/sonoro pulsante GPIO8
- Centralizzazione config in SOSConfig.h

---

## Crediti

Firmware sviluppato per uso personale/sperimentale basato su:
- **Meshtastic**: https://meshtastic.org
- **Hardware**: Heltec Automation
- **Sviluppo**: Assistenza Claude (Anthropic)

---

## Licenza

GPL-3.0 (eredita licenza Meshtastic)

**Uso a proprio rischio**. Questo firmware è sperimentale e non è supportato ufficialmente da Meshtastic o Heltec.
