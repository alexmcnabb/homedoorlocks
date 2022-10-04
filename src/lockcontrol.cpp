#include "lockcontrol.h"
#include "Arduino.h"

const uint32_t DELAY_BUTTON = 150;
const int PIN_SERIAL_TX = A2;

/*
The keypad communicates over some kind of async serial bus. One sequence is transmitted on each button press, and another when it times out and switches the light off
Bus is idle low, then high for 20mS, low for 1.5mS, 9 bits of data, then low for 9mS, then it repeats
Each bit of data is high for 300uS then low for 700uS for a 0, or 700uS high and 300uS low for a 1
The first 8 bits are specific to the button, the ninth bit is always 0 

We're Teed into the tx side of the bus with a 100k resistor on the keypad side and a 1k resistor on our side. When not transmitting, our output pin should be high-impedance

For the limit microswitches, both switches are wired up to D2 and D3 (INT0, INT1).

The original PCB connects the serial TX line to D10, but that's the SS pin for the SPI module and so we can't use it as an input, using A2 instead
*/

enum {BUTTON_LOCK = 0x0E, BUTTON_12 = 0x01, BUTTON_34 = 0x02, BUTTON_56 = 0x03, BUTTON_78 = 0x04, BUTTON_90 = 0x05, TIMEOUT = 0xD4};

volatile bool state = UNLOCK;
volatile bool last_state = UNLOCK;
void Int0_ISR() {
    if (state == LOCK) state = UNLOCK;
}
void Int1_ISR() {
    if (state == UNLOCK) state = LOCK;
}

void init_lock_control() {
    attachInterrupt(INT0, Int0_ISR, RISING);
    attachInterrupt(INT1, Int1_ISR, RISING);
}

bool lock_new_state_interrupt_triggered() {
    // Call this after waking from an interrupt to see if the lock moved
    bool state_is_updated = state != last_state;
    last_state = state;
    return state_is_updated;
}

uint8_t current_lock_state() {
    if (state == LOCK) {
        Serial.println("Read lock is locked");
    } else {
        Serial.println("Read lock is unlocked");
    }
    return state;
}

void send_bit(uint8_t bit) {
    if (bit) {
        digitalWrite(PIN_SERIAL_TX, HIGH);
        delayMicroseconds(700);
        digitalWrite(PIN_SERIAL_TX, LOW);
        delayMicroseconds(300);
    } else {
        digitalWrite(PIN_SERIAL_TX, HIGH);
        delayMicroseconds(300);
        digitalWrite(PIN_SERIAL_TX, LOW);
        delayMicroseconds(700);
    }
}
void write_keypad_button(uint8_t button) {
    digitalWrite(PIN_SERIAL_TX, HIGH);
    delayMicroseconds(20000);
    digitalWrite(PIN_SERIAL_TX, LOW);
    delayMicroseconds(1500);
    for (int i = 0; i < 8; i++) send_bit(button & (1 << (7-i)));
    send_bit(0);
    delayMicroseconds(9000);
    digitalWrite(PIN_SERIAL_TX, HIGH);
    delayMicroseconds(20000);
    digitalWrite(PIN_SERIAL_TX, LOW);
    delayMicroseconds(1500);
    for (int i = 0; i < 8; i++) send_bit(button & (1 << (7-i)));
    send_bit(0);
}
void send_keypad_command(uint8_t state) {
    pinMode(PIN_SERIAL_TX, OUTPUT);
    if (state == LOCK) {
        write_keypad_button(BUTTON_LOCK);
    } else {
        // Code 55713395
        write_keypad_button(BUTTON_56);
        delay(DELAY_BUTTON);
        write_keypad_button(BUTTON_56);
        delay(DELAY_BUTTON);
        write_keypad_button(BUTTON_78);
        delay(DELAY_BUTTON);
        write_keypad_button(BUTTON_12);
        delay(DELAY_BUTTON);
        write_keypad_button(BUTTON_34);
        delay(DELAY_BUTTON);
        write_keypad_button(BUTTON_34);
        delay(DELAY_BUTTON);
        write_keypad_button(BUTTON_90);
        delay(DELAY_BUTTON);
        write_keypad_button(BUTTON_56);
        delay(DELAY_BUTTON);
        write_keypad_button(BUTTON_LOCK);
    }
    pinMode(PIN_SERIAL_TX, INPUT);
}