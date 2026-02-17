/*
 * SOSModule.cpp - Modulo SOS per Meshtastic su Heltec T114
 * EG-SOS-v6
 */

#include "SOSModule.h"
#include "FSCommon.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "PowerFSM.h"
#include "buzz/buzz.h"
#include "configuration.h"
#include "graphics/Screen.h"
#include "gps/RTC.h"
#include "main.h"
#include "mesh/Router.h"
#include "mesh/generated/meshtastic/mesh.pb.h"

#if !defined(ARCH_PORTDUINO)
extern "C" void delay(uint32_t dwMs);
#endif

#if !defined(ARCH_ESP32) && !defined(ARCH_RP2040) && !defined(ARCH_PORTDUINO)
#include "Tone.h"
#endif

// Istanza globale
SOSModule *sosModule = nullptr;
SOSModule *SOSModule::instance = nullptr;

// ============================================================================
// Helpers
// ============================================================================

void SOSModule::getShortId(char *out, uint32_t node_num)
{
    snprintf(out, 5, "%04x", node_num & 0xFFFF);
}

void SOSModule::getMyShortId(char *out)
{
    getShortId(out, nodeDB->getNodeNum());
}

// ============================================================================
// Costruttore e init
// ============================================================================

SOSModule::SOSModule()
    : SinglePortModule("SOS", meshtastic_PortNum_TEXT_MESSAGE_APP),
      concurrency::OSThread("SOSModule"),
      sending_sos(false), sos_send_time(0), retry_count(0), sos_sequence(0),
      last_trigger_time(0), display_active(false), display_type(DisplayType::NONE),
      flash_state(false), last_flash_time(0),
      rx_lat(0), rx_lon(0), rx_timestamp(0), rx_estimated(false), rx_pos_hhmm(0),
      last_position_save_time(0),
      rx_seen_count(0),
      buzzer_active(false), buzzer_stop_time(0)
{
    memset(&ack_tracker, 0, sizeof(ack_tracker));
    memset(&last_known_position, 0, sizeof(last_known_position));
    memset(rx_sender_id, 0, sizeof(rx_sender_id));
    memset(rx_seen, 0, sizeof(rx_seen));

    // Registra istanza per i callback statici
    instance = this;

    // Carica ultima posizione da NVM
    loadLastPosition();

    // Controlla se ci sono messaggi SOS pending da forwarding BLE
    loadAndClearPending();

    // Avvia il thread
    setIntervalFromNow(1000);
}

// ============================================================================
// TRIGGER SOS (chiamato da ButtonThread o ISR pulsante esterno)
// ============================================================================

bool SOSModule::triggerSOS()
{
    uint32_t now = millis();

    // Rate limit
    if (last_trigger_time > 0 && (now - last_trigger_time) < SOS_RATE_LIMIT_MS) {
        uint32_t remaining = (SOS_RATE_LIMIT_MS - (now - last_trigger_time)) / 1000;
        LOG_WARN("SOS rate limited, attendere %us", remaining);
        // Mostra breve banner
        if (screen) {
            graphics::BannerOverlayOptions opts;
            static char rate_msg[40];
            snprintf(rate_msg, sizeof(rate_msg), "SOS: attendi %us", remaining);
            opts.message = rate_msg;
            opts.durationMs = 3000;
            screen->showOverlayBanner(opts);
        }
        return false;
    }

    last_trigger_time = now;
    retry_count = 0;
    sos_sequence = (sos_sequence + 1) % 1000;
    memset(&ack_tracker, 0, sizeof(ack_tracker));

    if (sendSOSMessage()) {
        sending_sos = true;
        sos_send_time = now;
        showSent();
        LOG_INFO("SOS inviato, seq=%03u", sos_sequence);
        return true;
    } else {
        LOG_ERROR("Errore invio SOS");
        return false;
    }
}

// ============================================================================
// INVIO SOS
// ============================================================================

bool SOSModule::sendSOSMessage()
{
    SOSPosition pos;
    if (!acquirePosition(pos)) {
        LOG_WARN("Nessuna posizione disponibile, uso 0.0");
        pos.latitude = 0.0;
        pos.longitude = 0.0;
        pos.valid = false;
        pos.estimated = false;
        pos.age_minutes = 0;
    }

    char text[meshtastic_Constants_DATA_PAYLOAD_LEN + 1];
    if (!buildSOSText(text, sizeof(text), pos)) {
        return false;
    }

    meshtastic_MeshPacket *p = allocSOSPacket(text);
    if (!p) {
        LOG_ERROR("Impossibile allocare pacchetto SOS");
        return false;
    }

    service->sendToMesh(p, RX_SRC_LOCAL, true);
    return true;
}

bool SOSModule::buildSOSText(char *buf, size_t buflen, const SOSPosition &pos)
{
    char my_id[5];
    getMyShortId(my_id);

    // Ora locale HH:MM
    uint32_t ts = getTime(true); // true = local time
    uint32_t hh = (ts % 86400) / 3600;
    uint32_t mm = (ts % 3600) / 60;

    if (pos.estimated && pos.valid) {
        // Posizione stimata: invia orario assoluto locale della posizione
        uint32_t pos_hh = (pos.timestamp % 86400) / 3600;
        uint32_t pos_mm = (pos.timestamp % 3600) / 60;
        snprintf(buf, buflen,
                 "%c%s id:%s seq:%03u t:%02u:%02u lat:%.6f lon:%.6f Epos@%02u:%02u",
                 SOS_BELL_CHAR,
                 SOS_MESSAGE_PREFIX,
                 my_id,
                 sos_sequence,
                 hh, mm,
                 pos.latitude,
                 pos.longitude,
                 pos_hh, pos_mm);
    } else if (pos.valid) {
        snprintf(buf, buflen,
                 "%c%s id:%s seq:%03u t:%02u:%02u lat:%.6f lon:%.6f",
                 SOS_BELL_CHAR,
                 SOS_MESSAGE_PREFIX,
                 my_id,
                 sos_sequence,
                 hh, mm,
                 pos.latitude,
                 pos.longitude);
    } else {
        snprintf(buf, buflen,
                 "%c%s id:%s seq:%03u t:%02u:%02u lat:0.000000 lon:0.000000",
                 SOS_BELL_CHAR,
                 SOS_MESSAGE_PREFIX,
                 my_id,
                 sos_sequence,
                 hh, mm);
    }

    return true;
}

meshtastic_MeshPacket *SOSModule::allocSOSPacket(const char *text)
{
    meshtastic_MeshPacket *p = router->allocForSending();
    if (!p)
        return nullptr;

    p->to = NODENUM_BROADCAST;
    p->channel = SOS_CHANNEL_INDEX;
    p->want_ack = false;                 // ACK applicativo gestito da noi
    p->priority = meshtastic_MeshPacket_Priority_HIGH;
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

    size_t len = strlen(text);
    if (len > meshtastic_Constants_DATA_PAYLOAD_LEN)
        len = meshtastic_Constants_DATA_PAYLOAD_LEN;

    memcpy(p->decoded.payload.bytes, text, len);
    p->decoded.payload.size = len;

    return p;
}

// ============================================================================
// RICEZIONE MESSAGGI DALLA MESH
// ============================================================================

bool SOSModule::wantPacket(const meshtastic_MeshPacket *p)
{
    // Riceviamo tutti i TEXT_MESSAGE (per filtrare quelli SOS)
    return (p->decoded.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP);
}

ProcessMessage SOSModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    // Estrai payload come stringa
    const auto &p = mp.decoded;
    if (p.payload.size == 0)
        return ProcessMessage::CONTINUE;

    char text[meshtastic_Constants_DATA_PAYLOAD_LEN + 1];
    size_t len = p.payload.size;
    if (len > meshtastic_Constants_DATA_PAYLOAD_LEN)
        len = meshtastic_Constants_DATA_PAYLOAD_LEN;
    memcpy(text, p.payload.bytes, len);
    text[len] = '\0';

    // Controlla se è un messaggio SOS applicativo (formato [SOS]...)
    // Oppure un ACK SOS (formato [SOSACK]...)
    if (strncmp(text, "\x07" SOS_MESSAGE_PREFIX, strlen(SOS_MESSAGE_PREFIX) + 1) == 0 ||
        strncmp(text, SOS_MESSAGE_PREFIX, strlen(SOS_MESSAGE_PREFIX)) == 0) {
        return handleSOSReceived(mp, text);
    }

    if (strncmp(text, "[SOSACK]", 8) == 0) {
        return handleACKReceived(mp, text);
    }

    return ProcessMessage::CONTINUE;
}

ProcessMessage SOSModule::handleSOSReceived(const meshtastic_MeshPacket &mp, const char *text)
{
    char sender_id[5];
    double lat, lon;
    uint32_t ts;
    uint16_t seq;
    bool estimated;
    uint16_t pos_hhmm;

    if (!parseSOSMessage(text, sender_id, &lat, &lon, &ts, &seq, &estimated, &pos_hhmm)) {
        LOG_WARN("SOS: impossibile parsare messaggio: %s", text);
        return ProcessMessage::CONTINUE;
    }

    // Deduplica
    if (isDuplicate(sender_id, seq)) {
        LOG_DEBUG("SOS: duplicato da %s seq=%u, ignorato", sender_id, seq);
        return ProcessMessage::STOP;
    }

    LOG_INFO("SOS ricevuto da %s seq=%u lat=%.6f lon=%.6f", sender_id, seq, lat, lon);

    // Salva in rx_seen
    if (rx_seen_count < RX_SEEN_MAX) {
        strncpy(rx_seen[rx_seen_count].sender, sender_id, 4);
        rx_seen[rx_seen_count].seq = seq;
        rx_seen_count++;
    }

    // Aggiorna stato display RECEIVED
    strncpy(rx_sender_id, sender_id, 4);
    rx_lat = lat;
    rx_lon = lon;
    rx_timestamp = ts;
    rx_estimated = estimated;
    rx_pos_hhmm = pos_hhmm;

    // Mostra schermata SOS ricevuto
    showReceived();

    // Suona il buzzer
    soundBuzzer();

    // Invia ACK applicativo al sender
    char my_id[5];
    getMyShortId(my_id);
    sendACK(mp.from, my_id);

    // Salva in NVM per forward BLE
    SOSPending pending;
    memset(&pending, 0, sizeof(pending));
    pending.valid = true;
    strncpy(pending.sender_id, sender_id, 4);
    pending.latitude = lat;
    pending.longitude = lon;
    pending.timestamp = ts;
    pending.estimated_pos = estimated;
    pending.pos_time_hhmm = pos_hhmm;

    // Crea testo breve (senza orario posizione, aggiunto dopo nel BLE)
    snprintf(pending.text, sizeof(pending.text),
             "SOS da %s lat:%.6f lon:%.6f", sender_id, lat, lon);

    savePending(pending);

    // Forward a BLE se già connesso, altrimenti attende
    forwardToBLE(pending);

    // STOP: abbiamo gestito noi, ma NON blocchiamo altri moduli
    // Il messaggio deve rimanere visibile tra i messaggi normali
    return ProcessMessage::CONTINUE;
}

ProcessMessage SOSModule::handleACKReceived(const meshtastic_MeshPacket &mp, const char *text)
{
    if (!sending_sos)
        return ProcessMessage::CONTINUE;

    // Formato: [SOSACK] id:xxxx seq:NNN
    char ack_id[5] = {0};
    uint16_t ack_seq = 0;
    const char *p = strstr(text, "id:");
    if (p) snprintf(ack_id, 5, "%.4s", p + 3);
    p = strstr(text, "seq:");
    if (p) ack_seq = (uint16_t)atoi(p + 4);

    if (ack_seq != sos_sequence)
        return ProcessMessage::CONTINUE;

    // Deduplica ACK per stesso nodo
    uint32_t from = mp.from;
    for (int i = 0; i < ack_tracker.node_count; i++) {
        if (ack_tracker.node_nums[i] == from)
            return ProcessMessage::CONTINUE;
    }
    if (ack_tracker.node_count < 10) {
        ack_tracker.node_nums[ack_tracker.node_count++] = from;
    }

    ack_tracker.count++;
    if (ack_tracker.count <= 3) {
        strncpy(ack_tracker.ids[ack_tracker.count - 1], ack_id, 4);
    }

    LOG_INFO("ACK SOS da %s (tot: %u)", ack_id, ack_tracker.count);
    showAck();

    return ProcessMessage::STOP;
}

// ============================================================================
// PARSE MESSAGGIO SOS
// ============================================================================

bool SOSModule::parseSOSMessage(const char *text, char *out_sender, double *out_lat,
                                 double *out_lon, uint32_t *out_ts, uint16_t *out_seq,
                                 bool *out_estimated, uint16_t *out_pos_hhmm)
{
    // Cerca prefisso [SOS] (saltando eventuale bell char)
    const char *start = strstr(text, SOS_MESSAGE_PREFIX);
    if (!start) return false;

    // id:xxxx
    const char *p = strstr(start, "id:");
    if (!p) return false;
    snprintf(out_sender, 5, "%.4s", p + 3);

    // seq:NNN
    p = strstr(start, "seq:");
    if (!p) return false;
    *out_seq = (uint16_t)atoi(p + 4);

    // t:HH:MM (orario SOS)
    p = strstr(start, "t:");
    uint32_t hh = 0, mm = 0;
    if (p) {
        hh = atoi(p + 2);
        const char *colon = strchr(p + 2, ':');
        if (colon) mm = atoi(colon + 1);
    }
    *out_ts = hh * 3600 + mm * 60;

    // lat:
    p = strstr(start, "lat:");
    if (!p) return false;
    *out_lat = atof(p + 4);

    // lon:
    p = strstr(start, "lon:");
    if (!p) return false;
    *out_lon = atof(p + 4);

    // Epos@HH:MM (orario posizione se stimata)?
    p = strstr(start, "Epos@");
    if (p) {
        *out_estimated = true;
        int pos_hh = 0, pos_mm = 0;
        if (sscanf(p + 5, "%d:%d", &pos_hh, &pos_mm) == 2) {
            *out_pos_hhmm = pos_hh * 100 + pos_mm; // formato HHMM: 1427 = 14:27
        } else {
            *out_pos_hhmm = 0;
        }
    } else {
        *out_estimated = false;
        *out_pos_hhmm = 0;
    }

    return true;
}

// ============================================================================
// DEDUPLICA
// ============================================================================

bool SOSModule::isDuplicate(const char *sender_id, uint16_t seq)
{
    for (int i = 0; i < rx_seen_count; i++) {
        if (strncmp(rx_seen[i].sender, sender_id, 4) == 0 && rx_seen[i].seq == seq)
            return true;
    }
    return false;
}

// ============================================================================
// INVIO ACK APPLICATIVO
// ============================================================================

void SOSModule::sendACK(uint32_t dest_node, const char *my_id)
{
    meshtastic_MeshPacket *p = router->allocForSending();
    if (!p) return;

    p->to = dest_node;
    p->channel = SOS_CHANNEL_INDEX;
    p->want_ack = true;
    p->priority = meshtastic_MeshPacket_Priority_RELIABLE;
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

    char ack_text[64];
    snprintf(ack_text, sizeof(ack_text), "[SOSACK] id:%s seq:%03u", my_id, sos_sequence > 0 ? sos_sequence : 0);
    // Nota: sos_sequence non è il mio ma quello del sender. Lo usiamo 0 per ACK
    // Meglio: usiamo il sequence del messaggio ricevuto
    // Qui passiamo l'ultimo seq visto nei rx_seen
    if (rx_seen_count > 0) {
        snprintf(ack_text, sizeof(ack_text), "[SOSACK] id:%s seq:%03u", my_id, rx_seen[rx_seen_count-1].seq);
    }

    size_t len = strlen(ack_text);
    memcpy(p->decoded.payload.bytes, ack_text, len);
    p->decoded.payload.size = len;

    service->sendToMesh(p, RX_SRC_LOCAL, false);
    LOG_INFO("ACK SOS inviato a 0x%08x", dest_node);
}

// ============================================================================
// POSIZIONE
// ============================================================================

bool SOSModule::acquirePosition(SOSPosition &pos)
{
    // Priorità 1: localPosition da NodeDB (aggiornata da GPS o da phone via BLE)
    if (nodeDB->hasLocalPositionSinceBoot()) {
        const meshtastic_Position &lp = localPosition;
        if (lp.latitude_i != 0 || lp.longitude_i != 0) {
            pos.latitude  = lp.latitude_i  * 1e-7;
            pos.longitude = lp.longitude_i * 1e-7;
            pos.timestamp = lp.time > 0 ? lp.time : getTime();
            pos.valid     = true;
            pos.estimated = false;
            pos.age_minutes = 0;

            // Salva in NVM se è passato abbastanza tempo
            if (millis() - last_position_save_time >= SOS_POSITION_UPDATE_MIN_MS) {
                saveLastPosition(pos);
            }
            return true;
        }
    }

    // Priorità 3: ultima posizione nota da NVM
    if (last_known_position.valid) {
        pos = last_known_position;
        pos.estimated = true;
        // Calcola età in minuti
        uint32_t age_s = (getTime() > pos.timestamp) ? (getTime() - pos.timestamp) : 0;
        pos.age_minutes = (uint8_t)min((uint32_t)255, age_s / 60);
        return true;
    }

    return false; // Nessuna posizione: il chiamante userà 0.0,0.0
}

bool SOSModule::loadLastPosition()
{
#ifdef FSCom
    auto f = FSCom.open(SOS_NVM_POSITION_FILE, FILE_O_READ);
    if (f) {
        if (f.read((uint8_t *)&last_known_position, sizeof(last_known_position)) == sizeof(last_known_position)) {
            LOG_INFO("SOS: posizione NVM caricata lat=%.6f lon=%.6f", last_known_position.latitude, last_known_position.longitude);
        } else {
            memset(&last_known_position, 0, sizeof(last_known_position));
        }
        f.close();
        return last_known_position.valid;
    }
#endif
    return false;
}

void SOSModule::saveLastPosition(const SOSPosition &pos)
{
#ifdef FSCom
    auto f = FSCom.open(SOS_NVM_POSITION_FILE, FILE_O_WRITE);
    if (f) {
        f.write((const uint8_t *)&pos, sizeof(pos));
        f.close();
        last_position_save_time = millis();
        LOG_DEBUG("SOS: posizione salvata in NVM");
    }
#endif
}

// ============================================================================
// NVM - PENDING SOS PER BLE
// ============================================================================

void SOSModule::savePending(const SOSPending &pending)
{
#ifdef FSCom
    auto f = FSCom.open(SOS_NVM_PENDING_FILE, FILE_O_WRITE);
    if (f) {
        f.write((const uint8_t *)&pending, sizeof(pending));
        f.close();
    }
#endif
}

void SOSModule::loadAndClearPending()
{
#ifdef FSCom
    if (!FSCom.exists(SOS_NVM_PENDING_FILE))
        return;

    auto f = FSCom.open(SOS_NVM_PENDING_FILE, FILE_O_READ);
    if (f) {
        SOSPending pending;
        if (f.read((uint8_t *)&pending, sizeof(pending)) == sizeof(pending) && pending.valid) {
            f.close();
            forwardToBLE(pending);
            // Cancella dopo aver letto
            FSCom.remove(SOS_NVM_PENDING_FILE);
        } else {
            f.close();
        }
    }
#endif
}

void SOSModule::forwardToBLE(const SOSPending &pending)
{
    if (!pending.valid) return;

    // Crea link Google Maps
    char maps_url[128];
    snprintf(maps_url, sizeof(maps_url),
             "https://maps.google.com/?q=%.6f,%.6f",
             pending.latitude, pending.longitude);

    // Componi messaggio completo
    char full_msg[meshtastic_Constants_DATA_PAYLOAD_LEN + 1];
    if (pending.estimated_pos) {
        uint8_t pos_hh = pending.pos_time_hhmm / 100;
        uint8_t pos_mm = pending.pos_time_hhmm % 100;
        snprintf(full_msg, sizeof(full_msg), "%s pos ore %02u:%02u\n%s",
                 pending.text, pos_hh, pos_mm, maps_url);
    } else {
        snprintf(full_msg, sizeof(full_msg), "%s\n%s", pending.text, maps_url);
    }

    // Invia come nuovo messaggio verso il telefono (via toPhoneQueue)
    meshtastic_MeshPacket *p = router->allocForSending();
    if (!p) return;

    p->to = nodeDB->getNodeNum();   // verso noi stessi → finirà nella toPhoneQueue
    p->from = nodeDB->getNodeNum();
    p->channel = SOS_CHANNEL_INDEX;
    p->want_ack = false;
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

    size_t len = strlen(full_msg);
    if (len > meshtastic_Constants_DATA_PAYLOAD_LEN)
        len = meshtastic_Constants_DATA_PAYLOAD_LEN;
    memcpy(p->decoded.payload.bytes, full_msg, len);
    p->decoded.payload.size = len;

    service->sendToPhone(p);
    LOG_INFO("SOS: messaggio inviato allo smartphone via BLE");
}

// ============================================================================
// DISPLAY
// ============================================================================

// Dati statici per i callback del display (necessari perché i callback sono statici)
static float s_rx_lat, s_rx_lon;
static char s_rx_sender[5];
static uint32_t s_rx_ts;
static bool s_rx_estimated;
static uint8_t s_rx_pos_hh, s_rx_pos_mm; // orario posizione (se stimata)
static uint8_t s_ack_count;
static char s_ack_ids[3][5];
static bool s_flash_state;

void SOSModule::showSent()
{
    display_type = DisplayType::SENT;
    display_active = true;
    display_expire_time = millis() + SOS_DISPLAY_SENT_MS;
    flash_state = true;
    last_flash_time = millis();
    s_flash_state = true;

    if (screen) {
        screen->setOn(true);   // Accende il display se era spento
        screen->startAlert(drawSentFrame);
    }
}

void SOSModule::showReceived()
{
    display_type = DisplayType::RECEIVED;
    display_active = true;
    display_expire_time = millis() + SOS_DISPLAY_RECEIVED_MS;
    flash_state = true;
    last_flash_time = millis();

    // Copia dati per callback statico
    s_rx_lat = (float)rx_lat;
    s_rx_lon = (float)rx_lon;
    strncpy(s_rx_sender, rx_sender_id, 4);
    s_rx_sender[4] = 0;
    s_rx_ts = rx_timestamp;
    s_rx_estimated = rx_estimated;
    
    // Estrai HH:MM dall'orario posizione (formato HHMM)
    if (rx_estimated) {
        s_rx_pos_hh = rx_pos_hhmm / 100;
        s_rx_pos_mm = rx_pos_hhmm % 100;
    }
    
    s_flash_state = true;

    if (screen) {
        screen->setOn(true);   // Accende il display se era spento
        screen->startAlert(drawReceivedFrame);
    }
}

void SOSModule::showAck()
{
    display_type = DisplayType::ACK;
    display_active = true;
    display_expire_time = millis() + SOS_DISPLAY_ACK_MS;
    flash_state = true;
    last_flash_time = millis();

    // Copia dati per callback statico
    s_ack_count = ack_tracker.count;
    for (int i = 0; i < 3; i++) {
        strncpy(s_ack_ids[i], ack_tracker.ids[i], 4);
        s_ack_ids[i][4] = 0;
    }
    s_flash_state = true;

    if (screen) {
        screen->setOn(true);   // Accende il display se era spento
        screen->startAlert(drawAckFrame);
    }
}

void SOSModule::stopDisplay()
{
    display_active = false;
    display_type = DisplayType::NONE;
    if (screen) {
        screen->endAlert();
        screen->setFrames(); // STOP_ALERT_FRAME non chiama setFrames() — dobbiamo farlo noi
    }
}

// ============================================================================
// CALLBACK DISPLAY (statici - chiamati da Screen)
// ============================================================================

void SOSModule::drawSentFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    int16_t w = display->width();
    int16_t h = display->height();

    if (s_flash_state) {
        display->setColor(BLACK);
        display->fillRect(x, y, w, h);
        display->setColor(WHITE);
    } else {
        display->setColor(WHITE);
        display->fillRect(x, y, w, h);
        display->setColor(BLACK);
    }

    display->setFont(FONT_LARGE);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(x + w / 2, y + h / 2 - FONT_HEIGHT_LARGE / 2, "SOS INVIATO");
}

void SOSModule::drawReceivedFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    int16_t w = display->width();
    int16_t h = display->height();

    if (s_flash_state) {
        display->setColor(BLACK);
        display->fillRect(x, y, w, h);
        display->setColor(WHITE);
    } else {
        display->setColor(WHITE);
        display->fillRect(x, y, w, h);
        display->setColor(BLACK);
    }

    display->setFont(FONT_MEDIUM);
    display->setTextAlignment(TEXT_ALIGN_LEFT);

    int16_t line_h = FONT_HEIGHT_MEDIUM + 2;
    int16_t y0 = y + 2;

    char line1[24];
    snprintf(line1, sizeof(line1), "Rx SOS da:%s", s_rx_sender);
    display->drawString(x + 2, y0, line1);
    y0 += line_h;

    uint32_t hh = (s_rx_ts % 86400) / 3600;
    uint32_t mm = (s_rx_ts % 3600) / 60;
    char line2[24];
    if (s_rx_estimated) {
        snprintf(line2, sizeof(line2), "t:%02u:%02u pos@%02u:%02u", hh, mm, s_rx_pos_hh, s_rx_pos_mm);
    } else {
        snprintf(line2, sizeof(line2), "time:%02u:%02u", hh, mm);
    }
    display->drawString(x + 2, y0, line2);
    y0 += line_h;

    char line3[24];
    snprintf(line3, sizeof(line3), "lat:%.6f", s_rx_lat);
    display->drawString(x + 2, y0, line3);
    y0 += line_h;

    char line4[24];
    snprintf(line4, sizeof(line4), "lon:%.6f", s_rx_lon);
    display->drawString(x + 2, y0, line4);
}

void SOSModule::drawAckFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    int16_t w = display->width();
    int16_t h = display->height();

    if (s_flash_state) {
        display->setColor(BLACK);
        display->fillRect(x, y, w, h);
        display->setColor(WHITE);
    } else {
        display->setColor(WHITE);
        display->fillRect(x, y, w, h);
        display->setColor(BLACK);
    }

    display->setFont(FONT_MEDIUM);
    display->setTextAlignment(TEXT_ALIGN_LEFT);

    int16_t line_h = FONT_HEIGHT_MEDIUM + 2;
    int16_t y0 = y + 2;

    char line1[32];
    snprintf(line1, sizeof(line1), "SOS ACK da %u nodi", s_ack_count);
    display->drawString(x + 2, y0, line1);
    y0 += line_h;

    char line2[32] = "da:";
    for (int i = 0; i < 3 && i < (int)s_ack_count; i++) {
        strncat(line2, " ", sizeof(line2) - strlen(line2) - 1);
        strncat(line2, s_ack_ids[i], sizeof(line2) - strlen(line2) - 1);
    }
    display->drawString(x + 2, y0, line2);
}

// ============================================================================
// BUZZER
// ============================================================================

void SOSModule::soundBuzzer()
{
#ifdef SOS_BUZZER_PASSIVE
    // Drive differenziale vero: pin A e pin B in push-pull opposto
    // Raddoppia la tensione sul piezo → suono più forte
    pinMode(SOS_BUZZER_PASSIVE_PIN_A, OUTPUT);
    pinMode(SOS_BUZZER_PASSIVE_PIN_B, OUTPUT);
    // tone() su pin A, pin B tenuto a VCC (opposto) per drive differenziale
    digitalWrite(SOS_BUZZER_PASSIVE_PIN_B, HIGH);
    tone(SOS_BUZZER_PASSIVE_PIN_A, SOS_BUZZER_FREQ_HZ);
#elif defined(SOS_BUZZER_ACTIVE)
    pinMode(SOS_BUZZER_PIN, OUTPUT);
    digitalWrite(SOS_BUZZER_PIN, HIGH);
#endif
    buzzer_active = true;
    buzzer_stop_time = millis() + SOS_BUZZER_DURATION_MS;
}

void SOSModule::stopBuzzer()
{
    if (!buzzer_active) return;
#ifdef SOS_BUZZER_PASSIVE
    noTone(SOS_BUZZER_PASSIVE_PIN_A);
    digitalWrite(SOS_BUZZER_PASSIVE_PIN_A, LOW);
    digitalWrite(SOS_BUZZER_PASSIVE_PIN_B, LOW);
#elif defined(SOS_BUZZER_ACTIVE)
    digitalWrite(SOS_BUZZER_PIN, LOW);
#endif
    buzzer_active = false;
}

// ============================================================================
// RETRY SOS
// ============================================================================

void SOSModule::runRetry()
{
    if (!sending_sos) return;

    uint32_t now = millis();

    // Se abbiamo ricevuto almeno 1 ACK, stop retry
    if (ack_tracker.count > 0) {
        sending_sos = false;
        return;
    }

    // Timeout retry
    if ((now - sos_send_time) >= SOS_ACK_TIMEOUT_MS) {
        if (retry_count < SOS_MAX_RETRIES) {
            retry_count++;
            LOG_INFO("SOS: ritrasmissione #%u", retry_count);
            sendSOSMessage();
            sos_send_time = now;
        } else {
            LOG_WARN("SOS: nessun ACK dopo %u tentativi, stop", SOS_MAX_RETRIES);
            sending_sos = false;
        }
    }
}

// ============================================================================
// LOOP PERIODICO (OSThread)
// ============================================================================

int32_t SOSModule::runOnce()
{
    uint32_t now = millis();

    // --- TIMER DISPLAY ---
    // Il timer viene gestito QUI, non nei callback del frame
    if (display_active) {
        // Aggiorna lampeggio
        if (now - last_flash_time >= SOS_FLASH_PERIOD_MS) {
            flash_state = !flash_state;
            s_flash_state = flash_state;
            last_flash_time = now;
        }
        // Controlla scadenza: chiudi l'alert dal thread, non dal callback
        if (now >= display_expire_time) {
            stopDisplay();
        }
    }

    // --- STOP BUZZER ---
    if (buzzer_active && now >= buzzer_stop_time) {
        stopBuzzer();
    }

    // --- POLLING PULSANTE ESTERNO GPIO8 (long-press: tieni premuto SOS_EXT_BUTTON_HOLD_MS) ---
    {
        static bool ext_last = true;
        static uint32_t ext_press_ms = 0;
        static bool ext_fired = false;
        static uint8_t ext_beep_count = 0;

        bool ext_pressed = (digitalRead(SOS_EXT_BUTTON_PIN) == LOW); // LOW = premuto (pull-up)

        if (ext_pressed && !ext_last) {
            // Fronte discesa: appena premuto
            ext_press_ms = now;
            ext_fired = false;
            ext_beep_count = 0;
            LOG_INFO("SOS GPIO%d: premuto, tieni %ums per SOS", SOS_EXT_BUTTON_PIN, SOS_EXT_BUTTON_HOLD_MS);
        }

        if (ext_pressed && !ext_fired) {
            uint32_t held = now - ext_press_ms;

            // Beep breve + display ogni secondo di tenuta
            uint8_t seconds_held = (uint8_t)(held / 1000);
            if (seconds_held > ext_beep_count) {
                ext_beep_count = seconds_held;
                // Beep breve di conferma (80ms)
                tone(SOS_BUZZER_PASSIVE_PIN_A, 2000, 80);
                // Mostra countdown sul display
                if (screen) {
                    static char prog_msg[32];
                    uint8_t secs_left = (SOS_EXT_BUTTON_HOLD_MS / 1000) - seconds_held;
                    snprintf(prog_msg, sizeof(prog_msg), "SOS in %us...", secs_left);
                    screen->startAlert(prog_msg);
                }
                LOG_INFO("SOS GPIO%d: tenuto %us / %us", SOS_EXT_BUTTON_PIN,
                         seconds_held, SOS_EXT_BUTTON_HOLD_MS / 1000);
            }

            if (held >= SOS_EXT_BUTTON_HOLD_MS) {
                ext_fired = true;
                // Chiudi il countdown prima di mostrare SOS INVIATO
                if (screen) {
                    screen->endAlert();
                    screen->setFrames();
                }
                LOG_INFO("SOS GPIO%d: soglia raggiunta, triggerSOS()", SOS_EXT_BUTTON_PIN);
                triggerSOS();
            }
        }

        if (!ext_pressed && ext_last) {
            // Rilasciato prima della soglia: ripristina display
            if (!ext_fired && ext_press_ms > 0 && (now - ext_press_ms) > 100) {
                if (screen) {
                    screen->endAlert();
                    screen->setFrames();
                }
                LOG_INFO("SOS GPIO%d: rilasciato prima della soglia, annullato", SOS_EXT_BUTTON_PIN);
            }
            ext_fired = false;
        }

        ext_last = ext_pressed;
    }

    // --- AGGIORNAMENTO POSIZIONE IN NVM ---
    if (nodeDB->hasLocalPositionSinceBoot()) {
        if (now - last_position_save_time >= SOS_POSITION_UPDATE_MIN_MS) {
            SOSPosition pos;
            if (acquirePosition(pos) && !pos.estimated) {
                saveLastPosition(pos);
            }
        }
    }

    // --- RETRY SOS ---
    runRetry();

    return 100; // Ogni 100ms per lampeggio e polling GPIO
}

// ============================================================================
// BOOT SCREEN - aggiunge indicazione EG-SOS-v6
// (Chiamato da UIRenderer::drawIconScreen - vedi patch UIRenderer)
// ============================================================================

const char *SOSModule::getVersionString()
{
    return SOS_VERSION_STRING;
}
