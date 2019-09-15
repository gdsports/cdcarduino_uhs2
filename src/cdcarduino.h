/*
 * Arduino board serial console port USB CDC ACM.
 * This adds to the generic CDC ACM class, Arduino board detection and
 * board reset. Works for Uno and Mega/Mega2560. Includes experimental
 * code for Leonardo(32u4) and Nano Every (4809) but they are not supported.
 */

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
#if !defined(__CDCARDUINO_H__)
#define __CDCARDUINO_H__

#include <cdcacm.h>

class ARD : public ACM {
    public:
        ARD(USB *pusb, CDCAsyncOper *pasync);
        uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        uint16_t idVendor() { return vid; }
        uint16_t idProduct() { return pid; }
        uint8_t reset_dtr_rts(int resetLowMsec);
        uint8_t reset_touch_1200bps();
        bool reset_target();

    private:
        bool find_target(uint16_t vid, uint16_t pid);
        uint16_t  pid, vid;    // ProductID, VendorID
        uint32_t  targetBaudRate;
        uint16_t  targetResetMsec;

        typedef struct BOARD_ACTIONS  {
            uint32_t  baudRate;
            uint16_t  resetMsec;  // 0 = 1200bps touch
        } BOARD_ACTIONS_t;

        // Arduino board specific parameters
        typedef enum BA {BA_UNO, BA_MEGA, BA_LEO, BA_NANO_EVERY} BA_t;
        const BOARD_ACTIONS_t BoardActions[4] = {
            {115200, 250},    // Uno/328p
            {115200,  50},    // Mega/Mega2560
            { 57600,   0},    // Leonardo/32u4
            {115200,   0},    // Nano Every
        };

        typedef struct VID_PID_BOARD {
            uint16_t vendorID;
            uint16_t productID;
            BA_t board_actions;
        } VID_PID_BOARD_t;

        // Someday load more entries from a file in SPIFFS/SD?
        // This comes from the Arduino IDE boards.txt
        VID_PID_BOARD_t Boards[17] = {
            {0x2341, 0x0043, BA_UNO},
            {0x2341, 0x0001, BA_UNO},
            {0x2A03, 0x0043, BA_UNO},
            {0x2341, 0x0243, BA_UNO},

            {0x2341, 0x0010, BA_MEGA},
            {0x2341, 0x0042, BA_MEGA},
            {0x2A03, 0x0010, BA_MEGA},
            {0x2A03, 0x0042, BA_MEGA},
            {0x2341, 0x0210, BA_MEGA},
            {0x2341, 0x0242, BA_MEGA},

            {0x2341, 0x0036, BA_LEO},
            {0x2341, 0x8036, BA_LEO},
            {0x2A03, 0x0036, BA_LEO},
            {0x2A03, 0x8036, BA_LEO},

            {0x2341, 0x0058, BA_NANO_EVERY},
        };
};
#endif // __CDCARDUINO_H__
