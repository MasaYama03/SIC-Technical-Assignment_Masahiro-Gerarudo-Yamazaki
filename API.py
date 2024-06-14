from flask import Flask, jsonify, render_template_string
import paho.mqtt.client as mqtt
import threading

app = Flask(__name__)

# MQTT settings
MQTT_BROKER = "test.mosquitto.org"
MQTT_PORT = 1883
MQTT_KEEPALIVE_INTERVAL = 60
MQTT_TOPIC_TEMPERATURE = "/sensor/data/temperature"
MQTT_TOPIC_HUMIDITY = "/sensor/data/humidity"

# Global variables to store temperature and humidity data
temperature_data = None
humidity_data = None

# Lock for thread-safe updates of global variables
data_lock = threading.Lock()

# Callback when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe(MQTT_TOPIC_TEMPERATURE)
    client.subscribe(MQTT_TOPIC_HUMIDITY)

# Callback when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global temperature_data, humidity_data
    with data_lock:
        if msg.topic == MQTT_TOPIC_TEMPERATURE:
            temperature_data = float(msg.payload.decode())
            print(f"Received Temperature: {temperature_data}")
        elif msg.topic == MQTT_TOPIC_HUMIDITY:
            humidity_data = float(msg.payload.decode())
            print(f"Received Humidity: {humidity_data}")

# Set up the MQTT client
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE_INTERVAL)
mqtt_client.loop_start()

@app.route('/api/dht', methods=['GET'])
def get_dht_data():
    with data_lock:
        # Return temperature and humidity data as JSON
        if temperature_data is None or humidity_data is None:
            return jsonify({"error": "No data available"}), 500
        
        return jsonify({
            "temperature": temperature_data,
            "humidity": humidity_data
        })

@app.route('/', methods=['GET'])
def home():
    with data_lock:
        temperature = temperature_data if temperature_data is not None else "N/A"
        humidity = humidity_data if humidity_data is not None else "N/A"

    html_content = """
    <!DOCTYPE html>
    <html>
    <head>
        <meta http-equiv='refresh' content='4'/>
        <meta name='viewport' content='width=device-width, initial-scale=1'>
        <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>
        <title>ESP32 DHT Server</title>
        <style>
            html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}
            h2 { font-size: 3.0rem; }
            p { font-size: 3.0rem; }
            .units { font-size: 1.2rem; }
            .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}
            .indicator { font-size: 5.0rem; }
        </style>
    </head>
    <body>
        <h2>ESP32 DHT Server</h2>
        <p>
            <i class='fas fa-thermometer-half' style='color:#ca3517;'></i>
            <span class='dht-labels'>Temperature</span>
            <span class='indicator'>{{ temperature }}</span>
            <sup class='units'>&deg;C</sup>
        </p>
        <p>
            <i class='fas fa-tint' style='color:#00add6;'></i>
            <span class='dht-labels'>Humidity</span>
            <span class='indicator'>{{ humidity }}</span>
            <sup class='units'>&percnt;</sup>
        </p>
    </body>
    </html>
    """
    return render_template_string(html_content, temperature=temperature, humidity=humidity)

if __name__ == '__main__':
    # Run the Flask app
    app.run(debug=True)
