import random
from paho.mqtt import client as mqtt_client
import requests
import json
import time  

broker = 'broker.hivemq.com'
port = 1883
topics = ["emqx/ESP32-POE_I2C"]
client_id = f'subscribe-{random.randint(0, 100)}'
access_token_temperatura = "OcRpfixBR05BJNll9Tc1"
access_token_vlaga = "iJ5PHDhQI9AyMYWHvcTR"
access_token_pressure = "scDMq5ZG3LXG6aXSrSvp"

temperature_api = f"https://thingsboard.cloud/api/v1/{access_token_temperatura}/telemetry" 
vlaga_endpoint = f"https://thingsboard.cloud/api/v1/{access_token_vlaga}/telemetry" 
pressure_endpoint = f"https://thingsboard.cloud/api/v1/{access_token_pressure}/telemetry"  

def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected!")
        else:
            print("Connection failed with code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

        # Extract temperature from the received JSON payload
        try:
            payload_json = json.loads(msg.payload.decode())
            temperature = payload_json.get("temperature")
            vlaga = payload_json.get("humidity")
            pressure = payload_json.get("pressure")
          
            if vlaga is not None:
                send_to_thingsboard(vlaga_endpoint,"temperature",vlaga)
            else:
                print("Vlaga key not found in the payload")

            if pressure is not None:
                send_to_thingsboard(pressure_endpoint,"temperatura",pressure)
            else:
                print("Pressure key not found in the payload")
            
            if temperature is not None:
                send_to_thingsboard(temperature_api,"temperature",temperature)
            else:
                print("Temperature key not found in the payload")
        except json.JSONDecodeError as e:
            print(f"Error decoding JSON payload: {e}")

    for topic in topics:
        client.subscribe(topic)
    client.on_message = on_message

def send_to_thingsboard(url,name,value):
    headers = {"Content-Type": "application/json"}
    payload = {f"{name}": value}

    response = requests.post(url, headers=headers, json=payload)

    if response.status_code == 200:
        print(f"{value} data sent to ThingsBoard successfully!")
    else:
        print(f"Failed to send temperature data. Response code: {response.status_code}")

def run():
    client = connect_mqtt()
    subscribe(client)
    
    while True:
        client.loop_start()
        time.sleep(2)  # Delay for 2 seconds before the next iteration

if __name__ == '__main__':
    run()
