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
MQTT_URL_PUB      = "/v1.6/devices/simple/" 	# publishing TOPIC
MQTT_URL_TOPIC    = "/v1.6/devices/simple/#"	# subscribing TOPIC
#------------------------------------------------------------#
# Variables used
#------------------------------------------------------------#
var1 = "ADC1"
var2 = "ADC2"
#------------------------------------------------------------#
HOST		= "aaaa::1"
CLIENT		= "aaaa::212:4b00:60d:b374" # Matching the border router address fe80::212:4b00:60d:b3ef
PORT		= 5678
CMD_PORT	= 8765
BUFSIZE		= 4096
#------------------------------------------------------------#
# If using a client based on the Z1 mote, then enable by equal to 1, else if
# using the RE-Mote equal to 0
EXAMPLE_WITH_Z1   = 0
#------------------------------------------------------------#
ENABLE_MQTT       = 1
ENABLE_LOG        = 1
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

routes_dict = {}
trafficlight_state = {"feu1": 0, "feu2": 2, "sensor1": 0, "sensor2": 2}

#------------------------------------------------------------#
# Export expected values from messages
#------------------------------------------------------------#
def jsonify_recv_data(msg):		# Get traffic light state from message
	for f_name, f_type in msg._fields_:
		if f_name =="ADC2":	# Get expected value
			if getattr(msg, f_name) != "0":
				value=getattr(msg, f_name) 	
			sensordata=value
	return str(sensordata)
# -----------------------------------------------------------#
def jsonify_recv_QOS(msg):		# Get QoS from message
	for f_name, f_type in msg._fields_:
		if f_name =="battery":	# Get expected value
			QOS=getattr(msg, f_name)		
	return str(QOS)
# -----------------------------------------------------------#
# Sender fonction to the border router
# -----------------------------------------------------------#
def send_udp_cmd(addr,FEU, s):		
	#sclient = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
	#sclient.bind(('fd00::1', PORT))
	print "Sending reply to " + CLIENT
	try:
		if FEU == "1" or FEU =="2":
			DONNEE="A"
		else:
			DONNEE="B"
		print (CLIENT, CMD_PORT)
		print "Data sent : "+DONNEE
		s.sendto(DONNEE, (CLIENT, CMD_PORT))
		
	except Exception as error:
		print error
	#sclient.close()

def update_trafficlights_cmd(var):	
	if var not in routes_dict:
		routes_dict[var] = {}
	print routes_dict[var].keys()
	for client in routes_dict[var].keys():
		print "Sending reply to " + client
		try:
			DATA = trafficlight_state[var]
			print (CLIENT, CMD_PORT)
			print "Data sent : "+DATA
			SOCK.sendto(DATA, (CLIENT, CMD_PORT))
			
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
		if pubid ==1:
			TOPIC="sensor1"
		if pubid ==2:
			TOPIC="sensor2"
		if pubid<1 or pubid>2:
			print "ID no recognized"
		print
		print "Data receiv : ", data, "by sensor : ", pubid
		print

		print "Collecting client address"
		lightid = int(pubid)%2
		if lightid == 0:
			lightid = 2
		strid = "sensor"+str(lightid)
		print strid, lightid
		if (strid) not in routes_dict:
			print "Creation of "+str(lightid)
			routes_dict[strid] = {}
		routes_dict[strid][addr] = data
		print "Import finished"
		print str(routes_dict)

		

		# Valeur changée et définie pour la soutenance
		# A commenter si des valeurs spécifiques sont attendues
		#------------------------------------------------------#
		# if data != 0:
		# 	data ="2" # Force le feu au vert
		#------------------------------------------------------#

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

		if (TOPIC == 'sensor1') and data == "1":
			#conn.publish.multiple(msgs, hostname=MQTT_URL)

			res, mid = conn.publish(MQTT_URL_PUB + 'sensor1', payload=data, qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'sensor1' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'sensor2', payload="8", qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'sensor2' + " with value : " + "8"
		else:
			res, mid = conn.publish(MQTT_URL_PUB + 'sensor1', payload="8", qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'sensor1' + " with value : " + "8"
			res, mid = conn.publish(MQTT_URL_PUB + 'sensor2', payload=data, qos=int(QOS))
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'sensor2' + " with value : " + data
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
	print(msg.topic + " " + str(msg.payload) + ", qos: " + str(msg.qos))
	if(msg.payload[0] != "{"):
		var = str(msg.topic.split('/',5)[4])
		print "Update required", "payload :",msg.payload, var
		trafficlight_state[var] = msg.payload
		print trafficlight_state
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
		print str(now)[:19] + " -> " + str(addr[0]) + ":" + str(addr[1]) + " " + str(len(data))
		msg_recv = SENSOR(data)
		
		if ENABLE_LOG:
			print_recv_data(msg_recv)
		
		# Get publisher values from message
		sensordata = jsonify_recv_data(msg_recv)	
		QOS = jsonify_recv_QOS(msg_recv)

		if ENABLE_MQTT:
			publish_recv_data(sensordata, msg_recv.id, client, addr[0],QOS)

		# if UPDATE_NEEDED:
		# 	print "UPDATE"
		# 	update_trafficlights_cmd(UPDATE_NEEDED)

		#send_udp_cmd(addr[0],str(msg_recv.id), s)


#------------------------------------------------------------#
# MAIN APP
#------------------------------------------------------------#
if __name__ == "__main__":
 	start_client()

