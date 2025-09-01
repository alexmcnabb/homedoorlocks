#include <Arduino.h>
#include "packet_engine.h"
#include "atmega32u4_custom_pindefs.h"

const int PIN_RF_CE = PIN_PD0;
const int PIN_RF_SS = PIN_PD1;
// IRQ is connected to PIN_PD2 and not used
const int PIN_RF_POWER = PIN_PE6;
const int PIN_RF_POWER_RAIL = PIN_PC6;

enum {LOCK = 0, UNLOCK = 1};
const uint8_t DEV_ID_BACK = 2;
const uint8_t DEV_ID_FRONT = 1;

BaseStationPacketEngine packet_engine(PIN_RF_SS, PIN_RF_CE, PIN_RF_POWER, PIN_RF_POWER_RAIL);

void setup() {
    Serial.begin(115200);
    while(!Serial) {;} // Wait for serial to be connected before operating
    Serial.println("Basestation starting startup");
    packet_engine.init(0x00ABCDEF);
    Serial.println("Done startup");
}

void loop() {
    while (Serial.available()) {
        uint8_t byte = Serial.read();
        Serial.print("Got byte from python: 0x");
        Serial.println(byte, HEX);
        if (byte && 0b11111100) {
            Serial.println("Invalid command, rejecting");
            continue;
        }
        uint8_t remote_device_id;
        if (byte & 0b00000010) {
            remote_device_id = DEV_ID_BACK;
            Serial.println("back door");
        } else {
            remote_device_id = DEV_ID_FRONT;
            Serial.println("front door");
        }
        Serial.print("Command is ");
        Serial.println(byte & 0x01);
        packet_engine.send_message(remote_device_id, byte & 0x01);
        Serial.println("Done sending");
    }

    if (packet_engine.try_recv_message()) {
        uint8_t payload = (uint8_t)packet_engine.received_data;
        if (packet_engine.received_device_id == DEV_ID_BACK) {
            Serial.println("Got message from back door lock!");
            Serial.write(payload | 0x02);
        }       
        if (packet_engine.received_device_id == DEV_ID_FRONT) {
            Serial.println("Got message from front door lock!");
            Serial.write(payload | 0x00);
        }
    }
}
