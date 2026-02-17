#pragma once
/*
 * SOSConfig.h - CONFIGURAZIONE MODULO SOS
 * 
 * ⚙️  QUESTO È L'UNICO FILE DA MODIFICARE PER CONFIGURARE IL MODULO SOS
 * 
 * Tutti i parametri (timing, GPIO, canale, buzzer, ecc.) sono qui.
 * NON serve modificare platformio.ini o altri file.
 * 
 * Dopo ogni modifica: ricompila il firmware.
 */

// ============================================================================
// VERSIONE
// ============================================================================
#define SOS_VERSION_STRING "EG-SOS-v6"

// ============================================================================
// TRIPLE-CLICK
// ============================================================================
// Finestra temporale entro cui vanno effettuati i 3 click (ms)
#define SOS_TRIPLE_CLICK_WINDOW_MS  1200
// Timeout click per OneButton (ms) - tempo max tra pressione e rilascio
#define SOS_CLICK_MS                250

// ============================================================================
// RATE LIMIT INVIO SOS
// ============================================================================
// Minimo tempo tra due invii SOS (ms)
#define SOS_RATE_LIMIT_MS           30000   // 30 secondi

// ============================================================================
// CANALE MESH
// ============================================================================
// Canale su cui inviare l'SOS (1 = canale secondario, 0 = canale primario)
#define SOS_CHANNEL_INDEX           1

// ============================================================================
// RETRY SOS
// ============================================================================
// Timeout attesa ACK prima di ritrasmettre (ms)
#define SOS_ACK_TIMEOUT_MS          120000  // 2 minuti
// Numero massimo di ritrasmissioni
#define SOS_MAX_RETRIES             3

// ============================================================================
// BUZZER
// ============================================================================
// GPIO buzzer attivo (singolo pin)
#define SOS_BUZZER_PIN              7
// GPIO per buzzer passivo (differenziale, pin A e pin B)
#define SOS_BUZZER_PASSIVE_PIN_A    7
#define SOS_BUZZER_PASSIVE_PIN_B    44
// Frequenza buzzer passivo (Hz)
#define SOS_BUZZER_FREQ_HZ          3400
// Durata suono buzzer alla ricezione SOS (ms)
#define SOS_BUZZER_DURATION_MS      10000

// ============================================================================
// TIPO BUZZER
// ============================================================================
// Seleziona tipo buzzer decommentando UNA SOLA delle seguenti righe:

// #define SOS_BUZZER_ACTIVE           // Buzzer attivo (singolo GPIO)
#define SOS_BUZZER_PASSIVE           // Disco piezo passivo (2 GPIO differenziali) ← DEFAULT

// ============================================================================
// PULSANTE ESTERNO SOS
// ============================================================================
// GPIO pulsante esterno (aggiuntivo al triplo-click su PRG)
#define SOS_EXT_BUTTON_PIN          8
// Attivo basso (pulled up)
#define SOS_EXT_BUTTON_ACTIVE_LOW   true
// Durata minima pressione pulsante esterno per attivare SOS (long-long-press, ms)
#define SOS_EXT_BUTTON_HOLD_MS      4000    // 4 secondi

// ============================================================================
// DISPLAY - DURATE SCHERMATE (ms)
// ============================================================================
#define SOS_DISPLAY_SENT_MS         5000    // "SOS INVIATO" - 5 secondi
#define SOS_DISPLAY_RECEIVED_MS     30000   // Ricezione SOS - 30 secondi
#define SOS_DISPLAY_ACK_MS          10000   // ACK ricevuto - 10 secondi

// Durata banner persistente in alto (ms)
#define SOS_BANNER_DURATION_MS      60000   // 60 secondi

// Velocità lampeggio (ms per stato ON e OFF)
#define SOS_FLASH_PERIOD_MS         500

// ============================================================================
// AGGIORNAMENTO POSIZIONE IN NVM
// ============================================================================
// Frequenza minima aggiornamento posizione in NVM (ms)
#define SOS_POSITION_UPDATE_MIN_MS  60000   // 1 minuto

// ============================================================================
// MESSAGGIO SOS
// ============================================================================
// Prefisso riconoscibile nel testo
#define SOS_MESSAGE_PREFIX          "[SOS]"
// Carattere bell (alert acustico per client standard)
#define SOS_BELL_CHAR               '\a'    // 0x07

// ============================================================================
// NVM - FILE PATHS
// ============================================================================
#define SOS_NVM_POSITION_FILE       "/prefs/sos_position.bin"
#define SOS_NVM_PENDING_FILE        "/prefs/sos_pending.bin"
