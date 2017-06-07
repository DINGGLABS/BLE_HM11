#ifndef _LIB_HM11_H_
#define _LIB_HM11_H_
/*******************************************************************************
* \file    HM11.h
********************************************************************************
* \author  Jascha Haldemann jh@oxon.ch
* \date    01.02.2017
* \version 1.0
*
* \brief   The HM11 is a V4.0 Bluetooth Low Power module
*
* \section DESCRIPTION
* The HM11 can be controlled with AT commands via UART.
* Instantiate either HM11_HardwareSerial or HM11_SoftwareSerial which
* inherites from the HM11 class.
*
* The HM11 unites the Master and Slave roles in one.
* - Transmission version: transmit data between two BLE devices
* - Remote Control: Control GPIOs of the HM11 remotely
* Factory defaults:
*  Name: HMSoft; Baud: 9600, N, 8, 1; Pin code: 1234; transmit Version
*
********************************************************************************
* BLE Library
*******************************************************************************/

/* ============================== Global imports ============================ */
#include <Arduino.h>

/* ==================== Global module constant declaration ================== */

/* ========================= Global macro declaration ======================= */
/* port manipulation makros */
#ifndef clearBit
  #define clearBit(reg, bit) (_SFR_BYTE(reg) &= ~_BV(bit))
#endif
#ifndef setBit
  #define setBit(reg, bit) (_SFR_BYTE(reg) |= _BV(bit))
#endif
#ifndef toggleBit
  #define toggleBit(reg, bit) (_SFR_BYTE(reg) ^= _BV(bit))
#endif
#ifndef getBit
  #define getBit(reg, bit) ((_SFR_BYTE(reg) & _BV(bit)) != 0)
#endif

/* ============================ Class declaration =========================== */
class HM11
{
public:
  /* Public member typedefs */
  typedef enum : uint32_t
  {
    BAUDRATE0 = 9600,   // default
    BAUDRATE1 = 19200,
    BAUDRATE2 = 38400,
    BAUDRATE3 = 57600,
    BAUDRATE4 = 115200
  } baudrate_t;

  typedef enum : uint8_t
  {
    INTERV_100MS  = 0,
    INTERV_150MS  = 1,
    INTERV_210MS  = 2,
    INTERV_320MS  = 3,
    INTERV_420MS  = 4,
    INTERV_550MS  = 5,
    INTERV_760MS  = 6,
    INTERV_850MS  = 7,
    INTERV_1020MS = 8,
    INTERV_1285MS = 9   // default
  } advertInterval_t;

  typedef enum : uint8_t
  {
    POWER_N23DBM  = 0,
    POWER_N6DBM   = 1,
    POWER_0DBM    = 2,  // default
    POWER_6DBM    = 3
  } txPower_t;

  typedef struct
  {
    String name;               // 12 bytes
    String uuid;               // 16 bytes -> 32 hex digits
    uint16_t marjor;           // 2 bytes
    uint16_t minor;            // 2 bytes
    advertInterval_t interv;   // 4 bytes
    String accessAddress;      // 8 bytes
    String deviceAddress;      // 12 bytes
    int16_t txPower;           // 2 bytes
  } iBeaconData_t;

  /* Public member data */
  //...

  /* Constructor(s) and  Destructor */
  HM11(volatile uint8_t *rxdPort, uint8_t rxd,
    volatile uint8_t *txdPort, uint8_t txd,
    volatile uint8_t *enPort, uint8_t enPin,
    volatile uint8_t *rstPort, uint8_t rstPin) :
    rxdPort_(rxdPort), rxd_(rxd),
    txdPort_(txdPort), txd_(txd),
    enPort_(enPort), enPin_(enPin),
    rstPort_(rstPort), rstPin_(rstPin) {};
  ~HM11() {};

  /* Public member functions */
  bool begin(uint32_t baudrate = uint32_t(DEFAULT_BAUDRATE));
  void end();
  void enable();
  void disable();
  bool isEnabled();
  void setTxPower(txPower_t txPower);
  txPower_t getTxPower();
  void setupAsIBeacon(iBeaconData_t *iBeacon);  // necessaray: name, uuid, marjor, minor, interv
  void setupAsIBeaconDetector();
  bool detectIBeacon(iBeaconData_t *iBeacon, uint16_t maxTimeToSearch = DEFAULT_DETECTION_TIME);      // necessary: uuid, marjor and minor (you want to search for)
  bool detectIBeaconUUID(iBeaconData_t *iBeacon, uint16_t maxTimeToSearch = DEFAULT_DETECTION_TIME);  // necessary: uuid (you want to search for)
  /* Example response:
    4C000215 – [P0] Company ID
    0005000100001000800000805F9B0131 – [P1] UUID
    00014667C3 – [P2] String Containing:
        0001 - Major ID
        4667 – Minor ID containing
        46 - Humidity (hex)
        67 - Temperature (hex)
        C3 – RSSI (from device)
    00A0500B1710 – [P3] MAC Address
    -078 – [P4] RSSI (dBm)
  */
  String getMacAddress();
  void connectToMacAddress(String macAddr, bool master);
  char readChar();
  bool handshaking(bool master, char handshakeChar = 'H');

  void forceRenew();  // try this if you can not communicate with the BLE-module anymore

  /* Public class functions (static) */
  static String byteToHexString(uint8_t hex);
  static uint8_t hexStringToByte(String str);

private:
  /*  Private constant declerations (static) */
  static const baudrate_t DEFAULT_BAUDRATE           = BAUDRATE0;
  static const uint8_t DEFAULT_RESPONSE_LENGTH       = 8;         // in characters
  static const uint16_t COMMAND_TIMEOUT_TIME         = 100;       // in ms (discovered empirically)
  static const uint16_t MAX_DELAY_AFTER_HW_RESET_BLE = 500;       // in ms (discovered empirically)
  static const uint16_t MAX_DELAY_AFTER_SW_RESET_BLE = 1000;      // in ms (discovered empirically)

  // I-Beacon detector
  static const uint16_t DEFAULT_DETECTION_TIME     = 5000;        // in ms
  static const uint16_t MIN_RAM                    = 253;         // in bytes -> keep the RAM > 200 to prevent bugs!
  //static const uint16_t MAX_NUMBER_IBEACONS        = 6;           // max = 6 (keep the RAM in minde!)
  //static const uint16_t NUMBER_CHARS_PER_DEVICE    = 78;          // including the "OK+DISC:"

  /* Private member data */
  volatile uint8_t *rxdPort_;
  uint8_t rxd_;
  volatile uint8_t *txdPort_;
  uint8_t txd_;
  volatile uint8_t *enPort_;
  uint8_t enPin_;
  volatile uint8_t *rstPort_;
  uint8_t rstPin_;

  uint32_t baudrate_;
  //iBeaconData_t iBeaconData_[MAX_NUMBER_IBEACONS];

  /* Private member functions */
  void hwResetBLE();
  void swResetBLE();
  bool renewBLE();
  bool setConf(String cmd);
  bool setBaudrate(baudrate_t baudrate);
  bool setBaudrate();
  String getConf(String cmd);
  uint32_t getBaudrate();
  String sendDirectBLECommand(String cmd);

  /* Private class functions (static) */
  static int16_t getFreeRAM();
  static char nibbleToHexCharacter(uint8_t nibble);
  static uint8_t hexCharacterToNibble(char hex);

  /* Private virtual functions */
  virtual void BLESerial_begin(int32_t baudrate);
  virtual void BLESerial_end();
  virtual bool BLESerial_ready();  // while(!BLESerial_ready());
  virtual uint16_t BLESerial_available();
  virtual void BLESerial_print(String str);
  virtual int16_t BLESerial_read();
  virtual void BLESerial_flush();
};

#endif
