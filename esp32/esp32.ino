#include <ESP32-TWAI-CAN.hpp>
#include "driver/twai.h"

#define CAN_TX        4   
#define CAN_RX        5
#define HEARTBEAT_PIN 8

#define ID1           0x7A
#define ID2           0x7F

CanFrame rxFrame;

const uint32_t SEND_INTERVAL_MS = 5;   // keep your original 100ms

uint32_t lastSendTs = 0;
uint32_t lastHeartbeatTs = 0;
bool heartbeatState = false;

void printAlertsAndStatus() {
  // print alerts (your existing code)
  uint32_t alerts = 0;
  if (twai_read_alerts(&alerts, pdMS_TO_TICKS(0)) == ESP_OK && alerts) {
    Serial.printf("ALERTS: 0x%08X\n", alerts);
    if (alerts & TWAI_ALERT_BUS_OFF)       Serial.println("  -> ALERT: BUS_OFF");
    if (alerts & TWAI_ALERT_BUS_RECOVERED) Serial.println("  -> ALERT: BUS_RECOVERED");
    if (alerts & TWAI_ALERT_TX_FAILED)     Serial.println("  -> ALERT: TX_FAILED");
    if (alerts & TWAI_ALERT_TX_SUCCESS)    Serial.println("  -> ALERT: TX_SUCCESS");
    if (alerts & TWAI_ALERT_ABOVE_ERR_WARN)Serial.println("  -> ALERT: ABOVE_ERR_WARN");
    if (alerts & TWAI_ALERT_BUS_ERROR)     Serial.println("  -> ALERT: BUS_ERROR");
    if (alerts & TWAI_ALERT_RX_QUEUE_FULL) Serial.println("  -> ALERT: RX_QUEUE_FULL");
  }

  // always read status and print TEC/REC/state
  twai_status_info_t s;
  if (twai_get_status_info(&s) == ESP_OK) {
    const char *state_str = "UNKNOWN";
    switch (s.state) {
      case TWAI_STATE_STOPPED:   state_str = "STOPPED";   break;
      case TWAI_STATE_RUNNING:   state_str = "RUNNING";   break;
      case TWAI_STATE_RECOVERING:state_str = "RECOVERING";break;
      case TWAI_STATE_BUS_OFF:   state_str = "BUS_OFF";   break;
    }
    Serial.printf("STATUS: state=%s(%d)  TEC=%u  REC=%u  msgs_to_tx=%u  msgs_to_rx=%u\n",
                  state_str, s.state, s.tx_error_counter, s.rx_error_counter,
                  s.msgs_to_tx, s.msgs_to_rx);
  } else {
    Serial.println("Failed to read TWAI status");
  }
}


void setup() {
  Serial.begin(115200);
  twai_filter_config_t fConfig;
  memset(&fConfig, 0, sizeof(fConfig));
  fConfig.acceptance_code = (0x000u << 21);                    // 0x00000000
  fConfig.acceptance_mask = (~(0x700u << 21)) & 0xFFFFFFFFu;   // 0x1FFFFFFF
  fConfig.single_filter   = true; 
  pinMode(HEARTBEAT_PIN, OUTPUT);

  digitalWrite(HEARTBEAT_PIN, HIGH);

  if (ESP32Can.begin(ESP32Can.convertSpeed(500), CAN_TX, CAN_RX, 9, 1, &fConfig, nullptr, nullptr)) {
    Serial.println("Can Bus started!");
  } else {
    Serial.println("Can Bus failed!");
    digitalWrite(HEARTBEAT_PIN, LOW);
    while(1);
  }

  xTaskCreate(canTask, "CAN-TX", 4096, NULL, 2, NULL);

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
  // if (now - lastSendTs >= (1000*SEND_INTERVAL_MS)) {
  //   lastSendTs = now;
  //   sendPeriodicFrame();
  // }

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

  printAlertsAndStatus();
  delay(1);
}


void canTask(void *arg)
{
    const TickType_t period = pdMS_TO_TICKS(SEND_INTERVAL_MS);
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        sendPeriodicFrame();
        vTaskDelayUntil(&last, period);
    }
}
