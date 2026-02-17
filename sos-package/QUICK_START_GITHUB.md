# 🚀 Quick Start - Pubblicazione GitHub

## TL;DR - 5 Comandi

```bash
cd C:\temp\sos-package          # vai nella cartella
git init                        # inizializza git
git add .                       # aggiungi tutto
git commit -m "Initial commit"  # commit
git branch -M main              # rinomina branch
git remote add origin https://github.com/TUO-USERNAME/meshtastic-sos.git
git push -u origin main         # pubblica!
```

## Prima di Iniziare

1. **Crea repository** su https://github.com/new
   - Nome: `meshtastic-sos`
   - **NON** selezionare "Initialize with README"
   
2. **Copia URL** del repository (es: `https://github.com/mario/meshtastic-sos.git`)

3. **Crea Personal Access Token** su https://github.com/settings/tokens
   - Scope: `repo` (tutto)
   - **Copia il token** (lo userai come password)

## Pubblicazione

```powershell
# Windows PowerShell
cd C:\temp\sos-package

git init
git add .
git commit -m "EG-SOS-v6 - Initial release"
git branch -M main
git remote add origin https://github.com/TUO-USERNAME/meshtastic-sos.git
git push -u origin main
```

Quando richiesto:
- **Username**: tuo username GitHub
- **Password**: il Personal Access Token (NON la password del tuo account)

## Crea Release

1. Vai su `https://github.com/TUO-USERNAME/meshtastic-sos`
2. Click **"Releases"** → **"Create a new release"**
3. Tag: `v6.0.0`
4. Title: `EG-SOS-v6`
5. Upload file: `meshtastic-EG-SOS-v6.zip`
6. Click **"Publish release"**

## ✅ Fatto!

Repository disponibile a:
```
https://github.com/TUO-USERNAME/meshtastic-sos
```

---

📖 **Guida completa**: vedi `PUBBLICAZIONE_GITHUB.md`
