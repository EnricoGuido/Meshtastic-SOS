# ============================================================
# MESHTASTIC EG-SOS-v6 - ISTRUZIONI INSTALLAZIONE
# Ambiente: Windows 10, PowerShell, PlatformIO CLI
# ============================================================

# PREREQUISITI
# - PlatformIO CLI installato: https://docs.platformio.org/en/latest/core/installation/index.html
# - Git installato
# - Python 3.x installato
# - Firmware Meshtastic 2.7.20 estratto in C:\dev\firmware-develop
#   (il tuo zip firmware-develop.zip estratto)

# PASSO 1: Scarica e posizionati nel firmware
cd C:\dev\firmware-develop

# PASSO 2: Copia i file SOS (NUOVI FILE)
$pkg = "C:\temp\sos-package"

# Crea cartella modulo SOS
New-Item -ItemType Directory -Force -Path "src\modules\SOS"

Copy-Item -Force "$pkg\src\modules\SOS\SOSConfig.h"   -Destination "src\modules\SOS\"
Copy-Item -Force "$pkg\src\modules\SOS\SOSModule.h"   -Destination "src\modules\SOS\"
Copy-Item -Force "$pkg\src\modules\SOS\SOSModule.cpp"  -Destination "src\modules\SOS\"

# PASSO 3: Sostituisci i file modificati
Copy-Item -Force "$pkg\src\input\InputBroker.cpp"      -Destination "src\input\"
Copy-Item -Force "$pkg\src\input\ButtonThread.cpp"     -Destination "src\input\"
Copy-Item -Force "$pkg\src\modules\Modules.cpp"        -Destination "src\modules\"
Copy-Item -Force "$pkg\src\graphics\draw\UIRenderer.cpp" -Destination "src\graphics\draw\"
Copy-Item -Force "$pkg\variants\nrf52840\heltec_mesh_node_t114\platformio.ini" `
          -Destination "variants\nrf52840\heltec_mesh_node_t114\"

# PASSO 4: Compila (prima compilazione richiede 15-20 minuti)
pio run -e heltec-mesh-node-t114-sos

# PASSO 5: Converti in .uf2 per DFU
# Cerca il .hex generato:
$hex = Get-ChildItem ".pio\build\heltec-mesh-node-t114-sos\firmware*.hex" | Select-Object -First 1
Write-Host "HEX trovato: $hex"

python bin\uf2conv.py $hex.FullName -c -f 0xADA52840 -o firmware-sos.uf2

# PASSO 6: Flash DFU
# 1. Premi RESET due volte velocemente sul T114
# 2. Il T114 appare come drive USB "T114Boot" o simile
# 3. Copia il file uf2:
Copy-Item firmware-sos.uf2 -Destination D:\   # Sostituisci D:\ con la lettera del drive DFU

# ============================================================
# NOTE
# ============================================================
# - Il modulo SOS usa l'environment "heltec-mesh-node-t114-sos"
# - Per buzzer ATTIVO invece di passivo:
#   in SOSConfig.h cambia:
#   #define SOS_BUZZER_PASSIVE  →  #define SOS_BUZZER_ACTIVE
# - Tutti i parametri timing sono in SOSConfig.h
# - Log seriale: pio device monitor --baud 115200

# ============================================================
# VERIFICA INSTALLAZIONE
# ============================================================
# Dopo il flash, aprire il monitor seriale:
pio device monitor --baud 115200

# Al boot dovresti vedere nella schermata iniziale: "EG-SOS-v6"
# Al triplo-click: SOS viene inviato, display mostra "SOS INVIATO"
