#pragma once
#include <stdint.h>
#include <Crypto.h>
#include <SHA256.h>
#include <AES.h>
#include "RF75.h"

class BaseStationPacketEngine {
public:
	BaseStationPacketEngine(uint16_t sspin, uint16_t cepin);
	void init(uint32_t address);
	void send_message(uint8_t device_id, uint32_t data);
	bool try_recv_message(); // See if message is available and receive it
	uint32_t received_data; // filled in by try_recv_message if successful
	uint8_t received_device_id;
private:
	RF75 rf75;
	uint32_t pipe_address;
	uint8_t buffer[17]; // Used for tx and rx data processing
};

class RemotePacketEngine {
public:
	RemotePacketEngine(uint16_t sspin, uint16_t cepin, uint16_t power_pin, uint16_t power_disc_pin);
	void init(uint32_t address);
	void send_message(uint32_t data);
	bool try_recv_message(); // See if message is available and receive it
	uint32_t received_data; // filled in by try_recv_message if successful
private:
	RF75 rf75;
	uint32_t pipe_address;
	uint8_t buffer[17]; // Used for tx and rx data processing
};