#ifndef _LIB_HM11_SoftwareSerial_H_
#define _LIB_HM11_SoftwareSerial_H_
/*******************************************************************************
* \file    HM11_SoftwareSerial.h
********************************************************************************
* \author  Jascha Haldemann jh@oxon.ch
* \date    01.04.2017
* \version 1.0
*
* \brief   SoftwareSerial implementation for the HM11
*
* \section DESCRIPTION
* Instantiate this class if you want to control the HM11 with a SoftwareSerial
* interface
*
********************************************************************************
* BLE Library
*******************************************************************************/

/* ============================== Global imports ============================ */
#include <SoftwareSerial.h>
#include "HM11.h"

/* ==================== Global module constant declaration ================== */

/* ========================= Global macro declaration ======================= */

/* ============================ Class declaration =========================== */
class HM11_SoftwareSerial : public HM11
{
public:
  /* Public member typedefs */
  //...

  /* Public member data */
  //...

  /* Constructor(s) and  Destructor*/
  HM11_SoftwareSerial(SoftwareSerial& BLESerial,
    volatile uint8_t *rxdPort, uint8_t rxd,
    volatile uint8_t *txdPort, uint8_t txd,
    volatile uint8_t *enPort, uint8_t enPin,
    volatile uint8_t *rstPort, uint8_t rstPin) :
    BLESerial_(BLESerial),
    HM11(rxdPort, rxd, txdPort, txd, enPort, enPin, rstPort, rstPin) {};
  ~HM11_SoftwareSerial() {};
  // Example instantation:
  // SoftwareSerial BLESerial(8, 9);
  // HM11 BLE(BLESerial, &PORTD, 8, &PORTD, 9, &PORTD, 7, &PORTB, 0);

  /* Public member functions */
  //...

private:
  /*  Private constant declerations (static) */
  //...

  /* Private member data */
  SoftwareSerial& BLESerial_;
  
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
