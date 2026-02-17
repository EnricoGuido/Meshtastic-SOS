# 📤 Istruzioni Pubblicazione GitHub

## Prerequisiti

1. **Account GitHub**: registrati su https://github.com se non l'hai già
2. **Git installato**: scarica da https://git-scm.com/downloads
3. **Personal Access Token** (per autenticazione HTTPS)

---

## Passo 1: Crea Repository su GitHub

### Interfaccia Web

1. Vai su https://github.com
2. Click sul pulsante **"+"** in alto a destra → **"New repository"**
3. Compila:
   - **Repository name**: `meshtastic-sos` (o nome a tua scelta)
   - **Description**: `Modulo SOS emergenza per Meshtastic Heltec T114`
   - **Public** o **Private**: scegli in base alle tue preferenze
   - ⚠️ **NON** selezionare "Initialize this repository with a README" (lo abbiamo già)
4. Click **"Create repository"**

GitHub ti mostrerà le istruzioni - **non chiudere la pagina**, la useremo dopo.

---

## Passo 2: Prepara la Struttura Locale

### Windows PowerShell

```powershell
# Vai nella cartella dove hai estratto il pacchetto
cd C:\temp\sos-package

# Verifica contenuto
dir
# Dovresti vedere: src/, variants/, README.md, MANUALE_USO.md, ecc.

# Inizializza repository git
git init

# Aggiungi tutti i file
git add .

# Primo commit
git commit -m "Initial commit - EG-SOS-v6 modulo SOS per Meshtastic"

# Rinomina branch principale in 'main' (se necessario)
git branch -M main
```

### Linux/macOS

```bash
# Vai nella cartella
cd /tmp/sos-package

# Verifica contenuto
ls -la
# Dovresti vedere: src/, variants/, README.md, MANUALE_USO.md, ecc.

# Inizializza repository git
git init

# Aggiungi tutti i file
git add .

# Primo commit
git commit -m "Initial commit - EG-SOS-v6 modulo SOS per Meshtastic"

# Rinomina branch principale in 'main'
git branch -M main
```

---

## Passo 3: Collega a GitHub

Sostituisci `TUO-USERNAME` e `meshtastic-sos` con i tuoi valori:

```bash
# Aggiungi remote origin
git remote add origin https://github.com/TUO-USERNAME/meshtastic-sos.git

# Push iniziale
git push -u origin main
```

### Autenticazione

Ti verrà chiesto username e password. **IMPORTANTE**:
- **Username**: il tuo username GitHub
- **Password**: **NON** la password del tuo account, ma un **Personal Access Token**

#### Creare Personal Access Token

1. Vai su https://github.com/settings/tokens
2. Click **"Generate new token"** → **"Generate new token (classic)"**
3. Dai un nome: `Meshtastic SOS Upload`
4. Seleziona scope: **`repo`** (seleziona tutto sotto "repo")
5. Click **"Generate token"**
6. **COPIA IL TOKEN** (lo vedrai una sola volta!)
7. Usalo come password quando fai `git push`

---

## Passo 4: Verifica Pubblicazione

1. Vai su `https://github.com/TUO-USERNAME/meshtastic-sos`
2. Dovresti vedere tutti i file pubblicati
3. Il README.md verrà visualizzato automaticamente nella home del repository

---

## Passo 5: Crea una Release (Consigliato)

Le release permettono agli utenti di scaricare facilmente versioni specifiche.

### Interfaccia Web

1. Nel tuo repository, click su **"Releases"** (nella sidebar destra)
2. Click **"Create a new release"**
3. Compila:
   - **Tag version**: `v6.0.0` (o `v6`)
   - **Release title**: `EG-SOS-v6 - First Release`
   - **Description**:
     ```markdown
     ## EG-SOS-v6 - Modulo SOS per Meshtastic
     
     Prima release pubblica del modulo SOS emergenza per Heltec T114.
     
     ### Funzionalità
     - Invio SOS tramite triple-click o pulsante GPIO8
     - Ricezione automatica con buzzer e display
     - Sistema ACK per conferma ricezione
     - Forward Bluetooth con Google Maps
     - Persistenza NVM
     
     ### Download
     - Scarica il file ZIP allegato
     - Segui le istruzioni nel README.md
     
     ### Changelog
     - ✅ Orari assoluti locali
     - ✅ Display wake automatico
     - ✅ Triple-click fix nodi senza display
     - ✅ Buzzer drive differenziale
     - ✅ Feedback GPIO8
     - ✅ Configurazione centralizzata
     ```
4. **Attach binaries**: Upload il file `meshtastic-EG-SOS-v6.zip`
5. Click **"Publish release"**

### Da Linea di Comando (alternativa)

```bash
# Crea tag
git tag -a v6.0.0 -m "EG-SOS-v6 First Release"

# Push tag
git push origin v6.0.0
```

Poi completa la release dall'interfaccia web aggiungendo il file ZIP.

---

## Passo 6: Aggiorna README con Link Corretti

Ora che il repository esiste, aggiorna i link nel README:

```bash
# Apri README.md con un editor di testo
# Sostituisci "TUO-USERNAME" con il tuo username GitHub reale

# Esempio:
# Da:   https://github.com/TUO-USERNAME/meshtastic-sos/releases
# A:    https://github.com/mario-rossi/meshtastic-sos/releases
```

Dopo la modifica:

```bash
git add README.md
git commit -m "Update README with correct GitHub links"
git push
```

---

## 📝 Aggiornamenti Futuri

Quando modifichi il firmware e vuoi pubblicare un aggiornamento:

```bash
# 1. Modifica i file
# ... fai le tue modifiche ...

# 2. Aggiungi modifiche
git add .

# 3. Commit con messaggio descrittivo
git commit -m "Fix: risolto problema buzzer GPIO8"

# 4. Push
git push

# 5. (Opzionale) Crea nuova release
git tag -a v6.1.0 -m "Bugfix release v6.1.0"
git push origin v6.1.0
```

---

## 🔐 Configurazione Git (Prima Volta)

Se è la prima volta che usi git sul tuo PC:

```bash
# Configura nome e email
git config --global user.name "Il Tuo Nome"
git config --global user.email "tua-email@example.com"

# Verifica configurazione
git config --list
```

---

## 🌿 Branch e Workflow (Avanzato)

Per progetti più complessi, puoi usare branch:

```bash
# Crea branch per nuova feature
git checkout -b feature/nuovo-canale-sos

# Lavora sulla feature
# ... modifiche ...

# Commit
git add .
git commit -m "Add: supporto canale dinamico SOS"

# Push branch
git push -u origin feature/nuovo-canale-sos

# Crea Pull Request su GitHub per merge in main
```

---

## 📋 Checklist Pubblicazione

- [ ] Repository GitHub creato
- [ ] Git inizializzato nella cartella locale
- [ ] File aggiunti e committati
- [ ] Remote origin configurato
- [ ] Push iniziale completato
- [ ] README.md visibile su GitHub
- [ ] Release v6.0.0 creata
- [ ] File ZIP caricato nella release
- [ ] Link README aggiornati con username corretto
- [ ] Repository testato (clone in altra cartella per verifica)

---

## 🧪 Test Finale

Verifica che tutto funzioni clonando il repository in una cartella diversa:

```bash
# Clone
git clone https://github.com/TUO-USERNAME/meshtastic-sos.git test-clone
cd test-clone

# Verifica contenuto
ls -la
# Dovresti vedere tutti i file

# Verifica che il README si veda correttamente
cat README.md
```

---

## 🆘 Problemi Comuni

### "Permission denied (publickey)"

**Soluzione**: Usa HTTPS invece di SSH, o configura chiavi SSH:
```bash
# Verifica remote URL
git remote -v

# Se vedi git@github.com, cambia in HTTPS
git remote set-url origin https://github.com/TUO-USERNAME/meshtastic-sos.git
```

### "fatal: remote origin already exists"

**Soluzione**:
```bash
# Rimuovi remote esistente
git remote remove origin

# Aggiungi di nuovo
git remote add origin https://github.com/TUO-USERNAME/meshtastic-sos.git
```

### Errore "Updates were rejected"

**Soluzione**:
```bash
# Pull prima di push
git pull origin main --allow-unrelated-histories

# Poi push
git push -u origin main
```

---

## 📚 Risorse Utili

- **Git Basics**: https://git-scm.com/book/en/v2/Getting-Started-Git-Basics
- **GitHub Guides**: https://guides.github.com
- **Markdown Syntax**: https://www.markdownguide.org/basic-syntax/

---

## ✅ Fatto!

Il tuo repository è ora pubblico (o privato) su GitHub! 🎉

Gli utenti possono:
- Clonarlo: `git clone https://github.com/TUO-USERNAME/meshtastic-sos.git`
- Scaricare ZIP dalla release
- Contribuire con Pull Request
- Aprire Issue per bug/feature

**Link da condividere**:
```
https://github.com/TUO-USERNAME/meshtastic-sos
```
