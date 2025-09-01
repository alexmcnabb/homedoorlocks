#!/bin/env python3

import sys
import os
from serial import Serial
from time import time, sleep
from threading import Thread
import paho.mqtt.client as mqtt

LOCK_COMMAND = 0x00
UNLOCK_COMMAND = 0x01
FRONTDOOR_ID = 0x00
BACKDOOR_ID = 0x02

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
            if command == "lb":
                port.write(bytes([LOCK_COMMAND | BACKDOOR_ID]))
            if command == "ub":
                port.write(bytes([UNLOCK_COMMAND | BACKDOOR_ID]))
            if command == "lf":
                port.write(bytes([LOCK_COMMAND | FRONTDOOR_ID]))
            if command == "uf":
                port.write(bytes([UNLOCK_COMMAND | FRONTDOOR_ID]))
    except EOFError:
        fprint("Disabling manual input, no terminal available")

manual_input_thread = Thread(target=manual_input_loop, daemon=True)
manual_input_thread.start()


def serial_input_loop():
    while(True):
        for byte in port.read():
            # TODO: this part
            if byte == LOCK_COMMAND | BACKDOOR_ID:
                fprint("Got locked signal from backdoor lock!")
                client.publish("home/frontdoor/state", "LOCK")
            elif byte == UNLOCK_COMMAND | BACKDOOR_ID:
                fprint("Got unlocked signal from backdoor lock!")
                client.publish("home/frontdoor/state", "UNLOCK")
            elif byte == LOCK_COMMAND | FRONTDOOR_ID:
                fprint("Got locked signal from frontdoor lock!")
                client.publish("home/frontdoor/state", "LOCK")
            elif byte == UNLOCK_COMMAND | FRONTDOOR_ID:
                fprint("Got unlocked signal from frontdoor lock!")
                client.publish("home/frontdoor/state", "UNLOCK")
            else:
                fprint(chr(byte), end="")

serial_input_thread = Thread(target=serial_input_loop, daemon=True)
serial_input_thread.start()


def on_connect(client, userdata, flags, rc):
    if (rc):
        fprint("Connect failed, returned error code", rc)
        return
    client.subscribe("home/locks/frontdoor/command")
    client.subscribe("home/locks/backdoor/command")
    fprint("MQTT Connected")

def on_message(client, userdata, msg):
    fprint(msg.topic+" "+str(msg.payload))
    if msg.topic == "home/locks/frontdoor/command":
        dev_id = FRONTDOOR_ID
    else:
        dev_id = BACKDOOR_ID
    if msg.payload == b'LOCK':
        fprint(f"Got lock command from MQTT for dev_id={dev_id}")
        port.write(bytes([LOCK_COMMAND | dev_id]))
    elif msg.payload == b'UNLOCK':
        fprint(f"Got unlock command from MQTT for dev_id={dev_id}")
        port.write(bytes([UNLOCK_COMMAND | dev_id]))
        
    
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set("homedoorlockrelay", "fdokimnbvdfbnmndkjfbkdj")
client.connect("localhost", 1883, 60)


client.loop_forever()
