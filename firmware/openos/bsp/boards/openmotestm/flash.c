/**
\brief openmoteSTM32 definition of the "leds" bsp module.

\author Chang Tengfei <tengfei.chang@gmail.com>,  July 2012.
*/

#include "stm32f10x_lib.h"
#include "flash.h"
#include "leds.h"
#include "stdint.h"
#include "string.h"

//=========================== defines =========================================

//=========================== prototypes ======================================

//=========================== public ==========================================

void flash_init()
{   
    FLASH_Unlock(); 
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP|FLASH_FLAG_PGERR |FLASH_FLAG_WRPRTERR);
}

uint8_t flash_erasePage(uint32_t address)
{
  uint8_t state;
  state = FLASH_ErasePage(address);
  return state;
}

uint8_t flash_write(uint32_t address,uint16_t data)
{
  uint8_t state;
  state = FLASH_ProgramHalfWord(address,data);
  return state;
}

uint16_t flash_read(uint32_t address)
{
  uint16_t temp = 0x11;
  temp = *(uint32_t*)address;
  return temp;
}

void flash_getID(uint32_t address)
{
  
}
//=========================== private =========================================