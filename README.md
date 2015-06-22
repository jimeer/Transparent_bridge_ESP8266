ESP8266-transparent-bridge
==========================

Based on the work done at https://github.com/beckdac/ESP8266-transparent-bridge

Absolutely transparent bridge for the ESP8266

This is really basic firmware for the ESP that creates a totally transparent TCP socket to ESP UART0 bridge. Characters come in on one interface and go out the other. The totally transparent bridge mode is something that has been missing and is available on higher priced boards.

The original work uses commands at the TCB end to set up the connection. This is a bit awkward for me as in order to establish a TCP connection I need to know the IP address etc.

The workaround is to have the ESP8266 come up as an access point and configure from there. I found it easier however to configure the device at the serial end and so swapped the commends to this end.

To configure the device a USB to serial is required and the following commands mirror the original ones

All commands must begin at the start of a new line and begin with <esc>[, the delimiter is ‘,’. When a command is entered ‘*’ is echoed and the command details are then entered. If no command is entered then the current state is printed.

esc[r	reset
esc[b	set baud rate
esc[p	set port
esc[m	set mode 1 to 3
esc[s	set station <ssid>,<password>
esc[a	set access point <ssid>,<password>,<channel>
esc[x    print info (current set up)

TCP command
There is just one TCP end command left that will toggle GPIO4 (if you have one). This is used to reset the attached microcontroller.

There is still work to do, but it works for the moment.
