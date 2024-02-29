#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <Wire.h>  // Include the Wire library for I2C
#include <Adafruit_MS8607.h>
#include <Adafruit_Sensor.h>

#include "DS3231.h"
#include "SM9333.h"

DS3231 RTC;
SM9333 sensor1;
SM9333 sensor2;
SM9333 sensor3;
SM9333 sensor4;
SM9333 sensor5;
#define SDA_PIN 13
#define SCL_PIN 16

Adafruit_MS8607 ms8607;


// WiFi
const char *ssid = "iPhone Luka";   // Enter your Wi-Fi name
const char *password = "lukaluka";  // Enter Wi-Fi password

// MQTT Broker
const char *mqtt_broker = "broker.hivemq.com";
const char *topic = "emqx/ESP32-POE_I2C";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long previousMillis = 0;
const long interval = 2000;  // 2 seconds interval

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);
  while (!Serial) delay(10);  // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MS8607 test!");

  // Try to initialize!
  if (!ms8607.begin()) {
    Serial.println("Failed to find MS8607 chip");
    while (1) { delay(10); }
  }
  Serial.println("MS8607 Found!");

  ms8607.setHumidityResolution(MS8607_HUMIDITY_RESOLUTION_OSR_8b);
  Serial.print("Humidity resolution set to ");
  switch (ms8607.getHumidityResolution()) {
    case MS8607_HUMIDITY_RESOLUTION_OSR_12b: Serial.println("12-bit"); break;
    case MS8607_HUMIDITY_RESOLUTION_OSR_11b: Serial.println("11-bit"); break;
    case MS8607_HUMIDITY_RESOLUTION_OSR_10b: Serial.println("10-bit"); break;
    case MS8607_HUMIDITY_RESOLUTION_OSR_8b: Serial.println("8-bit"); break;
  }
  // ms8607.setPressureResolution(MS8607_PRESSURE_RESOLUTION_OSR_4096);
  Serial.print("Pressure and Temperature resolution set to ");
  switch (ms8607.getPressureResolution()) {
    case MS8607_PRESSURE_RESOLUTION_OSR_256: Serial.println("256"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_512: Serial.println("512"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_1024: Serial.println("1024"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_2048: Serial.println("2048"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_4096: Serial.println("4096"); break;
    case MS8607_PRESSURE_RESOLUTION_OSR_8192: Serial.println("8192"); break;
  }
  Serial.println("");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the Wi-Fi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public EMQX MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  // // Publish and subscribe
  // client.publish(topic, "Hi, I'm ESP32 ^^");
  // client.subscribe(topic);
}

void callback(char *topic_, byte *payload, unsigned int length) {
  char OutputMessage[32], ShowDate = 0, ShowTime = 0;
  payload[length] = 0;
  RTC.UpdateData();
  ShowTime = strstr((char *)payload, "time") != NULL;
  ShowDate = strstr((char *)payload, "date") != NULL;
  if (ShowTime) {
    sprintf(OutputMessage, "Time: %02d:%02d:%02d", RTC.Hours, RTC.Minutes, RTC.Seconds);
    client.publish((const char *)topic, (const char *)OutputMessage, strlen(OutputMessage));
  }

  if (ShowDate) {
    sprintf(OutputMessage, "Date: %04d/%02d/%02d (%s)", RTC.Year + 2000, RTC.Month, RTC.Date, DaysOfTheWeek[RTC.Day]);
    client.publish((const char *)topic, (const char *)OutputMessage, strlen(OutputMessage));
  }
}

// void loop() {
//   // float temp = I2C_ReadDataFromSlave(0x6C, 0x2E);
//   // // float pressure = I2C_ReadDataFromSlavePressure(0x6C, 0x30);
//   // float hu = I2C_ReadDataFromSlaveHumidity(0x40, 0xF5);

//   // // double p1 = RTC.GetTemperature();
//   // //   double p1 = sensor1.readTemperature();
//   // //  Serial.println("Sensor 1: " + (String)p1 + " Pa");
//   // // Print the result to the console
//   // Serial.print("I2C temp : ");
//   // Serial.println(temp);

//   // // Serial.print("I2C pressure: ");
//   // // Serial.println(pressure);


//   // Serial.print("I2C humidi: ");
//   // Serial.println(hu);
//   // sensors_event_t temp, pressure, humidity;
//   // ms8607.getEvent(&pressure, &temp, &humidity);
//   // Serial.print("Temperature: ");Serial.print(temp.temperature); Serial.println(" degrees C");
//   // Serial.print("Pressure: ");Serial.print(pressure.pressure); Serial.println(" hPa");
//   // Serial.print("Humidity: ");Serial.print(humidity.relative_humidity); Serial.println(" %rH");
//   // Serial.println("");
//   sensors_event_t temp, pressure, humidity;
//   ms8607.getEvent(&pressure, &temp, &humidity);
//   Serial.print("Temperature: ");
//   Serial.print(temp.temperature);
//   Serial.println(" degrees C");
//   Serial.print("Pressure: ");
//   Serial.print(pressure.pressure);
//   Serial.println(" hPa");
//   Serial.print("Humidity: ");
//   Serial.print(humidity.relative_humidity);
//   Serial.println(" %rH");
//   Serial.println("");

//   delay(500);
//   client.loop();
// }
void loop() {
  sensors_event_t temp, pressure, humidity;
  ms8607.getEvent(&pressure, &temp, &humidity);

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degrees C");
  Serial.print("Pressure: ");
  Serial.print(pressure.pressure);
  Serial.println(" hPa");
  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println(" %rH");
  Serial.println("");

  // Convert sensor data to strings
  String temperatureStr = String(temp.temperature, 2); // 2 decimal places
  String pressureStr = String(pressure.pressure, 2);   // 2 decimal places
  String humidityStr = String(humidity.relative_humidity, 2); // 2 decimal places

  // Create a JSON payload with the sensor data
  String payload = "{\"temperature\":" + temperatureStr + ",\"pressure\":" + pressureStr + ",\"humidity\":" + humidityStr + "}";

  // Convert payload string to char array
  char charPayload[payload.length() + 1];
  payload.toCharArray(charPayload, payload.length() + 1);

  // Publish sensor data to MQTT topic
  client.publish(topic, charPayload);

  // Delay before next iteration
  delay(500);
  
  // Handle MQTT communication
  client.loop();
}
