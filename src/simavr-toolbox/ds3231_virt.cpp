/*
        ds3231_virt.c

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

#include "ds3231_virt.h"

#include <simavr/avr_ioport.h>
#include <simavr/avr_twi.h>
#include <simavr/sim_avr.h>
#include <simavr/sim_time.h>
#include <stdio.h>
#include <string.h>

#include "sim_base.hpp"

/*
 * Internal registers. Time is in BCD.
 * See p10 of the DS1388 datasheet.
 */
#define DS3231_VIRT_SECONDS 0x00
#define DS3231_VIRT_MINUTES 0x01
#define DS3231_VIRT_HOURS 0x02
#define DS3231_VIRT_DAY 0x03
#define DS3231_VIRT_DATE 0x04
#define DS3231_VIRT_MONTH 0x05
#define DS3231_VIRT_YEAR 0x06
#define DS3231_VIRT_CONTROL 0xE

/*
 * 12/24 hour select bit. When high clock is in 12 hour
 * mode and the AM/PM bit is operational. When low the
 * AM/PM bit becomes part of the tens counter for the
 * 24 hour clock.
 */
#define DS3231_VIRT_12_24_HR 6

/*
 * AM/PM flag for 12 hour mode. PM is high.
 */
#define DS3231_VIRT_AM_PM 5

/*
 * Control register flags. See p11 of the DS1388 datasheet.
 */
#define DS3231_CONTROL_A1IE 0
#define DS3231_CONTROL_A2IE 1
#define DS3231_CONTROL_INTCN 2
#define DS3231_CONTROL_RS1 3
#define DS3231_CONTROL_RS2 4
#define DS3231_CONTROL_CONV 5
#define DS3231_CONTROL_BBSQW 6
#define DS3231_CONTROL_EOSC 7

#define DS3231_CLK_FREQ 32768
#define DS3231_CLK_PERIOD_US (1000000 / DS3231_CLK_FREQ)

// Generic unpack of 8bit BCD register. Don't use on seconds or hours.
#define UNPACK_BCD(x) (((x) & 0x0F) + ((x) >> 4) * 10)

enum {
  DS3231_TWI_IRQ_OUTPUT = 0,
  DS3231_TWI_IRQ_INPUT,
  DS3231_SQW_IRQ_OUT,
  DS3231_IRQ_COUNT,
};

/*
 * Square wave out prescaler modes; see p11 of DS3231 datasheet.
 */
enum SqWaveFreq {
  Hz1 = 0,
  Hz1024k = 1,
  Hz4096 = 2,
  Hz8192k = 3,
};

/*
 * Describes the behaviour of the specified BCD register.
 *
 * The tens mask is used to avoid config bits present in
 * the same register.
 */
typedef struct bcd_reg_t {
  uint8_t *reg;
  uint8_t min_val;
  uint8_t max_val;
  uint8_t tens_mask;
} bcd_reg_t;

static inline int ds3231_get_flag(uint8_t reg, uint8_t bit) {
  return (reg & (1 << bit)) != 0;
}

/*
 * Increment the ds3231 register address.
 */
static void ds3231_virt_incr_addr(ds3231_virt_t *const p) {
  if (p->reg_addr < sizeof(p->nvram)) {
    p->reg_addr++;
  } else {
    // TODO Check if this wraps, or if it just stops incrementing
    p->reg_addr = 0;
  }
}

/*
 * Update the system behaviour after a control register is written to.
 */
static void ds3231_virt_update(const ds3231_virt_t *const p) {
  // The address of the register which was just updated
  switch (p->reg_addr) {
    case DS3231_VIRT_SECONDS:
      if (ds3231_get_flag(p->nvram[DS3231_VIRT_CONTROL], DS3231_CONTROL_EOSC) == 0) {
        sim_debug_log("DS3231 clock ticking\n");
      } else {
        sim_debug_log("DS3231 clock stopped\n");
      }
      break;
    case DS3231_VIRT_CONTROL:
      sim_debug_log("DS3231 control register updated\n");
      // TODO: Check if changing the prescaler resets the clock counter
      // and if so do it here?
      break;
    default:
      // No control register updated
      return;
  }
}

/*
 * Calculate days in month given the year. The year should be specified
 * in 4 digit format.
 */
static uint8_t ds3231_virt_days_in_month(uint8_t month, uint16_t year) {
  uint8_t is_leap_year = 1;
  if ((year & 3) == 0 && ((year % 25) != 0 || (year & 15) == 0)) is_leap_year = 0;

  uint8_t days;
  if (month == 2)
    days = 28 + is_leap_year;
  else
    days = 31 - (month - 1) % 7 % 2;

  return days;
}

/*
 * Ticks a BCD register according to the specified constraints.
 */
static uint8_t ds3231_virt_tick_bcd_reg(bcd_reg_t *bcd_reg) {
  // Unpack BCD
  uint8_t x = (*bcd_reg->reg & 0x0F) + 10 * ((*bcd_reg->reg & bcd_reg->tens_mask) >> 4);

  // Tick
  uint8_t cascade = 0;
  if (++x > bcd_reg->max_val) {
    x = bcd_reg->min_val;
    cascade = 1;
  }

  // Set the BCD part of the register
  *bcd_reg->reg &= ~(0x0F | bcd_reg->tens_mask);
  *bcd_reg->reg |= (x / 10 << 4) + x % 10;

  return cascade;
}

/*
 * Ticks the time registers. See table 3, p10 of the DS3231 datasheet.
 */
static void ds3231_virt_tick_time(ds3231_virt_t *p) {
  /*
   * Seconds
   */
  bcd_reg_t reg = {
      .reg = &p->nvram[DS3231_VIRT_SECONDS], .min_val = 0, .max_val = 59, .tens_mask = 0b01110000};
  uint8_t cascade = ds3231_virt_tick_bcd_reg(&reg);
  if (!cascade) return;

  /*
   * Minutes
   */
  reg.reg = &p->nvram[DS3231_VIRT_MINUTES];
  cascade = ds3231_virt_tick_bcd_reg(&reg);
  if (!cascade) return;

  /*
   * Hours
   */
  reg.reg = &p->nvram[DS3231_VIRT_HOURS];
  if (ds3231_get_flag(p->nvram[DS3231_VIRT_HOURS], DS3231_VIRT_12_24_HR)) {
    // 12 hour mode
    reg.min_val = 1;
    reg.max_val = 12;
    reg.tens_mask = 0b00010000;
    uint8_t pm = ds3231_get_flag(p->nvram[DS3231_VIRT_HOURS], DS3231_VIRT_AM_PM);
    cascade = ds3231_virt_tick_bcd_reg(&reg);
    if (cascade) {
      if (pm) {
        // Switch to AM
        p->nvram[DS3231_VIRT_HOURS] &= ~(1 << DS3231_VIRT_AM_PM);
      } else {
        // Switch to PM and catch the cascade
        p->nvram[DS3231_VIRT_HOURS] |= (1 << DS3231_VIRT_AM_PM);
        cascade = 0;
      }
    }
  } else {
    // 24 hour mode
    reg.min_val = 0;
    reg.max_val = 23;
    reg.tens_mask = 0b00110000;
    cascade = ds3231_virt_tick_bcd_reg(&reg);
  }
  if (!cascade) return;

  /*
   * Day
   */
  reg.reg = &p->nvram[DS3231_VIRT_DAY];
  reg.min_val = 1;
  reg.max_val = 7;
  reg.tens_mask = 0;
  ds3231_virt_tick_bcd_reg(&reg);

  /*
   * Date
   */
  reg.reg = &p->nvram[DS3231_VIRT_DATE];
  // Insert a y2.1k bug like they do in the original part
  uint16_t year = 2000 + UNPACK_BCD(p->nvram[DS3231_VIRT_YEAR]);
  reg.max_val = ds3231_virt_days_in_month(UNPACK_BCD(p->nvram[DS3231_VIRT_MONTH]), year);
  reg.tens_mask = 0b00110000;
  cascade = ds3231_virt_tick_bcd_reg(&reg);
  if (!cascade) return;

  /*
   * Month
   */
  reg.reg = &p->nvram[DS3231_VIRT_MONTH];
  reg.max_val = 12;
  reg.tens_mask = 0b00010000;
  cascade = ds3231_virt_tick_bcd_reg(&reg);
  if (!cascade) return;

  /*
   * Year
   */
  reg.reg = &p->nvram[DS3231_VIRT_YEAR];
  reg.min_val = 0;
  reg.max_val = 99;
  reg.tens_mask = 0b11110000;
  cascade = ds3231_virt_tick_bcd_reg(&reg);
}

static void ds3231_virt_cycle_square_wave(ds3231_virt_t *p) {
  p->square_wave = !p->square_wave;

  if (p->square_wave) {
    avr_raise_irq(p->irq + DS3231_SQW_IRQ_OUT, 1);
  } else {
    avr_raise_irq(p->irq + DS3231_SQW_IRQ_OUT, 0);
  }
}

/*
 * This function is left in for debugging.
 */
static void ds3231_print_time(ds3231_virt_t *p) {
  uint8_t seconds = (p->nvram[DS3231_VIRT_SECONDS] & 0xF) +
                    ((p->nvram[DS3231_VIRT_SECONDS] & 0b01110000) >> 4) * 10;
  uint8_t minutes =
      (p->nvram[DS3231_VIRT_MINUTES] & 0xF) + (p->nvram[DS3231_VIRT_MINUTES] >> 4) * 10;

  uint8_t pm = 0;
  uint8_t hours;
  if (ds3231_get_flag(p->nvram[DS3231_VIRT_HOURS], DS3231_VIRT_12_24_HR)) {
    // 12hr mode
    pm = ds3231_get_flag(p->nvram[DS3231_VIRT_HOURS], DS3231_VIRT_AM_PM);
    hours = (p->nvram[DS3231_VIRT_HOURS] & 0xF) +
            ((p->nvram[DS3231_VIRT_HOURS] & 0b00010000) >> 4) * 10;
  } else {
    // 24hr mode
    hours = (p->nvram[DS3231_VIRT_HOURS] & 0xF) +
            ((p->nvram[DS3231_VIRT_HOURS] & 0b00110000) >> 4) * 10;
  }

  uint8_t day = p->nvram[DS3231_VIRT_DAY] & 0b00000111;
  uint8_t date = (p->nvram[DS3231_VIRT_DATE] & 0xF) + (p->nvram[DS3231_VIRT_DATE] >> 4) * 10;
  uint8_t month = (p->nvram[DS3231_VIRT_MONTH] & 0xF) + (p->nvram[DS3231_VIRT_MONTH] >> 4) * 10;
  uint8_t year = (p->nvram[DS3231_VIRT_YEAR] & 0xF) + (p->nvram[DS3231_VIRT_YEAR] >> 4) * 10;

  if (p->verbose)
    sim_debug_log("Time: %02i:%02i:%02i  Day: %i Date: %02i:%02i:%02i PM:%01x\n", hours, minutes,
                  seconds, day, date, month, year, pm);
}

static avr_cycle_count_t ds3231_virt_clock_tick(struct avr_t *avr, avr_cycle_count_t when,
                                                void *pr) {
  ds3231_virt_t *p = (ds3231_virt_t *)pr;
  avr_cycle_count_t next_tick = when + avr_usec_to_cycles(avr, DS3231_CLK_PERIOD_US / 2);

  if (ds3231_get_flag(p->nvram[DS3231_VIRT_CONTROL], DS3231_CONTROL_EOSC) == 0) {
    // Oscillator is enabled. Note that this counter is allowed to wrap.
    p->rtc++;
  } else {
    // Avoid a condition match below with the clock switched off
    return next_tick;
  }

  /*
   * Update the time
   */
  if (p->rtc == 0) {
    // 1 second has passed
    ds3231_virt_tick_time(p);
    if (p->verbose) ds3231_print_time(p);
  }

  /*
   * Deal with the square wave output if enabled
   */
  if (ds3231_get_flag(p->nvram[DS3231_VIRT_CONTROL], DS3231_CONTROL_INTCN) == 0) {
    uint8_t prescaler_mode =
        ds3231_get_flag(p->nvram[DS3231_VIRT_CONTROL], DS3231_CONTROL_RS1) +
        (ds3231_get_flag(p->nvram[DS3231_VIRT_CONTROL], DS3231_CONTROL_RS2) << 1);

    switch (prescaler_mode) {
      case Hz1:
        if ((p->rtc + 1) % DS3231_CLK_FREQ == 0) {
          ds3231_virt_cycle_square_wave(p);
        }
        break;
      case Hz1024k:
        if ((p->rtc + 1) % (DS3231_CLK_FREQ / 32) == 0) ds3231_virt_cycle_square_wave(p);
        break;
      case Hz4096:
        if ((p->rtc + 1) % (DS3231_CLK_FREQ / 8) == 0) ds3231_virt_cycle_square_wave(p);
        break;
      case Hz8192k:
        if ((p->rtc + 1) % (DS3231_CLK_FREQ / 4) == 0) ds3231_virt_cycle_square_wave(p);
        break;
      default:
        sim_debug_log("DS3231 ERROR: PRESCALER MODE INVALID\n");
        break;
    }
  }

  return next_tick;
}

static void ds3231_virt_clock_xtal_init(struct avr_t *avr, ds3231_virt_t *p) {
  p->rtc = 0;

  /*
   * Set a timer for half the clock period to allow reconstruction
   * of the square wave output at the maximum possible frequency.
   */
  avr_cycle_timer_register_usec(avr, DS3231_CLK_PERIOD_US / 2, ds3231_virt_clock_tick, p);

  sim_debug_log("DS3231 clock crystal period %uS or %d cycles\n", DS3231_CLK_PERIOD_US,
                (int)avr_usec_to_cycles(avr, DS3231_CLK_PERIOD_US));
}

/*
 * Called when a RESET signal is sent
 */
static void ds3231_virt_in_hook(struct avr_irq_t *irq, uint32_t value, void *param) {
  ds3231_virt_t *p = (ds3231_virt_t *)param;
  avr_twi_msg_irq_t v;
  v.u.v = value;

  /*
   * If we receive a STOP, check it was meant to us, and reset the transaction
   */
  if (v.u.twi.msg & TWI_COND_STOP) {
    if (p->selected) {
      // Wahoo, it was us!
      if (p->verbose) sim_debug_log("DS3231 stop\n\n");
    }
    /* We should not zero the register address here because read mode uses the last
     * register address stored and write mode always overwrites it.
     */
    p->selected = 0;
    p->reg_selected = 0;
  }

  /*
   * If we receive a start, reset status, check if the slave address is
   * meant to be us, and if so reply with an ACK bit
   */
  if (v.u.twi.msg & TWI_COND_START) {
    // sim_debug_log("DS3231 start attempt: 0x%02x, mask: 0x%02x,
    // twi: 0x%02x\n", p->addr_base, p->addr_mask, v.u.twi.addr);
    p->selected = 0;
    // Ignore the read write bit
    if ((v.u.twi.addr >> 1) == (DS3231_VIRT_TWI_ADDR >> 1)) {
      // it's us !
      if (p->verbose) sim_debug_log("DS3231 start\n");
      p->selected = v.u.twi.addr;
      avr_raise_irq(p->irq + TWI_IRQ_INPUT, avr_twi_irq_msg(TWI_COND_ACK, p->selected, 1));
    }
  }

  /*
   * If it's a data transaction, first check it is meant to be us (we
   * received the correct address and are selected)
   */
  if (p->selected) {
    // Write transaction
    if (v.u.twi.msg & TWI_COND_WRITE) {
      // ACK the byte
      avr_raise_irq(p->irq + TWI_IRQ_INPUT, avr_twi_irq_msg(TWI_COND_ACK, p->selected, 1));
      // Write to the selected register (see p13. DS3231 datasheet for details)
      if (p->reg_selected) {
        if (p->verbose)
          sim_debug_log("DS3231 set register 0x%02x to 0x%02x\n", p->reg_addr, v.u.twi.data);
        p->nvram[p->reg_addr] = v.u.twi.data;
        ds3231_virt_update(p);
        ds3231_virt_incr_addr(p);
        // No register selected so select one
      } else {
        if (p->verbose) sim_debug_log("DS3231 select register 0x%02x\n", v.u.twi.data);
        p->reg_selected = 1;
        p->reg_addr = v.u.twi.data;
      }
    }
    // Read transaction
    if (v.u.twi.msg & TWI_COND_READ) {
      if (p->verbose)
        sim_debug_log("DS3231 READ data at 0x%02x: 0x%02x\n", p->reg_addr, p->nvram[p->reg_addr]);
      uint8_t data = p->nvram[p->reg_addr];
      ds3231_virt_incr_addr(p);
      avr_raise_irq(p->irq + TWI_IRQ_INPUT, avr_twi_irq_msg(TWI_COND_READ, p->selected, data));
    }
  }
}

static const char *_ds3231_irq_names[DS3231_IRQ_COUNT] = {
    [DS3231_TWI_IRQ_OUTPUT] = "32<ds3231.in",
    [DS3231_TWI_IRQ_INPUT] = "8>ds3231.out",
    [DS3231_SQW_IRQ_OUT] = ">ds3231_sqw.out",
};

/*
 * Initialise the DS3231 virtual part. This should be called before anything else.
 */
void ds3231_virt_init(struct avr_t *avr, ds3231_virt_t *p) {
  memset(p, 0, sizeof(*p));
  memset(p->nvram, 0x00, sizeof(p->nvram));
  // Default for day counter. Strangely it runs from 1-7.
  p->nvram[DS3231_VIRT_DAY] = 1;

  p->avr = avr;

  p->irq = avr_alloc_irq(&avr->irq_pool, 0, DS3231_IRQ_COUNT, _ds3231_irq_names);
  avr_irq_register_notify(p->irq + TWI_IRQ_OUTPUT, ds3231_virt_in_hook, p);

  ds3231_virt_clock_xtal_init(avr, p);
}

void ds3231_virt_free(ds3231_virt_t *p) {
  avr_free_irq(p->irq, DS3231_IRQ_COUNT);
}

/*
 *  "Connect" the IRQs of the DS3231 to the TWI/i2c master of the AVR.
 */
void ds3231_virt_attach_twi(ds3231_virt_t *p, uint32_t i2c_irq_base) {
  avr_connect_irq(p->irq + TWI_IRQ_INPUT, avr_io_getirq(p->avr, i2c_irq_base, TWI_IRQ_INPUT));
  avr_connect_irq(avr_io_getirq(p->avr, i2c_irq_base, TWI_IRQ_OUTPUT), p->irq + TWI_IRQ_OUTPUT);
}

/*
 * Optionally "connect" the square wave out IRQ to the AVR.
 */
void ds3231_virt_attach_square_wave_output(ds3231_virt_t *p, ds3231_pin_t *wiring) {
  avr_connect_irq(p->irq + DS3231_SQW_IRQ_OUT,
                  avr_io_getirq(p->avr, AVR_IOCTL_IOPORT_GETIRQ(wiring->port), wiring->pin));
}
