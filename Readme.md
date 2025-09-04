# Current Status:

The system worked with modified V1 boards in Regina without issue

I assembled the V2 boards, but couldn't reliably get it to run consistantly. Messages would either be dropped or arrive with corruption
I tried adding 10uF caps to the radio power rail, but at that point something happened and I couldn't get em to communicate at all anymore

Could try running with newer nrf24L01 modules, but hard to say if I can get genuine ones

Best solution is probably redesign with nrf54x chips or something


# System overview
Converts a powered deadbolt into a home-assistant connected smart lock
Designed to work with a Weiser Powerbolt 2

When compiling, select the correct environment in platformio.ini

## Lock Module
This attaches to and is powered by the door lock. It transmits the lock state on an interval, and when the lock is manually moved. It also receives commands from the base station to trigger the lock remotely.

## Modifications to Lock:

It interfaces with the lock in two ways - it listens to the signals from the two endstop switches, and it interrupts the serial line from the keypad to the controller
TODO: describe connections

## Base Station
This is connected to the server via USB, and is mounted near the door via a long USB cable. It forwards commands between the python code and the lock module.

## Messaging

### Over the air between basestation and lock modules

Messages to the lock:
    0x00 - Please lock door
    0x01 - Please unlock door.
Messages to the basestation:
    0x00 - Door is locked
    0x01 - Door is unlocked
See lib/PacketEngine/Readme.md for packet format details

### Over serial between basestation and python script:
Same messages as above except the second bit indicates which door
    Front door: 0x00 (dev ID 1)
    Back door: 0x02 (dev ID 2)

Only non-printable chars should be used here as the basestation will print log text to the serial port which should be printed by the python script

### Python script to HomeAssistant
There's two topics for each door, the state_topic and the command_topic. Subscribe for messages on the command_topic and send them to the doors. When door sends messages, post them to the state_topic.
The state topics are `home/locks/frontdoor/state` and `home/locks/backdoor/state`
The command topics are `home/locks/frontdoor/command` and `home/locks/backdoor/command`
The values sent should be either `LOCK` or `UNLOCK` on both topics.

Home Assistant Configuration Docs: https://www.home-assistant.io/integrations/lock.mqtt/#full-configuration

### Flashing basestation firmware from server:
Copy file over:
`scp .pio\build\baseStation\firmware.hex server:/home/alex/iot_code/homedoorlocks/python_code`
Install Uploader program: https://github.com/vanbwodonk/leonardoUploader
```
git clone https://github.com/vanbwodonk/leonardoUploader.git
cd leonardoUploader/linux
make
sudo make install
sudo chmod +x /usr/local/bin/leonardoUploader  # Seem to need this for some reason
leonardoUploader /dev/ttyACM0 firmware.hex
```


# PCB Issues:
The transistor to enable radio power on both boards is wired with source/drain backwards, so it doesn't actually switch anything.
The radio pulls a max 16mA, and the IO pins are rated for 40mA, so I just removed the transistor and am relying on the IO pin to power the radio

The CE line on the door module didn't get routed, so I have to bodge that wire on
