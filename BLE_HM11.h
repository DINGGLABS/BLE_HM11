/**-----------------------------------------------------------------------------
 * \file    BLE_HM11.h
 * \author  jh
 * \date    xx.01.2017
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

/* Typedefs ----------------------------------------------------*/

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

/* Defines -----------------------------------------------------*/

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
  enum mode : uint8_t {I_BEACON_MODE = 0, I_BEACON_DETECTOR_MODE = 1, TRANSCEIVER_MODE = 2};

  /* public typedefs */
  typedef enum
  {
    BAUDRATE0 = 9600,
    BAUDRATE1 = 19200,
    BAUDRATE2 = 38400,
    BAUDRATE3 = 57600,
    BAUDRATE4 = 115200
  } baudrate_t;
  
  typedef enum
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
    String name;
    String uuid;
    uint16_t major;
    uint16_t minor;
    advertInterval_t interv;
    String deviceAddress;
    int16_t txPower;
  } iBeaconData_t;

  /* public methods */
  void begin(uint32_t baudrate = uint32_t(DEFAULT_BAUDRATE));
  void end();
  void setupAsIBeacon(String name, String uuid, uint16_t major, uint16_t minor, advertInterval_t interv);
  //void setupAsIBeacon(iBeaconData_t *iBeacon);  // necessaray: name, uuid, major, minor, interv
  void setupAsIBeaconDetector();
  void detectIBeacons();

  void forceRenew();

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
  static const uint16_t DELAY_AFTER_ENABLE_BLE     = 200;        // in ms (discovered empirically)
  static const uint16_t DELAY_AFTER_HW_RESET_BLE   = 100;        // in ms (discovered empirically)
  static const uint16_t DELAY_AFTER_SW_RESET_BLE   = 900;        // in ms (discovered empirically)

  // I-Beacon detector
  static const uint16_t MAX_NUMBER_IBEACONS        = 6;          // max = 6 (keep the RAM in minde!)
  static const uint16_t NUMBER_DEVICES_TO_CONSIDER = 2;          // number of found devices to consider before sending a message

  // static const uint32_t OUT_OF_RANGE_TIME          = (2*60000);  // in ms
  // static const uint16_t DETECTION_TIME_OFFSET      = 700;        // in ms (discovered empirically)
  // static const uint16_t MIN_TIME_BEFORE_EN_FORCE   = 1000;       // in ms
  // static const uint16_t MAX_DETECTION_TIME         = 5000;       // in ms
  // static const uint16_t NUMBER_CHARS_PER_DEVICE    = 78;         // including the "OK+DISC:"
  
  /* private variables */
  uint32_t baudrate_;
  uint32_t timeOfLastMatch_;
  String lastMatchDeviceAddresses_[NUMBER_DEVICES_TO_CONSIDER];
  iBeaconData_t iBeaconData_[MAX_NUMBER_IBEACONS];

  /* private methods */
  void enableBLE();
  void disableBLE();
  bool isBLEEnabled();
  void hwResetBLE();
  void swResetBLE();
  bool setConf(String cmd);
  String getConf(String cmd);
  baudrate_t getBaudrate();
  String sendDirectBLECommand(String cmd);
  String byteToHexString(uint8_t hex);
  char nibbleToHexCharacter(uint8_t nibble);
};

#endif

/**
 * @}
 */
