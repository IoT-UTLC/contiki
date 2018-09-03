### Taken from https://pypi.python.org/pypi/paho-mqtt
### Requires Paho-MQTT package, install by:
### pip install paho-mqtt

import paho.mqtt.client as mqtt
import socket
#------------------------------------------------------------#
MQTT_URL   = "things.ubidots.com"	# Lien pour le broker MQTT
MQTT_TOPIC = "/v1.6/devices/zolertia/#"	# Va permettre la souscription au Broker

topic	   ="/v1.6/devices/zolertia/"	# Nom du TOPIC du Broker MQTT

# Compte le nombre de lettre du TOPIC pour récupérer ce que l'on souhaite
nbtopic    = len(topic)	
nblabel    = len('feu1')
#------------------------------------------------------------#
CLIENT		= "fd00::212:4b00:60d:b288"
#------------------------------------------------------------#
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    print("Subscribed to " + MQTT_TOPIC)
    client.subscribe(MQTT_TOPIC) 	# Permet la souscription

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print msg.topic[nbtopic:nbtopic+nblabel]  # Permet d'afficher uniquement le feu concerné

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("connecting to " + MQTT_URL)
# Va permettre la connexion au Broker MQTT (Login/MdP)
client.username_pw_set("A1E-dEzBw6HHcOgRUz22KIwvuYkfvmfixy", "A1E-dEzBw6HHcOgRUz22KIwvuYkfvmfixy")
client.connect(MQTT_URL, 1883, 60)

client.loop_forever()

