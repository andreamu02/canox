#include <ESP32-TWAI-CAN.hpp>

#define CAN_TX  4   // <--- choose a free GPIO on your Super Mini
#define CAN_RX  5

CanFrame rxFrame;

void setup() {
  Serial.begin(115200);
  ESP32Can.setPins(CAN_TX, CAN_RX);

  ESP32Can.setSpeed(ESP32Can.convertSpeed(500));

  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  if (ESP32Can.begin()) {
    Serial.println("Can Bus started!");
  } else {
    Serial.println("Can Bus failed!");
    digitalWrite(8, LOW);
    while(1);
  }

}

void loop() {
  static uint32_t last = 0;
  static uint32_t last_blink = 0;
  if (millis() - last > 100) {
    last = millis();
    CanFrame tx = {0};
    tx.identifier = 0x123;
    tx.extd = 0;
    tx.data_length_code = 3;
    tx.data[0] = 0x11;
    tx.data[1] = 0x22;
    tx.data[2] = 0x33;
    ESP32Can.writeFrame(tx);
  }

  if (millis() - last_blink > 500) {
    digitalWrite(8, !digitalRead(8));
    last_blink = millis();
  }

  if (ESP32Can.readFrame(rxFrame, 10)) {
    Serial.printf("RX ID: %03X len:%d\n", rxFrame.identifier, rxFrame.data_length_code);
    for (int i = 0; i<rxFrame.data_length_code; i++) Serial.printf("%02X ", rxFrame.data[i]);
    Serial.println();
  }
  uint32_t alerts = 0;
  if (twai_read_alerts(&alerts, pdMS_TO_TICKS(0)) == ESP_OK) {
    if (alerts & TWAI_ALERT_BUS_OFF) {
      Serial.println("CIAO");
    }
  }
  Serial.println(alerts);
}
