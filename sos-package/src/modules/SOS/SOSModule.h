#pragma once
/*
 * SOSModule.h - Modulo SOS per Meshtastic su Heltec T114
 * EG-SOS-v6
 *
 * Gestisce:
 * - Invio SOS al triple-click o pulsante esterno
 * - Ricezione SOS con buzzer, display e ACK automatico
 * - Retry se nessun ACK entro timeout
 * - Forward del messaggio SOS allo smartphone via BLE
 * - Salvataggio posizione in NVM
 */

#include "configuration.h"
#include "mesh/MeshModule.h"
#include "mesh/SinglePortModule.h"
#include "concurrency/OSThread.h"
#include "modules/SOS/SOSConfig.h"

// Struttura posizione salvata in NVM
struct SOSPosition {
    double latitude;
    double longitude;
    uint32_t timestamp;     // Unix time o millis() se RTC non disponibile
    bool valid;
    bool estimated;         // true se posizione stimata (last known)
    uint8_t age_minutes;    // età della posizione in minuti (per posizione stimata)
};

// Struttura messaggio SOS pending per BLE forward
struct SOSPending {
    bool valid;
    char sender_id[5];      // 4 char hex + null
    double latitude;
    double longitude;
    uint32_t timestamp;     // orario messaggio SOS
    char text[80];          // testo breve del messaggio SOS
    bool estimated_pos;
    uint16_t pos_time_hhmm; // orario posizione se stimata (formato: HH*100+MM, es: 1427 = 14:27)
};

// Struttura per tracking ACK ricevuti
struct SOSAckTracker {
    uint8_t count;
    char ids[3][5];         // primi 3 id che hanno risposto
    uint32_t node_nums[10]; // per deduplica (fino a 10 ACK)
    uint8_t node_count;
};

class SOSModule : public SinglePortModule, public concurrency::OSThread
{
public:
    SOSModule();

    // Chiamato dal ButtonThread al triple-click o pulsante esterno
    // Ritorna false se rate-limited
    bool triggerSOS();

    // Stringa versione per boot screen
    static const char *getVersionString();

    // Istanza globale
    static SOSModule *instance;

protected:
    // SinglePortModule - ricezione messaggi dalla mesh
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    virtual bool wantPacket(const meshtastic_MeshPacket *p) override;

    // OSThread - loop periodico per retry, display timer, ecc.
    virtual int32_t runOnce() override;

private:
    // --- STATO SENDER ---
    bool sending_sos;                   // SOS in corso
    uint32_t sos_send_time;             // quando è stato inviato l'SOS
    uint8_t retry_count;                // quante volte abbiamo ritrasmesso
    uint16_t sos_sequence;              // numero di sequenza corrente
    uint32_t last_trigger_time;         // per rate limit
    SOSAckTracker ack_tracker;          // traccia ACK ricevuti

    // --- STATO DISPLAY ---
    uint32_t display_expire_time;       // quando spegnere la schermata SOS
    bool display_active;                // schermata SOS attiva?
    enum class DisplayType { NONE, SENT, RECEIVED, ACK } display_type;
    bool flash_state;                   // per lampeggio
    uint32_t last_flash_time;

    // Dati per display RECEIVED
    char rx_sender_id[5];
    double rx_lat, rx_lon;
    uint32_t rx_timestamp;
    bool rx_estimated;
    uint16_t rx_pos_hhmm; // orario posizione se stimata (formato HHMM: 1427 = 14:27)

    // --- NVM POSIZIONE ---
    SOSPosition last_known_position;
    uint32_t last_position_save_time;

    // Ricezione SOS e ACK
    ProcessMessage handleSOSReceived(const meshtastic_MeshPacket &mp, const char *text);
    ProcessMessage handleACKReceived(const meshtastic_MeshPacket &mp, const char *text);

    // Retry SOS
    void runRetry();
    bool sendSOSMessage();
    bool buildSOSText(char *buf, size_t buflen, const SOSPosition &pos);
    meshtastic_MeshPacket *allocSOSPacket(const char *text);

    // --- RICEZIONE SOS ---
    bool parseSOSMessage(const char *text, char *out_sender, double *out_lat,
                         double *out_lon, uint32_t *out_ts, uint16_t *out_seq,
                         bool *out_estimated, uint16_t *out_pos_hhmm);
    bool isDuplicate(const char *sender_id, uint16_t seq);
    void sendACK(uint32_t dest_node, const char *my_id);
    void forwardToBLE(const SOSPending &pending);
    void savePending(const SOSPending &pending);
    void loadAndClearPending();
    bool hasPendingInNVM();

    // Deduplica SOS ricevuti
    struct RxSeen { char sender[5]; uint16_t seq; };
    static const int RX_SEEN_MAX = 8;
    RxSeen rx_seen[RX_SEEN_MAX];
    int rx_seen_count;

    // --- POSIZIONE ---
    bool acquirePosition(SOSPosition &pos);
    bool loadLastPosition();
    void saveLastPosition(const SOSPosition &pos);
    void updatePositionFromNodeDB();

    // --- DISPLAY ---
    void showSent();
    void showReceived();
    void showAck();
    void stopDisplay();

    // Callback statici per screen->startAlert()
    static void drawSentFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
    static void drawReceivedFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
    static void drawAckFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);

    // --- BUZZER ---
    void soundBuzzer();
    void stopBuzzer();
    bool buzzer_active;
    uint32_t buzzer_stop_time;

    // --- ID NODO ---
    void getShortId(char *out, uint32_t node_num); // 4 char hex lowercase
    void getMyShortId(char *out);
};

extern SOSModule *sosModule;
