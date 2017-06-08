#ifndef _LIB_HM11_SoftwareSerial0_H_
#define _LIB_HM11_SoftwareSerial0_H_
/*******************************************************************************
* \file    HM11_SoftwareSerial0.h
********************************************************************************
* \author  Jascha Haldemann jh@oxon.ch
* \date    01.04.2017
* \version 1.0
*
* \brief   SoftwareSerial0 implementation for the HM11
*
* \section DESCRIPTION
* Instantiate this class if you want to control the HM11 with a SoftwareSerial0
* interface
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
********************************************************************************
* BLE Library
*******************************************************************************/

/* ============================== Global imports ============================ */
#include <SoftwareSerial0.h>
#include "HM11.h"

/* ==================== Global module constant declaration ================== */

/* ========================= Global macro declaration ======================= */

/* ============================ Class declaration =========================== */
class HM11_SoftwareSerial0 : public HM11
{
public:
  /* Public member typedefs */
  //...

  /* Public member data */
  //...

  /* Constructor(s) and  Destructor*/
  HM11_SoftwareSerial0(SoftwareSerial0& BLESerial,
    volatile uint8_t *rxdPort, uint8_t rxd,
    volatile uint8_t *txdPort, uint8_t txd,
    volatile uint8_t *enPort, uint8_t enPin,
    volatile uint8_t *rstPort, uint8_t rstPin) :
    HM11(rxdPort, rxd, txdPort, txd, enPort, enPin, rstPort, rstPin),
    BLESerial_(BLESerial) {};
  ~HM11_SoftwareSerial0() {};
  // Example instantation:
  // SoftwareSerial0 BLESerial(8, 9);
  // HM11_SoftwareSerial0 BLE(BLESerial, &PORTD, 8, &PORTD, 9, &PORTD, 7, &PORTB, 0);

  /* Public member functions */
  //...

private:
  /* Private constant declerations (static) */
  //...

  /* Private member data */
  SoftwareSerial0& BLESerial_;

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
