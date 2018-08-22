/*
 * Copyright (c) 2017,  Zolertia - http://www.zolertia.io
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
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup
 * @{
 *    this demo is used to control a variable from the mqtt, the variable is
 *    loaded in <arg>, in the function it generates a PWM for each color of the
 *    RGB LED, in addition to publishing the value of the digital light sensor
 *    in each interruption.
 * @{
 *
 * \author
 *          Erik Bellido < erikbegr@gmail.com >
 */
/* -------------------------------------------------------------------------- */

#include "contiki.h"
#include "process.h"
#include "core/sys/ctimer.h"
#include "sys/etimer.h"
#include "dev/pwm.h"
#include "dev/gpio.h"
#include "dev/leds.h"
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"
#include "dev/button-sensor.h"
#include "trafficlight.h"
#include "mqtt-res.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#if DEBUG_APP
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
sensor_values_t trafficlight_sensors;
command_values_t trafficlight_commands;
/*---------------------------------------------------------------------------*/
process_event_t trafficlight_sensors_data_event;
process_event_t trafficlight_sensors_alarm_event;
/*---------------------------------------------------------------------------*/
static uint8_t detect_sensor = 0;
/*---------------------------------------------------------------------------*/
/* Keep global track of trafficlight state */
uint16_t my_state;

static struct etimer tim;

/* Flag to pause Re-mote etimer */
static int PAUSE = 0;

static struct etimer et;

static struct pt example_pt;
/*---------------------------------------------------------------------------*/
PROCESS(trafficlight_sensors_process, "trafficlight process");
PROCESS(trafficlight_timer_process, "trafficlight  timer process");
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int counter;
static struct ctimer ctim;
static void my_funct(void *ptr){
  printf("Helloooo you ");
  state_trafficlight(0);
}
void state_handler(int value)
{
  //Check if it's the right moment

  if (value == 0)
  {
    printf("Operation orange sanguine \n");
    state_trafficlight(1);

    //Wait for 5s
    ctimer_set(&ctim, 3*CLOCK_SECOND, my_funct, NULL);
  
    process_start(&trafficlight_timer_process, NULL);

    return;
  }

  printf("Terminus true value \n");
  state_trafficlight(value);
}
void state_trafficlight(int value)
{
  if (value > 2)
    printf("Err value: %u", value);

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
static void
poll_sensors(void)
{
  if (my_state == 0)
    trafficlight_sensors.sensor[REMOTE_SENSOR_TRAFFICLIGHT].value = 2;
  else
    trafficlight_sensors.sensor[REMOTE_SENSOR_TRAFFICLIGHT].value = 0;

  trafficlight_sensors.sensor[REMOTE_SENSOR_BATT].value = (uint16_t)vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED) / 10;

  mqtt_sensor_check(&trafficlight_sensors, trafficlight_sensors_alarm_event,
                    trafficlight_sensors_data_event);

  my_state = trafficlight_sensors.sensor[REMOTE_SENSOR_TRAFFICLIGHT].value;
}
/*---------------------------------------------------------------------------*/
static int
activate_color_led(int arg)
{
  uint8_t port, pin, i;
  uint16_t count;
  uint8_t timer_num, timer_ab; /* variable timers */

  if (arg > LED_MAX_VAL)
  {
    printf("Servo: invalid value (max %u)\n", LED_MAX_VAL);
    return -1;
  }

  timer_num = PWM_TIMER_0;
  timer_ab = PWM_TIMER_B;

  printf("PWM: %uHz GPTNUM %u GPTAB %u --> %u%% \n", LED_DEFAULT_FREQ, timer_num, timer_ab, arg);
  state_trafficlight(arg);
  my_state = arg;

  trafficlight_sensors.sensor[REMOTE_SENSOR_TRAFFICLIGHT].value = my_state;
  sensor_values_t *msgPtr = (sensor_values_t *)&trafficlight_sensors;
  publish_event(msgPtr, 0);

  printf("DATA_COMMAND is %u\n", arg);
  process_poll(&trafficlight_sensors_process);
  return 0;
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(trafficlight_sensors_process, ev, data)
{
  //static struct etimer et;

  /* This is where our process start */
  PROCESS_BEGIN();

  /* Configure the ADC ports */
  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);

  /* Load sensor defaults */
  trafficlight_sensors.num = 0;

  /* Register digital sensors */
  mqtt_sensor_register(&trafficlight_sensors, REMOTE_SENSOR_TRAFFICLIGHT,
                       DEFAULT_WRONG_VALUE, DEFAULT_PUBLISH_EVENT_TRAFFICLIGHT,
                       NULL, DEFAULT_SUBSCRIBE_CFG_TRAFFICLIGHTHR,
                       DEFAULT_TRAFFICLIGHT_MIN, DEFAULT_TRAFFICLIGHT_MAX,
                       DEFAULT_TRAFFICLIGHT_THRESH, DEFAULT_TRAFFICLIGHT_THRESL, 0);

  // mqtt_sensor_register(&trafficlight_sensors, REMOTE_SENSOR_TEMP,
  //                      DEFAULT_BMP180_TEMP_MIN, DEFAULT_PUBLISH_EVENT_TEMP,
  //                      DEFAULT_PUBLISH_ALARM_TEMP, DEFAULT_SUBSCRIBE_CMD_TEMPTHR,
  //                      DEFAULT_BMP180_TEMP_MIN, DEFAULT_BMP180_TEMP_MAX,
  //                      DEFAULT_TEMP_THRESH, DEFAULT_TEMP_THRESL, 10);

  // mqtt_sensor_register(&trafficlight_sensors, REMOTE_SENSOR_TEMP,
  //                      DEFAULT_CC2538_TEMP_MIN, DEFAULT_PUBLISH_EVENT_TEMP,
  //                      NULL, NULL, DEFAULT_CC2538_TEMP_MIN,
  //                      DEFAULT_CC2538_TEMP_MAX, DEFAULT_TEMP_THRESH,
  //                      DEFAULT_TEMP_THRESL, 100);

  mqtt_sensor_register(&trafficlight_sensors, REMOTE_SENSOR_BATT,
                       DEFAULT_BATT_MIN, DEFAULT_PUBLISH_EVENT_BATT,
                       NULL, NULL, DEFAULT_BATT_MIN,
                       DEFAULT_BATT_MAX, DEFAULT_BATT_THRESH,
                       DEFAULT_BATT_THRESL, 100);

  // mqtt_sensor_register(&trafficlight_sensors, REMOTE_SENSOR_ADC1,
  //                      DEFAULT_CC2538_ADC1_MIN, DEFAULT_PUBLISH_EVENT_ADC1,
  //                      NULL, NULL, DEFAULT_CC2538_ADC1_MIN,
  //                      DEFAULT_CC2538_ADC1_MAX, DEFAULT_ADC1_THRESH,
  //                      DEFAULT_ADC1_THRESL, 100);

  // mqtt_sensor_register(&trafficlight_sensors, REMOTE_SENSOR_ADC3,
  //                      DEFAULT_CC2538_ADC3_MIN, DEFAULT_PUBLISH_EVENT_ADC3,
  //                      NULL, NULL, DEFAULT_CC2538_ADC3_MIN,
  //                      DEFAULT_CC2538_ADC3_MAX, DEFAULT_ADC3_THRESH,
  //                      DEFAULT_ADC3_THRESL, 100);

  // /* We post alarms directly to the platform, instead of having the mqtt-sensors
  //  * checking for occurences
  //  */
  // mqtt_sensor_register(&trafficlight_sensors, REMOTE_SENSOR_BUTN,
  //                      DEFAULT_CC2538_BUTN_MIN, NULL, DEFAULT_PUBLISH_ALARM_BUTN,
  //                      NULL, DEFAULT_CC2538_BUTN_MIN,
  //                      DEFAULT_CC2538_BUTN_MAX, DEFAULT_BUTN_THRESH,
  //                      DEFAULT_BUTN_THRESL, 0);

  /* Sanity check */
  if (trafficlight_sensors.num != DEFAULT_SENSORS_NUM)
  {
    printf("Ubidots sensors: error! number of sensors mismatch\n");
    printf("Because sensor_values_t is %u and DEFAULT_SENSORS_NUM is %u\n", trafficlight_sensors.num, DEFAULT_SENSORS_NUM);
    PROCESS_EXIT();
  }

  /* Load commands default */
  trafficlight_commands.num = 1;

  memcpy(trafficlight_commands.command[0].command_name, DEFAULT_COMMAND_EVENT_LED, strlen(DEFAULT_COMMAND_EVENT_LED));

  trafficlight_commands.command[0].cmd = activate_color_led;

  if (trafficlight_commands.num != DEFAULT_COMMANDS_NUM)
  {
    printf("Ubidots sensors: error! number of commands mismatch\n");
    PROCESS_EXIT();
  }

  /* Get an event ID for our events */
  trafficlight_sensors_data_event = process_alloc_event();
  trafficlight_sensors_alarm_event = process_alloc_event();

  /* Activate external RGB LEDS for GPIO activation SENSORS_ACTIVATE(leds); */

  /* Start the periodic process */
  etimer_set(&et, 12 * DEFAULT_SAMPLING_INTERVAL);

  while (1)
  {

    PROCESS_YIELD();
    if (PAUSE)
    {
      etimer_stop(&et);
      printf("Pause\n");
      etimer_set(&tim, 3 * CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&tim));
      PAUSE = 0;
      state_trafficlight(0);
      printf("End Pause\n");
      etimer_restart(&et);
    }
    printf("Doooooo iiiit \n");
    if (ev == PROCESS_EVENT_TIMER && data == &et)
    {
      printf("mooove \n");
      poll_sensors();
      etimer_reset(&et);
    }
    else if (ev == sensors_stop_event)
    {
      PRINTF("Ubidots: sensor readings paused\n");
      etimer_stop(&et);
    }
    else if (ev == sensors_restart_event)
    {
      PRINTF("Ubidots: sensor readings enabled\n");
      etimer_reset(&et);
    }
  }

  PROCESS_END();
}

PROCESS_THREAD(trafficlight_timer_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT();
      printf("yoyoyo Ev numero %d\n", ev);
  }
  PROCESS_END();
}

// static PT_THREAD(exampleof(struct pt *pt)){
//   PT_BEGIN(pt);
//   while(1){
//     PT_WAIT_UNTIL(pt, counter == 1000);
//     printf("Reached\n");
//     counter = 0;
//   }
//   PT_END();
// }