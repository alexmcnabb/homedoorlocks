#include <Arduino.h>

#include "packet_engine.h"
#include "eeprom.h"
#include "device_keys.h"

#define MAX_HASH_MATCH_TRIES 40

// EEPROM Layout:
// Offset (i * 64) + 0 (4 bytes) : hash token
// Offset (i * 64) + 4 (32 bytes) : transmitted message hash
// Offset (i * 64) + 36 (32 bytes) : received message hash
// Where i is device_id-1 on the basestation, and 0 on the remote

#define PAYLOAD_BYTES 1
#define MSG_LEN 8 // Complete message
#define BODY_BYTES (MSG_LEN - 1) // payload + hash, not including dev id

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
    // print_buffer("BeforeHash", buffer, 32);
    sha256.reset();
    sha256.update(buffer, 32);
    sha256.finalize(buffer, 32);
    // print_buffer("AfterHash", buffer, 32);
}

void create_message(uint32_t payload, uint8_t device_id, uint8_t* message_buffer, uint16_t hash_eeprom_address) {
    message_buffer[0] = device_id;
    EEPROM.readArray(hash_buffer, 32, hash_eeprom_address);
    update_hash(hash_buffer);
    memcpy(&message_buffer[1], hash_buffer, BODY_BYTES);
    for(uint8_t i = 0; i < PAYLOAD_BYTES; i++)
        message_buffer[i + 1] ^= (payload >> (8 * i)) & 0xFF;
    print_buffer("Message after adding payload: ", message_buffer, MSG_LEN);
    EEPROM.writeArray(hash_buffer, 32, hash_eeprom_address);
}

bool test_and_decode_payload(uint32_t &received_data, uint8_t* message_buffer, uint16_t hash_eeprom_address) {
    print_buffer("Raw recieved message: ", message_buffer, MSG_LEN);
    EEPROM.readArray(hash_buffer, 32, hash_eeprom_address);
    Serial.print("Testing");
    for (int i = 0; i < MAX_HASH_MATCH_TRIES; i++) {
        update_hash(hash_buffer);
        Serial.print(".");
        if (!memcmp(&message_buffer[1 + PAYLOAD_BYTES], &hash_buffer[PAYLOAD_BYTES], BODY_BYTES - PAYLOAD_BYTES)) { // Compare the bytes following the payload to make sure things match
            EEPROM.writeArray(hash_buffer, 32, hash_eeprom_address); // Update the saved hash for next time
            received_data = 0;
            for(uint8_t j = 0; j < PAYLOAD_BYTES; j++) {
                received_data |= (message_buffer[1 + j] ^ hash_buffer[j]) << (8 * j);
            }
            Serial.println();
            Serial.print("Test passed after ");
            Serial.print(i + 1);
            Serial.println(" tries");
            return true;
        }
    }
    Serial.println();
    Serial.println("Failed to match hash");
    return false;
}


#ifdef DEVICE_ID // ----------------------------------Remote Specific Code-----------------------------------
RemotePacketEngine::RemotePacketEngine(uint16_t sspin, uint16_t cepin, uint16_t power_pin, uint16_t power_rail_pin) : rf75(sspin, cepin, power_pin, power_rail_pin) { }

void RemotePacketEngine::init(uint32_t address) {
    pipe_address = address;
    // Don't bother initializing the radio here. We initialize the radio when turning it on to recieve or transmit
    if (HASH_TOKEN != EEPROM.readUint32(0)) {
            Serial.println("Overwriting base hashes");
        EEPROM.writeArray(&BASE_HASH[0], 64, 4);
        EEPROM.writeUint32(HASH_TOKEN, 0);
    }
}
void RemotePacketEngine::send_message(uint32_t data) {
    rf75.init(pipe_address);
    rf75.setModeTX();
    create_message(data, DEVICE_ID, buffer, EEP_KEY_ADDRESS_REMOTE_SEND);
    rf75.sendPacket(buffer, MSG_LEN);
    rf75.turnOff();
}
bool RemotePacketEngine::try_recv_message() {
    rf75.init(pipe_address);
    rf75.setModeRX(MSG_LEN);
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
BaseStationPacketEngine::BaseStationPacketEngine(uint16_t sspin, uint16_t cepin, uint16_t power_pin, uint16_t power_rail_pin) : rf75(sspin, cepin, power_pin, power_rail_pin) { }

void BaseStationPacketEngine::init(uint32_t address) {
    pipe_address = address;
    rf75.init(pipe_address);
    rf75.setModeRX(MSG_LEN);
    for (int i = 0; i < REMOTE_COUNT; i++) {
        if (HASH_TOKENS[i] != EEPROM.readUint32(EEP_TOKEN_ADDRESS(i + 1))) {
            Serial.print("Overwriting base hashes for device ID ");
            Serial.println(i + 1);
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
        rf75.sendPacket(buffer, MSG_LEN);
    }
    rf75.init(pipe_address);
    rf75.setModeRX(MSG_LEN);
}
bool BaseStationPacketEngine::try_recv_message() {
    if (rf75.available()) {
        rf75.recievePacket(buffer);
        received_device_id = buffer[0];
        return test_and_decode_payload(received_data, buffer,  EEP_KEY_ADDRESS_BASE_RECEIVE(received_device_id));
    }
    return false;
}

void BaseStationPacketEngine::reboot_radio() {
    rf75.init(pipe_address);
    rf75.setModeRX(MSG_LEN);
}

#endif