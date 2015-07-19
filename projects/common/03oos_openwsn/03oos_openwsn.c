/**
\brief This project runs the full OpenWSN stack.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/
#include "thread.h"
#include "board_ow.h"
#include "crypto_engine.h"

#include "leds.h"
#include "scheduler.h"
#include "openstack.h"
#include "opendefs.h"

#include "03oos_openwsn.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static char openwsn_stack[THREAD_STACKSIZE_DEFAULT];
kernel_pid_t openwsn_pid = -1;
uint8_t owsn_mop;

void openwsn_init(void);
void* openwsn_start(void *arg);

int openwsn_start_thread(int argc, char** argv) {
    DEBUG("%s\n",__PRETTY_FUNCTION__);
    if (argc < 2) {
        printf("usage: %s (r|n)\n", argv[0]);
        puts("\tr\tinitialise as DAGROOT.");
        puts("\tn\tinitialise as node.");
        return -1;
    }

    char command = argv[1][0];
    if (command == 'r') {
        printf("Starting OpenWSN as root ... ");
        owsn_mop = 1;
        openwsn_pid = thread_create(openwsn_stack, THREAD_STACKSIZE_DEFAULT,
                                    PRIORITY_OPENWSN, CREATE_STACKTEST,
                                    openwsn_start, (void*)&owsn_mop, "openwsn thread");
    }
    else {
        printf("Starting OpenWSN as node ... ");
        owsn_mop = 0;
        openwsn_pid = thread_create(openwsn_stack, THREAD_STACKSIZE_DEFAULT,
                                    PRIORITY_OPENWSN, CREATE_STACKTEST,
                                    openwsn_start, (void*)&owsn_mop, "openwsn thread");
    }
    return 0;
}

void* openwsn_start(void *arg) {
    DEBUG("%s\n",__PRETTY_FUNCTION__);
    (void)arg;
    leds_all_off();
    board_init_ow();
    scheduler_init();
    openstack_init(*((uint8_t*)arg));
    puts("OpenWSN thread started.");
    scheduler_start();
    return NULL;
}

int mote_main(void) {

   // initialize
   // board_init();
   // CRYPTO_ENGINE.init();
   // scheduler_init();
   // openstack_init();

   // indicate

   // start
   // scheduler_start();
   return 0; // this line should never be reached
}
