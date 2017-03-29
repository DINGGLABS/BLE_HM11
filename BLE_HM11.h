/**-----------------------------------------------------------------------------
 * \file    BLE_HM11.h
 * \author  jh
 * \date    xx.02.2017
 *
 * \version 1.0
 *
 * \brief   The BLE_HM11 is a V4.0 Bluetooth Low Power module that can be controlled
 *          with AT commands via UART. It unites the Master and Slave roles in one.
 *          - Transmission version: transmit data between two BLE devices
 *          - Remote Control: Control GPIOs of the HM11 remotely
 *          Factory defaults:
 *            Name: HMSoft; Baud: 9600, N, 8, 1; Pin code: 1234; transmit Version
 *
 * @{
 -----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion -----------------------*/
#ifndef BLE_HM11_H_
#define BLE_HM11_H_

/* Includes --------------------------------------------------- */
#include <Arduino.h>
#include <SoftwareSerial.h>

/* Defines -----------------------------------------------------*/
//#define DEBUG_BLE  //blup: define to activate Debug prints

/* Macros ----------------------------------------------------- */
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

#ifdef DEBUG_BLE
  #define DebugBLE_print(...)     Serial.print(__VA_ARGS__)
  #define DebugBLE_println(...)   Serial.println(__VA_ARGS__)
#else
  #define DebugBLE_print(...)
  #define DebugBLE_println(...)
#endif

/* Class ------------------------------------------------------ */
class BLE_HM11
{
public:
  /* constructor(s) & deconstructor */
  BLE_HM11(SoftwareSerial& BLESerial,
           volatile uint8_t *enPort, uint8_t enPin,
           volatile uint8_t *rstPort, uint8_t rstPin) :
           BLESerial_(BLESerial),
           enPort_(enPort), enPin_(enPin),
           rstPort_(rstPort), rstPin_(rstPin) {};
    // Example instantation: BLE_HM11 BLE(BLESerial, &PORTD, 7, &PORTB, 0);
  ~BLE_HM11() {};

  /* public enumerations */
  //...

  /* public typedefs */
  typedef enum : uint32_t
  {
    BAUDRATE0 = 9600,
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
    INTERV_1285MS = 9
  } advertInterval_t;

  typedef struct
  {
    String name;               // 12 bytes
    String uuid;               // 16 bytes
    uint16_t marjor;           // 2 bytes
    uint16_t minor;            // 2 bytes
    advertInterval_t interv;   // 4 bytes
    String accessAddress;      // 8 bytes
    String deviceAddress;      // 12 bytes
    int16_t txPower;           // 2 bytes
  } iBeaconData_t;

  /* public methods */
  bool begin(uint32_t baudrate = uint32_t(DEFAULT_BAUDRATE));
  void end();
  void enable();
  void disable();
  bool isBLEEnabled();
  void setupAsIBeacon(iBeaconData_t *iBeacon);  // necessaray: name, uuid, marjor, minor, interv
  void setupAsIBeaconDetector();
  bool detectIBeacon(iBeaconData_t *iBeacon, uint16_t maxTimeToSearch = DEFAULT_DETECTION_TIME);  // necessary: uuid, marjor and minor (you want to search for)
  void forceRenew();  // try this if you can not communicate with the BLE-module anymore

private:
  /* attributes */
  SoftwareSerial& BLESerial_;
  volatile uint8_t *enPort_;
  uint8_t enPin_;
  volatile uint8_t *rstPort_;
  uint8_t rstPin_;

  /* private constants (static) */
  static const baudrate_t DEFAULT_BAUDRATE         = BAUDRATE0;
  static const uint8_t DEFAULT_RESPONSE_LENGTH     = 8;          // in characters
  static const uint16_t COMMAND_TIMEOUT_TIME       = 100;        // in ms (discovered empirically)
  static const uint16_t DELAY_AFTER_HW_RESET_BLE   = 300;        // in ms (discovered empirically)
  static const uint16_t DELAY_AFTER_SW_RESET_BLE   = 900;        // in ms (discovered empirically)

  // I-Beacon detector
  static const uint16_t DEFAULT_DETECTION_TIME     = 5000;       // in ms
  static const uint16_t MIN_RAM                    = 253;        // in bytes -> keep the RAM > 200 to prevent bugs!
  //static const uint16_t MAX_NUMBER_IBEACONS        = 6;          // max = 6 (keep the RAM in minde!)
  //static const uint16_t NUMBER_CHARS_PER_DEVICE    = 78;         // including the "OK+DISC:"
  
  /* private variables */
  uint32_t baudrate_;
  //iBeaconData_t iBeaconData_[MAX_NUMBER_IBEACONS];

  /* private methods */
  void hwResetBLE();
  void swResetBLE();
  void renewBLE();
  bool setConf(String cmd);
  bool setBaudrate(baudrate_t baudrate);
  bool setBaudrate();
  String getConf(String cmd);
  baudrate_t getBaudrate();
  String sendDirectBLECommand(String cmd);
  int16_t getFreeRAM();
  String byteToHexString(uint8_t hex);
  uint8_t hexStringToByte(String str);
  char nibbleToHexCharacter(uint8_t nibble);
  uint8_t hexCharacterToNibble(char hex);
};

#endif

/**
 * @}
 */
