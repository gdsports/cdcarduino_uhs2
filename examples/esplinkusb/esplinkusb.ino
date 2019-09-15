/*
 * MIT License
 *
 * Copyright (c) 2019 gdsports625@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * TCP transparent bridge to Arduino board via USB port. This program provides
 * functionality similar to esp-link (https://github.com/jeelabs/esp-link) but
 * through the Arduino board USB port so no UART wiring is required. A USB Host
 * mini board and the USB Host Shield 2.0 library are required to connect the
 * Arduino board to the ESP. esp-link has many more features. This program
 * only provides the minimum features to use avrdude with its network option
 * to upload HEX files to an attached Uno or Mega 2560 over WiFi.
 */


#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

// needed for WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <cdcarduino.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

// TCP socket on port 23 without Arduino reset
const int ConsolePort = 23;
WiFiServer ServerConsole(ConsolePort);
// TCP socket on port 2323 with Arduino reset
const int ResetPort = 2323;
WiFiServer ServerReset(ResetPort);
// Only 1 client allowed.
WiFiClient serverClient;

class ACMAsyncOper : public CDCAsyncOper
{
  public:
    uint8_t OnInit(ACM *pacm);
};

USB           Usb;
USBHub        Hub(&Usb);
ACMAsyncOper  AsyncOper;
ARD           Ard(&Usb, &AsyncOper);

uint8_t ACMAsyncOper::OnInit(ACM *pacm)
{
  return 0;
}

void setup()
{
  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
    Serial.println("USB host shield failed");

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();

  //set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  //if you get here you have connected to the WiFi
  Serial.println("connected");

  //start TCP servers
  ServerConsole.begin();
  ServerConsole.setNoDelay(true);
  Serial.print("For console port connect to ");
  Serial.print(WiFi.localIP());
  Serial.print(':');
  Serial.println(ConsolePort);

  ServerReset.begin();
  ServerReset.setNoDelay(true);
  Serial.print("For console port with Arduino reset connect to ");
  Serial.print(WiFi.localIP());
  Serial.print(':');
  Serial.println(ResetPort);
}

uint8_t OutBuf[64];
uint16_t OutBufLen;

void loop()
{
  uint8_t rcode;

  // check if there are any new clients on the Reset server
  if (ServerReset.hasClient()) {
    if (serverClient) {
      serverClient.stop();
      int i = 20;
      while (serverClient && i--) delay(10);
    }
    serverClient = ServerReset.available();
    Serial.println("New client with reset");
    OutBufLen = 0;
    Ard.reset_target();
  }

  // check if there are any new clients on the Console server
  if (ServerConsole.hasClient()) {
    if (serverClient) {
      serverClient.stop();
      int i = 20;
      while (serverClient && i--) delay(10);
    }
    serverClient = ServerConsole.available();
    Serial.println("New client");
    OutBufLen = 0;
  }

  Usb.Task();
  if ( Ard.isReady() && serverClient ) {
    /* Read from TCP clients for data and write to USB CDC ACM */
    if (OutBufLen > 0) {
      rcode = Ard.SndData(OutBufLen, OutBuf);
      if (rcode) {
        Serial.print("retry Ard.SndData=");
        Serial.println(rcode);
        if (rcode != hrNAK) {
          ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
          OutBufLen = 0;
        }
      }
      else {
        OutBufLen = 0;
      }
    }
    else {
      size_t bytesAvail = serverClient.available();
      if (bytesAvail > 0) {
        bytesAvail =
          (bytesAvail < sizeof(OutBuf)) ? bytesAvail : sizeof(OutBuf);
        OutBufLen = serverClient.read(OutBuf, bytesAvail);
        if (OutBufLen > 0) {
          rcode = Ard.SndData(OutBufLen, OutBuf);
          if (rcode) {
            Serial.print("Ard.SndData=");
            Serial.println(rcode);
            if (rcode != hrNAK) {
              ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
              OutBufLen = 0;
            }
          }
          else {
            OutBufLen = 0;
          }
        }
      }
    }

    for (int i = 0; i < 6; i++) {
      /* Read from USB CDC ACM and write to TCP socket */
      uint8_t inBuf[64];

      uint16_t bytesIn = sizeof(inBuf);
      rcode = Ard.RcvData(&bytesIn, inBuf);
      if (rcode && (rcode != hrNAK))
        ErrorMessage<uint8_t>(PSTR("Ret"), rcode);

      if (bytesIn > 0) {
        int bytesOut = serverClient.write(inBuf, bytesIn);
        if (bytesOut != bytesIn) {
          Serial.println("Error reading from USB CDCACM and writing to TCP socket");
        }
      }
      /* TODO skip delay if native USB such as Leonardo, Micro, Pro Micro */
      delay(1);
    }
  }
}
