/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2015, Zolertia - http://www.zolertia.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup zoul
 * @{
 *
 * \defgroup zoul-examples Zoul examples
 * @{
 *
 * \defgroup zoul-demo Zoul demo application
 *
 *   Example project demonstrating the Zoul module on the RE-Mote and Firefly
 *   platforms.
 *
 * - Boot sequence: LEDs flashing (Red, then yellow, finally green)
 *
 * - etimer/clock : Every LOOP_INTERVAL clock ticks (LOOP_PERIOD secs) the LED
 *                  defined as LEDS_PERIODIC will turn on
 * - rtimer       : Exactly LEDS_OFF_HYSTERISIS rtimer ticks later,
 *                  LEDS_PERIODIC will turn back off
 * - ADC sensors  : On-chip VDD / 3, temperature, and Phidget sensor
 *                  values are printed over UART periodically.
 * - UART         : Every LOOP_INTERVAL the Remote will print something over
 *                  the UART. Receiving an entire line of text over UART (ending
 *                  in \\r) will cause LEDS_SERIAL_IN to toggle
 * - Radio comms  : BTN_USER sends a rime broadcast. Reception of a rime
 *                  packet will toggle LEDs defined as LEDS_RF_RX
 * - Button       : Keeping the button pressed will print a counter that
 *                  increments every BUTTON_PRESS_EVENT_INTERVAL ticks
 *
 * @{
 *
 * \file
 *     Example demonstrating the Zoul module on the RE-Mote & Firefly platforms
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "cpu.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"
#include "dev/leds.h"
#include "dev/uart.h"
#include "dev/button-sensor.h"
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"
#include "dev/watchdog.h"
#include "dev/serial-line.h"
#include "dev/sys-ctrl.h"
#include "net/netstack.h"
#include "net/rime/broadcast.h"
#include "net/rime/rime.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#define LOOP_PERIOD         1
#define LOOP_INTERVAL       (CLOCK_SECOND * LOOP_PERIOD)
#define LEDS_SAFE     		LEDS_BLUE
#define LEDS_ALERT       	LEDS_RED
#define LEDS_INC      		LEDS_GREEN
#define LEDS_REBOOT         LEDS_ALL
#define LEDS_RF_RX          (LEDS_YELLOW | LEDS_RED)
#define BUTTON_PRESS_EVENT_INTERVAL (CLOCK_SECOND)
int emergency_state = 0 ;
/*---------------------------------------------------------------------------*/
/* Timer */
static struct etimer et;
/*---------------------------------------------------------------------------*/
PROCESS(zoulmate_demo_process, "Zoulmate demo process");
AUTOSTART_PROCESSES(&zoulmate_demo_process);
/*---------------------------------------------------------------------------*/
/* function broadcast received for further use to receive alerts from other motes*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  leds_toggle(LEDS_RF_RX);
  /* on affiche le message reçu */
  printf("*** Received %u bytes from %u:%u: '0x%04x'\n", packetbuf_datalen(),
         from->u8[0], from->u8[1], *(uint16_t *)packetbuf_dataptr());
}
/* unicast functions to communicate with the watchman*/
static void
sent_uc(struct unicast_conn *c, int status, int num_tx)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);
}
/* When the watchman answers, we change the emergency state to 2, meaning the watchman is INComming*/
/* Led is turned to GREEN */
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);
  printf("unicast message received '%s'\n", (char *)packetbuf_dataptr());
  printf("Watchman incomming\n");
  emergency_state = 2;
  leds_off(LEDS_REBOOT);
  leds_on(LEDS_INC);
}
/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks bc_rx = { broadcast_recv };
static struct broadcast_conn bc;
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(zoulmate_demo_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&bc))

  PROCESS_BEGIN();
  linkaddr_t addr;

  /* Disable the radio duty cycle and keep the radio on */
  NETSTACK_MAC.off(1);

  broadcast_open(&bc, BROADCAST_CHANNEL, &bc_rx);
  unicast_open(&uc, 146, &unicast_callbacks);


  /* Configure the ADC ports */
  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);

  printf("Zoulmate test application\n");

  etimer_set(&et, LOOP_INTERVAL);


  while(1) {

    PROCESS_YIELD();
/* Every second */
    if(ev == PROCESS_EVENT_TIMER) {
      printf("Value: %u\n", adc_zoul.value(ZOUL_SENSORS_ADC1));
    /* If the lid is well in place */
      if (adc_zoul.value(ZOUL_SENSORS_ADC1) > 20000) {
    	  /* We turn the blue led on */
    	  leds_off(LEDS_REBOOT);
          leds_on(LEDS_SAFE);
    	  printf("RAS - Socle fermé\n");
    	  emergency_state = 0;
      }

    /* If the lid is lifted */
      else {
        
			  /* If that's the first occurence */
			  if (emergency_state == 0) {
					 /* we trigger the emergency state */
					  emergency_state = 1;
					 /* We turn on the red light and send an unicast message to the watchman with the name of the object that is under attack */
					  leds_off(LEDS_REBOOT);
					  leds_on(LEDS_ALERT);
					  printf("Sending unicast to the watchman\n");
					  packetbuf_copyfrom("Etoile du nord!", 16);
					      addr.u8[0] = 0x62;
					      addr.u8[1] = 0x3B;
					      if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
					        unicast_send(&uc, &addr);
					      }

			  }

    	  }
      }

      etimer_set(&et, LOOP_INTERVAL);

    }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */

