#!/bin/bash
IDEVER="1.8.10"
WORKDIR="/tmp/autobuild_$$"
mkdir -p ${WORKDIR}
# Install Ardino IDE in work directory
if [ -f ~/Downloads/arduino-${IDEVER}-linux64.tar.xz ]
then
    tar xf ~/Downloads/arduino-${IDEVER}-linux64.tar.xz -C ${WORKDIR}
else
    wget -O arduino.tar.xz https://downloads.arduino.cc/arduino-${IDEVER}-linux64.tar.xz
    tar xf arduino.tar.xz -C ${WORKDIR}
    rm arduino.tar.xz
fi
# Create portable sketchbook and library directories
IDEDIR="${WORKDIR}/arduino-${IDEVER}"
LIBDIR="${IDEDIR}/portable/sketchbook/libraries"
mkdir -p "${LIBDIR}"
export PATH="${IDEDIR}:${PATH}"
cd ${IDEDIR}
which arduino
cd $LIBDIR
arduino --install-library WiFiManager
arduino --install-library "USB Host Shield Library 2.0"
git clone https://github.com/gdsports/cdcarduino_uhs2.git
# Switch to ESP8266
arduino --pref "boardsmanager.additional.urls=https://arduino.esp8266.com/stable/package_esp8266com_index.json" --save-prefs
arduino --install-boards "esp8266:esp8266"
BOARD="esp8266:esp8266:generic"
arduino --board "${BOARD}" --save-prefs
CC="arduino --verify --board ${BOARD}"
cd cdcarduino_uhs2/examples
(find . -name '*.ino' -print0 | xargs -0 -n 1 $CC >/tmp/esp8266_$$.txt 2>&1)&
