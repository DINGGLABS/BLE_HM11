#ifndef _LIB_HM11_HardwareSerial_H_
#define _LIB_HM11_HardwareSerial_H_
/*******************************************************************************
* \file    HM11_HardwareSerial.h
********************************************************************************
* \author  Jascha Haldemann jh@oxon.ch
* \date    01.04.2017
* \version 1.0
*
* \brief   HardwareSerial implementation for the HM11
*
* \section DESCRIPTION
* Instantiate this class if you want to control the HM11 with a HardwareSerial
* interface
*
********************************************************************************
* BLE Library
*******************************************************************************/

/* ============================== Global imports ============================ */
#include "HM11.h"

/* ==================== Global module constant declaration ================== */

/* ========================= Global macro declaration ======================= */

/* ============================ Class declaration =========================== */
class HM11_HardwareSerial : public HM11
{
public:
  /* Public member typedefs */
  //...

  /* Public member data */
  //...

  /* Constructor(s) and  Destructor*/
  HM11_HardwareSerial(HardwareSerial& BLESerial,
    volatile uint8_t *rxdPort, uint8_t rxd,
    volatile uint8_t *txdPort, uint8_t txd,
    volatile uint8_t *enPort, uint8_t enPin,
    volatile uint8_t *rstPort, uint8_t rstPin) :
    BLESerial_(BLESerial),
    HM11(rxdPort, rxd, txdPort, txd, enPort, enPin, rstPort, rstPin) {};
  ~HM11_HardwareSerial() {};
  // Example instantation:
  // HM11_HardwareSerial BLE(Serial1, &PORTB, 4, &PORTB, 3, &PORTD, 7, &PORTB, 0);

  /* Public member functions */
  //...

private:
  /*  Private constant declerations (static) */
  //...

  /* Private member data */
  HardwareSerial& BLESerial_;

  /* Private member functions */
  void BLESerial_begin(int32_t baudrate) {BLESerial_.begin(baudrate);}
  void BLESerial_end() {BLESerial_.end();}
  bool BLESerial_ready() {return BLESerial_;}
  uint16_t BLESerial_available() {return BLESerial_.available();}
  void BLESerial_print(String str) {BLESerial_.print(str);}
  int16_t BLESerial_read() {return BLESerial_.read();}
  void BLESerial_flush() {BLESerial_.flush();}
};

#endif
