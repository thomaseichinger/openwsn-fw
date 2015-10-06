/**
\brief openmoteSTM32 definition of the "radiotimer" bsp module.
On openmoteSTM32, we use RTC for the radiotimer module.
\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, February 2012.
\author Chang Tengfei <tengfei.chang@gmail.com>,  July 2012.
*/

#include "stdint.h"

#include "periph/timer.h"
#include "hwtimer.h"

#include "leds.h"
#include "radiotimer.h"
#include "board_info.h"

#include "riot.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define FACTOR    50


//=========================== variables =======================================

enum  radiotimer_irqstatus_enum{
    RADIOTIMER_NONE     = 0x00, //alarm interrupt default status
    RADIOTIMER_OVERFLOW = 0x01, //alarm interrupt caused by overflow
    RADIOTIMER_COMPARE  = 0x02, //alarm interrupt caused by compare
};

typedef struct {
   radiotimer_compare_cbt    overflow_cb;
   radiotimer_compare_cbt    compare_cb;
   uint8_t                   overflowORcompare;//indicate RTC alarm interrupt status
   uint16_t                  currentSlotPeriod;
} radiotimer_vars_t;

volatile radiotimer_vars_t radiotimer_vars;
uint16_t current_period;

//=========================== prototypes ======================================
// extern int timer_set_relative(tim_t, int channel, unsigned int rel_value);
//=========================== public ==========================================

//===== admin

void radiotimer_init(void) {
   // clear local variables
   memset((void*)&radiotimer_vars,0,sizeof(radiotimer_vars_t));
}

void radiotimer_setOverflowCb(radiotimer_compare_cbt cb) {
   radiotimer_vars.overflow_cb    = cb;
}

void radiotimer_setCompareCb(radiotimer_compare_cbt cb) {
   radiotimer_vars.compare_cb     = cb;
}

void radiotimer_setStartFrameCb(radiotimer_capture_cbt cb) {
  puts("Nooooooo");
   while(1);
}

void radiotimer_setEndFrameCb(radiotimer_capture_cbt cb) {
  puts("nOoooooo");
   while(1);
}

void radiotimer_start(PORT_RADIOTIMER_WIDTH period) {
    DEBUG("%s %d\n", __PRETTY_FUNCTION__, period);
    timer_init(OWSN_TIMER, 1, &radiotimer_isr);
    timer_set(OWSN_TIMER, 1, period*FACTOR);
    
   radiotimer_vars.currentSlotPeriod = period*FACTOR;
   radiotimer_vars.overflowORcompare = RADIOTIMER_OVERFLOW;
}

//===== direct access

PORT_RADIOTIMER_WIDTH radiotimer_getValue(void) {
    DEBUG("%s %d\n", __PRETTY_FUNCTION__, timer_read(OWSN_TIMER));
    return (PORT_RADIOTIMER_WIDTH) timer_read(OWSN_TIMER);
}

void radiotimer_setPeriod(PORT_RADIOTIMER_WIDTH period) {
    DEBUG("%s %d\n", __PRETTY_FUNCTION__, period);

    timer_set(OWSN_TIMER, 1, period*FACTOR);
    // rtt_set_alarm(period, rtt_cmp_cb, NULL);
}

PORT_RADIOTIMER_WIDTH radiotimer_getPeriod(void) {
    DEBUG("%s %d\n", __PRETTY_FUNCTION__, radiotimer_vars.currentSlotPeriod);
    return radiotimer_vars.currentSlotPeriod;
}

//===== compare

void radiotimer_schedule(PORT_RADIOTIMER_WIDTH offset) {
    DEBUG("%s %d\n", __PRETTY_FUNCTION__, offset);
    timer_irq_disable(OWSN_TIMER);
    timer_set(OWSN_TIMER, 1, offset*FACTOR);
    timer_irq_enable(OWSN_TIMER);
    // rtt_set_alarm(offset, rtt_cmp_cb, NULL);
    //set radiotimer irpstatus
    radiotimer_vars.overflowORcompare = RADIOTIMER_COMPARE;
}

void radiotimer_cancel(void) {
    DEBUG("%s\n", __PRETTY_FUNCTION__);
    timer_reset(OWSN_TIMER);

    //set radiotimer irpstatus
    radiotimer_vars.overflowORcompare = RADIOTIMER_OVERFLOW;
}

//===== capture

inline PORT_RADIOTIMER_WIDTH radiotimer_getCapturedTime(void) {
    return timer_read(OWSN_TIMER);
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================
void radiotimer_isr(int c) {

    uint8_t taiv_temp = radiotimer_vars.overflowORcompare;
    (void) c;
    switch (taiv_temp) {
        case RADIOTIMER_COMPARE:
            DEBUG("%s cmp\n", __PRETTY_FUNCTION__);
            if (radiotimer_vars.compare_cb!=NULL) {
                radiotimer_vars.compare_cb();
                // kick the OS
                // return KICK_SCHEDULER;
            }
            break;
        case RADIOTIMER_OVERFLOW: // timer overflows
            DEBUG("%s of\n", __PRETTY_FUNCTION__);
            if (radiotimer_vars.overflow_cb!=NULL) {
                //Wait until last write operation on RTC registers has finished
                // timer_set(OWSN_TIMER, 1, radiotimer_vars.currentSlotPeriod);
                // call the callback
                radiotimer_vars.overflow_cb();
                DEBUG("returned...\n");
                // kick the OS
                // return KICK_SCHEDULER;
            }
            break;
      case RADIOTIMER_NONE:                     // this should not happen
            DEBUG("%s none\n", __PRETTY_FUNCTION__);
      default:
            DEBUG("%s default\n", __PRETTY_FUNCTION__);
            // while(1);                               // this should not happen
    }
    // return DO_NOT_KICK_SCHEDULER;
}