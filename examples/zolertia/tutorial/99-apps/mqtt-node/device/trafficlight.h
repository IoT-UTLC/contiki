/*
 * Copyright (c) 2017, Erik Bellido < erikbegr@gmail.com >
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
#ifndef trafficlight_H_
#define trafficlight_H_
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include "mqtt-sensors.h"
#include "dev/pwm.h"
/*---------------------------------------------------------------------------*/
enum { REMOTE_SENSOR_TRAFFICLIGHT = 0,
        REMOTE_SENSOR_BATT };
enum { trafficlight_COMMAND = 0 };
/*---------------------------------------------------------------------------*/
/* Sensor process events */
extern process_event_t trafficlight_sensors_data_event;
extern process_event_t trafficlight_sensors_alarm_event;
/*---------------------------------------------------------------------------*/
extern sensor_values_t trafficlight_sensors;
/*---------------------------------------------------------------------------*/
extern command_values_t trafficlight_commands;

extern uint16_t my_state;

extern void state_handler(int value);
/*---------------------------------------------------------------------------*/
/* PUBLISH strings */
#define DEFAULT_PUBLISH_EVENT_TRAFFICLIGHT           "trafficlight"
#define DEFAULT_PUBLISH_ALARM_TRAFFICLIGHT           "alarm_trafficlight"

#define DEFAULT_PUBLISH_EVENT_BATT            "battery"
#define DEFAULT_PUBLISH_ALARM_BATT           "alarm_battery"

/* SUBSCRIBE strings */
#define DEFAULT_SUBSCRIBE_CFG_TRAFFICLIGHTHR         "trafficlight_thresh"
#define DEFAULT_SUBSCRIBE_CMD_BATTTHR         "battery_thresh"

/* Minimum and maximum values for the sensors */
#define DEFAULT_TRAFFICLIGHT_MIN             0
#define DEFAULT_TRAFFICLIGHT_MAX             30000

#define DEFAULT_WRONG_VALUE                   (-300)

#define DEFAULT_BATT_MIN       0
#define DEFAULT_BATT_MAX       12000

/* Default sensor state and thresholds (not checking trfor alarms) */
#define DEFAULT_TRAFFICLIGHT_THRESH          DEFAULT_TRAFFICLIGHT_MAX
#define DEFAULT_TRAFFICLIGHT_THRESL          DEFAULT_TRAFFICLIGHT_MIN

#define DEFAULT_BATT_THRESL                   DEFAULT_BATT_MIN


#define DEFAULT_BATT_THRESH           3000

/*led color red*/
#define DEFAULT_COMMAND_EVENT_LED                       "/led_toggle/lv"
#define DEFAULT_CMD_STRING                              DEFAULT_COMMAND_EVENT_LED

#define DEFAULT_COMMAND_EVENT_TRAFFICLIGHT              "/+/lv"
#define DEFAULT_CMD_STRING                              DEFAULT_COMMAND_EVENT_TRAFFICLIGHT

#define DEFAULT_COMMAND_EVENT_TRAFFICLIGHT_TIMER        "/next_trafficlight/lv"
#define DEFAULT_CMD_TIMER_STRING                              DEFAULT_COMMAND_EVENT_TRAFFICLIGHT_TIMER

#define DEFAULT_COMMAND_EVENT_TRAFFICLIGHT_1            "/feu1/lv"
#define DEFAULT_CMD_FEU1_STRING                              DEFAULT_COMMAND_EVENT_TRAFFICLIGHT_1

#define DEFAULT_COMMAND_EVENT_TRAFFICLIGHT_2            "/feu2/lv"
#define DEFAULT_CMD_FEU2_STRING                              DEFAULT_COMMAND_EVENT_TRAFFICLIGHT_2

#define DEFAULT_CONF_ALARM_TIME               30

/*define te pins of the led in the re-mote_revb*/
#if PLATFORM_HAS_LEDS == 1
#define LEDS_RED_PIN 4
#define LEDS_RED_PORT GPIO_D_NUM
#define LEDS_GREEN_PIN 7
#define LEDS_GREEN_PORT GPIO_B_NUM
#define LEDS_BLUE_PIN 6
#define LEDS_BLUE_PORT GPIO_B_NUM
#endif

/*---------------------------------------------------------------------------*/

#ifndef LED_CONF_FREQ
#define LED_DEFAULT_FREQ       244 /*< Hz */
#else
#define LED_DEFAULT_FREQ       LED_CONF_FREQ
#endif

#ifndef LED_CONF_MIN_VAL
#define LED_MIN_VAL            0
#endif

#ifndef LED_CONF_MAX_VAL
#define LED_MAX_VAL            100
#endif

/* This is the base-time unit, if using a DEFAULT_SAMPLING_INTERVAL of 1 second
 * (given by the CLOCK_SECOND macro) the node will periodically publish every
 * DEFAULT_PUBLISH_INTERVAL seconds.
 */
#define DEFAULT_PUBLISH_INTERVAL      (-1)
#define DEFAULT_SAMPLING_INTERVAL     CLOCK_SECOND

#endif /* trafficlight_H_ */