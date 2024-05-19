/*
        ds3231_virt.h

        Copyright 2014 Doug Szumski <d.s.szumski@gmail.com>

        Based on i2c_eeprom example by:

        Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

        This file is part of simavr.

        simavr is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        simavr is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  A virtual DS3231 real time clock which runs on the TWI bus.
 *
 *  Features:
 *
 *  > External oscillator is synced to the AVR core
 *  > Square wave output with scalable frequency
 *  > Leap year correction until 2100
 *
 *  Should also work for the pin compatible DS3232 device.
 */

#ifndef DS3231_VIRT_H_
#define DS3231_VIRT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <simavr/sim_avr.h>
#include <simavr/sim_irq.h>

// TWI address is fixed -- Shifted 1 left
#define DS3231_VIRT_TWI_ADDR 0xD0

// TODO: This should be generic, is also used in ssd1306, and maybe elsewhere..
typedef struct ds3231_pin_t {
  char port;
  uint8_t pin;
} ds3231_pin_t;

/*
 * DS3231 I2C clock
 */
typedef struct ds3231_virt_t {
  struct avr_t* avr;
  avr_irq_t* irq;  // irq list
  uint8_t verbose;
  uint8_t selected;      // selected address
  uint8_t reg_selected;  // register selected for write
  uint8_t reg_addr;      // register pointer
  uint8_t nvram[64];     // battery backed up NVRAM
  uint16_t rtc;          // RTC counter
  uint8_t square_wave;
} ds3231_virt_t;

void ds3231_virt_init(struct avr_t* avr, ds3231_virt_t* p);

void ds3231_virt_free(ds3231_virt_t* p);

/*
 * Attach the ds3231 to the AVR's TWI master code,
 * pass AVR_IOCTL_TWI_GETIRQ(0) for example as i2c_irq_base
 */
void ds3231_virt_attach_twi(ds3231_virt_t* p, uint32_t i2c_irq_base);

void ds3231_virt_attach_square_wave_output(ds3231_virt_t* p, ds3231_pin_t* wiring);

#ifdef __cplusplus
}
#endif

#endif /* DS3231_VIRT_H_ */
