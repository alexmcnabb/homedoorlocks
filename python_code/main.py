#!/bin/env python3

import sys
import os
from serial import Serial
from time import time
from threading import Thread
import paho.mqtt.client as mqtt

port = Serial("COM14", 115200)

LOCK_COMMAND = b"\x00"
UNLOCK_COMMAND = b"\x01"



def manual_input_loop():
    while(True):
        command = input()
        if command == "l":
            port.write(LOCK_COMMAND)
        if command == "u":
            port.write(UNLOCK_COMMAND)

manual_input_thread = Thread(target=manual_input_loop, daemon=True)
manual_input_thread.start()


def serial_input_loop():
    while(True):
        for byte in port.read():
            if byte == ord(LOCK_COMMAND):
                print("Got locked signal from lock!")
                # TODO: Send mqtt message
            elif byte == ord(UNLOCK_COMMAND):
                print("Got unlocked signal from lock!")
                # TODO: Send mqtt message
            else:
                print(chr(byte), end="")

serial_input_thread = Thread(target=serial_input_loop, daemon=True)
serial_input_thread.start()


# def on_connect(client, userdata, flags, rc):
#     client.subscribe("home/frontdoor/command")

# def on_message(client, userdata, msg):
#     print(msg.topic+" "+str(msg.payload))

# client = mqtt.Client()
# client.on_connect = on_connect
# client.on_message = on_message

# client.connect("localhost", 1883, 60)

# client.loop_forever()