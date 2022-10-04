#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "lockcontrol.h"
#include "packet_engine.h"

/*
Lock connnector - 6 pin JST connector
    1 - G/W - RX of main lock board
    2 - G   - TX of keypad
    3 - B/W - Lock/Unlock microswitch
    4 - B   - Lock/Unlock microswitch
    5 - O/W - Ground
    6 - O   - 3v3
Serial interface for debug: Connects to ftdi cable TX, RX and GND
    Works with TTL-232R-3v3 cable or similar
    1 - BLK - Ground
    4 - ORA - FTDI TX
    5 - YEL - FTDI RX
*/

// Pin connections
const int PIN_RF_CE = 8;
const int PIN_RF_SS = 9;
const int PIN_RF_POWER = 5; // Low to enable power to the radio module
const int PIN_RF_POWER_DISC = A1; // Low to pull down power rail to radio module, high impedance otherwise
// RF75 SPI SCK, MISO, MOSI
// inputs from each microswitch in the lock - INT0, INT1 (2, 3)
// Digital output to the keypad serial bus - 10 (Needs to be an output since it's the default SS pin for the SPI bus)

const uint32_t STATE_TX_INTERVAL_S = 60 * 5; // 5 minutes, counted in watchdog intervals of 1 second


RemotePacketEngine packet_engine(PIN_RF_SS, PIN_RF_CE, PIN_RF_POWER, PIN_RF_POWER_DISC);

ISR(WDT_vect) {}

void do_sleep(){
    Serial.flush();
    ADCSRA = B00000000;//shut down adc
    PRR = B00001111;//shut down timer1/0, usi, adc
    lock_new_state_interrupt_triggered(); // Clear the interrupt flags
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);//set sleep mode
    sleep_enable();//enable sleep
    sleep_bod_disable();//disable BOD
    sleep_cpu();//go to sleep
    sleep_disable();//turn off sleep
    PRR = B00000000;//turn stuff back on
    ADCSRA = B10000000;//turn on adc
}

uint16_t lock_state_update_counter = 0;
bool time_to_tx_lock_state() {
    if (lock_state_update_counter++ > STATE_TX_INTERVAL_S) {
        lock_state_update_counter = 0;
        return true;
    }
    return false;
}
bool force_tx_lock_state = false;

void setup() {
    // Set up watchdog timer for 1s timeout interrupt
	MCUSR &= ~(1<<WDRF); //Clear the reset flag.
	WDTCSR |= (1<<WDCE) | (1<<WDE); // Enable changes
	WDTCSR = 1<<WDP1 | 1<<WDP2; // 1 second timeout
	WDTCSR |= _BV(WDIE); //Enable the WD interrupt (note no reset).
    Serial.begin(115200);
    Serial.println("Starting");
    packet_engine.init(0x00ABCDEF);
    init_lock_control();
    Serial.println("Done setup");
}

void loop() {
    do_sleep();
    if (lock_new_state_interrupt_triggered() || time_to_tx_lock_state() || force_tx_lock_state){
        if (force_tx_lock_state) do_sleep(); // If sending right away, give an extra second delay here
        force_tx_lock_state = false;
        Serial.println("Sending lock state");
        packet_engine.send_message(current_lock_state());
    } else {
        Serial.println("Scanning for messages");
        // We woke up from the watchdog timer
        if (packet_engine.try_recv_message()) {
            Serial.println("Got one!");
            send_keypad_command(packet_engine.received_data);
            force_tx_lock_state = true; // Force an update next wakeup
        }
    }
}
