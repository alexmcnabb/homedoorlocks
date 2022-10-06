
# System overview
Converts a powered deadbolt into a home-assistant connected smart lock

## Lock Module
This attaches to and is powered by the door lock. It transmits the lock state on an interval, and when the lock is manually moved. It also receives commands from the base station to trigger the lock remotely.

## Base Station
This is connected to the server via USB, and is mounted near the door via a long USB cable. It forwards commands between the python code and the lock module.

## Messaging
The python code translates between the messages passed to the lock, and MQTT messages passed to HomeAssistant. 
Messages sent via serial to the BaseStation and then forwarded to the lock:
    0x00 - Please lock door
    0x01 - Please unlock door.
Messages received via serial from the BaseStation and the lock:
    0x00 - Door is locked
    0x01 - Door is unlocked
MQTT messages passed to HomeAssistant:
There's two topics, the state_topic and the command_topic. Subscribe for messages on the command_topic and send them to the door. When door sends messages, post them to the state_topic.
The state_topic is `home/frontdoor/state`
The command_topic is `home/frontdoor/command`
The values sent should be either `LOCK` or `UNLOCK` on both topics.

Home Assistant Configuration Docs: https://www.home-assistant.io/integrations/lock.mqtt/#full-configuration



# PCB Changes to make - Lock module:
- Connect lock serial TX line to A2 instead of pin 10 - Pin 10 needs to be an output since it's the default SS pin
- Construct new power supply system for radio module. Current design is as follows:
    1u and 100n caps at radio module VCC. Radio module VCC is connected to drain, source connected to Vin. Gate is connected to ground via 1M resistor, and to pin 5. Pin A1 is connected directly to the drain. A 47u cap is connected between Vin and ground.
    Currently using a TP2104 P Mosfet
# PCB Changes to make - Base Station:
- Add 100n cap to power rail at radio, 22uf cap to power rail
- Add the option to cycle radio power from an IO pin like on the base station, with a jumper to bypass it
