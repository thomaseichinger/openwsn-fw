/**

\note: Since the bsp modules for different platforms have the same declaration,
       you can use this project with any platform.
*/

#include "stdint.h"
#include "stdio.h"
// bsp modules required
#include "board.h"
#include "leds.h"
#include "flash.h"
#include "uart.h"

//=========================== define ==========================================
#define LAST_PAGE_ADDRESS  0x0807F800
#define ID_ADDRESS         0x0807FFF0
#define FLASH_SIZE_ADDRESS 0x1FFFF7E0
#define ID_LENGTH          8
//address:0x14 15 92 00 00 00 02
#define HEADBYTE_FIR  0x1514  
#define HEADBYTE_SEC  0x0092
#define HEADBYTE_THR  0x0000
#define HEADBYTE_FOU  0x0100

//=========================== variables =======================================

typedef struct {
   uint8_t lastTxByteIndex;
   uint8_t stringToSend[20];
   uint16_t temp;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void cb_uartTxDone();
void cb_uartRxCb();

//=========================== main ============================================

int mote_main(void) {

   board_init();
   
   // clear local variable
   memset(&app_vars,0,sizeof(app_vars_t));
   
   // setup UART
   uart_setCallbacks(cb_uartTxDone,cb_uartRxCb);
   uart_enableInterrupts();
   
   RCC_HSICmd(ENABLE);
   flash_init();
   flash_erasePage(LAST_PAGE_ADDRESS);
  
   flash_write(ID_ADDRESS,  HEADBYTE_FIR);
   leds_debug_toggle();
   flash_write(ID_ADDRESS+2,HEADBYTE_SEC);
   leds_radio_toggle();
   flash_write(ID_ADDRESS+4,HEADBYTE_THR);
   leds_sync_toggle();
   flash_write(ID_ADDRESS+6,HEADBYTE_FOU);
   leds_error_toggle();
   
   app_vars.stringToSend[0] = '~';
   app_vars.temp = flash_read(ID_ADDRESS);
   app_vars.stringToSend[1] = (app_vars.temp>>8)&0xFF;
   app_vars.stringToSend[2] = (app_vars.temp>>0)&0xFF;
   
   app_vars.temp = flash_read(ID_ADDRESS+2);
   app_vars.stringToSend[3] = (app_vars.temp>>8)&0xFF;
   app_vars.stringToSend[4] = (app_vars.temp>>0)&0xFF;
   
   app_vars.temp = flash_read(ID_ADDRESS+4);
   app_vars.stringToSend[5] = (app_vars.temp>>8)&0xFF;
   app_vars.stringToSend[6] = (app_vars.temp>>0)&0xFF;
   
   app_vars.temp = flash_read(ID_ADDRESS+6);
   app_vars.stringToSend[7] = (app_vars.temp>>8)&0xFF;
   app_vars.stringToSend[8] = (app_vars.temp>>0)&0xFF;

   app_vars.stringToSend[9] = '~';
   
   leds_all_off();
   
      // send stringToSend over UART
   app_vars.lastTxByteIndex = 0;
   uart_writeByte(app_vars.stringToSend[app_vars.lastTxByteIndex]);
   
   while (1) {
     //board_sleep();
   }
}

//=========================== callbacks =======================================

void cb_uartTxDone() {
   app_vars.lastTxByteIndex++;
   if (app_vars.lastTxByteIndex<sizeof(app_vars.stringToSend)) {
      uart_writeByte(app_vars.stringToSend[app_vars.lastTxByteIndex]);
   }
}

void cb_uartRxCb() {
   uint8_t byte;
   
   // toggle LED
   leds_error_toggle();
   
   // read received byte
   byte = uart_readByte();
   
   // echo that byte over serial
   uart_writeByte(byte);
}