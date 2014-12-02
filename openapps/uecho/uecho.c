#include "opendefs.h"
#include "uecho.h"
#include "openudp.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"

#include "riot.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

//=========================== variables =======================================
uint8_t expect_echo;
//=========================== prototypes ======================================

//=========================== public ==========================================

void uecho_init(void) {
}

void uecho_receive(OpenQueueEntry_t* request) {
   uint16_t          temp_l4_destination_port;
   OpenQueueEntry_t* reply;

   reply = openqueue_getFreePacketBuffer(COMPONENT_UECHO);
   if (reply==NULL) {
      openserial_printError(
         COMPONENT_UECHO,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      openqueue_freePacketBuffer(request); //clear the request packet as well
      return;
   }
   else {
      openqueue_freePacketBuffer(request);
      expect_echo = FALSE;
   }
}

void uecho_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

bool uecho_debugPrint(void) {
   return FALSE;
}

//=========================== private =========================================
