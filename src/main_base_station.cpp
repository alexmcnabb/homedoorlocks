#include <Arduino.h>
#include "packet_engine.h"

const int PIN_RF_CE = 3;
const int PIN_RF_SS = 2;
enum {LOCK = 0, UNLOCK = 1};

BaseStationPacketEngine packet_engine(PIN_RF_SS, PIN_RF_CE);
const uint8_t REMOTE_DEVICE_ID = 1;

void setup() {
    Serial.begin(115200);
    while(!Serial) {;}
    Serial.println("Starting startup");
    packet_engine.init(0x00ABCDEF);
    Serial.println("Done startup");
}

void loop() {
    while (Serial.available()) {
        uint8_t byte = Serial.read();
        Serial.println("Got command from python!");
        packet_engine.send_message(REMOTE_DEVICE_ID, byte);
        Serial.println("Done sending");
    }
    if (packet_engine.try_recv_message()) {
        if (packet_engine.received_device_id == REMOTE_DEVICE_ID) {
            Serial.println("Got message from door lock!");
            Serial.write((uint8_t)packet_engine.received_data);
        }
    }
}
