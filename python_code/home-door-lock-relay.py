#!/bin/env python3

import sys
import os
from serial import Serial
from time import time, sleep
from threading import Thread
import paho.mqtt.client as mqtt

LOCK_COMMAND = b"\x00"
UNLOCK_COMMAND = b"\x01"

port = Serial("/dev/serial/by-id/usb-SparkFun_SparkFun_Pro_Micro-if00", 115200)

def fprint(*args, **kwargs):
    # Required when running as a service, prints won't come out right otherwise
    print(*args, **kwargs)
    sys.stdout.flush()


client = mqtt.Client()

def manual_input_loop():
    try:
        while(True):
            command = input()
            if command == "l":
                port.write(LOCK_COMMAND)
            if command == "u":
                port.write(UNLOCK_COMMAND)
    except EOFError:
        fprint("Disabling manual input, no terminal available")

manual_input_thread = Thread(target=manual_input_loop, daemon=True)
manual_input_thread.start()


def serial_input_loop():
    while(True):
        for byte in port.read():
            if byte == ord(LOCK_COMMAND):
                fprint("Got locked signal from lock!")
                client.publish("home/frontdoor/state", "LOCK")
            elif byte == ord(UNLOCK_COMMAND):
                fprint("Got unlocked signal from lock!")
                client.publish("home/frontdoor/state", "UNLOCK")
            else:
                fprint(chr(byte), end="")

serial_input_thread = Thread(target=serial_input_loop, daemon=True)
serial_input_thread.start()


def on_connect(client, userdata, flags, rc):
    if (rc):
        fprint("Connect failed, returned error code", rc)
        return
    client.subscribe("home/frontdoor/command")
    fprint("MQTT Connected")

def on_message(client, userdata, msg):
    # print(msg.topic+" "+str(msg.payload))
    if msg.payload == b'LOCK':
        fprint("Got lock command from MQTT")
        port.write(LOCK_COMMAND)
    elif msg.payload == b'UNLOCK':
        fprint("Got unlock command from MQTT")
        port.write(UNLOCK_COMMAND)
        
    
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set("homedoorlockrelay", "fdokimnbvdfbnmndkjfbkdj")
client.connect("localhost", 1883, 60)


client.loop_forever()
