/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"

#include "../example.h"
#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif
#include <stdio.h>
#include <string.h>

/* Only for TMOTE Sky? */
#include "dev/serial-line.h"
#include "dev/uart1.h"
#include "net/ipv6/uip-ds6-route.h"

#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"

#include "dev/leds.h"
#include "dev/button-sensor.h"

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5679

#define UDP_EXAMPLE_ID 190

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

#ifndef PERIOD
#define PERIOD 30
#endif

#define START_INTERVAL (15 * CLOCK_SECOND)
#define SEND_INTERVAL (PERIOD * CLOCK_SECOND)
#define SEND_TIME (random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN 30

/*---------------------------------------------------------------------------*/
/* Variable used to send only once with user button*/
int i = 0;
/*---------------------------------------------------------------------------*/
/* The structure used in the Simple UDP library to create an UDP connection */
static struct uip_udp_conn *client_conn;

/* This is the server IPv6 address */
static uip_ipaddr_t server_ipaddr;

/* Keeps account of the number of messages sent */
static uint16_t counter = 0;
/*---------------------------------------------------------------------------*/
/* Create a structure and pointer to store the data to be sent as payload */
static struct my_msg_t msg;
static struct my_msg_t *msgPtr = &msg;
/* Create a structure and pointer to store the data to be received as payload */
static struct my_msg_t msg_rcv;
static struct my_msg_t *msgPtrRcv = &msg_rcv;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static int seq_id;
static int reply;

static void
tcpip_handler(void)
{
    char *str;

    if (uip_newdata())
    {
        str = uip_appdata;
        str[uip_datalen()] = '\0';
        //msg_rcv = (my_msg_t) uip_appdata;
        reply++;
        printf("DATA recv '%s' (s:%d, r:%d)\n", str, seq_id, reply);
        //printf("ID: %u, Value: %d,QoS: %d, batt: %u, counter: %u\n",
        //   msg_rcv.id, msg_rcv.value1, msg_rcv.value2, msg_rcv.battery, msg_rcv.counter);
    }
}
/*---------------------------------------------------------------------------*/
static void
state_trafficlight(int value)
{
    switch (value)
    {
    case 0:
        leds_off(LEDS_ALL);
        leds_on(LEDS_RED);
        break;
    case 1:
        leds_off(LEDS_ALL);
        leds_on(LEDS_BLUE);
        break;
    case 2:
        leds_off(LEDS_ALL);
        leds_on(LEDS_GREEN);
        break;
    default:
        leds_off(LEDS_ALL);
        leds_on(LEDS_PURPLE);
        break;
    }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
    /*
    char buf[MAX_PAYLOAD_LEN];

#ifdef SERVER_REPLY
    uint8_t num_used = 0;
    uip_ds6_nbr_t *nbr;

    nbr = nbr_table_head(ds6_neighbors);
    while (nbr != NULL)
    {
        nbr = nbr_table_next(ds6_neighbors, nbr);
        num_used++;
    }

    if (seq_id > 0)
    {
        ANNOTATE("#A r=%d/%d,color=%s,n=%d %d\n", reply, seq_id,
                 reply == seq_id ? "GREEN" : "RED", uip_ds6_route_num_routes(), num_used);
    }
#endif /* SERVER_REPLY */

/*    seq_id++;
    PRINTF("DATA send to %d 'Hello %d'\n",
           server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1], seq_id);
    sprintf(buf, "Hello %d from the client", seq_id);

    uip_udp_packet_sendto(client_conn, buf, strlen(buf),
                          &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
*/
    uint16_t aux;
    counter++;

    msg.id = 0x2; /* Set traffic light/sensor ID */
    msg.counter = counter;
    msg.value1 = rand() % 3; /* Set traffic light state */
    msg.value2 = 0;          /* Set QoS */

    aux = vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
    msg.battery = (uint16_t)aux;

    /* Print the sensor data */
    printf("ID: %u, Value: %d,QoS: %d, batt: %u, counter: %u\n",
           msg.id, msg.value1, msg.value2, msg.battery, msg.counter);

    state_trafficlight(msg.value1);

    /* Convert to network byte order as expected by the UDP Server application */
    msg.counter = UIP_HTONS(msg.counter);
    msg.value1 = UIP_HTONS(msg.value1);
    msg.battery = UIP_HTONS(msg.battery);

    PRINTF("Send readings to %u'\n",
           server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1]);

    uip_udp_packet_sendto(client_conn, msgPtr, sizeof(msg),
                          &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
}
/*---------------------------------------------------------------------------*/
static void
send_packet_sensor(void)
{
    uint16_t aux;
    counter++;

    msg.id = 0x2; /* Set traffic light/sensor ID */
    msg.counter = counter;
    msg.value1 = rand() % 3; /* Set traffic light state */
    msg.value2 = 2;          /* Set QoS */

    aux = vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
    msg.battery = (uint16_t)aux;

    /* Print the sensor data */
    printf("ID: %u, Value: %d,QoS: %d, batt: %u, counter: %u\n",
           msg.id, msg.value1, msg.value2, msg.battery, msg.counter);

    state_trafficlight(msg.value1);

    /* Convert to network byte order as expected by the UDP Server application */
    msg.counter = UIP_HTONS(msg.counter);
    msg.value1 = UIP_HTONS(msg.value1);
    msg.battery = UIP_HTONS(msg.battery);

    PRINTF("Send readings to %u'\n",
           server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1]);

    uip_udp_packet_sendto(client_conn, msgPtr, sizeof(msg),
                          &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
    int i;
    uint8_t state;

    PRINTF("Client IPv6 addresses: ");
    for (i = 0; i < UIP_DS6_ADDR_NB; i++)
    {
        state = uip_ds6_if.addr_list[i].state;
        if (uip_ds6_if.addr_list[i].isused &&
            (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
        {
            PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
            PRINTF("\n");
            /* hack to make address "final" */
            if (state == ADDR_TENTATIVE)
            {
                uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
            }
        }
    }
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
    uip_ipaddr_t ipaddr;

    uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
    uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

    /* The choice of server address determines its 6LoWPAN header compression.
 * (Our address will be compressed Mode 3 since it is derived from our
 * link-local address)
 * Obviously the choice made here must also be selected in udp-server.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the
 * 6LowPAN protocol preferences,
 * e.g. set Context 0 to fd00::. At present Wireshark copies Context/128 and
 * then overwrites it.
 * (Setting Context 0 to fd00::1111:2222:3333:4444 will report a 16 bit
 * compressed address of fd00::1111:22ff:fe33:xxxx)
 *
 * Note the IPCMV6 checksum verification depends on the correct uncompressed
 * addresses.
 */

#if 0
/* Mode 1 - 64 bits inline */
   uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);
#elif 1
    /* Mode 2 - 16 bits inline */
    uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);
    //uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
#else
    /* Mode 3 - derived from server link-local (MAC) address */
    uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
    static struct etimer periodic;
    static struct ctimer backoff_timer;
#if WITH_COMPOWER
    static int print = 0;
#endif

    PROCESS_BEGIN();

    PROCESS_PAUSE();

    set_global_address();

    PRINTF("UDP client process started nbr:%d routes:%d\n",
           NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);

    print_local_addresses();

    PRINTF("Server address: ");
    PRINT6ADDR(&server_ipaddr);
    PRINTF("\n");

    /* new connection with remote host */
    client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
    if (client_conn == NULL)
    {
        PRINTF("No UDP connection available, exiting the process!\n");
        PROCESS_EXIT();
    }
    udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

    PRINTF("Created a connection with the server ");
    PRINT6ADDR(&client_conn->ripaddr);
    PRINTF(" local/remote port %u/%u\n",
           UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

    /* initialize serial line */
    uart1_set_input(serial_line_input_byte);
    serial_line_init();

    /* Activate the sensors */
#if CONTIKI_TARGET_ZOUL
    adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);
#else /* Default is Z1 */
    SENSORS_ACTIVATE(adxl345);
    SENSORS_ACTIVATE(tmp102);
    SENSORS_ACTIVATE(battery_sensor);
#endif

    SENSORS_ACTIVATE(button_sensor);
    leds_on(LEDS_RED); //Traffic lights 2 & 4 start at green, 1 & 3 at red

#if WITH_COMPOWER
    powertrace_sniff(POWERTRACE_ON);
#endif

    etimer_set(&periodic, SEND_INTERVAL);
    while (1)
    {
        PROCESS_YIELD();
        if (ev == tcpip_event)
        {
            tcpip_handler();
        }
        i = i + 1;
        if (ev == serial_line_event_message && data != NULL)
        {
            char *str;
            str = data;
            if (str[0] == 'r')
            {
                uip_ds6_route_t *r;
                uip_ipaddr_t *nexthop;
                uip_ds6_defrt_t *defrt;
                uip_ipaddr_t *ipaddr;
                defrt = NULL;
                if ((ipaddr = uip_ds6_defrt_choose()) != NULL)
                {
                    defrt = uip_ds6_defrt_lookup(ipaddr);
                }
                if (defrt != NULL)
                {
                    PRINTF("DefRT: :: -> %02d", defrt->ipaddr.u8[15]);
                    PRINTF(" lt:%lu inf:%d\n", stimer_remaining(&defrt->lifetime),
                           defrt->isinfinite);
                }
                else
                {
                    PRINTF("DefRT: :: -> NULL\n");
                }

                for (r = uip_ds6_route_head();
                     r != NULL;
                     r = uip_ds6_route_next(r))
                {
                    nexthop = uip_ds6_route_nexthop(r);
                    PRINTF("Route: %02d -> %02d", r->ipaddr.u8[15], nexthop->u8[15]);
                    /* PRINT6ADDR(&r->ipaddr); */
                    /* PRINTF(" -> "); */
                    /* PRINT6ADDR(nexthop); */
                    PRINTF(" lt:%lu\n", r->state.lifetime);
                }
            }
        }

        if (etimer_expired(&periodic))
        {
            etimer_reset(&periodic);
            ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);

#if WITH_COMPOWER
            if (print == 0)
            {
                powertrace_print("#P");
            }
            if (++print == 3)
            {
                print = 0;
            }
#endif
        }
        /* QoS 2: Priority data when pressing the user button */
        if (ev == sensors_event && data == &button_sensor)
        {
            if (i % 2 == 0)
            {
                send_packet_sensor();
            }
        }
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
