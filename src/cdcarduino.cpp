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
#include "cdcarduino.h"

ARD::ARD(USB *p, CDCAsyncOper *pasync) :
    ACM(p, pasync),
    vid(0), pid(0), targetBaudRate(0), targetResetMsec(0) {
    }

uint8_t ARD::reset_dtr_rts(int resetLowMsec)
{
    uint8_t rcode;

    // Set DTR=RTS=0
    rcode = ACM::SetControlLineState(0);
    if (rcode) {
        ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
        return rcode;
    }
    delay(resetLowMsec);

    // Set DTR=RTS=1
    rcode = ACM::SetControlLineState(3);
    if (rcode) {
        ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
        return rcode;
    }
    return 0;
}

uint8_t ARD::reset_touch_1200bps()
{
    uint8_t rcode;

    // Set DTR=1, RTS=1
    rcode = ACM::SetControlLineState(0x03);
    if (rcode) {
        ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
        return rcode;
    }
    delay(4);

    // 1200 baud, 8 data bits, no parity, 1 stop bit
    LINE_CODING	lc;
    lc.dwDTERate	= 1200;
    lc.bCharFormat	= 0;
    lc.bParityType	= 0;
    lc.bDataBits	= 8;

    rcode = ACM::SetLineCoding(&lc);

    if (rcode) {
        ErrorMessage<uint8_t>(PSTR("SetLineCoding"), rcode);
        return rcode;
    }
    delay(4);

    // Set DTR=0, RTS=1
    rcode = ACM::SetControlLineState(0x02);
    if (rcode) {
        ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
        return rcode;
    }
    delay(4);

    // Set DTR=0, RTS=0
    rcode = ACM::SetControlLineState(0);
    if (rcode) {
        ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
        return rcode;
    }
    return 0;
}

bool ARD::reset_target()
{
    if (targetResetMsec) {
        reset_dtr_rts(targetResetMsec);
    }
    else {
        //reset_touch_1200bps();
    }
}

bool ARD::find_target(uint16_t vid, uint16_t pid)
{
    for (int i = 0; i < sizeof(Boards)/sizeof(Boards[0]); i++) {
        if ((vid == Boards[i].vendorID) && (pid == Boards[i].productID)) {
            targetBaudRate = BoardActions[Boards[i].board_actions].baudRate;
            targetResetMsec = BoardActions[Boards[i].board_actions].resetMsec;
            return true;
        }
    }
    targetBaudRate = 115200;
    targetResetMsec = 250;
    return false;
}

uint8_t ARD::Init(uint8_t parent, uint8_t port, bool lowspeed) {

    const uint8_t constBufSize = sizeof (USB_DEVICE_DESCRIPTOR);

    uint8_t buf[constBufSize];
    USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);

    uint8_t rcode;
    UsbDevice *p = NULL;
    EpInfo *oldep_ptr = NULL;
    uint8_t num_of_conf; // number of configurations

    AddressPool &addrPool = pUsb->GetAddressPool();

    USBTRACE("ARD Init\r\n");

    if(bAddress)
        return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;

    // Get pointer to pseudo device with address 0 assigned
    p = addrPool.GetUsbDevicePtr(0);

    if(!p)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    if(!p->epinfo) {
        USBTRACE("epinfo\r\n");
        return USB_ERROR_EPINFO_IS_NULL;
    }

    // Save old pointer to EP_RECORD of address 0
    oldep_ptr = p->epinfo;

    // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
    p->epinfo = epInfo;

    p->lowspeed = lowspeed;

    // Get device descriptor
    rcode = pUsb->getDevDescr(0, 0, constBufSize, (uint8_t*)buf);
    USBTRACE2("getDevDescr ", rcode);
    // Restore p->epinfo
    p->epinfo = oldep_ptr;

    if(rcode)
        goto FailGetDevDescr;

    // Allocate new address according to device class
    bAddress = addrPool.AllocAddress(parent, false, port);
    USBTRACE2("bAddress=", bAddress);
    if(!bAddress)
        return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

    // Assign new address to the device
    rcode = pUsb->setAddr(0, 0, bAddress);

    if(rcode) {
        p->lowspeed = false;
        addrPool.FreeAddress(bAddress);
        bAddress = 0;
        USBTRACE2("setAddr:", rcode);
        return rcode;
    }

    USBTRACE2("Addr:", bAddress);

    p->lowspeed = false;

    // Get device descriptor
    rcode = pUsb->getDevDescr(bAddress, 0, constBufSize, (uint8_t*)buf);
    USBTRACE2("getDevDescr ", rcode);
    vid = udd->idVendor;
    pid = udd->idProduct;

    // Extract Max Packet Size from the device descriptor
    epInfo[0].maxPktSize = udd->bMaxPacketSize0;

    p = addrPool.GetUsbDevicePtr(bAddress);

    if(!p)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    p->lowspeed = lowspeed;

    num_of_conf = udd->bNumConfigurations;

    // Assign epInfo to epinfo pointer
    rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);

    if(rcode)
        goto FailSetDevTblEntry;

    USBTRACE("VID:"), D_PrintHex(vid, 0x80);
    USBTRACE(" PID:"), D_PrintHex(pid, 0x80);
    USBTRACE2(" NC:", num_of_conf);

    for(uint8_t i = 0; i < num_of_conf; i++) {
        ConfigDescParser< USB_CLASS_COM_AND_CDC_CTRL,
            CDC_SUBCLASS_ACM, 0,
            CP_MASK_COMPARE_CLASS |
                CP_MASK_COMPARE_SUBCLASS > CdcControlParser(this);

        ConfigDescParser<USB_CLASS_CDC_DATA, 0, 0,
            CP_MASK_COMPARE_CLASS> CdcDataParser(this);

        rcode = pUsb->getConfDescr(bAddress, 0, i, &CdcControlParser);

        if(rcode)
            goto FailGetConfDescr;

        rcode = pUsb->getConfDescr(bAddress, 0, i, &CdcDataParser);

        if(rcode)
            goto FailGetConfDescr;

        if(bNumEP > 1)
            break;
    } // for

    if(bNumEP < 4)
        return USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED;

    // Assign epInfo to epinfo pointer
    rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);

    USBTRACE2("Conf:", bConfNum);

    // Set Configuration Value
    rcode = pUsb->setConf(bAddress, 0, bConfNum);

    if(rcode)
        goto FailSetConfDescr;

    // Set up features status
    _enhanced_status = enhanced_features();
    half_duplex(false);
    autoflowRTS(false);
    autoflowDSR(false);
    autoflowXON(false);
    wide(false); // Always false, because this is only available in custom mode.
    find_target(vid, pid);
    USBTRACE2("Baud=", targetBaudRate);
    USBTRACE2("Reset (ms)=", targetResetMsec);
    if (targetResetMsec) {
        // Set DTR = 1 RTS=1
        rcode = ACM::SetControlLineState(3);
        if (rcode) goto FailOnInit;
    }
    LINE_CODING	lc;
    lc.dwDTERate	= targetBaudRate;
    lc.bCharFormat	= 0;
    lc.bParityType	= 0;
    lc.bDataBits	= 8;
    rcode = ACM::SetLineCoding(&lc);
    if (rcode) goto FailOnInit;

    rcode = pAsync->OnInit(this);
    if(rcode) goto FailOnInit;

    USBTRACE("ARD configured\r\n");

    ready = true;
    return 0;

FailGetDevDescr:
#ifdef DEBUG_USB_HOST
    NotifyFailGetDevDescr();
    goto Fail;
#endif

FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
    NotifyFailSetDevTblEntry();
    goto Fail;
#endif

FailGetConfDescr:
#ifdef DEBUG_USB_HOST
    NotifyFailGetConfDescr();
    goto Fail;
#endif

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
    NotifyFailSetConfDescr();
    goto Fail;
#endif

FailOnInit:
#ifdef DEBUG_USB_HOST
    USBTRACE("OnInit:");
#endif

#ifdef DEBUG_USB_HOST
Fail:
    NotifyFail(rcode);
#endif
    Release();
    return rcode;
}

