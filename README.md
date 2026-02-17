# EG-SOS-v6 - Modulo SOS per Meshtastic

![Version](https://img.shields.io/badge/version-v6-blue)
![Meshtastic](https://img.shields.io/badge/Meshtastic-2.7.20-green)
![Hardware](https://img.shields.io/badge/Hardware-Heltec_T114-orange)
![License](https://img.shields.io/badge/license-GPL--3.0-red)

Modulo firmware per **Meshtastic** che aggiunge funzionalità di **emergenza SOS** ai dispositivi Heltec Mesh Node T114.

## 🚨 Funzionalità Principali

- **📍 Invio SOS con posizione GPS** tramite triple-click o pulsante esterno
- **📡 Ricezione automatica** con buzzer e display lampeggiante
- **🔄 Sistema ACK** per conferma ricezione
- **📲 Forward Bluetooth** con link Google Maps allo smartphone
- **💾 Persistenza NVM** per posizione e messaggi pending
- **🔁 Retry automatico** fino a 3 volte se nessuna risposta

## 📥 Download

Scarica l'ultimo pacchetto dalla sezione [**Releases**](https://github.com/EnricoGuido/Meshtastic-SOS/releases)

## 🎯 Hardware Supportato

- **Heltec Mesh Node T114 V2** (nRF52840 + SX1262)
- Display TFT 1.14" 240x135 (ST7789)
- GPS L76K esterno (opzionale)
- Buzzer piezo passivo o attivo

## 📦 Contenuto Pacchetto

```
meshtastic-EG-SOS-v6.zip
├── src/
│   ├── modules/SOS/           # Modulo SOS principale
│   ├── input/                 # Patch InputBroker e ButtonThread
│   ├── modules/Modules.cpp    # Patch integrazione
│   └── graphics/draw/         # Patch UIRenderer
├── variants/nrf52840/heltec_mesh_node_t114/
│   └── platformio.ini         # Environment compilazione
├── DESCRIZIONE_FIRMWARE.md    # Documentazione tecnica
├── MANUALE_USO.md             # Guida utente
├── INSTALL.ps1                # Script installazione Windows
└── README.md                  # Questo file
```

## 🚀 Installazione Rapida

### Windows (PowerShell)

```powershell
# 1. Estrai il pacchetto
Expand-Archive meshtastic-EG-SOS-v6.zip -DestinationPath C:\temp

# 2. Esegui lo script di installazione
cd C:\temp
.\INSTALL.ps1

# 3. Compila
cd C:\dev\firmware-develop
pio run -e heltec-mesh-node-t114-sos
```

### Linux/macOS

```bash
# 1. Estrai
unzip meshtastic-EG-SOS-v6.zip -d /tmp/sos-package

# 2. Copia i file
cp -r /tmp/sos-package/src/* ~/meshtastic-firmware/src/
cp /tmp/sos-package/variants/nrf52840/heltec_mesh_node_t114/platformio.ini \
   ~/meshtastic-firmware/variants/nrf52840/heltec_mesh_node_t114/

# 3. Compila
cd ~/meshtastic-firmware
pio run -e heltec-mesh-node-t114-sos
```

## 📖 Documentazione

- **[MANUALE_USO.md](MANUALE_USO.md)** - Guida completa per utenti
- **[DESCRIZIONE_FIRMWARE.md](DESCRIZIONE_FIRMWARE.md)** - Documentazione tecnica sviluppatori

## ⚙️ Configurazione

Tutti i parametri sono in un unico file: **`src/modules/SOS/SOSConfig.h`**

```cpp
// Esempi parametri configurabili
#define SOS_CHANNEL_INDEX        1      // Canale mesh (0-7)
#define SOS_RATE_LIMIT_MS        30000  // Rate limit (30s)
#define SOS_EXT_BUTTON_PIN       8      // GPIO pulsante esterno
#define SOS_EXT_BUTTON_HOLD_MS   4000   // Long-press (4s)
#define SOS_BUZZER_FREQ_HZ       3400   // Frequenza buzzer
```

Dopo ogni modifica: ricompila il firmware.

## 🆘 Invio SOS

### Metodo 1: Triple-Click Pulsante PRG
Premi rapidamente 3 volte il pulsante PRG sul dispositivo.

### Metodo 2: Pulsante Esterno GPIO8
Collega un pulsante tra GPIO8 e GND, tieni premuto per 4 secondi.

Durante la pressione vedrai:
```
SOS in 3s...
SOS in 2s...
SOS in 1s...
```

## 📡 Formato Messaggio

```
[SOS] id:abcd seq:001 t:14:32 lat:45.123456 lon:9.234567
```

Se posizione stimata:
```
[SOS] id:abcd seq:001 t:14:32 lat:45.123456 lon:9.234567 Epos@12:15
```

## 🔧 Requisiti Sviluppo

- **PlatformIO** (VSCode extension o CLI)
- **Firmware Meshtastic 2.7.20** (develop branch)
- **Python 3.x** (per uf2conv.py)

## 🏗️ Build dal Sorgente

```bash
# 1. Clone Meshtastic firmware
git clone https://github.com/meshtastic/firmware.git
cd firmware
git checkout 2.7.20

# 2. Applica il modulo SOS
unzip meshtastic-EG-SOS-v6.zip
cp -r sos-package/src/* src/
cp sos-package/variants/nrf52840/heltec_mesh_node_t114/platformio.ini \
   variants/nrf52840/heltec_mesh_node_t114/

# 3. Compila
pio run -e heltec-mesh-node-t114-sos

# 4. Output
# .pio/build/heltec-mesh-node-t114-sos/firmware.hex
```

## 📸 Screenshots

### SOS Inviato
```
┌───────────────┐
│               │
│ SOS INVIATO   │ (lampeggiante)
│               │
└───────────────┘
```

### SOS Ricevuto
```
┌───────────────┐
│Rx SOS da:abcd │
│t:14:32 pos@12:│ (lampeggiante 30s)
│lat:45.123456  │
│lon:9.234567   │
└───────────────┘
```

## ⚠️ Avvertenze

- ⚠️ **NON sostituisce i servizi di emergenza ufficiali** (112, 118)
- 📶 Funziona solo se altri nodi mesh sono nel raggio LoRa
- 🔋 Richiede batteria carica
- 📍 Posizione accurata solo con GPS fix recente

## 🐛 Risoluzione Problemi

### Triple-click non funziona
- Premi circa 1 click/secondo (non troppo veloce)
- Aspetta 30 secondi dall'ultimo SOS (rate limit)

### Buzzer non suona
- Verifica tipo in `SOSConfig.h`: `SOS_BUZZER_PASSIVE` o `SOS_BUZZER_ACTIVE`
- Controlla connessioni GPIO7 e GPIO44 (piezo) o GPIO7 e GND (attivo)

### Pulsante GPIO8 non risponde
- Verifica connessione: GPIO8 → Pulsante → GND
- Usa pulsante normalmente aperto (NO)
- Tieni premuto per tutti i 4 secondi

## 📊 Specifiche Tecniche

| Caratteristica | Valore                    |
|----------------|---------------------------|
| Base Firmware  | Meshtastic 2.7.20         |
| Hardware       | Heltec T114 V2 (nRF52840) |
| Canale Default | 1 (configurabile)         |
| Rate Limit     | 1 SOS / 30 secondi        |
| Retry          | 3 tentativi automatici    |
| Timeout ACK    | 2 minuti                  |
| Precisione GPS | 6 decimali (~11cm)        |

## 🤝 Contributi

I contributi sono benvenuti! Per favore:

1. Fork il repository
2. Crea un branch per la feature (`git checkout -b feature/AmazingFeature`)
3. Commit le modifiche (`git commit -m 'Add AmazingFeature'`)
4. Push al branch (`git push origin feature/AmazingFeature`)
5. Apri una Pull Request

## 📝 Changelog

### v6 (2025-02-17)
- ✅ Orari assoluti locali invece di età posizione
- ✅ Display wake automatico su ricezione SOS
- ✅ Triple-click fix per nodi senza display
- ✅ Buzzer drive differenziale (+100% volume)
- ✅ Feedback visivo/sonoro pulsante GPIO8
- ✅ Configurazione centralizzata in SOSConfig.h

## 📄 Licenza

GPL-3.0 (eredita licenza Meshtastic)

Questo è firmware sperimentale **non ufficialmente supportato** da Meshtastic o Heltec.

## 👤 Autore

Sviluppato con assistenza di Claude (Anthropic)

## 🔗 Link Utili

- [Meshtastic Project](https://meshtastic.org)
- [Heltec Automation](https://heltec.org)
- [Documentazione Meshtastic](https://meshtastic.org/docs)

---

**⚡ Usa a tuo rischio. Sempre priorità ai servizi di emergenza ufficiali! ⚡**
