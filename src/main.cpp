#include <SparkFun_Bio_Sensor_Hub_Library.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include "Adafruit_BME680.h"

#define ARDUINOJSON_USE_LONG_LONG 1
#define BME_SCK 17
#define BME_MISO 23
#define BME_MOSI 14
#define BME_CS 13
#define RES_PIN 4
#define MFIO_PIN 5
#define READINGS_LIMIT 10
#define DELAY_INTERVAL 1000

const char* ssid = "xxx";
const char* password = "xxxxxxxxxx";
const char* awsEndpoint = "xxxxxx.iot.us-east-1.amazonaws.com";
const char* topic = "topic";
const char* subscribeTopic = topic;

Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);
SparkFun_Bio_Sensor_Hub bioHub(RES_PIN, MFIO_PIN); 
bioData body;  

WiFiClientSecure wiFiClient;
PubSubClient pubSubClient(awsEndpoint, 8883, wiFiClient); 
void connectToBroker();

char message[250];
float temperature;
int counter=0;
long initial_timestamp;
StaticJsonDocument<600> jsonDoc;
JsonArray oxygen_array,heart_rate_array,temperature_array;

const char* certificate_pem_crt = \

"Paste certificate here";

// xxxxxxxxxx-private.pem.key
const char* private_pem_key = \

"Paste certificate here";
// This key should be fine as is. It is just the root certificate.
const char* rootCA = \
"Paste certificate here";

void init_WiFiClient(WiFiClientSecure &wiFiClientSecure) {
  Serial.print("Connecting to "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  Serial.print(", WiFi connected, IP address: "); 
  Serial.println(WiFi.localIP());
  wiFiClientSecure.setCACert(rootCA);
  wiFiClientSecure.setCertificate(certificate_pem_crt);
  wiFiClientSecure.setPrivateKey(private_pem_key);

}

void init_BME680(Adafruit_BME680 &bme680) {
  if (!bme680.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
  Serial.println("BME680 Started");
  bme680.setTemperatureOversampling(BME680_OS_8X);
}

void init_MAX30101(SparkFun_Bio_Sensor_Hub &bioSensorHub) {
  Wire.begin();
  int result = bioSensorHub.begin();
  if (!result)
    Serial.println("Sensor started!");
  else
    Serial.println("Could not communicate with the sensor!!!");

  Serial.println("Configuring Sensor...."); 
  int error = bioSensorHub.configBpm(MODE_ONE); 
  if(!error){
    Serial.println("Sensor configured.");
  }
  else {
    Serial.println("Error configuring sensor.");
    Serial.print("Error: "); 
    Serial.println(error); 
  }
  delay(4000); 
}
void readTemperature(Adafruit_BME680 &bme680, float &temperature) {
  if (! bme680.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  temperature = bme680.temperature;
}

void readHeartRate(SparkFun_Bio_Sensor_Hub &bioSensorHub, bioData &data) {
  do {
    //delay(100);
    data = bioSensorHub.readBpm();
  }while(data.status!=3||data.confidence<50||data.oxygen==0);
}

void logData(bioData body, float temperature) {
  Serial.print("Heartrate: ");
  Serial.println(body.heartRate); 
  Serial.print("Confidence: ");
  Serial.println(body.confidence); 
  Serial.print("Oxygen: ");
  Serial.println(body.oxygen); 
  Serial.print("Status: ");
  Serial.println(body.status);
  Serial.print("Temperature: ");
  Serial.println(temperature);
}

void resetJson() {
  jsonDoc.clear();
  jsonDoc.garbageCollect();
  jsonDoc["delay_interval"] = DELAY_INTERVAL;
  oxygen_array = jsonDoc.createNestedArray("oxygen_level_readings");
  heart_rate_array = jsonDoc.createNestedArray("heart_rate_readings");
  temperature_array = jsonDoc.createNestedArray("temperature_readings");
}
void addReading(bioData bodyReading,float temperatureReading) {
  oxygen_array.add(bodyReading.oxygen);
  heart_rate_array.add(bodyReading.heartRate);
  temperature_array.add(temperatureReading);
}

void publishJson(){
  serializeJson(jsonDoc,message);
  //serializeJson(jsonDoc,Serial);
  boolean rc = pubSubClient.publish(topic, message);
  Serial.println("Published ="); 
  Serial.println( (rc ? "OK: " : "FAILED: ") );
}

void setup(){

  Serial.begin(115200);
  Serial.println();
  Serial.printf("SDK version: %s\n", ESP.getSdkVersion());
  init_BME680(bme);
  init_MAX30101(bioHub);
  init_WiFiClient(wiFiClient);
  resetJson();
  connectToBroker();
}




void loop(){

    if(counter == 0) {
      //initial_timestamp = millis();
      //jsonDoc["initial_timestamp"] = initial_timestamp;
    }
    readTemperature(bme,temperature);
    readHeartRate(bioHub,body);
    logData(body,temperature);
    addReading(body,temperature);
    counter++;
    if(counter == READINGS_LIMIT) {
      counter = 0;
      publishJson();
      resetJson();
    }
    delay(DELAY_INTERVAL);
}

void connectToBroker() {
  if (!pubSubClient.connected()) {
    Serial.print("PubSubClient connecting to: "); 
    Serial.print(awsEndpoint);
    while (!pubSubClient.connected()) {
      Serial.print(".");
      pubSubClient.connect("client1");
      delay(1000);
    }
    Serial.println(" connected");
  }
  pubSubClient.loop();
}