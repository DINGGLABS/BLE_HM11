/*******************************************************************************
* \file    HM11.cpp
********************************************************************************
* \author  Jascha Haldemann jh@oxon.ch
* \date    01.02.2017
* \version 1.0
*******************************************************************************/

/* ================================= Imports ================================ */
#include "HM11.h"

/* ======================= Module constant declaration ====================== */

/* ======================== Public member Functions ========================= */
/** -------------------------------------------------------------------------
  * \fn     begin
  * \brief  init the HW
  *
  * \param  baudrate  baudrate (see enumerator in the header-file)
  * \return true if setting the baudrate succeeded
  --------------------------------------------------------------------------- */
  bool HM11::begin(uint32_t baudrate)
  {
    #ifdef DEBUG_BLE
      Serial.begin(DEBUG_BLE_BAUDRATE);
    #endif

    baudrate_ = baudrate_t(baudrate);
    enable();
    return setBaudrate();
  }

/** -------------------------------------------------------------------------
  * \fn     end
  * \brief  deinit the HW
  --------------------------------------------------------------------------- */
  void HM11::end()
  {
    disable();
  }

/** -------------------------------------------------------------------------
  * \fn     enable
  * \brief  enable the HW
  --------------------------------------------------------------------------- */
  void HM11::enable()
  {
    DebugBLE_println(F("enable BLE"));
    if (baudrate_ == 0) baudrate_ = DEFAULT_BAUDRATE;
    setBit(*rstPort_, rstPin_);     // stop resetting
    clearBit(*enPort_, enPin_);     // enable BLE
    BLESerial_begin(baudrate_);
    while(!BLESerial_ready());
    while(BLESerial_available()) (void)BLESerial_read();  // empty tx-buffer
    hwResetBLE();
  }

/** -------------------------------------------------------------------------
  * \fn     disable
  * \brief  disable the HW
  --------------------------------------------------------------------------- */
  void HM11::disable()
  {
    DebugBLE_println(F("disable BLE"));
    clearBit(*rstPort_, rstPin_);   // to prevent supply throug reset
    BLESerial_end();
    clearBit(rxdPort_, rxd_);       // to prevent supply throug rxd
    clearBit(txdPort_, txd_);       // to prevent supply throug txd
    setBit(*enPort_, enPin_);       // disable BLE
  }

/** -------------------------------------------------------------------------
  * \fn     isEnabled
  * \brief  return the state of the HW
  *
  * \return true if the HW is enabled
  --------------------------------------------------------------------------- */
  bool HM11::isEnabled()
  {
    return !getBit(*enPort_, enPin_);
  }

/** -------------------------------------------------------------------------
  * \fn     setupAsIBeacon
  * \brief  setup module as iBeacon with given data
  *
  * \param  iBeacon  iBeacon structure pointer (see struct in the header-file)
  * \return None
  --------------------------------------------------------------------------- */
  void HM11::setupAsIBeacon(iBeaconData_t *iBeacon)
  {
    DebugBLE_println(F("setup BLE-Module as I-Beacon"));

    bool uuidValid = true;

    /* control if given parameters are valid */
    if ((iBeacon->name).length() > 12)  {DebugBLE_println(F("name ist too long!")); return;}
    if ((iBeacon->uuid).length() != 32) {DebugBLE_println(F("UUID is invalid!")); return;}
    if (iBeacon->marjor == 0 || iBeacon->marjor >= 0xFFFE) {DebugBLE_println(F("marjor have to be between 0 and 65'534!")); return;}
    if (iBeacon->minor  == 0 || iBeacon->minor  >= 0xFFFE) {DebugBLE_println(F("minor have to be between 0 and 65'534!")); return;}
    if (iBeacon->interv > INTERV_1285MS) {DebugBLE_println(F("unallowed interval!")); return;}

    /* convert given parameters to hex values */
    String marjorHex = byteToHexString(uint8_t((iBeacon->marjor & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->marjor));
    String minorHex  = byteToHexString(uint8_t((iBeacon->minor  & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->minor));

    /* I-Beacon setup */
    uint32_t t = millis();
    renewBLE();    // necessary!
    swResetBLE();
    setConf("MARJ0x" + marjorHex);
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
    swResetBLE();

    /* show BLT address */
    getConf("ADDR");

    DebugBLE_print(F("dt setup BLE =\t")); DebugBLE_print(String(millis() - t)); DebugBLE_println(F("ms"));
    DebugBLE_println("");

    // //Debug:
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
    DebugBLE_println(F("setup BLE-Module as I-Beacon detector"));

    /* iBeacon-Detector setup */
    setConf(F("IMME1"));     // module work type (1 = responds only to AT-commands)
    setConf(F("ROLE1"));     // module role (1 = central = master)
    swResetBLE();
  }

/** -------------------------------------------------------------------------
  * \fn     detectIBeacon
  * \brief  detects near iBeacons and returns data of the first found
  *
  * \param  iBeacon           iBeacon structure pointer (see struct in the header-file)
  * \param  maxTimeToSearch   max time to search for iBeacons in ms
  * \return None
  --------------------------------------------------------------------------- */
  bool HM11::detectIBeacon(iBeaconData_t *iBeacon, uint16_t maxTimeToSearch)
  {
    DebugBLE_println(F("detect I-Beacons"));

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
      if (timeout) hwResetBLE();

      /* get total device count */
      uint16_t j = 0;
      byte deviceCounter;
      for(deviceCounter = 0; data.indexOf("OK+DISC:", j) >= 0; deviceCounter++)
      {
        j = data.indexOf("OK+DISC:", j) + DEFAULT_RESPONSE_LENGTH;
      }
      DebugBLE_print(deviceCounter); DebugBLE_println(F(" device(s) found"));

      /* convert given uuid to hex */
      String uuidHex = "";
      uuidHex.reserve(32);
      DebugBLE_print(F("iBeacon->uuid = ")); DebugBLE_println(iBeacon->uuid);
      for (uint8_t n = 0; n < 16; n++)
      {
        uuidHex.concat(byteToHexString(uint8_t((iBeacon->uuid)[n] - '0')));
      }
      DebugBLE_print("uuidHex = "); DebugBLE_println(uuidHex);

      /* convert given marjor and minor to hex */
      String marjorHex = byteToHexString(uint8_t((iBeacon->marjor & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->marjor));
      DebugBLE_print("marjorHex = "); DebugBLE_println(marjorHex);
      String minorHex = byteToHexString(uint8_t((iBeacon->minor & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->minor));
      DebugBLE_print("minorHex = "); DebugBLE_println(minorHex);

      /* check for a UUID match */
      uint16_t indexUUID = data.indexOf(uuidHex);
      if ((indexUUID >= 0) && (data.indexOf(marjorHex, indexUUID) >= 0) && (data.indexOf(minorHex, indexUUID) >= 0))
      {
        DebugBLE_println(F("match!"));
        match = true;

        /* process data */
        uint16_t indexMatch = data.indexOf(uuidHex) - 9;
        iBeacon->accessAddress = data.substring(indexMatch, indexMatch+8);
        iBeacon->deviceAddress = data.substring(indexMatch+53, indexMatch+65);
        iBeacon->marjor        = hexStringToByte(data.substring(indexMatch+42, indexMatch+44)) << 8;
        iBeacon->marjor       |= hexStringToByte(data.substring(indexMatch+44, indexMatch+46));
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
     //   unsigned int deviceIndex = deviceCounter * NUMBER_CHARS_PER_DEVICE;
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
     //     iBeaconData[n].marjor = deviceString[n].substring(50, 54);
     //     DebugBLE_print(F("Major_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].marjor);
     //     iBeaconData[n].minor = deviceString[n].substring(54, 58);
     //     //DebugBLE_print(F("Minor_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].minor);
     //     iBeaconData[n].deviceAddress = deviceString[n].substring(61, 73);
     //     DebugBLE_print(F("deviceAdr_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].deviceAddress);
     //     iBeaconData[n].txPower = deviceString[n].substring(74, 78).toInt();
     //     DebugBLE_print(F("TxPower_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].txPower);
     //     DebugBLE_println("");

     //     /* handle match */
     //     if (iBeaconData[currentMatchDeviceNr].marjor == String(KEY_MAJOR_ID))
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
  //  (void)BLESerial_readString();
  //  BLESerial_begin(baudrate_);
  //  BLESerial_flush();
  // }

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
      for (uint8_t n = 0; (n < 5) && !setConf(F("RENEW")); n++) delay(DELAY_AFTER_SW_RESET_BLE);
    }
    disable();
  }

/* ==================== Public static member Functions ====================== */
/** -------------------------------------------------------------------------
  * \fn     byteToHexString
  * \brief  converts given hex byte to a String with two characters
  *
  * \param  hex   hex byte
  * \return Strig with two HEX characters
  --------------------------------------------------------------------------- */
  static String HM11::byteToHexString(uint8_t hex)
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
  static uint8_t HM11::hexStringToByte(String str)
  {
    return ((hexCharacterToNibble(str[0]) << 4) & 0xF0) | (hexCharacterToNibble(str[1]) & 0x0F);
  }

/* ======================= Private member Functions ========================= */
/** -------------------------------------------------------------------------
  * \fn     hwResetBLE
  * \brief  reset BLE module by HW
  --------------------------------------------------------------------------- */
  void HM11::hwResetBLE()
  {
    clearBit(*rstPort_, rstPin_);
    delay(5);
    setBit(*rstPort_, rstPin_);
    delay(DELAY_AFTER_HW_RESET_BLE);   // discovered empirically
  }

/** -------------------------------------------------------------------------
  * \fn     swResetBLE
  * \brief  reset BLE module by SW
  --------------------------------------------------------------------------- */
  void HM11::swResetBLE()
  {
    setConf(F("RESET"));
    delay(DELAY_AFTER_SW_RESET_BLE);  // a long delay is necessary
  }

/** -------------------------------------------------------------------------
  * \fn     renewBLE
  * \brief  restore BLE module to factory default
  --------------------------------------------------------------------------- */
  void HM11::renewBLE()
  {
    setConf(F("RENEW"));              // restore all setup to factory default
    delay(DELAY_AFTER_SW_RESET_BLE);  // a long delay is necessary
    setBaudrate();
  }

/** -------------------------------------------------------------------------
  * \fn     setConf
  * \brief  configure BLE module by writing given AT command
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
  * \brief  get configured value of the BLE module with given AT command
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
  * \brief  set baudrtae of the BLE module
  *
  * \param  baudrate  baudrate (see enumerator in the header-file)
  * \return true if it succeeded
  --------------------------------------------------------------------------- */
  bool HM11::setBaudrate(baudrate_t baudrate)
  {
    baudrate_ = baudrate;
    return setBaudrate();
  }

/** -------------------------------------------------------------------------
  * \fn     setBaudrate
  * \brief  set baudrtae of the BLE module
  *
  * \return true if it succeeded
  --------------------------------------------------------------------------- */
  bool HM11::setBaudrate()
  {
    bool successful = true;
    uint32_t currentBaudrate = getBaudrate();
    DebugBLE_print(F("currentBaudrate = ")); DebugBLE_println(String(currentBaudrate));
    DebugBLE_println("");

    if ((currentBaudrate != 0) && (currentBaudrate != baudrate_))
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

    return successful;
  }

/** -------------------------------------------------------------------------
  * \fn     getBaudrate
  * \brief  get baudrtae of the BLE module
  *
  * \return baudrate  baudrate (see enumerator in the header-file)
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
      DebugBLE_println(F("determining the current baudrate of the BLE failed!"));
      baudratesArray[i-1] = baudrate_t(0);//while(1);
    }

    return  baudratesArray[i-1];
  }

/** -------------------------------------------------------------------------
  * \fn     sendDirectBLECommand
  * \brief  sends a direct AT command to the BLE module
  *
  * \param  cmd  AT command
  * \return response of the BLE module as a string
  --------------------------------------------------------------------------- */
  String HM11::sendDirectBLECommand(String cmd)
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
          DebugBLE_print(F("got:\t\t")); DebugBLE_println(response);
        #else
          delayMicroseconds(100);
        #endif
      }

      /* stop waiting for more data if we got a '+' */
      if (waitForMore && response.indexOf("+") >= 0) waitForMore = false;

      if ((millis() - startMillis_BLE) >= COMMAND_TIMEOUT_TIME)
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

/* ==================== Private static member Functions ===================== */
/** -------------------------------------------------------------------------
  * \fn     getFreeRAM
  * \brief  returns the size in bytes between the heap and the stack
  *
  * \return free memory between heap and stack
  --------------------------------------------------------------------------- */
  static int16_t HM11::getFreeRAM()
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
  static char HM11::nibbleToHexCharacter(uint8_t nibble)
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
  static uint8_t HM11::hexCharacterToNibble(char hex)
  {
    return (hex >= 'A') ? (uint8_t)(hex - 'A' + 10) : (uint8_t)(hex - '0');
  }
