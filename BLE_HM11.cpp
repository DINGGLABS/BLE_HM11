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
  Serial.print(F("currentBaudrate = ")); Serial.println(String(currentBaudrate));

  if (currentBaudrate != baudrate_)
  {
    /* set baudrate */
    Serial.println(F("set new baudrate..."));
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
        Serial.println(F("invalid baudrate!"));  
        while(1);
      }
    }
    
    swResetBLE();
    
    BLESerial_.begin(baudrate_);
    while(!BLESerial_);
    
    /* check if setting the baudrate failed */
    if (getConf(F("BAUD")).indexOf("OK") < 0) //handleError("set baudrate failed!");
    {
      Serial.println(F("set baudrate failed!"));
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
  Serial.println(F("setup BLE-Module as I-Beacon"));

  /* control if given parameters are valid */
  if ((iBeacon->name).length() > 12) {Serial.println(F("name ist too long!")); return;}
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
    Serial.println(F("The UUID has to consist of 16 numbers (1... 9)!"));
    return;
  }
  if (iBeacon->major == 0) {Serial.println(F("major can not be 0!")); return;}
  if (iBeacon->minor == 0) {Serial.println(F("minor can not be 0!")); return;}
  if ((iBeacon->interv == 0) || (iBeacon->interv > INTERV_1285MS)) {Serial.println(F("unallowed interval!")); return;}

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
  Serial.print(F("dt setup =\t")); Serial.print(String(millis() - t)); Serial.println(F("ms"));
  Serial.println("");

  /* show BLT address */
  getConf("ADDR");
}

void BLE_HM11::setupAsIBeaconDetector()
{
  Serial.println(F("setup BLE-Module as I-Beacon detector"));

  for (byte n = 0; n < NUMBER_DEVICES_TO_CONSIDER; n++) lastMatchDeviceAddresses_[n] = "";  // clear found device history
  timeOfLastMatch_ = millis();
 
  /* iBeacon-Detector setup */
  setConf(F("IMME1"));     // module work type (1 = responds only to AT-commands)
  setConf(F("ROLE1"));     // module role (1 = central = master)
  swResetBLE();
}

void BLE_HM11::detectIBeacons()
{
  Serial.println(F("setup BLE-Module as I-Beacon detector"));

  for (byte n = 0; n < NUMBER_DEVICES_TO_CONSIDER; n++) lastMatchDeviceAddresses_[n] = "";  // clear found device history
  timeOfLastMatch_ = millis();
 
  /* iBeacon-Detector setup */
  setConf(F("IMME1"));     // module work type (1 = responds only to AT-commands)
  setConf(F("ROLE1"));     // module role (1 = central = master)
  swResetBLE();
}







// void wakeUpBLE()
// {
//  Serial.println(F("wakeing up BLE..."));

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
    Serial.println(baudratesArray[i]);
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
  Serial.println(F("enable BLE"));
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
  Serial.println(F("disable BLE"));
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
  Serial.println(F("getBaudrate..."));

  baudrate_t baudratesArray[] = {BAUDRATE0, BAUDRATE1, BAUDRATE2, BAUDRATE3, BAUDRATE4};
  
  uint8_t i;
  String response = "";
  for (i = 0; (response.indexOf("OK") < 0) && (i < sizeof(baudratesArray)/sizeof(baudrate_t)); i++)
  {
    Serial.println(baudratesArray[i]);
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
    Serial.println(F("determining the current baudrate of the BLE failed!"));
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
  Serial.print(F("send:\t\t")); Serial.println(cmd);
  BLESerial_.print(cmd);

  /* get response */
  uint32_t startMillis_BLE = millis();
  while ((response.indexOf("OK") < 0 || BLESerial_.available()) && !failed)
  {
    if (BLESerial_.available())
    {
      response += char(BLESerial_.read());

      /* special delay: */
      Serial.print(F("got:\t\t")); Serial.println(response);
    }

    if ((millis() - startMillis_BLE) >= COMMAND_TIMEOUT_TIME)
    {
      failed = true;
      response = "error";
      Serial.println(F("reading response timeouted!"));
    }

//    if (response.length() > (MAX_NUMBER_IBEACONS * NUMBER_CHARS_PER_DEVICE))
//    {
//      failed = true;
//      response = "error";
//      Serial.println(F("reading response exceeded the defined memory space!"));
//    }
  }

//  /* filter OK-message */
//  if (response != "error") {
//    response = response.substring(response.indexOf("OK"), -1);
//  }

  /* print response */
  Serial.print(F("received:\t")); Serial.println(response);
  Serial.print(F("dt =\t\t")); Serial.print(String(millis() - startMillis_BLE)); Serial.println(F("ms"));
  //Serial.print(F("dt =\t\t")); Serial.print(String(micros() - startMicros_BLE)); Serial.println(F("us"));
  Serial.println("");

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

// //    data = Serial.readStringUntil('\n');  // "OK+DISC:..."
//     boolean timeout = false;
//     unsigned long lastDataFound = millis();
//     while(((data.length() < NUMBER_CHARS_PER_DEVICE)    // wait for at least one device or...
//             || (Serial.available() > 0)                 // stay as long as data is available or...
//             || ((millis() - lastDataFound) < DETECTION_TIME_OFFSET))            // wait some defined time (discovered empirically) before leaving
//             && (data.length() < (MAX_NUMBER_IBEACONS*NUMBER_CHARS_PER_DEVICE))  // leave if max number of iBeacons have been found
//             && !timeout)                                // leave if timeouted
//     {
//       if (Serial.available() > 0)
//       {
//         data += String(char(Serial.read()));
//         lastDataFound = millis();
//       }

//       if ((millis() - startMillis_BLE_total) >= MAX_DETECTION_TIME)
//       {
//         timeout = true;
//         Serial.println(F("timeouted!"));
//       }
//     }
//     //Serial.print(F("dt data =\t"));Serial.print((millis() - startMillis_BLE_total)); Serial.println(F("ms"));
//     //Serial.print(F("data =\t\t")); Serial.println(data);

//     /* HW reset to prevent the "AT+DISCE" */
//     hwResetBLE();
//     //while(data.indexOf("OK+DISCE") < 0){if (Serial.available()) data += String(char(Serial.read()));}
    
//     /* get total device count */
//     unsigned int j = 0;
//     byte deviceCounter;
//     for(deviceCounter = 0; data.indexOf("OK+DISC:", j) >= 0; deviceCounter++)
//     {
//       j = data.indexOf("OK+DISC:", j) + DEFAULT_RESPONSE_LENGTH;
//     }
//     Serial.print(deviceCounter); Serial.println(F(" device(s) found"));
//     //Serial.print(F("dt total =\t")); Serial.print((millis() - startMillis_BLE_total)); Serial.println(F("ms"));
//     Serial.print(F("data =\t\t")); Serial.println(data);
    
//     if (data.indexOf(KEY_UUID) > 0)
//     {
//       match = true;

//       /* when "process data" is commented */
//       currentMatchDeviceNr = 0;
//       iBeaconData[0].deviceAddress = data.substring(data.indexOf(KEY_UUID)+45, data.indexOf(KEY_UUID)+57);
//       //iBeaconData[0].deviceAddress = data.substring(data.indexOf(KEY_MAJOR_ID)+12, data.indexOf(KEY_MAJOR_ID)+24);
//       //Serial.println(iBeaconData[0].deviceAddress);
//     }
    
// //    /* process data -> extract the devices from the data */
// //    byte deviceCounter;
// //    String deviceString[MAX_NUMBER_IBEACONS];
// //    for (deviceCounter = 0; deviceCounter < (data.length()/NUMBER_CHARS_PER_DEVICE) && (deviceCounter < MAX_NUMBER_IBEACONS); deviceCounter++)
// //    {
// //      unsigned int deviceIndex = deviceCounter * NUMBER_CHARS_PER_DEVICE;
// //      deviceString[deviceCounter].reserve(NUMBER_CHARS_PER_DEVICE);  // reserve memory to prevent fragmentation
// //      deviceString[deviceCounter] = data.substring(deviceIndex, deviceIndex + NUMBER_CHARS_PER_DEVICE);
// //      Serial.print(F("device_")); Serial.print(deviceCounter); Serial.print(F(" =\t")); Serial.println(deviceString[deviceCounter]);
// //    }
// //    Serial.println("");
// //
// //    if (deviceString[0].length() == NUMBER_CHARS_PER_DEVICE)
// //    {
// //      /* process device data */
// //      for (byte n = 0; n < deviceCounter; n++)
// //      {
// //        iBeaconData[n].accessAddress = deviceString[n].substring(8, 16);
// //        //Serial.print(F("accessAdr_")); Serial.print(String(n)); Serial.print(F(" =\t")); Serial.println(iBeaconData[n].accessAddress);
// //        iBeaconData[n].uuid = deviceString[n].substring(17, 49);
// //        //Serial.print(F("UUID_")); Serial.print(String(n)); Serial.print(F(" =\t")); Serial.println(iBeaconData[n].uuid);
// //        iBeaconData[n].major = deviceString[n].substring(50, 54);
// //        Serial.print(F("Major_")); Serial.print(n); Serial.print(F(" =\t")); Serial.println(iBeaconData[n].major);
// //        iBeaconData[n].minor = deviceString[n].substring(54, 58);
// //        //Serial.print(F("Minor_")); Serial.print(n); Serial.print(F(" =\t")); Serial.println(iBeaconData[n].minor);
// //        iBeaconData[n].deviceAddress = deviceString[n].substring(61, 73);
// //        Serial.print(F("deviceAdr_")); Serial.print(n); Serial.print(F(" =\t")); Serial.println(iBeaconData[n].deviceAddress);
// //        iBeaconData[n].txPower = deviceString[n].substring(74, 78).toInt();
// //        Serial.print(F("TxPower_")); Serial.print(n); Serial.print(F(" =\t")); Serial.println(iBeaconData[n].txPower);
// //        Serial.println("");
// //        
// //        /* handle match */
// //        if (iBeaconData[currentMatchDeviceNr].major == String(KEY_MAJOR_ID))
// //        {
// //          currentMatchDeviceNr = n;
// //          Serial.print(F("Device ")); Serial.print(currentMatchDeviceNr); Serial.println(F(" mached!"));
// //          match = true;
// //          break;
// //        }
// //      }
// //    }
//     Serial.println("");
//   }

//   return match;
// }






