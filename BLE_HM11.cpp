/**-----------------------------------------------------------------------------
 * \file    BLE_HM11.cpp
 * \author  jh
 * \date    xx.01.2017
 * @{
 -----------------------------------------------------------------------------*/

 /* Includes --------------------------------------------------- */
#include "BLE_HM11.h"

/* Public ----------------------------------------------------- */
void BLE_HM11::begin(uint32_t baudrate)
{
  baudrate_ = baudrate_t(baudrate);

  enableBLE();

  uint32_t currentBaudrate = getBaudrate();
  DebugBLE_print(F("currentBaudrate = ")); DebugBLE_println(String(currentBaudrate));

  if (currentBaudrate != baudrate_)
  {
    /* set baudrate */
    DebugBLE_println(F("set new baudrate..."));
    setConf(F("RENEW"));  // restore all setup to factory default
    delay(DELAY_AFTER_SW_RESET_BLE);  //blup: a long delay is necessary
    
    BLESerial_.begin(DEFAULT_BAUDRATE);
    while(!BLESerial_);

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
        while(1);
      }
    }
    
    swResetBLE();
    
    BLESerial_.begin(baudrate_);
    while(!BLESerial_);
    
    /* check if setting the baudrate failed */
    if (getConf(F("BAUD")).indexOf("OK") < 0) //handleError("set baudrate failed!");
    {
      DebugBLE_println(F("set baudrate failed!"));
      while(1);
    }
  }
}

void BLE_HM11::end()
{
  disableBLE();
}

void BLE_HM11::setupAsIBeacon(iBeaconData_t *iBeacon)
{
  DebugBLE_println(F("setup BLE-Module as I-Beacon"));

  /* control if given parameters are valid */
  if ((iBeacon->name).length() > 12) {DebugBLE_println(F("name ist too long!")); return;}
  bool uuidValid = true;
  if ((iBeacon->uuid).length() != 16) uuidValid = false;
  for (uint8_t n = 0; n < 16; n++)
  {
    if ((iBeacon->uuid)[n] < '1' || (iBeacon->uuid)[n] > '9')
    {
      uuidValid = false;
      break;
    }
  }
  if (!uuidValid)
  {
    DebugBLE_println(F("The UUID has to consist of 16 numbers (1... 9)!"));
    return;
  }
  if (iBeacon->major == 0) {DebugBLE_println(F("major can not be 0!")); return;}
  if (iBeacon->minor == 0) {DebugBLE_println(F("minor can not be 0!")); return;}
  if ((iBeacon->interv == 0) || (iBeacon->interv > INTERV_1285MS)) {DebugBLE_println(F("unallowed interval!")); return;}

  /* convert given parameters to hex values */
  String uuidHex = "";
  uuidHex.reserve(32);
  for (uint8_t n = 0; n < 16; n++)
  {
    uuidHex += byteToHexString(uint8_t((iBeacon->uuid)[n] - '0'));
  }
  String majorHex = byteToHexString(uint8_t((iBeacon->major & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->major));
  String minorHex = byteToHexString(uint8_t((iBeacon->minor & 0xFF00) >> 8)) + byteToHexString(uint8_t(iBeacon->minor));

  /* I-Beacon setup */
  uint32_t t = millis();
  swResetBLE();
  setConf("MARJ0x" + majorHex);
  setConf("MINO0x" + minorHex);
  setConf("IBE0" + uuidHex.substring(0, 8));
  setConf("IBE1" + uuidHex.substring(8, 16));
  setConf("IBE2" + uuidHex.substring(16, 24));
  setConf("IBE3" + uuidHex.substring(24, 32));
  setConf("NAME" + iBeacon->name);
  setConf("ADVI" + String(iBeacon->interv));
  setConf(F("ADTY3"));     // advertising type (3 = advertising only)
  setConf(F("IBEA1"));     // enable iBeacon
  setConf(F("DELO2"));     // iBeacon deploy mode (2 = broadcast only)
  setConf(F("PWRM1"));     // auto sleep OFF
  //setConf("PWRM0");        // auto sleep ON
  swResetBLE();
  DebugBLE_print(F("dt setup =\t")); DebugBLE_print(String(millis() - t)); DebugBLE_println(F("ms"));
  DebugBLE_println("");

  /* show BLT address */
  getConf("ADDR");
}

void BLE_HM11::setupAsIBeaconDetector()
{
  DebugBLE_println(F("setup BLE-Module as I-Beacon detector"));

  for (byte n = 0; n < NUMBER_DEVICES_TO_CONSIDER; n++) lastMatchDeviceAddresses_[n] = "";  // clear found device history
  timeOfLastMatch_ = millis();
 
  /* iBeacon-Detector setup */
  setConf(F("IMME1"));     // module work type (1 = responds only to AT-commands)
  setConf(F("ROLE1"));     // module role (1 = central = master)
  swResetBLE();
}

void BLE_HM11::detectIBeacons()
{
  DebugBLE_println(F("setup BLE-Module as I-Beacon detector"));

  for (byte n = 0; n < NUMBER_DEVICES_TO_CONSIDER; n++) lastMatchDeviceAddresses_[n] = "";  // clear found device history
  timeOfLastMatch_ = millis();
 
  /* iBeacon-Detector setup */
  setConf(F("IMME1"));     // module work type (1 = responds only to AT-commands)
  setConf(F("ROLE1"));     // module role (1 = central = master)
  swResetBLE();
}







// void wakeUpBLE()
// {
//  DebugBLE_println(F("wakeing up BLE..."));

//  /* wake up -> send a long random string */
//  String wakeUpStr = "";
//  wakeUpStr.reserve(200);
//  for(int n = 0; n < 200; n++)
//  {
//    wakeUpStr += char(random(66, 90));  // 66 = 'B'... 90 = 'Z'
//  }
//  BLESerial_.begin(BAUDRATE0);
//  BLESerial_.print(wakeUpStr);
//  (void)BLESerial_.readString();
//  BLESerial_.begin(baudrate_);
//  BLESerial_.flush();
// }

void BLE_HM11::forceRenew()
{
  baudrate_t baudratesArray[] = {BAUDRATE0, BAUDRATE1, BAUDRATE2, BAUDRATE3, BAUDRATE4};
  enableBLE();
  for (uint8_t i = 0; i < sizeof(baudratesArray)/sizeof(baudrate_t); i++)
  {
    DebugBLE_println(baudratesArray[i]);
    BLESerial_.begin(baudratesArray[i]);
    while(!BLESerial_);
    for (uint8_t n = 0; n < 5; n++) sendDirectBLECommand(F("AT"));
    for (uint8_t n = 0; (n < 5) && !setConf(F("RENEW")); n++) delay(DELAY_AFTER_SW_RESET_BLE);
  }
  disableBLE();
}

/* Private ---------------------------------------------------- */
void BLE_HM11::enableBLE()
{
  DebugBLE_println(F("enable BLE"));
  if (baudrate_ == 0) baudrate_ = DEFAULT_BAUDRATE;  //blup
  setBit(*rstPort_, rstPin_);     // stop resetting
  clearBit(*enPort_, enPin_);     // enable BLE
  BLESerial_.begin(baudrate_);
  while(!BLESerial_);
  //delay(DELAY_AFTER_ENABLE_BLE);  // discovered empirically  //blup: nötig?
  BLESerial_.flush();
}

void BLE_HM11::disableBLE()
{
  DebugBLE_println(F("disable BLE"));
  clearBit(*rstPort_, rstPin_);   // to prevent supply throug reset
  BLESerial_.end();
  setBit(*enPort_, enPin_);       // disable BLE
}

bool BLE_HM11::isBLEEnabled()
{
  return !getBit(*enPort_, enPin_);
}

void BLE_HM11::hwResetBLE()
{
  clearBit(*rstPort_, rstPin_);
  delay(5);
  setBit(*rstPort_, rstPin_);
  delay(DELAY_AFTER_HW_RESET_BLE);   // discovered empirically
}

void BLE_HM11::swResetBLE()
{
  setConf(F("RESET"));
  delay(DELAY_AFTER_SW_RESET_BLE);
}

bool BLE_HM11::setConf(String cmd)
{
  String response = sendDirectBLECommand("AT+" + cmd);
  return response.indexOf("OK") > -1 ? true : false;
}

String BLE_HM11::getConf(String cmd)
{
  return sendDirectBLECommand("AT+" + cmd + "?");
}

BLE_HM11::baudrate_t BLE_HM11::getBaudrate()
{
  DebugBLE_println(F("getBaudrate..."));

  baudrate_t baudratesArray[] = {BAUDRATE0, BAUDRATE1, BAUDRATE2, BAUDRATE3, BAUDRATE4};
  
  uint8_t i;
  String response = "";
  for (i = 0; (response.indexOf("OK") < 0) && (i < sizeof(baudratesArray)/sizeof(baudrate_t)); i++)
  {
    DebugBLE_println(baudratesArray[i]);
    BLESerial_.begin(baudratesArray[i]);
    while(!BLESerial_);
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
    while(1);
  }
    
  return  baudratesArray[i-1];
}

String BLE_HM11::sendDirectBLECommand(String cmd)
{
  bool failed = false;
  String response = "";
  response.reserve(DEFAULT_RESPONSE_LENGTH);

  /* send command */
  DebugBLE_print(F("send:\t\t")); DebugBLE_println(cmd);
  BLESerial_.print(cmd);

  /* get response */
  uint32_t startMillis_BLE = millis();
  while ((response.indexOf("OK") < 0 || BLESerial_.available()) && !failed)
  {
    if (BLESerial_.available())
    {
      response += char(BLESerial_.read());
      DebugBLE_print(F("got:\t\t")); DebugBLE_println(response);
    }

    if ((millis() - startMillis_BLE) >= COMMAND_TIMEOUT_TIME)
    {
      failed = true;
      response = "error";
      DebugBLE_println(F("reading response timeouted!"));
    }

//    if (response.length() > (MAX_NUMBER_IBEACONS * NUMBER_CHARS_PER_DEVICE))
//    {
//      failed = true;
//      response = "error";
//      DebugBLE_println(F("reading response exceeded the defined memory space!"));
//    }
  }

//  /* filter OK-message */
//  if (response != "error") {
//    response = response.substring(response.indexOf("OK"), -1);
//  }

  /* print response */
  DebugBLE_print(F("received:\t")); DebugBLE_println(response);
  DebugBLE_print(F("dt =\t\t")); DebugBLE_print(String(millis() - startMillis_BLE)); DebugBLE_println(F("ms"));
  //DebugBLE_print(F("dt =\t\t")); DebugBLE_print(String(micros() - startMicros_BLE)); DebugBLE_println(F("us"));
  DebugBLE_println("");

  BLESerial_.flush();

  response.trim();

  return response;
}

String BLE_HM11::byteToHexString(uint8_t hex)
{
  String str;
  str.reserve(2);
  str += nibbleToHexCharacter((hex & 0xF0) >> 4);
  str += nibbleToHexCharacter(hex & 0x0F);
  return str;
}

char BLE_HM11::nibbleToHexCharacter(uint8_t nibble)
{
  return (nibble > 9) ? (char)(nibble + 'A' - 10) : (char)(nibble + '0');
}

/**
 * @}
 */












// /** ===========================================================
//  * \fn      handleBLE
//  * \brief   handles the BLE module to detect iBeacons
//  ============================================================== */
// boolean handleBLE()
// {
//   boolean match = false;

//   /* find near iBeacons */
//   String response = getConf(F("DISI"));

//   /* if successful: continue reading to get the devices */
//   if (response.indexOf("OK+DISIS") >= 0)
//   {
//     unsigned long startMillis_BLE_total = millis();
//     String data = "";
//     data.reserve(MAX_NUMBER_IBEACONS*NUMBER_CHARS_PER_DEVICE+DEFAULT_RESPONSE_LENGTH);

// //    data = DebugBLE_readStringUntil('\n');  // "OK+DISC:..."
//     boolean timeout = false;
//     unsigned long lastDataFound = millis();
//     while(((data.length() < NUMBER_CHARS_PER_DEVICE)    // wait for at least one device or...
//             || (DebugBLE_available() > 0)                 // stay as long as data is available or...
//             || ((millis() - lastDataFound) < DETECTION_TIME_OFFSET))            // wait some defined time (discovered empirically) before leaving
//             && (data.length() < (MAX_NUMBER_IBEACONS*NUMBER_CHARS_PER_DEVICE))  // leave if max number of iBeacons have been found
//             && !timeout)                                // leave if timeouted
//     {
//       if (DebugBLE_available() > 0)
//       {
//         data += String(char(DebugBLE_read()));
//         lastDataFound = millis();
//       }

//       if ((millis() - startMillis_BLE_total) >= MAX_DETECTION_TIME)
//       {
//         timeout = true;
//         DebugBLE_println(F("timeouted!"));
//       }
//     }
//     //DebugBLE_print(F("dt data =\t"));DebugBLE_print((millis() - startMillis_BLE_total)); DebugBLE_println(F("ms"));
//     //DebugBLE_print(F("data =\t\t")); DebugBLE_println(data);

//     /* HW reset to prevent the "AT+DISCE" */
//     hwResetBLE();
//     //while(data.indexOf("OK+DISCE") < 0){if (DebugBLE_available()) data += String(char(DebugBLE_read()));}
    
//     /* get total device count */
//     unsigned int j = 0;
//     byte deviceCounter;
//     for(deviceCounter = 0; data.indexOf("OK+DISC:", j) >= 0; deviceCounter++)
//     {
//       j = data.indexOf("OK+DISC:", j) + DEFAULT_RESPONSE_LENGTH;
//     }
//     DebugBLE_print(deviceCounter); DebugBLE_println(F(" device(s) found"));
//     //DebugBLE_print(F("dt total =\t")); DebugBLE_print((millis() - startMillis_BLE_total)); DebugBLE_println(F("ms"));
//     DebugBLE_print(F("data =\t\t")); DebugBLE_println(data);
    
//     if (data.indexOf(KEY_UUID) > 0)
//     {
//       match = true;

//       /* when "process data" is commented */
//       currentMatchDeviceNr = 0;
//       iBeaconData[0].deviceAddress = data.substring(data.indexOf(KEY_UUID)+45, data.indexOf(KEY_UUID)+57);
//       //iBeaconData[0].deviceAddress = data.substring(data.indexOf(KEY_MAJOR_ID)+12, data.indexOf(KEY_MAJOR_ID)+24);
//       //DebugBLE_println(iBeaconData[0].deviceAddress);
//     }
    
// //    /* process data -> extract the devices from the data */
// //    byte deviceCounter;
// //    String deviceString[MAX_NUMBER_IBEACONS];
// //    for (deviceCounter = 0; deviceCounter < (data.length()/NUMBER_CHARS_PER_DEVICE) && (deviceCounter < MAX_NUMBER_IBEACONS); deviceCounter++)
// //    {
// //      unsigned int deviceIndex = deviceCounter * NUMBER_CHARS_PER_DEVICE;
// //      deviceString[deviceCounter].reserve(NUMBER_CHARS_PER_DEVICE);  // reserve memory to prevent fragmentation
// //      deviceString[deviceCounter] = data.substring(deviceIndex, deviceIndex + NUMBER_CHARS_PER_DEVICE);
// //      DebugBLE_print(F("device_")); DebugBLE_print(deviceCounter); DebugBLE_print(F(" =\t")); DebugBLE_println(deviceString[deviceCounter]);
// //    }
// //    DebugBLE_println("");
// //
// //    if (deviceString[0].length() == NUMBER_CHARS_PER_DEVICE)
// //    {
// //      /* process device data */
// //      for (byte n = 0; n < deviceCounter; n++)
// //      {
// //        iBeaconData[n].accessAddress = deviceString[n].substring(8, 16);
// //        //DebugBLE_print(F("accessAdr_")); DebugBLE_print(String(n)); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].accessAddress);
// //        iBeaconData[n].uuid = deviceString[n].substring(17, 49);
// //        //DebugBLE_print(F("UUID_")); DebugBLE_print(String(n)); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].uuid);
// //        iBeaconData[n].major = deviceString[n].substring(50, 54);
// //        DebugBLE_print(F("Major_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].major);
// //        iBeaconData[n].minor = deviceString[n].substring(54, 58);
// //        //DebugBLE_print(F("Minor_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].minor);
// //        iBeaconData[n].deviceAddress = deviceString[n].substring(61, 73);
// //        DebugBLE_print(F("deviceAdr_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].deviceAddress);
// //        iBeaconData[n].txPower = deviceString[n].substring(74, 78).toInt();
// //        DebugBLE_print(F("TxPower_")); DebugBLE_print(n); DebugBLE_print(F(" =\t")); DebugBLE_println(iBeaconData[n].txPower);
// //        DebugBLE_println("");
// //        
// //        /* handle match */
// //        if (iBeaconData[currentMatchDeviceNr].major == String(KEY_MAJOR_ID))
// //        {
// //          currentMatchDeviceNr = n;
// //          DebugBLE_print(F("Device ")); DebugBLE_print(currentMatchDeviceNr); DebugBLE_println(F(" mached!"));
// //          match = true;
// //          break;
// //        }
// //      }
// //    }
//     DebugBLE_println("");
//   }

//   return match;
// }






