# -*- coding: utf-8 -*-
#! /usr/bin/env python
import sys
import paho.mqtt.client as mqtt
import json
import time
import socket
#from socket import *
#from socket import error
import datetime
from random import randint
import struct
from ctypes import *
#------------------------------------------------------------#
ID_STRING      = "V0.1"
#------------------------------------------------------------#
MQTT_URL          = "things.ubidots.com"
MQTT_PORT         = 1883
MQTT_KEEPALIVE    = 60
MQTT_URL_PUB      = "/v1.6/devices/zolertia2/"
MQTT_URL_TOPIC    = "/cmd"
#------------------------------------------------------------#
# Variable used
topicF1 = MQTT_URL_PUB+"feu1"
topicF2 = MQTT_URL_PUB+"feu2"
topicF3 = MQTT_URL_PUB+"feu3"
topicF4 = MQTT_URL_PUB+"feu4"
payloadF1 = str(randint(0, 2))
payloadF2 = str(randint(0, 2))
payloadF3 = str(randint(0, 2))
payloadF4 = str(randint(0, 2))

#------------------------------------------------------------#
HOST		= "aaaa::1"
PORT		= 5678
CMD_PORT	= 8765
BUFSIZE		= 8192 #4096
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
# Message structure
#------------------------------------------------------------#
if EXAMPLE_WITH_Z1:
	var1 = "temperature"
	var2 = "x_axis"
	var3 = "y_axis"
	var4 = "z_axis"
else:
	#var1 = topicF1
	#var2 = topicF2
	#var3 = topicF3
	#var4 = topicF4  
	var1 = "ADC1"
	#var1 = "core_temp"
	#var2 = "ADC1"
	#var3 = "ADC2"
	#var4 = "ADC3"


class SENSOR(Structure):
	_pack_   = 1
	_fields_ = [("id",c_uint16),("counter",c_uint8),(var1,c_uint16),("battery",c_uint8)]

	def __new__(self, socket_buffer):
        	return self.from_buffer_copy(socket_buffer)

	def __init__(self, socket_buffer):
        	pass
#------------------------------------------------------------#
# Helper functions
#------------------------------------------------------------#
def jsonify_recv_data(msg):
	#sensordata = '{"values":['
	for f_name, f_type in msg._fields_:
		if f_name =="battery":
			if getattr(msg, f_name) != "0":
				value=getattr(msg, f_name)
			#sensordata=jsonify(f_name, value)
			sensordata=value
		#sensordata += jsonify(f_name, getattr(msg, f_name)) + ","
	#sensordata = sensordata[:-1]
	#sensordata += ']}'
  # Paho MQTT client doesn't support sending JSON objects
	#json_parsed = json.loads(sensordata)
	#if DEBUG_PRINT_JSON:
		#print json.dumps(json_parsed, indent=2)

	return str(sensordata)
# -----------------------------------------------------------#
def send_udp_cmd(addr):
	client = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)

	print "Sending reply to " + addr

	try:
		client.sendto("Hello from the server", (addr, CMD_PORT))
	except Exception as error:
		print error

	client.close()
# -----------------------------------------------------------#
def print_recv_data(msg):
	print "***"
	for f_name, f_type in msg._fields_:
		print "{0}:{1} ".format(f_name, getattr(msg, f_name)),
	print
	print "***"
# -----------------------------------------------------------#
def jsonify(keyval, val):
	return json.dumps(dict(value=val, key=keyval))
# -----------------------------------------------------------#
def publish_recv_data(data, pubid, conn, addr):
	try:
		print "pubid : " + str(pubid)
		if pubid ==1:
			TOPIC='feu1'
		if pubid ==2:
			TOPIC='feu2'
		if pubid ==3:
			TOPIC='feu3'
		if pubid ==4:
			TOPIC='feu4'
		if pubid<1 or pubid>4:
			print "ID no recognized"

		print "data : " + data

		if (TOPIC == 'feu1' or TOPIC == 'feu2') and data == "2":
			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu3', payload=0, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu3' + " with value : " + "0"
			res, mid = conn.publish(MQTT_URL_PUB + 'feu4', payload=0, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu4' + " with value : " + "0"

		if (TOPIC == 'feu3' or TOPIC == 'feu4') and data == "2":
			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload=0, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + "0"
			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload=0, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + "0"
			res, mid = conn.publish(MQTT_URL_PUB + 'feu3', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu3' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu4', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu4' + " with value : " + data

		if (TOPIC == 'feu1' or TOPIC == 'feu2') and data == "0":
			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu3', payload=0, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu3' + " with value : " + "2"
			res, mid = conn.publish(MQTT_URL_PUB + 'feu4', payload=0, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu4' + " with value : " + "2"

		if (TOPIC == 'feu3' or TOPIC == 'feu4') and data == "0":
			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload=0, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + "2"
			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload=0, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + "2"
			res, mid = conn.publish(MQTT_URL_PUB + 'feu3', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu3' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu4', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu4' + " with value : " + data

		if data == "1":
			res, mid = conn.publish(MQTT_URL_PUB + 'feu1', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu1' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu2', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu2' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu3', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu3' + " with value : " + data
			res, mid = conn.publish(MQTT_URL_PUB + 'feu4', payload=data, qos=1)
			print "MQTT: Publishing to " + MQTT_URL_PUB + 'feu4' + " with value : " + data

		
		#res, mid = conn.publish(MQTT_URL_PUB + TOPIC, payload=data, qos=1)
		print "MQTT: Publishing to " + MQTT_URL_PUB + TOPIC
		#print "MQTT: Publishing to {0}... " + "{1} ({2})".format(mid, res, TOPIC)
	except Exception as error:
		print error
# -----------------------------------------------------------#
# MQTT related functions
# -----------------------------------------------------------#
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
	print("Connected with result code "+str(rc))
	client.subscribe(MQTT_URL_PUB + MQTT_URL_TOPIC)
	print("usrData: "+str(userdata))
	print("client: " +str(client))
	print("flags: " +str(flags))
#------------------------------------------------------------#
# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
	print("message qos=",msg.qos)
    	#print("TopicT: ", msg.topicT+" "+str(msg.payloadT))
	#print("TopicH: ", msg.topicH+" "+str(msg.payloadH))
	#pass
#------------------------------------------------------------#
def on_publish(mosq, obj, mid):
	#print "Publishing " + payloadT + " to topic: " + topicT + " ..."
	#print "Publishing " + payloadH + " to topic: " + topicH + " ..."
    print("Publish mid: " + str(mid))
    	#pass
#------------------------------------------------------------#
def on_subscribed(mosq, obj, mid, granted_qos):
	print("Subscribed mid: " + str(mid) + ", qos: " + str(granted_qos))
#------------------------------------------------------------#
def on_log(mosq, obj, mid, string):
    print("Log: " + str(string))
	#pass
#------------------------------------------------------------#
def start_client():
	now = datetime.datetime.now()
	print "UDP6-MQTT server side application "  + ID_STRING
	print "Started " + str(now)
	# Datagram (udp) socket
	try:
		s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
		print 'Socket created'
		s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	except Exception :
		print 'Failed to create socket.'
		sys.exit()
	# Bind socket to local host and port
	try:
		#s.bind(("",PORT))
		
		server_address = (HOST, PORT)
		print >>sys.stderr, 'starting up on %s port %s' % server_address
		s.bind(server_address)
		print "socket : " + str(s)
		print "setsockopt : " + str(s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1))
	except socket.error as msg:
		print('Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1])
		sys.exit()
		return
	print 'UDP6-MQTT server ready: %s'% PORT
	print "msg structure size: ", sizeof(SENSOR)
	print ""

	
	
	if ENABLE_MQTT:
		# Initialize MQTT connection
		try:
			client = mqtt.Client()
		except Exception as error:
			print error
			raise

		client.on_connect = on_connect
		client.on_message = on_message
		client.on_publish = on_publish
		client.username_pw_set("A1E-Zdfehlc7EmA9JEer6YIARbtHIMK1y8", "") #old : A1E-dEzBw6HHcOgRUz22KIwvuYkfvmfix

		try:
			client.connect(MQTT_URL, MQTT_PORT, MQTT_KEEPALIVE)
		except Exception as error:
			print error
			raise

	client.on_log = on_log

	#client.on_subscribed = on_subscribed

	# Start the MQTT thread and handle reconnections, also ensures the callbacks
	# being triggered
	client.loop_start()

	#print "Socket : " + str(s.recvfrom(BUFSIZE))
	#client.publish(topicF1,payloadF1,qos=1)#publish
	#print "Publishing " + payloadF1 + " to topic: " + topicF1 #+ " with qos: " + QOS + "..." 
	#client.publish(topicF2,payloadF2,qos=1)#publish
	#print "Publishing " + payloadF2 + " to topic: " + topicF2 #+ " with qos: " + QOS + " ..."
	#client.publish(topicF3,payloadF3,qos=1)#publish
	#print "Publishing " + payloadF3 + " to topic: " + topicF3 #+ " with qos: " + QOS + "..." 
	#client.publish(topicF4,payloadF4,qos=1)#publish
	#print "Publishing " + payloadF4 + " to topic: " + topicF4 #+ " with qos: " + QOS + " ..."
	#time.sleep(4) # wait
	
	while True:

		# receive data from client (data, addr)
		print >>sys.stderr, '\nwaiting to receive message'
		data, addr = s.recvfrom(BUFSIZE)
		now = datetime.datetime.now()
		print str(now)[:19] + " -> " + str(addr[0]) + ":" + str(addr[1]) + " " + str(len(data))
		msg_recv = SENSOR(data)
		if ENABLE_LOG:
			print_recv_data(msg_recv)
		sensordata = jsonify_recv_data(msg_recv)
		if ENABLE_MQTT:
			publish_recv_data(sensordata, msg_recv.id, client, addr[0])

		send_udp_cmd(addr[0])

	#client.loop_stop() #stop the loop
	#s.close()
#------------------------------------------------------------#
# MAIN APP
#------------------------------------------------------------#
if __name__ == "__main__":
 	start_client()

