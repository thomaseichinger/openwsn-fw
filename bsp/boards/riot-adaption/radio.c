#include "board_ow.h"
#include "radio.h"
#include "board.h"
#include "ng_at86rf2xx.h"
#include "net/ng_pktbuf.h"
#include "net/ng_netbase.h"
#include "radiotimer.h"
#include "debugpins.h"
#include "leds.h"
#include "periph_conf.h"

#define ENABLE_DEBUG (0)
#include "debug.h"


//=========================== defines =========================================

//=========================== variables =======================================

typedef struct {
   radiotimer_capture_cbt    startFrame_cb;
   radiotimer_capture_cbt    endFrame_cb;
   radio_state_t             state;
} radio_vars_t;

radio_vars_t radio_vars;

/* RIOT's network device */
ng_at86rf2xx_t radio;
static ng_pktsnip_t *tmp_pkt;

//=========================== prototypes ======================================

// void    radio_spiWriteReg(uint8_t reg_addr, uint8_t reg_setting);
// uint8_t radio_spiReadReg(uint8_t reg_addr);
// void    radio_spiWriteTxFifo(uint8_t* bufToWrite, uint8_t lenToWrite);
// void    radio_spiReadRxFifo(uint8_t* pBufRead,
//                             uint8_t* pLenRead,
//                             uint8_t  maxBufLen,
//                             uint8_t* pLqi);
// uint8_t radio_spiReadRadioInfo(void);
void event_cb(ng_netdev_event_t type, void *arg);

//=========================== public ==========================================

//===== admin

void radio_init(void) {
   DEBUG("%s\n", __PRETTY_FUNCTION__);
   ng_at86rf2xx_init(&radio, AT86RF231_SPI, AT86RF231_SPI_CLK,
                     AT86RF231_CS, AT86RF231_INT,
                     AT86RF231_SLEEP, AT86RF231_RESET);
   ng_netconf_enable_t enable = NETCONF_ENABLE;
   radio.driver->set((ng_netdev_t *)&radio, NETCONF_OPT_PRELOADING, (void *)enable, sizeof(ng_netconf_enable_t));
   radio.driver->set((ng_netdev_t *)&radio, NETCONF_OPT_RAWMODE, (void *)enable, sizeof(ng_netconf_enable_t));
   radio.driver->add_event_callback((ng_netdev_t *)&radio, event_cb);

}

void radio_setOverflowCb(radiotimer_compare_cbt cb) {
   radiotimer_setOverflowCb(cb);
}

void radio_setCompareCb(radiotimer_compare_cbt cb) {
   radiotimer_setCompareCb(cb);
}

void radio_setStartFrameCb(radiotimer_capture_cbt cb) {
   radio_vars.startFrame_cb  = cb;
}

void radio_setEndFrameCb(radiotimer_capture_cbt cb) {
   radio_vars.endFrame_cb    = cb;
}

//===== reset

void radio_reset(void) {
  ng_netconf_state_t state = NETCONF_STATE_RESET;
   radio.driver->set((ng_netdev_t *)&radio, NETCONF_OPT_STATE, (void *)&state, sizeof(ng_netconf_state_t));
}

//===== timer

void radio_startTimer(PORT_TIMER_WIDTH period) {
   radiotimer_start(period);
}

PORT_TIMER_WIDTH radio_getTimerValue(void) {
   return radiotimer_getValue();
}

void radio_setTimerPeriod(PORT_TIMER_WIDTH period) {
   radiotimer_setPeriod(period);
}

PORT_TIMER_WIDTH radio_getTimerPeriod(void) {
   return radiotimer_getPeriod();
}

//===== RF admin

void radio_setFrequency(uint8_t frequency) {
   // change state
   radio_vars.state = RADIOSTATE_SETTING_FREQUENCY;

   // configure the radio to the right frequecy
   radio.driver->set((ng_netdev_t *)&radio, NETCONF_OPT_CHANNEL, (void *)&frequency, sizeof(uint8_t));

   // change state
   radio_vars.state = RADIOSTATE_FREQUENCY_SET;
}

void radio_rfOn(void) {
  ng_netconf_state_t state = NETCONF_STATE_IDLE;
   radio.driver->set((ng_netdev_t *)&radio, NETCONF_OPT_STATE, (void *)&state, sizeof(ng_netconf_state_t));
}

void radio_rfOff(void) {
    DEBUG("%s\n",__PRETTY_FUNCTION__);
//    // change state
   radio_vars.state = RADIOSTATE_TURNING_OFF;

//    debugpins_radio_clr();
   leds_radio_off();
   ng_netconf_state_t state = NETCONF_STATE_OFF;
   radio.driver->set((ng_netdev_t *)&radio, NETCONF_OPT_STATE, (void *)&state, sizeof(ng_netconf_state_t));
//    DEBUG("step 4\n");
//    // change state
   radio_vars.state = RADIOSTATE_RFOFF;
//    DEBUG("step 5\n");
}

//===== TX

void radio_loadPacket(uint8_t* packet, uint8_t len) {
   DEBUG("rf load\n");
   // change state
   radio_vars.state = RADIOSTATE_LOADING_PACKET;

   /* create pkt */
   ng_pktsnip_t *pkt = NULL;
   pkt = ng_pktbuf_add(NULL, NULL, len, NG_NETTYPE_UNDEF);
   if (pkt == NULL ) {
        DEBUG("[openwsn] error: unable to allocate RAW data\n");
        return;
   }
   pkt->data = packet;
   pkt->size = len;
   // load packet in TXFIFO
   radio.driver->send_data((ng_netdev_t *)&radio, pkt);

   // change state
   radio_vars.state = RADIOSTATE_PACKET_LOADED;
}

void radio_txEnable(void) {
   // change state
   radio_vars.state = RADIOSTATE_ENABLING_TX;
   DEBUG("rf tx en\n");
//    // wiggle debug pin
//    debugpins_radio_set();
   leds_radio_on();

   // change state
   radio_vars.state = RADIOSTATE_TX_ENABLED;
}

void radio_txNow(void) {
   PORT_TIMER_WIDTH val;
   // change state
   radio_vars.state = RADIOSTATE_TRANSMITTING;
   leds_radio_toggle();

   ng_netconf_state_t state = NETCONF_STATE_TX;
   radio.driver->set((ng_netdev_t *)&radio, NETCONF_OPT_STATE, (void *)state, sizeof(ng_netconf_state_t));

   leds_radio_toggle();
   // The AT86RF231 does not generate an interrupt when the radio transmits the
   // SFD, which messes up the MAC state machine. The danger is that, if we leave
   // this funtion like this, any radio watchdog timer will expire.
   // Instead, we cheat an mimick a start of frame event by calling
   // ieee154e_startOfFrame from here. This also means that software can never catch
   // a radio glitch by which #radio_txEnable would not be followed by a packet being
   // transmitted (I've never seen that).
   if (radio_vars.startFrame_cb!=NULL) {
      // call the callback
      val=radiotimer_getCapturedTime();
      radio_vars.startFrame_cb(val);
   }
   DEBUG("SENT");
}

//===== RX

void radio_rxEnable(void) {
   // change state
   radio_vars.state = RADIOSTATE_ENABLING_RX;

   ng_netconf_state_t state = NETCONF_STATE_IDLE;
   radio.driver->set((ng_netdev_t *)&radio, NETCONF_OPT_STATE, (void *)&state, sizeof(ng_netconf_state_t));

   leds_radio_on();

   // change state
   radio_vars.state = RADIOSTATE_LISTENING;
}

void radio_rxNow(void) {
   // nothing to do

}

void radio_getReceivedFrame(uint8_t* pBufRead,
                            uint8_t* pLenRead,
                            uint8_t  maxBufLen,
                             int8_t* pRssi,
                            uint8_t* pLqi,
                               bool* pCrc) {
   uint8_t temp_reg_value;

   //===== crc
   temp_reg_value  = 0x11; //at86rf231_reg_read(AT86RF231_REG__PHY_RSSI);
   *pCrc           = (temp_reg_value & 0x80)>>7;  // msb is whether packet passed CRC
   *pRssi          = (temp_reg_value & 0x0f);
   /* set payload size */
   *pLenRead = tmp_pkt->size;
   /* copy payload from pkt to buffer */
   memcpy(pBufRead, tmp_pkt->data, tmp_pkt->size);
   *pLqi = pBufRead[(*pLenRead)-1];
   ng_pktbuf_release(tmp_pkt);
   tmp_pkt = NULL;
}

//=========================== callbacks =======================================
void event_cb(ng_netdev_event_t event, void *arg) {
   PORT_TIMER_WIDTH capturedTime;

   // capture the time
   capturedTime = radiotimer_getCapturedTime();

   // start of frame event
   if (event == NETDEV_EVENT_RX_STARTED) {
       DEBUG("Start of frame.\n");
      // change state
      radio_vars.state = RADIOSTATE_RECEIVING;
      if (radio_vars.startFrame_cb!=NULL) {
         // call the callback
         radio_vars.startFrame_cb(capturedTime);
         // kick the OS
         return;
      } else {
         while(1);
      }
   }
   // end of frame event
   if (event == NETDEV_EVENT_RX_COMPLETE) {
       DEBUG("End of Frame.\n");
      // change state
      radio_vars.state = RADIOSTATE_TXRX_DONE;
      if (radio_vars.endFrame_cb!=NULL) {
        /* save pointer to received pkt */
        tmp_pkt = (ng_pktsnip_t *) arg;
         // call the callback
         radio_vars.endFrame_cb(capturedTime);
         // kick the OS
         return;
      } else {
         while(1);
      }
   }
}