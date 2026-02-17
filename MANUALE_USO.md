# EG-SOS-v6 - Manuale d'Uso

## Introduzione

Il firmware EG-SOS-v6 aggiunge la funzionalità di **emergenza SOS** al tuo dispositivo Meshtastic Heltec T114. Puoi inviare un segnale di emergenza alla rete mesh premendo un pulsante, e ricevere automaticamente gli SOS inviati da altri nodi.

---

## 🆘 Come Inviare un SOS

Hai **due modi** per attivare l'SOS:

### Metodo 1: Triple-Click sul Pulsante PRG

1. Premi rapidamente **3 volte** il pulsante PRG (sul lato del dispositivo)
2. Il display mostra "**SOS INVIATO**" lampeggiante per 5 secondi
3. Il messaggio SOS viene trasmesso sulla rete mesh

**Nota**: Devi premere abbastanza velocemente ma non troppo (circa 1 click al secondo funziona bene)

### Metodo 2: Pulsante Esterno su GPIO8

1. Collega un pulsante tra **GPIO8** e **GND**
2. Tieni premuto il pulsante per **4 secondi**
3. Durante la pressione:
   - Il display mostra il countdown: "SOS in 3s...", "SOS in 2s...", "SOS in 1s..."
   - Il buzzer emette un beep breve ogni secondo
4. Quando raggiungi i 4 secondi, l'SOS viene inviato

**Vantaggi pulsante esterno**:
- Puoi montarlo in posizione più comoda (es: manubrio bici)
- Pressione lunga evita attivazioni accidentali
- Feedback visivo e sonoro durante la pressione

---

## 📱 Cosa Succede Quando Invii un SOS

### Sul Tuo Dispositivo

1. **Display**: mostra "SOS INVIATO" (5 secondi, lampeggiante)
2. **Messaggio mesh**: viene inviato sulla rete con:
   - La tua posizione GPS (o ultima posizione nota)
   - Orario di invio
   - Un numero di sequenza

### Protezioni

- **Rate limit**: Puoi inviare massimo 1 SOS ogni 30 secondi
- **Retry automatico**: Se nessuno risponde entro 2 minuti, il sistema riprova fino a 3 volte
- **Conferma ricezione**: Quando altri nodi ricevono il tuo SOS, ti inviano un ACK

### Quando Ricevi ACK

Gli altri nodi che ricevono il tuo SOS inviano automaticamente una conferma (ACK).

**Sul tuo display vedrai** (10 secondi, lampeggiante):
```
SOS ACK da 3 nodi
da: abcd wxyz 1234
```

Questo ti fa sapere che il tuo SOS è stato ricevuto da almeno 3 dispositivi.

---

## 📡 Cosa Succede Quando Ricevi un SOS

### Ricezione Automatica

Quando qualcun altro nella rete invia un SOS:

1. **Display si accende** automaticamente (anche se era spento)
2. **Buzzer suona** per 10 secondi
3. **Schermata lampeggiante** (30 secondi) mostra:
   ```
   Rx SOS da:abcd
   t:14:32 pos@12:15
   lat:45.123456
   lon:9.234567
   ```
   
   Dove:
   - `abcd` = ID del nodo che ha inviato l'SOS
   - `t:14:32` = orario di invio dell'SOS
   - `pos@12:15` = orario a cui si riferisce la posizione (se stimata)
   - Coordinate GPS precise

4. **ACK automatico**: Il tuo dispositivo invia automaticamente una conferma al sender

### Sul Tuo Smartphone (se connesso via Bluetooth)

Se hai lo smartphone connesso al dispositivo tramite l'app Meshtastic, riceverai automaticamente:

```
SOS da abcd lat:45.123456 lon:9.234567 pos ore 12:15
https://maps.google.com/?q=45.123456,9.234567
```

Puoi cliccare il link Google Maps per vedere immediatamente dove si trova la persona in difficoltà.

**Nota**: Se lo smartphone non è connesso quando arriva l'SOS, il messaggio viene salvato e inviato alla prossima connessione.

---

## 🗺️ Come Funziona la Posizione

### Quando Invii un SOS

Il sistema cerca la tua posizione in questo ordine:

1. **GPS esterno** (se collegato e ha fix)
2. **Posizione del telefono** (se connesso via Bluetooth)
3. **Ultima posizione conosciuta** (salvata in memoria)
4. Se nessuna posizione disponibile: `0.000000, 0.000000` (fallback)

### Posizione "Stimata"

Se il GPS non ha fix e viene usata l'ultima posizione salvata, il messaggio indica:
- **L'orario** a cui quella posizione è stata rilevata (es: `pos@12:15`)
- Così chi riceve sa se la posizione è recente o vecchia

**Esempio**:
```
t:14:32 pos@12:15
```
Significa: "SOS inviato alle 14:32, ma la posizione è quella delle 12:15" (2 ore fa)

### Salvataggio Automatico Posizione

Il dispositivo salva automaticamente la tua posizione GPS ogni minuto (se cambiata). Questo garantisce che anche senza GPS fix al momento dell'SOS, hai sempre una posizione recente.

---

## 🔊 Buzzer e Segnalazioni Sonore

### Buzzer alla Ricezione SOS
- **Durata**: 10 secondi continui
- **Tipo**: Tono fisso a 3400Hz (piezo passivo) o beep (buzzer attivo)
- **Volume**: Massimo (drive differenziale per piezo passivo)

### Feedback Pulsante GPIO8
Durante la pressione del pulsante esterno:
- **Beep breve** (80ms) ogni secondo
- Conferma che il sistema sta rilevando la pressione

---

## 💡 Indicatori Display

### Boot
All'accensione vedi "**EG-SOS-v6**" in basso a sinistra per 4 secondi.

### Schermate SOS
Tutte le schermate SOS sono **lampeggianti** (inversione bianco/nero ogni 500ms) per massima visibilità.

| Schermata | Durata | Quando Appare |
|-----------|--------|---------------|
| `SOS INVIATO` | 5 secondi | Quando invii un SOS |
| `Rx SOS da:...` | 30 secondi | Quando ricevi un SOS |
| `SOS ACK da N nodi` | 10 secondi | Quando ricevi conferme al tuo SOS |
| `SOS in Xs...` | Durante pressione | Countdown pulsante GPIO8 |

Dopo la durata, il display torna automaticamente alla schermata normale Meshtastic.

---

## ⚙️ Configurazione Avanzata

**Nota**: Queste impostazioni richiedono ricompilazione del firmware.

Tutti i parametri sono nel file `src/modules/SOS/SOSConfig.h`:

### Cambiare il Canale Mesh
```cpp
#define SOS_CHANNEL_INDEX    1    // 0 = primario, 1-7 = secondari
```

### Durata Pressione Pulsante Esterno
```cpp
#define SOS_EXT_BUTTON_HOLD_MS    4000    // millisecondi (4 secondi)
```

### Tipo Buzzer
```cpp
// Scegli UNO dei due:
// #define SOS_BUZZER_ACTIVE           // Buzzer attivo
#define SOS_BUZZER_PASSIVE           // Piezo passivo (default)
```

### Rate Limit
```cpp
#define SOS_RATE_LIMIT_MS    30000    // Minimo tempo tra 2 SOS (30 secondi)
```

### GPIO Pulsante Esterno
```cpp
#define SOS_EXT_BUTTON_PIN    8    // Numero GPIO
```

Dopo ogni modifica, ricompila con:
```bash
pio run -e heltec-mesh-node-t114-sos
```

---

## 🔧 Risoluzione Problemi

### Il triple-click non funziona

**Problema**: Premo 3 volte ma non succede niente

**Soluzioni**:
1. Non premere troppo velocemente: circa 1 click al secondo
2. Premi con decisione (click netti)
3. Aspetta 30 secondi dall'ultimo SOS (rate limit)
4. Verifica che il firmware sia compilato con `-DUSE_SOS_MODULE`

### Il pulsante GPIO8 non risponde

**Problema**: Tengo premuto ma non parte il countdown

**Soluzioni**:
1. Verifica connessione: GPIO8 → Pulsante → GND
2. Usa un pulsante normalmente aperto (NO)
3. Il dispositivo ha un pull-up interno, non serve resistenza
4. Controlla nei log seriali: `pio device monitor --baud 115200`

### Il buzzer non suona

**Problema**: Ricevo SOS ma nessun suono

**Soluzioni**:
1. Verifica tipo buzzer in `SOSConfig.h`:
   - Piezo passivo: `SOS_BUZZER_PASSIVE`
   - Buzzer attivo: `SOS_BUZZER_ACTIVE`
2. Controlla connessioni:
   - Piezo: GPIO7 e GPIO44
   - Attivo: GPIO7 e GND
3. Il piezo passivo deve essere da ~27mm, 3400Hz

### Non ricevo gli SOS degli altri

**Problema**: Gli altri inviano SOS ma io non li vedo

**Soluzioni**:
1. Verifica di essere sullo **stesso canale** (vedi configurazione)
2. Controlla che la rete mesh sia attiva (prova un ping)
3. Distanza: SOS usa priorità alta ma segue limiti LoRa normali

### Lo smartphone non riceve il link Google Maps

**Problema**: Ricevo SOS ma niente sullo smartphone

**Soluzioni**:
1. Connetti lo smartphone via Bluetooth all'app Meshtastic
2. Se l'SOS è arrivato mentre eri disconnesso: riconnettiti, il messaggio verrà inviato
3. Verifica permessi Bluetooth dell'app Meshtastic

---

## 📊 Specifiche Tecniche Rapide

| Caratteristica | Valore |
|----------------|--------|
| Rate limit SOS | 1 ogni 30 secondi |
| Retry automatici | 3 volte (se no ACK) |
| Timeout ACK | 2 minuti |
| Canale default | 1 (secondario) |
| Durata buzzer | 10 secondi |
| Display SOS ricevuto | 30 secondi |
| Display SOS inviato | 5 secondi |
| Long-press GPIO8 | 4 secondi |
| Precisione GPS | 6 decimali (~11cm) |
| Formato orario | Locale (HH:MM) |

---

## 📍 Scenario d'Uso Esempio

### Escursione in Montagna

**Setup**:
- 4 amici con dispositivi Meshtastic + modulo SOS
- Rete mesh attiva
- GPS esterni collegati
- Smartphone connessi via Bluetooth

**Situazione**:
1. Alice cade e si fa male
2. Bob (vicino ad Alice) preme 3 volte il pulsante PRG
3. **Tutti i dispositivi**:
   - Display si accende e mostra posizione di Bob
   - Buzzer suona per 10 secondi
   - Smartphone ricevono link Google Maps
4. **Carlo e Diana**:
   - Aprono Google Maps sul telefono
   - Vedono esattamente dove sono Bob e Alice
   - Si dirigono verso di loro
5. **Bob vede sul display**:
   - "SOS ACK da 2 nodi"
   - Sa che Carlo e Diana hanno ricevuto il messaggio

**Risultato**: Soccorso coordinato in pochi secondi grazie a posizione precisa e conferme automatiche.

---

## ⚠️ Avvertenze Importanti

### Uso Responsabile
- ⚠️ **NON sostituisce sistemi di emergenza ufficiali** (112, 118, soccorso alpino)
- ⚠️ Funziona solo se altri nodi mesh sono nel raggio (tipico: 2-10km in campo aperto)
- ⚠️ La batteria deve essere carica
- ⚠️ Il GPS può impiegare minuti per il fix iniziale

### Privacy
- 📍 La tua posizione viene trasmessa in chiaro sulla rete mesh
- 📍 Chiunque sulla rete può vedere il tuo SOS
- 📍 I messaggi non sono crittografati end-to-end

### Limitazioni
- 🔋 Ogni SOS consuma batteria (trasmissione radio)
- 📶 Range limitato dalla tecnologia LoRa (~2-10km, dipende da ostacoli)
- ⏱️ Non real-time: delay possibili di secondi/minuti
- 🗺️ Posizione accurata solo con GPS fix recente

---

## 🆘 In Caso di Vera Emergenza

**SEMPRE chiamare i servizi di emergenza ufficiali**:
- 🇮🇹 Italia: **112** (numero unico emergenze)
- 🇪🇺 Europa: **112**
- 🏔️ Soccorso Alpino: **118**

Il modulo SOS è un **supporto aggiuntivo** per coordinamento gruppo, non un sostituto del soccorso professionale.

---

## 📞 Supporto

Per problemi tecnici:
1. Controlla la sezione "Risoluzione Problemi" sopra
2. Verifica i log seriali: `pio device monitor --baud 115200`
3. Consulta il file `DESCRIZIONE_FIRMWARE.md` per dettagli tecnici

---

## 📝 Registro Modifiche Utente

**Versione 6** (Febbraio 2025)
- ✅ Orari sempre in formato locale (invece di UTC)
- ✅ Display si accende automaticamente alla ricezione SOS
- ✅ Triple-click funziona anche su nodi senza display
- ✅ Buzzer più forte (drive differenziale)
- ✅ Feedback visivo/sonoro su pulsante GPIO8
- ✅ Configurazione semplificata (solo SOSConfig.h)

---

**Buon utilizzo e siate prudenti! 🏔️📡**
