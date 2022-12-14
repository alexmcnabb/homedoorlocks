#include <Arduino.h>

#include "packet_engine.h"
#include "eeprom.h"
#include "device_keys.h"

#define MAX_HASH_MATCH_TRIES 50
#define EEP_TOKEN_ADDRESS(device_id) (device_id - 1) * (64+4)
// First hash, for remote sends, base receives
#define EEP_KEY_ADDRESS_REMOTE_SEND 4
#define EEP_KEY_ADDRESS_BASE_RECEIVE(device_id) (device_id - 1) * (64+4) + 4
// Second hash, for base sends, remote receives
#define EEP_KEY_ADDRESS_BASE_SEND(device_id) (device_id - 1) * (64+4) + (4 + 32)
#define EEP_KEY_ADDRESS_REMOTE_RECEIVE 4 + 32


void print_buffer(String header, const uint8_t *buffer, uint8_t len) {
    Serial.print(header);
    for (uint8_t i = 0; i < len; i++) {
        Serial.print(buffer[i], HEX);
        Serial.print(", ");
    }
    Serial.println("");
}

SHA256 sha256;
uint8_t hash_buffer[32];
void update_hash(uint8_t* buffer) {
    sha256.reset();
    sha256.update(buffer, 32);
    sha256.finalize(buffer, 32);
}

void create_message(uint32_t payload, uint8_t device_id, uint8_t* message_buffer, uint16_t hash_eeprom_address) {
    message_buffer[0] = device_id;
    EEPROM.readArray(hash_buffer, 32, hash_eeprom_address);
    update_hash(hash_buffer);
    memcpy(&message_buffer[1], hash_buffer, 16);
    for(uint8_t i = 0; i < 4; i++)
        message_buffer[i + 1] ^= (payload >> (8 * i)) & 0xFF;
    print_buffer("Message after adding payload: ", message_buffer, 17);
    EEPROM.writeArray(hash_buffer, 32, hash_eeprom_address);
}

bool test_and_decode_payload(uint32_t &received_data, uint8_t* message_buffer, uint16_t hash_eeprom_address) {
    print_buffer("Raw recieved message: ", message_buffer, 17);
    EEPROM.readArray(hash_buffer, 32, hash_eeprom_address);
    for (int i = 0; i < MAX_HASH_MATCH_TRIES; i++) {
        update_hash(hash_buffer);
        if (!memcmp(&message_buffer[5], &hash_buffer[4], 12)) { // Compare the last 12 bytes of the bytes in the hash to make sure things match
            EEPROM.writeArray(hash_buffer, 32, hash_eeprom_address); // Update the saved hash for next time
            received_data = 0;
            for(uint8_t i = 0; i < 4; i++) {
                received_data |= (message_buffer[1 + i] ^ hash_buffer[i]) << (8 * i);
            }
            return true;
        } else {
            Serial.print("Message authentication failed!");
        }
    }
    return false;
}


#ifdef DEVICE_ID // ----------------------------------Remote Specific Code-----------------------------------
RemotePacketEngine::RemotePacketEngine(uint16_t sspin, uint16_t cepin, uint16_t power_pin, uint16_t power_disc_pin) : rf75(sspin, cepin, power_pin, power_disc_pin) { }

void RemotePacketEngine::init(uint32_t address) {
    pipe_address = address;
    // Don't bother initializing the radio here. We initialize the radio when turning it on to recieve or transmit
    if (HASH_TOKEN != EEPROM.readUint32(0)) {
            Serial.println("Overwriting base hashes!");
        EEPROM.writeArray(&BASE_HASH[0], 64, 4);
        EEPROM.writeUint32(HASH_TOKEN, 0);
    }
}
void RemotePacketEngine::send_message(uint32_t data) {
    rf75.init(pipe_address);
    rf75.setModeTX();
    create_message(data, DEVICE_ID, buffer, EEP_KEY_ADDRESS_REMOTE_SEND);
    rf75.sendPacket(buffer, 17);
    rf75.turnOff();
}
bool RemotePacketEngine::try_recv_message() {
    rf75.init(pipe_address);
    rf75.setModeRX(17);
    delay(2);
    bool success = false;
    if (rf75.available()) {
        rf75.recievePacket(buffer);
        if (buffer[0] != DEVICE_ID) {
            Serial.println("Device ID doesn't match, discarding.");
        } else {
            success = test_and_decode_payload(received_data, buffer, EEP_KEY_ADDRESS_REMOTE_RECEIVE);
        }
    }
    rf75.turnOff();
    return success;
}

#else // --------------------------------Base Station Specific Code----------------------------------
BaseStationPacketEngine::BaseStationPacketEngine(uint16_t sspin, uint16_t cepin) : rf75(sspin, cepin, 0, 0) { }

void BaseStationPacketEngine::init(uint32_t address) {
    pipe_address = address;
    rf75.init(pipe_address);
    rf75.setModeRX(17);
    for (int i = 0; i < REMOTE_COUNT; i++) {
        if (HASH_TOKENS[i] != EEPROM.readUint32(EEP_TOKEN_ADDRESS(i + 1))) {
            Serial.println("Overwriting base hashes!");
            EEPROM.writeArray(&BASE_HASHS[i][0], 64, EEP_KEY_ADDRESS_BASE_RECEIVE(i + 1));
            EEPROM.writeUint32(HASH_TOKENS[i], EEP_TOKEN_ADDRESS(i + 1));
        }
    }        
}
void BaseStationPacketEngine::send_message(uint8_t device_id, uint32_t data) {
    rf75.setModeTX();
    create_message(data, device_id, buffer, EEP_KEY_ADDRESS_BASE_SEND(device_id));
    uint32_t starttime = millis();
    while (millis() - starttime < 1000) {
        rf75.sendPacket(buffer, 17);
    }
    rf75.init(pipe_address);
    rf75.setModeRX(17);
}
bool BaseStationPacketEngine::try_recv_message() {
    if (rf75.available()) {
        rf75.recievePacket(buffer);
        received_device_id = buffer[0];
        return test_and_decode_payload(received_data, buffer,  EEP_KEY_ADDRESS_BASE_RECEIVE(received_device_id));
    }
    return false;
}
#endif