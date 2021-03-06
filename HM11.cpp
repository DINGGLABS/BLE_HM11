/*******************************************************************************
* \file    HM11.cpp
********************************************************************************
* \author  Jascha Haldemann jh@oxon.ch
* \date    01.02.2017
* \version 1.0
*
* \license LGPL-V2.1
* Copyright (c) 2017 OXON AG. All rights reserved.
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, see 'http://www.gnu.org/licenses/'
*******************************************************************************/

/* ================================= Imports ================================ */
//#include <SoftwareSerial2.h>      // to Debug
#include "HM11.h"

/* ======================= Module constant declaration ====================== */
#define DEBUG_BLE                    //blup: define to activate the Serial Debug prints
#define DEBUG_BLE_PIN         14        // Arduino Pin
#define DEBUG_BLE_BAUDRATE    115200    // in Baud

/* ======================== Module macro declaration ======================== */
#ifdef DEBUG_BLE
  #include <SoftwareSerial3.h>
  #define DebugBLE_begin(...)     DebugBLE.begin(__VA_ARGS__)
  #define DebugBLE_print(...)     DebugBLE.print(__VA_ARGS__)
  #define DebugBLE_println(...)   DebugBLE.println(__VA_ARGS__)
  SoftwareSerial3 DebugBLE(-1, DEBUG_BLE_PIN);
#else
  #define DebugBLE_begin(...)
  #define DebugBLE_print(...)
  #define DebugBLE_println(...)
#endif

/* ====================== Module class instantiations ======================= */

/* ======================== Public member Functions ========================= */
/** -------------------------------------------------------------------------
  * \fn     begin
  * \brief  enables and inits the HM11
  *
  * \param  baudrate  baudrate (see enumerator in the header file)
  * \return true if setting the baudrate succeeded
  --------------------------------------------------------------------------- */
  bool HM11::begin(uint32_t baudrate)
  {
    DebugBLE_begin(DEBUG_BLE_BAUDRATE);
    baudrate_ = baudrate_t(baudrate);
    enable();
    return renewBLE();    // reset everything
  }

/** -------------------------------------------------------------------------
  * \fn     end
  * \brief  deinits and disables the HM11
  --------------------------------------------------------------------------- */
  void HM11::end()
  {
    baudrate_ = DEFAULT_BAUDRATE;
    disable();
  }

/** -------------------------------------------------------------------------
  * \fn     enable
  * \brief  enables the HM11
  --------------------------------------------------------------------------- */
  void HM11::enable()
  {
    DebugBLE_println(F("enable BLE"));
    if (baudrate_ == 0) baudrate_ = DEFAULT_BAUDRATE;
    setBit(*rstPort_, rstPin_);     // stop resetting
    clearBit(*enPort_, enPin_);     // enable BLE
    BLESerial_begin(baudrate_);
    while(!BLESerial_ready());
    while(BLESerial_available()) BLESerial_read();  // empty tx-buffer
    hwResetBLE();
  }

/** -------------------------------------------------------------------------
  * \fn     disable
  * \brief  disables the HM11
  --------------------------------------------------------------------------- */
  void HM11::disable()
  {
    DebugBLE_println(F("disable BLE"));
    clearBit(*rstPort_, rstPin_);   // to prevent supply throug reset
    BLESerial_end();
    clearBit(*rxdPort_, rxd_);      // to prevent supply throug rxd
    clearBit(*txdPort_, txd_);      // to prevent supply throug txd
    setBit(*enPort_, enPin_);       // disable BLE
  }

/** -------------------------------------------------------------------------
  * \fn     isEnabled
  * \brief  return the enable state of the HM11
  *
  * \return true if the HM11 is enabled
  --------------------------------------------------------------------------- */
  bool HM11::isEnabled()
  {
    return !getBit(*enPort_, enPin_);
  }

/** -------------------------------------------------------------------------
  * \fn     setTxPower
  * \brief  sets the transmission power
  *
  * \param  txPower  transmission power (see enumerator in the header file)
  * \return None
  --------------------------------------------------------------------------- */
  void HM11::setTxPower(txPower_t txPower)
  {
    setConf("POWE" + String(txPower));
  }

/** -------------------------------------------------------------------------
  * \fn     getTxPower
  * \brief  gets the transmission power
  *
  * \param  None
  * \return transmission power (see enumerator in the header file)
  --------------------------------------------------------------------------- */
  HM11::txPower_t HM11::getTxPower()
  {
    return txPower_t(getConf(F("POWE")).substring(7).toInt());  // "OK+Set:2"
  }

/** -------------------------------------------------------------------------
  * \fn     setupAsIBeacon
  * \brief  setup module as iBeacon with given data
  *
  * \param  iBeacon  iBeacon structure pointer (see struct in the header file)
  * \return None
  --------------------------------------------------------------------------- */
  void HM11::setupAsIBeacon(iBeaconData_t *iBeacon)
  {
    DebugBLE_println(F("setup as iBeacon"));

    /* control if given parameters are valid */
    if ((iBeacon->name).length() > 12)  {DebugBLE_println(F("name ist too long!")); return;}
    if ((iBeacon->uuid).length() != 32) {DebugBLE_println(F("UUID is invalid!")); return;}
    if (iBeacon->major == 0 || iBeacon->major >= 0xFFFE) {DebugBLE_println(F("major have to be between 0 and 65'534!")); return;}
    if (iBeacon->minor  == 0 || iBeacon->minor  >= 0xFFFE) {DebugBLE_println(F("minor have to be between 0 and 65'534!")); return;}
    if (iBeacon->interv > INTERV_1285MS) {DebugBLE_println(F("unallowed interval!")); return;}

    /* convert given parameters to hex values */
    String majorHex = byteToHexString(uint8_t((iBeacon->major & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->major));
    String minorHex  = byteToHexString(uint8_t((iBeacon->minor  & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->minor));

    /* I-Beacon setup */
    #ifdef DEBUG_BLE
      uint32_t t = millis();
    #endif
    //swResetBLE(); // not necessary
    setConf("MARJ0x" + majorHex);
    setConf("MINO0x" + minorHex);
    setConf("IBE0" + iBeacon->uuid.substring(0, 8));
    setConf("IBE1" + iBeacon->uuid.substring(8, 16));
    setConf("IBE2" + iBeacon->uuid.substring(16, 24));
    setConf("IBE3" + iBeacon->uuid.substring(24, 32));
    setConf("NAME" + iBeacon->name);
    setConf("ADVI" + String(iBeacon->interv));
    setConf(F("ADTY3"));     // advertising type (3 = advertising only)
    setConf(F("IBEA1"));     // enable iBeacon
    setConf(F("DELO2"));     // iBeacon deploy mode (2 = broadcast only)
    setConf(F("PWRM1"));     // auto sleep OFF
    //setConf("PWRM0");        // auto sleep ON   //blup: this should be used -> to send new AT-commands implement wakeUpBLE
    //swResetBLE(); // not necessary

    #ifdef DEBUG_BLE
      /* show BLT address */
      getConf(F("ADDR"));
    #endif

    DebugBLE_print(F("dt setup BLE =\t")); DebugBLE_print(String(millis() - t)); DebugBLE_println(F("ms"));
    DebugBLE_println("");

    // //Debug:
    // sendDirectBLECommand("AT+HELP?", 2000);
    // sendDirectBLECommand("AT+VERS", 2000);
    // sendDirectBLECommand("AT+VER?", 2000);
    // sendDirectBLECommand("AT+VERR?", 2000);
    // sendDirectBLECommand("AT+VERS?", 2000);
    // getConf(F("MARJ"));
    // getConf(F("MINO"));
    // getConf(F("IBE0"));
    // getConf(F("IBE1"));
    // getConf(F("IBE2"));
    // getConf(F("IBE3"));
    // getConf(F("NAME"));
    // getConf(F("ADVI"));
    // getConf(F("ADTY"));     // advertising type (3 = advertising only)
    // getConf(F("IBEA"));     // enable iBeacon
    // getConf(F("DELO"));     // iBeacon deploy mode (2 = broadcast only)
    // getConf(F("PWRM"));     // auto sleep OFF
  }

/** -------------------------------------------------------------------------
  * \fn     setupAsIBeaconDetector
  * \brief  setup module as iBeacon detector
  --------------------------------------------------------------------------- */
  void HM11::setupAsIBeaconDetector()
  {
    DebugBLE_println(F("setup as iBeacon detector"));

    /* iBeacon-Detector setup */
    setConf(F("IMME1"));     // module work type (1 = responds only to AT-commands)
    setConf(F("ROLE1"));     // module role (1 = central = master)
    swResetBLE();
  }

/** -------------------------------------------------------------------------
  * \fn     detectIBeacon
  * \brief  detects near iBeacons and returns data of the first found
  *
  * \param  iBeacon           iBeacon structure pointer (see struct in the header file)
  * \param  maxTimeToSearch   max time to search for iBeacons in ms
  * \return true if an iBeacon was found
  --------------------------------------------------------------------------- */
  bool HM11::detectIBeacon(iBeaconData_t *iBeacon, uint16_t maxTimeToSearch)
  {
    DebugBLE_println(F("detect iBeacons"));

    bool match = false;

    BLESerial_flush();

    /* find near I-Beacons */
    String response = getConf(F("DISI"));

    /* if successful: continue reading to get the devices */
    if (response.indexOf("OK+DISIS") >= 0)
    {
      DebugBLE_println(F("search for devices..."));
      bool timeout = false;
      uint32_t startMillis_BLE_total = millis();
      String data = "";
      iBeacon->accessAddress.reserve(8);
      iBeacon->deviceAddress.reserve(12);
      DebugBLE_print(F("getFreeRAM() = ")); DebugBLE_println(getFreeRAM());
      int16_t freeBytes = getFreeRAM() - MIN_RAM;
      data.reserve(freeBytes);  // reserve other Strings before this one!
      DebugBLE_print(F("getFreeRAM() = ")); DebugBLE_println(getFreeRAM());
      while(data.indexOf("OK+DISCE") < 0 && !timeout && freeBytes > 0)
      {
        if (BLESerial_available() > 0)
        {
          data.concat(String(char(BLESerial_read())));
          freeBytes--;
        }

        if ((millis() - startMillis_BLE_total) >= maxTimeToSearch)
        {
          timeout = true;
          DebugBLE_println(F("timeouted!"));
        }
      }
      DebugBLE_print(F("dt data =\t")); DebugBLE_print((millis() - startMillis_BLE_total)); DebugBLE_println(F("ms"));
      DebugBLE_print(F("data =\t\t")); DebugBLE_println(data);

      /* HW reset to prevent the "AT+DISCE" */
      if (timeout)
      {
        hwResetBLE();
        while(BLESerial_available()) BLESerial_read();  //BLESerial_flush();
      }

      /* get total device count */
      uint16_t j = 0;
      byte deviceCounter;
      for(deviceCounter = 0; data.indexOf("OK+DISC:", j) >= 0; deviceCounter++)
      {
        j = data.indexOf("OK+DISC:", j) + DEFAULT_RESPONSE_LENGTH;
      }
      DebugBLE_print(deviceCounter); DebugBLE_println(F(" device(s) found"));

      // /* convert given uuid to hex */
      // String uuidHex = "";
      // uuidHex.reserve(32);
      // DebugBLE_print(F("iBeacon->uuid = ")); DebugBLE_println(iBeacon->uuid);
      // for (uint8_t n = 0; n < 16; n++)
      // {
      //   uuidHex.concat(byteToHexString(uint8_t((iBeacon->uuid)[n] - '0')));
      // }
      // DebugBLE_print("uuidHex = "); DebugBLE_println(uuidHex);

      DebugBLE_print(F("uuidHex =\t")); DebugBLE_println(iBeacon->uuid);

      /* convert given major and minor to hex */
      String majorHex = byteToHexString(uint8_t((iBeacon->major & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->major));
      DebugBLE_print(F("majorHex =\t")); DebugBLE_println(majorHex);
      String minorHex = byteToHexString(uint8_t((iBeacon->minor & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->minor));
      DebugBLE_print(F("minorHex =\t")); DebugBLE_println(minorHex);


      /* check for a UUID, major and minor match */
      int16_t indexUUID = data.indexOf(iBeacon->uuid);
      if ((indexUUID >= 0) && (data.indexOf(majorHex, indexUUID) >= 0) && (data.indexOf(minorHex, indexUUID) >= 0))
      {
        DebugBLE_println(F("match!"));
        match = true;

        /* process data */
        int16_t indexMatch = data.indexOf(iBeacon->uuid) - 9;
        iBeacon->accessAddress = data.substring(indexMatch, indexMatch+8);
        iBeacon->deviceAddress = data.substring(indexMatch+53, indexMatch+65);
        iBeacon->major        = hexStringToByte(data.substring(indexMatch+42, indexMatch+44)) << 8;
        iBeacon->major       |= hexStringToByte(data.substring(indexMatch+44, indexMatch+46));
        iBeacon->minor         = hexStringToByte(data.substring(indexMatch+46, indexMatch+48)) << 8;
        iBeacon->minor        |= hexStringToByte(data.substring(indexMatch+48, indexMatch+50));
        iBeacon->txPower       = hexStringToByte(data.substring(indexMatch+67, indexMatch+68)) << 8;
        iBeacon->txPower      |= hexStringToByte(data.substring(indexMatch+68, indexMatch+70));
        iBeacon->txPower      *= -1;
      }
      else
      {
        DebugBLE_println(F("no match"));
      };
    }
    DebugBLE_print(F("getFreeRAM() = ")); DebugBLE_println(getFreeRAM());  // make sure its the same as at the beginning of the function

    return match;
  }

/** -------------------------------------------------------------------------
  * \fn     detectIBeaconUUID
  * \brief  detects near iBeacons and returns data of the first found
  *
  * \param  iBeacon           iBeacon structure pointer (see struct in the header file)
  * \param  maxTimeToSearch   max time to search for iBeacons in ms
  * \return true if an iBeacon was found
  --------------------------------------------------------------------------- */
  bool HM11::detectIBeaconUUID(iBeaconData_t *iBeacon, uint16_t maxTimeToSearch)
  {
    DebugBLE_println(F("detect iBeacons"));

    bool match = false;

    BLESerial_flush();

    /* find near I-Beacons */
    String response = getConf(F("DISI"));

    /* if successful: continue reading to get the devices */
    if (response.indexOf("OK+DISIS") >= 0)
    {
      DebugBLE_println(F("search for devices..."));
      bool timeout = false;
      uint32_t startMillis_BLE_total = millis();
      DebugBLE_print(F("getFreeRAM() = ")); DebugBLE_println(getFreeRAM());
      String data = "";
      iBeacon->accessAddress.reserve(8);
      iBeacon->deviceAddress.reserve(12);
      String temp = "";
      temp.reserve(78);
      data.reserve(86);
      //78 = sizeOf(OK+DISC:4C000215:00D7D3EE02E4470E97DA78CFAC4027CC:00C80007BA:000780031354:-071)
      //86 = sizeOf(OK+DISC:4C000215:00D7D3EE02E4470E97DA78CFAC4027CC:00C80007BA:000780031354:-071OK+DISCE)
      DebugBLE_print(F("getFreeRAM() = ")); DebugBLE_println(getFreeRAM());
      while(data.indexOf("OK+DISCE") < 0 && !timeout)
      {
        if (BLESerial_available() > 0)
        {
          if (!match)
          {
            data.concat(String(char(BLESerial_read())));
            if (data.length() >= 78 && data.indexOf("OK+DISCE") < 0)
            {
              DebugBLE_print(F("data = ")); DebugBLE_println(data);
              if (data.indexOf(iBeacon->uuid) >= 0) match = true;
              else data = ""; // reset String
            }
          }
          else
          {
            temp.concat(String(char(BLESerial_read())));
            if (temp.length() >= 78)
            {
              DebugBLE_print(F("temp = ")); DebugBLE_println(temp);
              temp = "";
            }
            else if (temp.indexOf("OK+DISCE") >= 0) data += temp;
          }
        }

        if ((millis() - startMillis_BLE_total) >= maxTimeToSearch)
        {
          timeout = true;
          DebugBLE_println(F("timeouted!"));
        }
      }
      DebugBLE_print(F("dt data =\t")); DebugBLE_print((millis() - startMillis_BLE_total)); DebugBLE_println(F("ms"));
      DebugBLE_print(F("data =\t\t")); DebugBLE_println(data);

      /* HW reset to prevent the "AT+DISCE" */
      if (timeout)
      {
        hwResetBLE();
        while(BLESerial_available()) BLESerial_read();  //BLESerial_flush();
      }

      /* check for a UUID match */
      DebugBLE_print(F("uuidHex =\t")); DebugBLE_println(iBeacon->uuid);
      if (match)
      {
        DebugBLE_println(F("match!"));

        /* process data */
        int16_t indexMatch = data.indexOf(iBeacon->uuid) - 9;
        iBeacon->accessAddress = data.substring(indexMatch, indexMatch+8);
        iBeacon->deviceAddress = data.substring(indexMatch+53, indexMatch+65);
        iBeacon->major        = hexStringToByte(data.substring(indexMatch+42, indexMatch+44)) << 8;
        iBeacon->major       |= hexStringToByte(data.substring(indexMatch+44, indexMatch+46));
        iBeacon->minor         = hexStringToByte(data.substring(indexMatch+46, indexMatch+48)) << 8;
        iBeacon->minor        |= hexStringToByte(data.substring(indexMatch+48, indexMatch+50));
        iBeacon->txPower       = hexStringToByte(data.substring(indexMatch+67, indexMatch+68)) << 8;
        iBeacon->txPower      |= hexStringToByte(data.substring(indexMatch+68, indexMatch+70));
        iBeacon->txPower      *= -1;
      }
      else
      {
        DebugBLE_println(F("no match"));
      };
    }
    DebugBLE_print(F("getFreeRAM() = ")); DebugBLE_println(getFreeRAM());  // make sure its the same as at the beginning of the function

    return match;
  }

  //TODO: implement detectIBeacons() (plural)

     // /* process data -> extract the devices from the data */
     // byte deviceCounter;
     // String deviceString[MAX_NUMBER_IBEACONS];
     // for (deviceCounter = 0; deviceCounter < (data.length()/NUMBER_CHARS_PER_DEVICE) && (deviceCounter < MAX_NUMBER_IBEACONS); deviceCounter++)
     // {
     //   uint16_t deviceIndex = deviceCounter * NUMBER_CHARS_PER_DEVICE;
     //   deviceString[deviceCounter].reserve(NUMBER_CHARS_PER_DEVICE);  // reserve memory to prevent fragmentation
     //   deviceString[deviceCounter] = data.substring(deviceIndex, deviceIndex + NUMBER_CHARS_PER_DEVICE);
     //   DebugBLE_print(F("device_")); DebugBLE_print(deviceCounter); DebugBLE_print(F(" =\t")); DebugBLE_println(deviceString[deviceCounter]);
     // }
     // DebugBLE_println("");

     // if (deviceString[0].length() == NUMBER_CHARS_PER_DEVICE)
     // {
     //   /* process device data */
     //   for (byte n = 0; n < deviceCounter; n++)
     //   {
     //     iBeaconData[n].accessAddress = deviceString[n].substring(8, 16);
     //     //DebugBLE_print(F("accessAdr_")); DebugBLE_print(String(n)); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].accessAddress);
     //     iBeaconData[n].uuid = deviceString[n].substring(17, 49);
     //     //DebugBLE_print(F("UUID_")); DebugBLE_print(String(n)); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].uuid);
     //     iBeaconData[n].major = deviceString[n].substring(50, 54);
     //     DebugBLE_print(F("Major_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].major);
     //     iBeaconData[n].minor = deviceString[n].substring(54, 58);
     //     //DebugBLE_print(F("Minor_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].minor);
     //     iBeaconData[n].deviceAddress = deviceString[n].substring(61, 73);
     //     DebugBLE_print(F("deviceAdr_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].deviceAddress);
     //     iBeaconData[n].txPower = deviceString[n].substring(74, 78).toInt();
     //     DebugBLE_print(F("TxPower_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].txPower);
     //     DebugBLE_println("");

     //     /* handle match */
     //     if (iBeaconData[currentMatchDeviceNr].major == String(KEY_MAJOR_ID))
     //     {
     //       currentMatchDeviceNr = n;
     //       DebugBLE_print(F("Device ")); DebugBLE_print(currentMatchDeviceNr); DebugBLE_println(F(" mached!"));
     //       match = true;
     //       break;
     //     }
     //   }
     // }


  //TODO: implement sleepBLE and test wakeUpBLE

  // void wakeUpBLE()
  // {
  //  DebugBLE_println(F("wakeing up BLE..."));

  //  /* wake up -> send a long random string */
  //  String wakeUpStr = "";
  //  wakeUpStr.reserve(200);
  //  for(int n = 0; n < 200; n++)
  //  {
  //    wakeUpStr.concat(char(random(66, 90)));  // 66 = 'B'... 90 = 'Z'
  //  }
  //  BLESerial_begin(BAUDRATE0);
  //  BLESerial_print(wakeUpStr);
  //  BLESerial_readString();
  //  BLESerial_begin(baudrate_);
  //  BLESerial_flush();
  // }

/** -------------------------------------------------------------------------
  * \fn     getMacAddress
  * \brief  reads the mac address of the BLE module
  *
  * \return mac address
  --------------------------------------------------------------------------- */
  String HM11::getMacAddress()
  {
    String macAddr = F("error");
    macAddr.reserve(12);
    String response = getConf(F("ADDR"));
    if (response.startsWith(F("OK")))
    {
      response = response.substring(8);   // OK+ADDR:MACAddress
      if (response.length() == 12) macAddr = response;
    }
    return macAddr;
  }

/** -------------------------------------------------------------------------
  * \fn     connectToMacAddress
  * \brief  connects to given mac Address
  *
  * \param  macAddr mac address
  * \param  master  connect as master (true) or slave (false)
  * \return None
  --------------------------------------------------------------------------- */
  void HM11::connectToMacAddress(String macAddr, bool master)
  {
    setConf(F("IMME1"));      // module work type (1 = responds only to AT-commands)
    setConf("ROLE" + String(master));
    swResetBLE();
    setConf("CON" + macAddr);
  }

/** -------------------------------------------------------------------------
  * \fn     readChar
  * \brief  reads and converts a character from BLESerial
  *
  * \param  None
  * \return character
  --------------------------------------------------------------------------- */
  char HM11::readChar()
  {
    static uint8_t lastB = 0;
    uint8_t b = BLESerial_read();
    if (lastB == 13 && b == 225) b = 10; // handle cr/lf
    if (b >= 128) b -= 128;
    lastB = b;
    return char(b);
  }

/** -------------------------------------------------------------------------
  * \fn     handshaking
  * \brief  handshaking to sync a P2P connection
  *
  * \param  master          sync as master (true) or slave (false)
  * \param  handshakeChar   random handshake character
  * \return true if it succeeded
  --------------------------------------------------------------------------- */
  bool HM11::handshaking(bool master, char handshakeChar)
  {
    const uint16_t timeout = 10000;
    const uint16_t dtMax = 100;
    uint32_t msTimeout = millis();

    /* empty the recive buffer */
    while(BLESerial_available())
    {
      BLESerial_read();
      if ((millis() - msTimeout) >= timeout) return false;
    }
    msTimeout = millis();

    /* handshaking */
    uint32_t ms = millis();
    if (master)
    {
      DebugBLE_print(F("wait for handshake char..."));
      while(BLESerial_read() != handshakeChar)
      {
        ms = millis();

        DebugBLE_print(F("."));
        if ((millis() - msTimeout) >= timeout) return false;
        while(!BLESerial_available() && (millis() - ms) < dtMax);
      }
      BLESerial_print(String(handshakeChar));
      delay(dtMax);
    }
    else
    {
      DebugBLE_print(F("send handshake char..."));
      while(BLESerial_read() != handshakeChar)
      {
        BLESerial_print(String(handshakeChar));
        ms = millis();

        DebugBLE_print(F("."));
        if ((millis() - msTimeout) >= timeout) return false;
        while(!BLESerial_available() && (millis() - ms) < dtMax);
      }
      uint16_t dt = millis() - ms;
      // DebugBLE_print(F("dt = ")); DebugBLE_println(dt);
      delay(dtMax - dt/2);
    }
    DebugBLE_println();
    DebugBLE_println(F("handshake succeeded!"));
    return true;
  }

/** -------------------------------------------------------------------------
  * \fn     forceRenew
  * \brief  try this if you can not communicate with the BLE module anymore
  --------------------------------------------------------------------------- */
  void HM11::forceRenew()
  {
    baudrate_t baudratesArray[] = {BAUDRATE0, BAUDRATE1, BAUDRATE2, BAUDRATE3, BAUDRATE4};
    enable();
    for (uint8_t i = 0; i < sizeof(baudratesArray)/sizeof(baudrate_t); i++)
    {
      DebugBLE_println(baudratesArray[i]);
      BLESerial_begin(baudratesArray[i]);
      while(!BLESerial_ready());
      for (uint8_t n = 0; n < 5; n++) sendDirectBLECommand(F("AT"));
      for (uint8_t n = 0; (n < 5) && !setConf(F("RENEW")); n++) delay(MAX_DELAY_AFTER_SW_RESET_BLE);
    }
    disable();
  }

/* ======================== Public class Functions ========================== */
/** -------------------------------------------------------------------------
  * \fn     byteToHexString
  * \brief  converts given hex byte to a String with two characters
  *
  * \param  hex   hex byte
  * \return Strig with two HEX characters
  --------------------------------------------------------------------------- */
  String HM11::byteToHexString(uint8_t hex)
  {
    String str;
    str.reserve(2);
    str.concat(nibbleToHexCharacter((hex & 0xF0) >> 4));
    str.concat(nibbleToHexCharacter(hex & 0x0F));
    return str;
  }

/** -------------------------------------------------------------------------
  * \fn     hexStringToByte
  * \brief  converts given hex String with two characters to a byte
  *
  * \param  str   hex String
  * \return hex byte
  --------------------------------------------------------------------------- */
  uint8_t HM11::hexStringToByte(String str)
  {
    return ((hexCharacterToNibble(str[0]) << 4) & 0xF0) | (hexCharacterToNibble(str[1]) & 0x0F);
  }

/* ======================= Private member Functions ========================= */
/** -------------------------------------------------------------------------
  * \fn     hwResetBLE
  * \brief  resets BLE module by HW
  --------------------------------------------------------------------------- */
  void HM11::hwResetBLE()
  {
    clearBit(*rstPort_, rstPin_);
    delay(RESET_DELAY);
    setBit(*rstPort_, rstPin_);
    uint32_t ms = millis();
    while(!sendDirectBLECommand(F("AT")).startsWith(F("OK")) && ((millis() - ms) < MAX_DELAY_AFTER_HW_RESET_BLE));
  }

/** -------------------------------------------------------------------------
  * \fn     swResetBLE
  * \brief  resets BLE module by SW
  --------------------------------------------------------------------------- */
  void HM11::swResetBLE()
  {
    setConf(F("RESET"));
    uint32_t ms = millis();
    /* first wait until RESET starts to work (~582ms)... */
    while(sendDirectBLECommand(F("AT")).startsWith(F("OK")) && ((millis() - ms) < MAX_DELAY_AFTER_SW_RESET_BLE));
    /* then wait until the BLE module is ready again (~120ms) */
    while(!sendDirectBLECommand(F("AT")).startsWith(F("OK")) && ((millis() - ms) < MAX_DELAY_AFTER_SW_RESET_BLE));
  }

/** -------------------------------------------------------------------------
  * \fn     renewBLE
  * \brief  restores BLE module to factory default
  *
  * \return true if it succeeded
  --------------------------------------------------------------------------- */
  bool HM11::renewBLE()
  {
    setConf(F("RENEW"));              // restore all setup to factory default
    uint32_t ms = millis();
    /* first wait until RENEW starts to work (~327ms)... */
    while(!sendDirectBLECommand(F("AT")).startsWith(F("OK")) && ((millis() - ms) < MAX_DELAY_AFTER_SW_RESET_BLE));
    /* then wait while the BLE module is busy (~250ms)... */
    while(sendDirectBLECommand(F("AT")).startsWith(F("OK")) && ((millis() - ms) < MAX_DELAY_AFTER_SW_RESET_BLE));
    /* then wait until the BLE module is ready again (~230ms) */
    while(!sendDirectBLECommand(F("AT")).startsWith(F("OK")) && ((millis() - ms) < MAX_DELAY_AFTER_SW_RESET_BLE));
    return setBaudrate();
  }

/** -------------------------------------------------------------------------
  * \fn     setConf
  * \brief  configures BLE module by writing given AT command
  *
  * \param  cmd  AT command
  * \return ture if it succeeded
  --------------------------------------------------------------------------- */
  bool HM11::setConf(String cmd)
  {
    String response = sendDirectBLECommand("AT+" + cmd);
    return response.indexOf("OK") > -1 ? true : false;
  }

/** -------------------------------------------------------------------------
  * \fn     getConf
  * \brief  gets configured value of the BLE module with given AT command
  *
  * \param  cmd  AT command
  * \return configured value as a string
  --------------------------------------------------------------------------- */
  String HM11::getConf(String cmd)
  {
    return sendDirectBLECommand("AT+" + cmd + "?");
  }

/** -------------------------------------------------------------------------
  * \fn     setBaudrate
  * \brief  sets baudrtae of the BLE module
  *
  * \param  baudrate  baudrate (see enumerator in the header file)
  * \return true if it succeeded
  --------------------------------------------------------------------------- */
  bool HM11::setBaudrate(baudrate_t baudrate)
  {
    baudrate_ = baudrate;
    return setBaudrate();
  }

/** -------------------------------------------------------------------------
  * \fn     setBaudrate
  * \brief  sets baudrtae of the BLE module
  *
  * \return true if it succeeded
  --------------------------------------------------------------------------- */
  bool HM11::setBaudrate()
  {
    bool successful = true;
    uint32_t currentBaudrate = getBaudrate();
    DebugBLE_print(F("currentBaudrate = ")); DebugBLE_println(currentBaudrate);
    DebugBLE_println("");

    if (currentBaudrate != 0)
    {
      if (currentBaudrate != baudrate_)
      {
        /* set baudrate */
        DebugBLE_println(F("set new baudrate..."));
        if (currentBaudrate != BAUDRATE0) renewBLE();

        BLESerial_begin(DEFAULT_BAUDRATE);
        while(!BLESerial_ready());

        switch(baudrate_)
        {
          case BAUDRATE0: setConf(F("BAUD0")); break;
          case BAUDRATE1: setConf(F("BAUD1")); break;
          case BAUDRATE2: setConf(F("BAUD2")); break;
          case BAUDRATE3: setConf(F("BAUD3")); break;
          case BAUDRATE4: setConf(F("BAUD4")); break;
          default: //handleError("invalid baudrate!");
          {
            DebugBLE_println(F("invalid baudrate!"));
            successful = false;//while(1);
          }
        }

        swResetBLE();

        BLESerial_begin(baudrate_);
        while(!BLESerial_ready());

        /* check if setting the baudrate failed */
        if (getConf(F("BAUD")).indexOf("OK") < 0) //handleError("set baudrate failed!");
        {
          DebugBLE_println(F("set baudrate failed!"));
          successful = false;//while(1);
        }
      }
    }
    else successful = false;

    return successful;
  }

/** -------------------------------------------------------------------------
  * \fn     getBaudrate
  * \brief  gets baudrtae of the BLE module
  *
  * \return baudrate (see enumerator in the header file)
  --------------------------------------------------------------------------- */
  uint32_t HM11::getBaudrate()
  {
    DebugBLE_println(F("getBaudrate..."));

    baudrate_t baudratesArray[] = {BAUDRATE0, BAUDRATE1, BAUDRATE2, BAUDRATE3, BAUDRATE4};

    uint8_t i;
    String response = "";
    for (i = 0; (response.indexOf("OK") < 0) && (i < sizeof(baudratesArray)/sizeof(baudrate_t)); i++)
    {
      DebugBLE_println(baudratesArray[i]);
      BLESerial_begin(baudratesArray[i]);
      while(!BLESerial_ready());
      for (uint8_t n = 0; (n < 5) && (response.indexOf("OK") < 0); n++)
      {
        /* try 5 times per baudrate */
        response = sendDirectBLECommand(F("AT"));
      }
    }

    if (i >= sizeof(baudratesArray)/sizeof(baudrate_t))
    {
      //handleError(F("determining the current baudrate of the BLE failed!"));
      DebugBLE_println(F("determining the baudrate failed!"));
      baudratesArray[i-1] = baudrate_t(0);//while(1);
    }

    return  baudratesArray[i-1];
  }

/** -------------------------------------------------------------------------
  * \fn     sendDirectBLECommand
  * \brief  sends a direct AT command to the BLE module
  *
  * \param  cmd       AT command
  * \param  timeout   time in ms before timeout
  * \return response of the BLE module as a string
  --------------------------------------------------------------------------- */
  String HM11::sendDirectBLECommand(String cmd, uint16_t timeout)
  {
    bool failed = false;
    String response = "";
    response.reserve(DEFAULT_RESPONSE_LENGTH);
    /* wait for more data if the cmd has a '+' */
    bool waitForMore = false;
    if(cmd.indexOf("+") >= 0) waitForMore = true;
    /* send command */
    DebugBLE_print(F("send:\t\t")); DebugBLE_println(cmd);
    BLESerial_print(cmd);
    /* get response */
    uint32_t startMillis_BLE = millis();
    while ((response.indexOf("OK") < 0 || BLESerial_available() || waitForMore) && !failed)
    {
      if (BLESerial_available())
      {
        response.concat(char(BLESerial_read()));

        /* special delay (necessary! -> wait some time to let the Serial buffer be filled) */
        #ifdef DEBUG_BLE
          //DebugBLE_print(F("got:\t\t")); DebugBLE_println(response);
          delay(1); // >= 1ms
        #else
          delay(1); // >= 1ms
        #endif
      }

      /* stop waiting for more data if we got a '+' */
      if (waitForMore && response.indexOf("+") >= 0) waitForMore = false;

      if ((millis() - startMillis_BLE) >= timeout)
      {
        failed = true;
        response = "error";
        DebugBLE_println(F("reading response timeouted!"));
      }
    }

    /* print response */
    DebugBLE_print(F("received:\t")); DebugBLE_println(response);
    DebugBLE_print(F("dt =\t\t")); DebugBLE_print(String(millis() - startMillis_BLE)); DebugBLE_println(F("ms"));
    DebugBLE_println("");

    BLESerial_flush();

    response.trim();

    return response;
  }

/* ======================= Private class Functions ========================== */
/** -------------------------------------------------------------------------
  * \fn     getFreeRAM
  * \brief  returns the size in bytes between the heap and the stack
  *
  * \return free memory between heap and stack
  --------------------------------------------------------------------------- */
  int16_t HM11::getFreeRAM()
  {
    extern int16_t __heap_start, *__brkval;
    int16_t v;
    return (int16_t) &v - (__brkval == 0 ? (int16_t) &__heap_start : (int16_t) __brkval);
  }

/** -------------------------------------------------------------------------
  * \fn     nibbleToHexCharacter
  * \brief  converts given nibble to a hex character
  *
  * \param  nibble   nibble (as byte)
  * \return hex character
  --------------------------------------------------------------------------- */
  char HM11::nibbleToHexCharacter(uint8_t nibble)
  {
    return (nibble > 9) ? (char)(nibble + 'A' - 10) : (char)(nibble + '0');
  }

/** -------------------------------------------------------------------------
  * \fn     hexCharacterToNibble
  * \brief  converts given hex character to a nibble
  *
  * \param  hex   hex character
  * \return nibble (as byte)
  --------------------------------------------------------------------------- */
  uint8_t HM11::hexCharacterToNibble(char hex)
  {
    return (hex >= 'A') ? (uint8_t)(hex - 'A' + 10) : (uint8_t)(hex - '0');
  }
