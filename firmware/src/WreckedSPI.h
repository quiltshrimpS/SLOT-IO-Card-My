/* Arduino DigitalIO Library
 * Copyright (C) 2013 by William Greiman
 *
 * This file is part of the Arduino DigitalIO Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino DigitalIO Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file
 * @brief  Software SPI.
 *
 * @defgroup softSPI Software SPI
 * @details  Software SPI Template Class.
 * @{
 */

#ifndef WreckedSPI_h
#define WreckedSPI_h
#include "DigitalPin.h"
//------------------------------------------------------------------------------
/** Nop for timing. */
#define nop asm volatile ("nop\n\t")
//------------------------------------------------------------------------------
/** Pin Mode for MISO is input.*/
#define MISO_MODE INPUT
/** Pullups disabled for MISO are disabled. */
#define MISO_LEVEL false
/** Pin Mode for MOSI is output.*/
#define MOSI_MODE  OUTPUT
/** Pin Mode for SCK is output. */
#define SCK_MODE  OUTPUT
//------------------------------------------------------------------------------
/**
 * @class SoftSPI
 * @brief Fast software SPI with 2 Sck pins.
 */
template<uint8_t MisoPin, uint8_t MosiPin, uint8_t SckPinMiso, uint8_t SckPinMosi, uint8_t ModeMiso = 2, uint8_t ModeMosi = 0>
class WreckedSPI {
 public:
  //----------------------------------------------------------------------------
  /** Initialize SoftSPI pins. */
  void begin() {
    fastPinConfig(MisoPin, MISO_MODE, MISO_LEVEL);
    fastPinConfig(MosiPin, MOSI_MODE, !MODE_CPHA(ModeMosi));
    fastPinConfig(SckPinMiso, SCK_MODE, MODE_CPOL(ModeMiso));
    fastPinConfig(SckPinMosi, SCK_MODE, MODE_CPOL(ModeMosi));
  }
  //----------------------------------------------------------------------------
  /** Soft SPI receive byte.
   * @return Data byte received.
   */
  inline __attribute__((always_inline))
  uint8_t receive() {
    uint8_t data = 0;
    receiveBit(7, &data);
    receiveBit(6, &data);
    receiveBit(5, &data);
    receiveBit(4, &data);
    receiveBit(3, &data);
    receiveBit(2, &data);
    receiveBit(1, &data);
    receiveBit(0, &data);
    return data;
  }
  //----------------------------------------------------------------------------
  /** Soft SPI send byte.
   * @param[in] data Data byte to send.
   */
  inline __attribute__((always_inline))
  void send(uint8_t data) {
    sendBit(7, data);
    sendBit(6, data);
    sendBit(5, data);
    sendBit(4, data);
    sendBit(3, data);
    sendBit(2, data);
    sendBit(1, data);
    sendBit(0, data);
  }
  //----------------------------------------------------------------------------
  /** Soft SPI transfer byte.
   * @param[in] txData Data byte to send.
   * @return Data byte received.
   */
  inline __attribute__((always_inline))
  uint8_t transfer(uint8_t txData) {
    uint8_t rxData = 0;
    transferBit(7, &rxData, txData);
    transferBit(6, &rxData, txData);
    transferBit(5, &rxData, txData);
    transferBit(4, &rxData, txData);
    transferBit(3, &rxData, txData);
    transferBit(2, &rxData, txData);
    transferBit(1, &rxData, txData);
    transferBit(0, &rxData, txData);
    return rxData;
  }

 private:
  //----------------------------------------------------------------------------
  inline __attribute__((always_inline))
  bool MODE_CPHA(uint8_t mode) {return (mode & 1) != 0;}
  inline __attribute__((always_inline))
  bool MODE_CPOL(uint8_t mode) {return (mode & 2) != 0;}
  inline __attribute__((always_inline))
  void receiveBit(uint8_t bit, uint8_t* data) {
    if (MODE_CPHA(ModeMiso)) {
      fastDigitalWrite(SckPinMiso, !MODE_CPOL(ModeMiso));
    }
    fastDigitalWrite(SckPinMiso, MODE_CPHA(ModeMiso) ? MODE_CPOL(ModeMiso) : !MODE_CPOL(ModeMiso));
    if (fastDigitalRead(MisoPin)) *data |= 1 << bit;
    if (!MODE_CPHA(ModeMiso)) {
      fastDigitalWrite(SckPinMiso, MODE_CPOL(ModeMiso));
    }
  }
  //----------------------------------------------------------------------------
  inline __attribute__((always_inline))
  void sendBit(uint8_t bit, uint8_t data) {
    if (MODE_CPHA(ModeMosi)) {
      fastDigitalWrite(SckPinMosi, !MODE_CPOL(ModeMosi));
    }
    fastDigitalWrite(MosiPin, data & (1 << bit));
    fastDigitalWrite(SckPinMosi, MODE_CPHA(ModeMosi) ? MODE_CPOL(ModeMosi) : !MODE_CPOL(ModeMosi));
    if (!MODE_CPHA(ModeMosi)) {
      fastDigitalWrite(SckPinMosi, MODE_CPOL(ModeMosi));
    }
  }
  //----------------------------------------------------------------------------
  inline __attribute__((always_inline))
  void transferBit(uint8_t bit, uint8_t* rxData, uint8_t txData) {
    if (MODE_CPHA(ModeMiso)) {
      fastDigitalWrite(SckPinMiso, !MODE_CPOL(ModeMiso));
    }
    if (MODE_CPHA(ModeMosi)) {
      fastDigitalWrite(SckPinMosi, !MODE_CPOL(ModeMosi));
    }
    fastDigitalWrite(MosiPin, txData & (1 << bit));
    fastDigitalWrite(SckPinMiso, MODE_CPHA(ModeMiso) ? MODE_CPOL(ModeMiso) : !MODE_CPOL(ModeMiso));
    fastDigitalWrite(SckPinMosi, MODE_CPHA(ModeMosi) ? MODE_CPOL(ModeMosi) : !MODE_CPOL(ModeMosi));
    if (fastDigitalRead(MisoPin)) *rxData |= 1 << bit;
    if (!MODE_CPHA(ModeMiso)) {
      fastDigitalWrite(SckPinMiso, MODE_CPOL(ModeMiso));
    }
    if (!MODE_CPHA(ModeMosi)) {
      fastDigitalWrite(SckPinMosi, MODE_CPOL(ModeMosi));
    }
  }
  //----------------------------------------------------------------------------
};
#endif  // SoftSPI_h
/** @} */