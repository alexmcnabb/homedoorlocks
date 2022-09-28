#pragma once

//This is the length of the EEPROM on the ATMega168
// This code should work properly on the ATMega168, 328, and 32u4, maybe the attinys too
#define EEPROM_SIZE 512

class EEPROMClass {
public:
	uint8_t readByte(uint16_t addr);
	void writeByte(uint8_t data, uint16_t addr);
	void readArray(uint8_t data[], uint16_t length, uint16_t baseaddr);
	void writeArray(const uint8_t data[], uint16_t length, uint16_t baseaddr);
	uint32_t readUint32(uint16_t baseaddr);
	void writeUint32(uint32_t data, uint16_t baseaddr);
private:
	void setAddr(uint16_t addr);
};

extern EEPROMClass EEPROM;