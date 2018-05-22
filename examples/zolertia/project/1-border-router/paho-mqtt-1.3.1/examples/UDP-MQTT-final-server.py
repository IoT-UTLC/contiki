# -*- coding: utf-8 -*-
#! /usr/bin/env python
import sys
import paho.mqtt.client as mqtt
import json
import time
import socket
import datetime
from random import randint
import struct
from ctypes import *
#------------------------------------------------------------#
# Explication des variables
#------------------------------------------------------------#
# Nous avions définis 3 valeurs pour 3 couleurs différentes à envoyer au Broker
# Nous récupérions à la base des valeurs aléatoire envoyés 
# 0 : Feu doit être Rouge
# 1 : Feu doit être Orange
# 2 : Feu doit être Vert
#------------------------------------------------------------#
ID_STRING      = "V0.1"
#------------------------------------------------------------#
MQTT_URL          = "things.ubidots.com"		# Url for accesing the Broker MQTT
MQTT_PORT         = 1883						# MQTT port
MQTT_KEEPALIVE    = 60
MQTT_URL_PUB      = "/v1.6/devices/zolertia/" 	# publishing TOPIC
MQTT_URL_TOPIC    = "/v1.6/devices/zolertia/#"	# subscribing TOPIC
#------------------------------------------------------------#
# Variables used
#------------------------------------------------------------#
var1 = "ADC1"
var2 = "ADC2"
#------------------------------------------------------------#
HOST		= "aaaa::1"
CLIENT		= "aaaa::212:4b00:616:f5f" # Matching the border router address fe80::212:4b00:60d:b3ef
PORT		= 5678
CMD_PORT	= 8765
BUFSIZE		= 4096
#------------------------------------------------------------#
# If using a client based on the Z1 mote, then enable by equal to 1, else if
# using the RE-Mote equal to 0
EXAMPLE_WITH_Z1   = 0
#------------------------------------------------------------#
ENABLE_MQTT       = 1
ENABLE_LOG        = 0
#------------------------------------------------------------#
DEBUG_PRINT_JSON  = 1

#------------------------------------------------------------#
# Socket used thoughout the process
#------------------------------------------------------------#
SOCK = None
#------------------------------------------------------------#
# Message structure
#------------------------------------------------------------#
class SENSOR(Structure):
	_pack_   = 1
		# Structure Key/Value
	_fields_ = 	[("id",c_uint16),	
			("counter",c_uint8),
			(var1,c_uint16),
			(var2,c_uint8),
			("battery",c_uint8)]

	def __new__(self, socket_buffer):
        	return self.from_buffer_copy(socket_buffer)

	def __init__(self, socket_buffer):
        	pass

#routes_dict = {}
routes_dict = {'feu1': {'aaaa::212:4b00:60d:b318': '0', 'aaaa::212:4b00:60d:b288': '0'}, 'feu2': {'aaaa::212:4b00:60d:b41c': '2', 'aaaa::212:4b00:60d:b374': '2'}, 'feu3': {}, 'sensorRoad1': {}, 'sensorRoad2': {}} #Just for testing, automatically add new addresses and topics
trafficlight_state = {"feu1": 0, "feu2": 2}

#------------------------------------------------------------#
# Export expected values from messages
#------------------------------------------------------------#
def jsonify_recv_data(msg, no_str=None):		# Get traffic light state from message
	for f_name, f_type in msg._fields_:
		if f_name =="ADC2":	# Get expected value
			if getattr(msg, f_name) != "0":
				value=getattr(msg, f_name) 	
			sensordata=value
	if no_str:
		return sensordata
	return str(sensordata)
# -----------------------------------------------------------#
def jsonify_recv_QOS(msg):						# Get QoS from message
	for f_name, f_type in msg._fields_:
		if f_name =="battery":	# Get expected value
			QOS=getattr(msg, f_name)		
	return str(QOS)
# -----------------------------------------------------------#
# Sender fonction to test modules
# -----------------------------------------------------------#
def send_udp_cmd(msg=None):		
	print "Sending reply to my test" + CLIENT
	try:
		print "id", msg.id, "counter", msg.counter, "ADC1", jsonify_recv_data(msg), "ADC2", jsonify_recv_QOS(msg)
		my_msg = struct.pack("III", int(jsonify_recv_data(msg)), int(jsonify_recv_QOS(msg)), 0)
		
		SOCK.sendto(my_msg, (CLIENT, CMD_PORT))
		
	except Exception as error:
		print error

def update_trafficlights_cmd(var, urgent=None):	
	if var not in routes_dict:
		routes_dict[var] = {}
	print routes_dict[var].keys()
	if var == "all": # All clients from every traffic topics
		db = [routes_dict[c].keys() for c in routes_dict] # TO DO
		print db
	else: # clients from a specific topic
		db = routes_dict[var].keys()
	for client in db:
		print "Sending reply to " + client
		try:
			data = trafficlight_state[var]
			if urgent:
				qos = 2
			else:
				qos = 0
			print "Data sent : ",data,"Qos :",qos
			my_msg = struct.pack("III", int(data), qos, 0)
			SOCK.sendto(my_msg, (client, CMD_PORT))
			
		except Exception as error:
			print error
# -----------------------------------------------------------#
# Print publisher's message received
# -----------------------------------------------------------#
def print_recv_data(msg):		
	print "***"
	for f_name, f_type in msg._fields_:
		print "{0}:{1} ".format(f_name, getattr(msg, f_name)), 
	print
	print "***"
# -----------------------------------------------------------#
# Publish to MQTT Broker
# -----------------------------------------------------------#
def publish_recv_data(data, pubid, conn, addr,QOS):
	try:
		# Select which traffic light sent something
		if pubid == 1:
			TOPIC = "feu1"
		if pubid == 2:
			TOPIC = "feu2"
		if pubid == 3:
			TOPIC = "feu1"
		if pubid == 4:
			TOPIC = "feu2"
		if pubid == 5:
			TOPIC = "sensorRoad1"
		if pubid == 6:
			TOPIC = "sensorRoad2"
		if pubid <1 and pubid > 6 :
			print "ID no recognized"
		print
		print "Data received : ", data, "by trafficlight : ", pubid, "addr : ",addr, "QoS : ",QOS
		print

		if TOPIC not in routes_dict:
			print "Creation of "+TOPIC
			routes_dict[TOPIC] = {}

		if addr not in routes_dict[TOPIC]:
			print "Collecting client address"
			routes_dict[TOPIC][addr] = data
			print str(routes_dict)

		if QOS == '2':
			print
			print "Important data received, waiting to publish"
			print
		if QOS=='1':
			print
			print "Data sent less once on Ubidots"
			print
		if QOS=='0':
			print
			print "Data directly sent to Ubidots without verification"
			print

		if (pubid == 5) or ((pubid == 1) and data == "2" and trafficlight_state[TOPIC] != data): # trafficlight 1 goes to green
			#conn.publish.multiple(msgs, hostname=MQTT_URL)

			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload="0", qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + "0" + " and QoS : "+QOS
			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload=data, qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + data + " and QoS : "+QOS

		if (pubid == 6) or ((pubid == 2) and data == "2" and trafficlight_state[TOPIC] != data): # trafficlight 2 goes to green

			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload="0", qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + "0" + " and QoS : "+QOS
			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload=data, qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + data + " and QoS : "+QOS



		if (pubid == 2) and data == "0" and trafficlight_state[TOPIC] != data: # trafficlight 2 goes to red
			#conn.publish.multiple(msgs, hostname=MQTT_URL)

			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload=data, qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + data + " and QoS : "+QOS
			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload="2", qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + "2" + " and QoS : "+QOS

		if (pubid == 1) and data == "0" and trafficlight_state[TOPIC] != data: # trafficlight 1 goes to red

			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload=data, qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + data + " and QoS : "+QOS
			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload="2", qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + "2" + " and QoS : "+QOS
			
			
		if data == "1" and (pubid < 3):
			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload=data, qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + data + " and QoS : "+QOS
			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload=data, qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + data + " and QoS : "+QOS
		print("Data sent to server")
		
	except Exception as error:
		print error
# -----------------------------------------------------------#
# MQTT Function
# -----------------------------------------------------------#
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
	print("Connected with result code "+str(rc))
	print("Subscribed to " + MQTT_URL_TOPIC)
	client.subscribe(MQTT_URL_TOPIC)
	#print("usrData: "+str(userdata))
	#print("client: " +str(client))
	#print("flags: " +str(flags))
#------------------------------------------------------------#
# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
	print ("Published message from server ")
	print(msg.topic + " " + str(msg.payload) + ", QoS: " + str(msg.qos))
	if(msg.payload[0] != "{"):
		var = str(msg.topic.split('/',5)[4])
		if(var != "feu3" and trafficlight_state[var] != msg.payload ): # Update valid and needed
			trafficlight_state[var] = msg.payload
			update_trafficlights_cmd(var)


#------------------------------------------------------------#
# Main function
#------------------------------------------------------------#
def start_client():
	global SOCK
	now = datetime.datetime.now()
	print "UDP6-MQTT server side application "  + ID_STRING
	print "Started " + str(now)
	# Datagram (udp) socket
	try:
		SOCK = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
		print 'Socket created'
		SOCK.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	except Exception :
		print 'Failed to create socket.'
		sys.exit()
	# Set socket address and port
	try:
		server_address = (HOST, PORT)
		print >>sys.stderr, 'starting up on %s port %s' % server_address
		SOCK.bind(server_address)
		print "socket : " + str(SOCK)
		print "setsockopt : " + str(SOCK.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1))
	except socket.error as msg:
		print('Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1])
		sys.exit()
		return
	print 'UDP6-MQTT server ready: %s'% PORT
	print "msg structure size: ", sizeof(SENSOR)
	print

	if ENABLE_MQTT:
		# Initialize MQTT connexion
		try:
			client = mqtt.Client()
		except Exception as error:
			print error
			raise
		# Allow connexion to the broker
		client.on_connect = on_connect
		# Set your Ubidots default token
		client.username_pw_set("A1E-Zdfehlc7EmA9JEer6YIARbtHIMK1y8", "") #A1E-dEzBw6HHcOgRUz22KIwvuYkfvmfixy
		client.on_message = on_message

		try:
			client.connect(MQTT_URL, MQTT_PORT, MQTT_KEEPALIVE)
		except Exception as error:
			print error
			raise

	# Start the MQTT thread and handle reconnections, also ensures the callbacks
	# being triggered
	client.loop_start()
	
	while True:

		# Receiving client data (data, addr)
		print >>sys.stderr, '\nwaiting to receive message from sensor'
		data, addr = SOCK.recvfrom(BUFSIZE)
		now = datetime.datetime.now()
		#print str(now)[:19] + " -> " + str(addr[0]) + ":" + str(addr[1]) + " " + str(len(data))
		msg_recv = SENSOR(data)
		
		if ENABLE_LOG:
			print_recv_data(msg_recv)
		
		# Get publisher values from message
		sensordata = jsonify_recv_data(msg_recv)	
		QOS = jsonify_recv_QOS(msg_recv)

		if ENABLE_MQTT:
			publish_recv_data(sensordata, msg_recv.id, client, addr[0],QOS)

		print routes_dict

		if QOS == "2":
			print "QOS 2 --------"
			# TO DO send all
			update_trafficlights_cmd("feu1", True)
			update_trafficlights_cmd("feu2", True)


#------------------------------------------------------------#
# MAIN APP
#------------------------------------------------------------#
if __name__ == "__main__":
 	start_client()

