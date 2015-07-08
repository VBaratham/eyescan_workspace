/*
 * main.c
 *
 *  Created on: Dec 1, 2013
 *      Author: hobbs
 */

#include "xmk.h"
#include "sys/init.h"
#include "platform.h"

void *hello_world(void *args);
void *monitor(void *argv);
void *test_i2c(void *argv);
void *network_main_thread(void *argv);
void *es_controller_thread(char * arg);

int main()
{
    init_platform();

    /* Initialize xilkernel */
    xilkernel_init();

    /* add threads to be launched once xilkernel starts */
    //xmk_add_static_thread(hello_world,0);      /* Just prints on console */
    xmk_add_static_thread(monitor,0);          /* Puts uptime and system info in LCD messages */
    /* xmk_add_static_thread(test_i2c,0); */           /* Should be obvious.  Uses console I/O */
    xmk_add_static_thread(network_main_thread,0); /* Ditto */
    xmk_add_static_thread(es_controller_thread,0);
    /* start xilkernel - does not return control */

    xilkernel_start();

    /* Never reached */
    cleanup_platform();

    return 0;
}


