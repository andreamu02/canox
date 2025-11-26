#include <ESP32-TWAI-CAN.hpp>
#include "driver/twai.h"

#define CAN_TX        4   
#define CAN_RX        5
#define HEARTBEAT_PIN 8

#define ID1           0x110
#define ID2           0x111

CanFrame rxFrame;

const uint32_t SEND_INTERVAL_MS = 1;   // keep your original 100ms

uint32_t lastSendTs = 0;
uint32_t lastHeartbeatTs = 0;
bool heartbeatState = false;


void setup() {
  twai_filter_config_t fConfig;
  memset(&fConfig, 0, sizeof(fConfig));
  fConfig.acceptance_code = (0x000u << 21);                    // 0x00000000
  fConfig.acceptance_mask = (~(0x000u << 21)) & 0x00000000u;   // 0x1FFFFFFF
                                                               //
  fConfig.single_filter   = true; 
  pinMode(HEARTBEAT_PIN, OUTPUT);

  digitalWrite(HEARTBEAT_PIN, HIGH);

  if (ESP32Can.begin(ESP32Can.convertSpeed(500), CAN_TX, CAN_RX, 9, 1, &fConfig, nullptr, nullptr)) {
  } else {
    digitalWrite(HEARTBEAT_PIN, LOW);
    while(1);
  }

}

bool id1 = true;

void sendPeriodicFrame() {
  CanFrame tx = {0};
  tx.identifier = id1 ? ID1 : ID2;
  tx.extd = 0;
  tx.data_length_code = 3;
  tx.data[0] = id1 ? 0x11 : 0x44;
  tx.data[1] = id1 ? 0x22 : 0x55;
  tx.data[2] = id1 ? 0x33 : 0x66;

  esp_err_t res = ESP32Can.writeFrame(tx); 

  id1 = !id1;
}

void loop() {
  uint32_t now = micros();

  // periodic send
  if (now - lastSendTs >= (1000*SEND_INTERVAL_MS)) {
    lastSendTs = now;
    sendPeriodicFrame();
  }

  // heartbeat blink if not error-passive or bus-off
  twai_status_info_t s;
  bool allowedHeartbeat = true;
  if (twai_get_status_info(&s) == ESP_OK) {
    bool errorPassive = (s.tx_error_counter >= 128) || (s.rx_error_counter >= 128);
    bool busOff = (s.state == TWAI_STATE_BUS_OFF);
    if (errorPassive || busOff) allowedHeartbeat = false;
  }

  if (allowedHeartbeat) {
    if (now - lastHeartbeatTs >= 500) {
      lastHeartbeatTs = now;
      heartbeatState = !heartbeatState;
      digitalWrite(HEARTBEAT_PIN, heartbeatState ? HIGH : LOW);
    }
  } else {
    digitalWrite(HEARTBEAT_PIN, HIGH);
  }
}
