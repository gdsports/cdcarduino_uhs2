# CDC Arduino for USB Host Shield 2.0 Library

The cdcarduino library provides support for Arduino USB CDC ACM boards such as
the Uno and Mega 2560. The library detects Arduino boards based on their USB
Vendor ID and Product ID. Methods to reset (DTR and RTS) boards are provided to
force the boards to run their bootloaders for Flash burning. This will not work
with clone boards using CH340, CP210x, or other USB serial chips. The boards
must have the Arduino USB Vendor IDs and Product IDs. This may also not work if
boards are running non-standard bootloaders. Testing has been done only on
genuine Arduino Uno and Mega 2560 boards running factory installed bootloaders.

## Example: esplinkusb

TCP transparent bridge to Arduino board via USB port. This program provides
firmware update functionality similar to
[esp-link](https://github.com/jeelabs/esp-link) but through the Arduino board
USB port without connecting wires to UART pins. A USB Host mini board and the
USB Host Shield 2.0 library are required to connect the Arduino board to the
ESP. This program provides the minimum features to use avrdude with its -Pnet:
network option to upload HEX files to an attached Uno or Mega 2560 over WiFi.

`Uno -USB- USB Host mini -SPI- ESP8266 -WiFi- avrdude -Pnet:<IP addr>:2323`

The program listens for TCP connections on port 23 and 2323. Connections on
port 2323 reset the attached board so it is ready for Flash burning. Use
avrdude with this port.

Connections on port 23 do not reset the board can be used to access the serial
console without disturbing the board. Use telnet or a similar program.

The program uses [WiFiManager](https://github.com/tzapu/WiFiManager) so the
SSID and password can be configured without changing the source code.

avresplink is a BASH shell script that runs avrdude with the correct parameters
depending on the board. This can be used with this program as well as with
esp-link.

`$ avresplink 192.168.x.y Blink.ino.standard.hex`

This will run avrdude with the following parameters. The parameters that are
the same for all boards are not shown.

```
-patmega328p
-carduino
-Pnet:192.168.x.y:2323
-Uflash:w:Blink.ino.standard.hex:i
```

`$ avresplink 192.168.x.y Blink.ino.mega.hex`

This will run avrdude with the following parameters. The parameters that are
the same for all boards are not shown.

```
-patmega2560
-cwiring
-Pnet:192.168.x.y:2323
-Uflash:w:Blink.ino.mega.hex:i
```
