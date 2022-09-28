#pragma once
#include <stdint.h>

// This radio supports several features we're not using here like auto-ack and retransmit, multiple data pipes, etc
// Currently only supports a single data pipe with a 3 byte address shared between each end

// sspin = CSN input = SPI slave select
// cepin = CE input = Chip enable for RX/TX
// power_pin = Power supply to radio. Active low. Set to zero to ignore.

// The radio power pin will need a pulldown, as the radio will lock up the SPI bus otherwise.
// The init(), turnOff() functions will manage power. Do not use turnOn()

// If not using a power pin, use turnOn() and turnOff() to power up and down the radio. These might not work right.
// I've seen these radios sometimes refuse work properly out of power_down, or after re-running init()


// This seems to match the BK2425 pretty closely - https://docs.ai-thinker.com/_media/2.4g/docs/bk2425_datasheet_v1.3.pdf

class RF75 {
public:
	RF75(uint16_t sspin, uint16_t cepin, uint16_t power_pin, uint16_t power_disc_pin);
	void init(uint32_t address); // address is 3 bytes, msb should be zero
	bool available(); // Returns true if something is in the RX fifo
	void recievePacket(uint8_t *buff); // Length specified from setModeRX()
	void sendPacket(uint8_t* payload, uint8_t len);
	void turnOff(); // Set to power-down mode
	void turnOn(); // Bring back out of power-down mode
	void setModeTX(); // Prepare for sending data
	void setModeRX(uint8_t payload_len); // Begin recieving data
	uint8_t carrier_detect();
private:
	uint16_t RF_PIN_SS, RF_PIN_CE, POWER_PIN, POWER_DESC_PIN;
	void setCE();
	void clearCE();
	void setSS();
	void clearSS();

	uint8_t readRegVal(uint8_t cmd);
	void writeRegVal(uint8_t cmd, uint8_t val);
	void writeRegBuf(uint8_t * cmdbuf, uint8_t len);
	void writeRegBufPgm(uint8_t * cmdbuf, uint8_t len);
	void readRegBuf(uint8_t reg, uint8_t * buf, uint8_t len);
	uint8_t writeRegCmdBuf(uint8_t cmd, uint8_t * buf, uint8_t len);

	void selectBank(uint8_t bank);
	void setChannel(uint8_t cnum);
	uint8_t getChannel();
	void setPower(uint8_t pwr);
	void flushTxFIFO();
	void flushRxFIFO();
	uint8_t read_status();
	void activate();
};



