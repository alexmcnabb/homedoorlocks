#include "RF75.h"
#include <SPI.h>

// SPI commands
#define RFM73_CMD_READ_REG 0x00 // Define read command to register - OR with the register address
#define RFM73_CMD_WRITE_REG 0x20 // Define write command to register - OR with the register address
#define RFM73_CMD_RD_RX_PLOAD 0x61 // Define RX payload command
#define RFM73_CMD_WR_TX_PLOAD 0xA0 // Define TX payload command
#define RFM73_CMD_FLUSH_TX 0xE1 // Define flush TX register command
#define RFM73_CMD_FLUSH_RX 0xE2 // Define flush RX register command
#define RFM73_CMD_REUSE_TX_PL 0xE3 // Define reuse TX payload register command
#define RFM73_CMD_W_TX_PAYLOAD_NOACK 0xb0 // Define TX payload NOACK command
#define RFM73_CMD_W_ACK_PAYLOAD 0xa8 // Define Write ack command
#define RFM73_CMD_ACTIVATE 0x50 // Define feature activation command
#define RFM73_CMD_RX_PL_WID 0x60 // Define received payload width command
#define RFM73_CMD_NOP_NOP 0xFF // Define No Operation, might be used to read status register

// Register addresses
#define RFM73_REG_CONFIG 0x00 // 'Config' register address
#define RFM73_REG_EN_AA 0x01 // 'Enable Auto Acknowledgment' register address
#define RFM73_REG_EN_RXADDR 0x02 // 'Enabled RX addresses' register address
#define RFM73_REG_SETUP_AW 0x03 // 'Setup address width' register address
#define RFM73_REG_SETUP_RETR 0x04 // 'Setup Auto. Retrans' register address
#define RFM73_REG_RF_CH 0x05 // 'RF channel' register address
#define RFM73_REG_RF_SETUP 0x06 // 'RF setup' register address
#define RFM73_REG_STATUS 0x07 // 'Status' register address
#define RFM73_REG_OBSERVE_TX 0x08 // 'Observe TX' register address
#define RFM73_REG_CD 0x09 // 'Carrier Detect' register address
#define RFM73_REG_RX_ADDR_P0 0x0A // 'RX address pipe0' register address
#define RFM73_REG_RX_ADDR_P1 0x0B // 'RX address pipe1' register address
#define RFM73_REG_RX_ADDR_P2 0x0C // 'RX address pipe2' register address
#define RFM73_REG_RX_ADDR_P3 0x0D // 'RX address pipe3' register address
#define RFM73_REG_RX_ADDR_P4 0x0E // 'RX address pipe4' register address
#define RFM73_REG_RX_ADDR_P5 0x0F // 'RX address pipe5' register address
#define RFM73_REG_TX_ADDR 0x10 // 'TX address' register address
#define RFM73_REG_RX_PW_P0 0x11 // 'RX payload width, pipe0' register address
#define RFM73_REG_RX_PW_P1 0x12 // 'RX payload width, pipe1' register address
#define RFM73_REG_RX_PW_P2 0x13 // 'RX payload width, pipe2' register address
#define RFM73_REG_RX_PW_P3 0x14 // 'RX payload width, pipe3' register address
#define RFM73_REG_RX_PW_P4 0x15 // 'RX payload width, pipe4' register address
#define RFM73_REG_RX_PW_P5 0x16 // 'RX payload width, pipe5' register address
#define RFM73_REG_FIFO_STATUS 0x17 // 'FIFO Status Register' register address
#define RFM73_REG_DYNPD 0x1C // 'Enable dynamic payload length' register address
#define RFM73_REG_FEATURE 0x1D // 'Feature' register address

// // Interrupts
#define RFM73_IRQ_STATUS_RX_DR 0x40 // Status bit RX_DR IRQ
#define RFM73_IRQ_STATUS_TX_DS 0x20 // Status bit TX_DS IRQ
#define RFM73_IRQ_STATUS_MAX_RT 0x10 // Status bit MAX_RT IRQ
#define RFM73_IRQ_STATUS_TX_FULL 0x01


// FIFO status
#define RFM73_FIFO_STATUS_TX_REUSE 0x40
#define RFM73_FIFO_STATUS_TX_FULL 0x20
#define RFM73_FIFO_STATUS_TX_EMPTY 0x10
#define RFM73_FIFO_STATUS_RX_FULL 0x02
#define RFM73_FIFO_STATUS_RX_EMPTY 0x01

//Commands
const uint8_t PROGMEM RFM73_cmd_switch_cfg[] = { 0x50, 0x53 };
const uint8_t PROGMEM RFM73_cmd_flush_rx[] = { 0xe2, 0x00 };
const uint8_t PROGMEM RFM73_cmd_flush_tx[] = { 0xe1, 0x00 };
const uint8_t PROGMEM RFM73_cmd_activate[] = { 0x50, 0x73 };
const uint8_t PROGMEM RFM73_cmd_tog1[] = { (0x20 | 0x04), 0xF9 | 0x06, 0x96, 0x8A, 0xDB }; // For 256kbps - toggles bits 25, 26 in bank 1
const uint8_t PROGMEM RFM73_cmd_tog2[] = { (0x20 | 0x04), 0xF9 & ~0x06, 0x96, 0x8A, 0xDB };
// const uint8_t PROGMEM RFM73_cmd_tog1[] = { (0x20 | 0x04), 0xF8, 0x96, 0x8A, 0xDB }; // For 256kbps - toggles bits 4-8 in bank 1
// const uint8_t PROGMEM RFM73_cmd_tog2[] = { (0x20 | 0x04), 0xF9, 0x96, 0x8A, 0xDB };

const uint8_t PROGMEM RFM73_bank0Init[][2] = {
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_CONFIG), 0x03 }, //Disable CRC ,CRC=1
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_EN_AA), 0x00 }, //Disable auto ack
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_EN_RXADDR), 0x01 }, //Enable RX Address pipe 0
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_SETUP_AW), 0x01 }, //RX/TX address field width 3 bytes
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_SETUP_RETR), 0x00 }, //No auto retransmit
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_RF_SETUP), 0x27 }, // 256kbps, LNA gain high, 5dBM
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_STATUS), 0x70 }, // Clear all interrupts
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_DYNPD), 0x00 }, //Dynamic payload disabled
	{ (RFM73_CMD_WRITE_REG | RFM73_REG_FEATURE), 0x01 }  //Enable payload without ack (This one requres the chip to be 'activated')
};

const uint8_t PROGMEM RFM73_bank1Init[][5] = {
	{ (RFM73_CMD_WRITE_REG | 0x00), 0x40, 0x4B, 0x01, 0xE2 },
	{ (RFM73_CMD_WRITE_REG | 0x01), 0xC0, 0x4B, 0x00, 0x00 },
	{ (RFM73_CMD_WRITE_REG | 0x02), 0xD0, 0xFC, 0x8C, 0x02 },
	{ (RFM73_CMD_WRITE_REG | 0x03), 0x99, 0x00, 0x39, 0x21 }, // I had 41 instead of 21 for some reason
	{ (RFM73_CMD_WRITE_REG | 0x04), 0xF9, 0x96, 0x8A, 0xDB }, // 250Kb/s, max power
	{ (RFM73_CMD_WRITE_REG | 0x05), 0x24, 0x06, 0x0F, 0xB6 }, // 250Kb/s
	{ (RFM73_CMD_WRITE_REG | 0x06), 0x00, 0x00, 0x00, 0x00 },
	{ (RFM73_CMD_WRITE_REG | 0x07), 0x00, 0x00, 0x00, 0x00 },
	{ (RFM73_CMD_WRITE_REG | 0x08), 0x00, 0x00, 0x00, 0x00 },
	{ (RFM73_CMD_WRITE_REG | 0x09), 0x00, 0x00, 0x00, 0x00 },
	{ (RFM73_CMD_WRITE_REG | 0x0a), 0x00, 0x00, 0x00, 0x00 },
	{ (RFM73_CMD_WRITE_REG | 0x0b), 0x00, 0x00, 0x00, 0x00 },
	{ (RFM73_CMD_WRITE_REG | 0x0C), 0x00, 0x12, 0x73, 0x05 }, // 05 instead of 00 to add 10us PLL time according to online library
	{ (RFM73_CMD_WRITE_REG | 0x0D), 0x36, 0xb4, 0x80, 0x00 } // 250Kb/s
};
//Bank1 register 14
const uint8_t PROGMEM RFM73_bank1R0EInit[] = {
	(0x20 | 0x0E), 0x41, 0x20, 0x08, 0x04, 0x81, 0x20, 0xCF, 0xF7, 0xFE, 0xFF, 0xFF
};

RF75::RF75(uint16_t sspin, uint16_t cepin, uint16_t power_pin, uint16_t power_rail_pin) : RF_PIN_SS(sspin), RF_PIN_CE(cepin), POWER_PIN(power_pin), POWER_RAIL_PIN(power_rail_pin) {}

void RF75::init(uint32_t address) {
	pinMode(RF_PIN_SS, OUTPUT);
	pinMode(RF_PIN_CE, OUTPUT);
	if (POWER_PIN) {
        pinMode(POWER_PIN, OUTPUT);
		SPI.end();
		setSS(); // Active high, so need to set low to allow radio to shut down
		clearCE();
		digitalWrite(13, LOW); // Set SCK and MOSI low while the radio is off so that they dont source any current
		digitalWrite(11, LOW);
        digitalWrite(POWER_PIN, HIGH);
		digitalWrite(POWER_RAIL_PIN, LOW);
		pinMode(POWER_RAIL_PIN, OUTPUT);
        delayMicroseconds(600); // 200 seems long enough with 100n capacitance, 600 is enough for a 1u cap
		pinMode(POWER_RAIL_PIN, INPUT);
		// digitalWrite(POWER_RAIL_PIN, HIGH);
		clearSS();
        digitalWrite(POWER_PIN, LOW);
		SPI.begin();
        delay(18); // This seems long enough that the radio is initialized in time. If too small, the status reg will return all zeros.
	} else { // Initialize it assuming it's already been powered on
		clearCE();
		clearSS();
		SPI.begin();
	}
	// Init bank 0 registers
	selectBank(0);
	activate();
	for (uint8_t i = 0; i < sizeof(RFM73_bank0Init) / 2; i++)
		writeRegVal(pgm_read_byte(&RFM73_bank0Init[i][0]), pgm_read_byte(&RFM73_bank0Init[i][1]));
	setChannel(1);
	// Write matching rx pipe 0 and tx addresses - Using 3 byte addresses
	uint8_t tmp[4];
	tmp[0] = (RFM73_CMD_WRITE_REG | RFM73_REG_RX_ADDR_P0);
	tmp[1] = address >> 16; tmp[2] = address >> 8; tmp[3] = address;
	writeRegBuf(tmp, 4);
	tmp[0] = (RFM73_CMD_WRITE_REG | RFM73_REG_TX_ADDR);
	writeRegBuf(tmp, 4);
	// Init bank 1 registers
	selectBank(1);
	for (uint8_t i = 0; i < sizeof(RFM73_bank1Init) / sizeof(RFM73_bank1Init[0]); i++)
		writeRegBufPgm((uint8_t *)RFM73_bank1Init[i], sizeof(RFM73_bank1Init[0]));
	// set ramp curve register
	writeRegBufPgm((uint8_t *)RFM73_bank1R0EInit, sizeof(RFM73_bank1R0EInit));
	delay(2);
	// Toggle some bits
	writeRegBufPgm((uint8_t *)RFM73_cmd_tog1, sizeof(RFM73_cmd_tog1));
	delayMicroseconds(20);
	writeRegBufPgm((uint8_t *)RFM73_cmd_tog2, sizeof(RFM73_cmd_tog2));
	selectBank(0);
}

void RF75::setCE(){ digitalWrite(RF_PIN_CE, HIGH); } // Active high
void RF75::clearCE(){ digitalWrite(RF_PIN_CE, LOW); }
void RF75::setSS(){ digitalWrite(RF_PIN_SS, LOW); } // Active low
void RF75::clearSS(){ digitalWrite(RF_PIN_SS, HIGH); }

uint8_t RF75::readRegVal(uint8_t cmd){
	uint8_t res;
	setSS();
	SPI.transfer(cmd);
	res = SPI.transfer(0);
	clearSS();
	return res;
}

void RF75::writeRegVal(uint8_t cmd, uint8_t val){
	setSS();
	SPI.transfer(cmd);
	SPI.transfer(val);
	clearSS();
}

void RF75::readRegBuf(uint8_t reg, uint8_t * buf, uint8_t len){
	setSS();
	SPI.transfer(reg);
	for (uint8_t byte_ctr = 0; byte_ctr < len; byte_ctr++)
		buf[byte_ctr] = SPI.transfer(0);
	clearSS();
}

void RF75::writeRegBuf(uint8_t * cmdbuf, uint8_t len){
	setSS();
	while (len--) SPI.transfer(*(cmdbuf++));
	clearSS();
}

void RF75::writeRegBufPgm(uint8_t * cmdbuf, uint8_t len){
	setSS();
	while (len--) SPI.transfer(pgm_read_byte(cmdbuf++));
	clearSS();
}

uint8_t RF75::writeRegCmdBuf(uint8_t cmd, uint8_t * buf, uint8_t len) {
	setSS();
	SPI.transfer(cmd);
	while (len--) SPI.transfer(*(buf++));
	clearSS();
	return 1;
}

void RF75::selectBank(uint8_t bank){
	uint8_t tmp = read_status() & 0x80;
	if ((bank && !tmp) || (!bank && tmp)) {
		writeRegBufPgm((uint8_t *)RFM73_cmd_switch_cfg, sizeof(RFM73_cmd_switch_cfg));
	}
}

void RF75::setChannel(uint8_t cnum){ writeRegVal( RFM73_CMD_WRITE_REG | RFM73_REG_RF_CH, cnum); }

uint8_t RF75::getChannel() { return readRegVal(RFM73_REG_RF_CH); }

void RF75::flushTxFIFO(){ writeRegBuf((uint8_t *)RFM73_cmd_flush_tx, sizeof(RFM73_cmd_flush_tx)); }

void RF75::flushRxFIFO(){ writeRegBuf((uint8_t *)RFM73_cmd_flush_rx, sizeof(RFM73_cmd_flush_rx)); }

void RF75::turnOff(){
	clearCE();
	if (POWER_PIN) {
		setSS(); // Need to set low to allow radio to shut down
		digitalWrite(POWER_PIN, HIGH);
		SPI.end();
		digitalWrite(13, LOW); // Set SCK and MOSI low while the radio is off so that they dont source any current
		digitalWrite(11, LOW);
		digitalWrite(POWER_RAIL_PIN, LOW);
		pinMode(POWER_RAIL_PIN, OUTPUT);
        digitalWrite(POWER_PIN, HIGH);
	} else {
		uint8_t val = readRegVal(RFM73_REG_CONFIG) & ~0x02;
		writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_CONFIG, val);
	}
	
}

void RF75::turnOn(){
	// This function doesn't seem to work right. The radio eventually gets into a weird state and won't reset properly without a power cycle.
	uint8_t val = readRegVal(RFM73_REG_CONFIG) | 0x02;
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_CONFIG, val);
	activate();
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_FEATURE, 0x01); // This one gets cleared on power cycle I think
	delay(2);
	selectBank(1);
	writeRegBufPgm((uint8_t *)RFM73_cmd_tog1, sizeof(RFM73_cmd_tog1));
	delayMicroseconds(20);
	writeRegBufPgm((uint8_t *)RFM73_cmd_tog2, sizeof(RFM73_cmd_tog2));
	selectBank(0);
}

void RF75::setModeRX(uint8_t payload_len){
	clearCE();
	flushRxFIFO();
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_STATUS, read_status()); // Clear interrupts
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_CONFIG, readRegVal(RFM73_REG_CONFIG) | 0x01); // Set RX mode
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_RX_PW_P0, payload_len); // Prepare for correct length messages
	setCE();
}

void RF75::setModeTX(void){
	clearCE();
	flushTxFIFO();
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_STATUS, read_status()); // Clear interrupts
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_CONFIG, readRegVal(RFM73_REG_CONFIG) & ~0x01); // Set TX mode
}

void RF75::recievePacket(uint8_t *buff) {
	uint8_t status = readRegVal(RFM73_REG_STATUS);
	uint8_t len = readRegVal(RFM73_CMD_RX_PL_WID); // Payload width
	readRegBuf(RFM73_CMD_RD_RX_PLOAD, buff, len);
	// Note: Not sure this logic is 100% great
	uint8_t fifo_sta = readRegVal(RFM73_REG_FIFO_STATUS);
	if (fifo_sta & RFM73_FIFO_STATUS_RX_EMPTY) {
		status |= 0x40 & 0xCF; // clear status bit rx_dr
		writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_STATUS, status);
	}
}

void RF75::sendPacket(uint8_t* payload, uint8_t len){
	setCE();
	setSS();
	SPI.transfer(RFM73_CMD_W_TX_PAYLOAD_NOACK);
	for (int i = 0; i < len; i++) SPI.transfer(*(payload++));
	clearSS();
	clearCE();
	while (!(read_status() & RFM73_IRQ_STATUS_TX_DS)) {} // Wait until we're done transmitting
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_STATUS, RFM73_IRQ_STATUS_TX_DS); // And clear that interrupt
}

bool RF75::available() {
	return read_status() & RFM73_IRQ_STATUS_RX_DR;
}

uint8_t RF75::read_status() {
	setSS();
	uint8_t res = SPI.transfer(RFM73_CMD_NOP_NOP);
	clearSS();
	return res;
}

void RF75::activate() {
	clearCE();
	// Run the activate command, but only if we're not already activated
	uint8_t original_features = readRegVal(RFM73_REG_FEATURE);
	// If it's not activated the feature register will always read 0
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_FEATURE, original_features | 0x01); // Make sure it's not just all zeros but still activated
	uint8_t new_features = readRegVal(RFM73_REG_FEATURE);
	if (!new_features) {
		writeRegBufPgm((uint8_t *)RFM73_cmd_activate, sizeof(RFM73_cmd_activate));
	}
	writeRegVal(RFM73_CMD_WRITE_REG | RFM73_REG_FEATURE, original_features);
}

uint8_t RF75::carrier_detect() {
	return readRegVal(RFM73_REG_CD);
}
