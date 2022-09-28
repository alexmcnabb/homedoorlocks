#include <stdint.h>
#include <Arduino.h>
#include "eeprom.h"

uint8_t EEPROMClass::readByte(uint16_t addr){
	setAddr(addr);
	while (EECR & (1<<EEPE));//Wait til any previous operation done
	EECR |= (1<<EERE);//Start Read
	return EEDR;//Return read data
}
void EEPROMClass::writeByte(uint8_t data, uint16_t addr){
	setAddr(addr);
	EEDR = data;//Set the new data
	while (EECR & (1<<EEPE));//Wait til any previous operation done
	cli();//Disable interrupts
	EECR |= (1<<EEMPE);//Set MasterEnable
	EECR |= (1<<EEPE);//Set Enable to begin write
	while (EECR & (1<<EEPE));//Wait til write done
	sei();	
}

void EEPROMClass::setAddr(uint16_t addr){
	EEARL = addr;
	EEARH = addr >> 8;
}

void EEPROMClass::writeArray(const uint8_t data[], uint16_t length, uint16_t baseaddr){
	for(uint16_t i = 0; i < length; i++)
		writeByte(data[i], baseaddr + i);
}

void EEPROMClass::readArray(uint8_t data[], uint16_t length, uint16_t baseaddr){
	for(uint16_t i = 0; i < length; i++)
		data[i] = readByte(baseaddr + i);
}

uint32_t EEPROMClass::readUint32(uint16_t baseaddr){
	uint32_t result = 0;
	for(uint8_t i = 0; i < 4; i++) {
		result |= (uint32_t)readByte(baseaddr + i) << (8 * i);
	}
	return result;
}

void EEPROMClass::writeUint32(uint32_t data, uint16_t baseaddr){
	for(uint8_t i = 0; i < 4; i++) {
		writeByte((data >> (8 * i)) & 0xFF, baseaddr + i);
	}
}

EEPROMClass EEPROM;