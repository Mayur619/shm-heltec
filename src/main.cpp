#include <SparkFun_Bio_Sensor_Hub_Library.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include "BLEDevice.h"
#include <EEPROM.h>

#define EEPROM_SIZE 1024
#define ARDUINOJSON_USE_LONG_LONG 1
#define RES_PIN 4
#define MFIO_PIN 5
#define READINGS_LIMIT 2
#define DELAY_INTERVAL 1000




// The remote service we wish to connect to.
BLEUUID serviceUUID("00000001-0000-1000-8000-00805f9b34fb");
BLEUUID characteristic1UUID("00000002-0000-1000-8000-00805f9b34fb");
BLEUUID characteristic2UUID("00000003-0000-1000-8000-00805f9b34fb");
BLEUUID characteristic3UUID("00000004-0000-1000-8000-00805f9b34fb");

boolean doConnect = false;
boolean connected = false;
boolean doScan = false;
BLERemoteCharacteristic *characteristic1, *characteristic2, *characteristic3;
BLEAdvertisedDevice* myDevice;


const char* ssid = "512";
const char* password = "whatsyourpassword";
const char* awsEndpoint = "a352sqe0zq5ghh-ats.iot.us-east-1.amazonaws.com";
const char* topic = "shm-sensor-data/data";
const char* subscribeTopic = topic;

SparkFun_Bio_Sensor_Hub bioHub(RES_PIN, MFIO_PIN); 
bioData body;  

WiFiClientSecure wiFiClient;
PubSubClient pubSubClient(awsEndpoint, 8883, wiFiClient); 
void connectToBroker();

char message[512];
int counter=0;
boolean publishFlag = false;
StaticJsonDocument<800> jsonDoc;
JsonArray oxygen_array,heart_rate_array;//temperature_array;
JsonArray accel_x, accel_y, accel_z, magneto_x, magneto_y, magneto_z;
float a_x, a_y, a_z, m_x, m_y, m_z;
int hr;
BLEClient *pClient;


const char* certificate_pem_crt = \

"-----BEGIN CERTIFICATE-----\n" \
"MIIDWjCCAkKgAwIBAgIVAOMb38gN1eeVvQLnhZveA4Y1CRSqMA0GCSqGSIb3DQEB\n" \
"CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n" \
"IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMTAyMDEyMDEy\n" \
"MDZaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n" \
"dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDIqTFNMPON8qWChf1Q\n" \
"kXBU18GGrfdwJkslGgqrdSEFSkx8C12nhnXgqSuBSmkZBW4w11RNx1bnZGUKRvJv\n" \
"5XiuFZ7EDDO42zy9FNDP8EN/Cb6rtG3sFD8OM9uzD2GMcxBPCB3lrGIqqQXtAg+z\n" \
"hgsP0mEwT11MRz/stQnmxMugJSSWg4F89DGrJZqEPXaCEiSe2J/7btJf9SYIBVhR\n" \
"pqNp74kTLXXAnADXhckOeZVrhHCTBybgRiMkYqAyOXIeMHTG01sH6ESZ21t3XnQz\n" \
"fL7nCY+P4Gf0xI7fKfP2EMMGFeCt6yyMl9uqlY6F1qxRAL8ZSF/fjlcOQ++DuLc1\n" \
"Lmn1AgMBAAGjYDBeMB8GA1UdIwQYMBaAFArZw+RsO2wEl0lqfZ/siCLYz/d/MB0G\n" \
"A1UdDgQWBBThEGJ28rbYgY/QTj9i5bTzd52UfTAMBgNVHRMBAf8EAjAAMA4GA1Ud\n" \
"DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEArVkodCYLujuynJLUOEFszDl6\n" \
"NeULR8ApF6xYjC1vQW/8oO5tz2tgzx9d84WgOcgw5xQkc+eBkvKKXokhwh6bi7V7\n" \
"GylJa8w3pp73r65Il99zotZTEwjjYdWeYrZSEF8Dh1Quv1nhSMudg/aUIkjGXURc\n" \
"qOMI+RoUCc3EMENilwxmKvf2gpG0BGv15rPAjIwxtxaq1v64fPiZVoeqIqLhelba\n" \
"5/zBV7G6fz9oKvoXgFYFQjnOIETgSQ057IqUxjcyCVgLag5RDRXvEB9v50M8IWAG\n" \
"/i7lxFTWzxWizuyI3rUn2RUMArBTBcpklimm0MJZa4fbVyGUKib05DMt7Idi0w==\n" \
"-----END CERTIFICATE-----\n";

// xxxxxxxxxx-private.pem.key
const char* private_pem_key = \

"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIIEpAIBAAKCAQEAyKkxTTDzjfKlgoX9UJFwVNfBhq33cCZLJRoKq3UhBUpMfAtd\n" \
"p4Z14KkrgUppGQVuMNdUTcdW52RlCkbyb+V4rhWexAwzuNs8vRTQz/BDfwm+q7Rt\n" \
"7BQ/DjPbsw9hjHMQTwgd5axiKqkF7QIPs4YLD9JhME9dTEc/7LUJ5sTLoCUkloOB\n" \
"fPQxqyWahD12ghIkntif+27SX/UmCAVYUaajae+JEy11wJwA14XJDnmVa4Rwkwcm\n" \
"4EYjJGKgMjlyHjB0xtNbB+hEmdtbd150M3y+5wmPj+Bn9MSO3ynz9hDDBhXgress\n" \
"jJfbqpWOhdasUQC/GUhf345XDkPvg7i3NS5p9QIDAQABAoIBAQCHzxtzJyJwfD3Q\n" \
"7WbZVNY2ViDBSpUh7okFf26hRREoR3Ukr3yfmb3GZDjxtK8VJDvECrkgOz5yNdy8\n" \
"6+/CFAltqWxfO/L1tkyRnLkHQ5IrNSk7hU6wbbOPpUGZG1vmdyoek1vIyrdWMDe2\n" \
"haKi2qI5/yWQUObQbZiyWIVvDtrSc54C9fVaMIxzofCoFTAahfLmQvqv84IvMHEz\n" \
"wPbfOc2kB4iY65JtH1CVr//Sy9czt9vDFHeaLfA+q5+WkwUSz711aZi4Adx3jToo\n" \
"gMC8Rz3tz1+Va3rtcw+0HV9QD15j576asutUjiZLzDXVzfmbPyPIPUEkdWfZzAKN\n" \
"UqGg66GBAoGBAO1lqiuEARoERYBqVzASdW5Vo79fch0mvpHShKvuey0dxkqAQRai\n" \
"9cYBl7580CxD8oMuQHlNOcCQVVp2bPsNghCrgi1ouaklWgvc8U12JpR0z53s/v7A\n" \
"BK4xsUVAdEzQ+0j6G/Nk0ds692rbKh0/GSGMhB9B1At9URcGDCNMrOwNAoGBANhi\n" \
"k4Wqyu5B+d08Jf3DWymw/Xc0gNbp1B16FBXGCv+OUZJrYEoyKaXy2mjV+ZmLlXB7\n" \
"VGkEWGShJHAJlVCX6flrgAHdXFZ3VbznuXF2fGgOdufjsCqpLvvO8JgZmp+ycr5J\n" \
"uISgV7/4BKGoe2OjY1at+zZJbR354AFqIvfxM7OJAoGBANN/VpsMMLsQASeGFgUy\n" \
"/MH+tDLkAtNr2C4sIpzWi5DHTQjEuLhFGLd+ZcWEhyZYpq8uPqyTG5euPwyoIzGL\n" \
"eEwHsKNqALZsG4wkHCrJz1dFtk5ZfVhSlBjpFqi+4p4pNSZwBQTT1o79Or5NYXjK\n" \
"5UXZXUBHsnVGAT+E2L1+KzTlAoGACXZ2IRhQ+45jEOu6dZh/ehlFXFstbziWkBtw\n" \
"mWspu/dLVe3gLh53d8xhBMimu7JA/MRH8re/7damM0gYAYhDMSckV/CIQzfAxhwU\n" \
"HgQieKJ5TLbGU3dGmLh6CQzFmEpDflLXAOXKMJj2CuPFUZwpkfbVz4/yd6cfxrAP\n" \
"i2w+6JECgYB/y0J1dY0QFihk9OCiqubh6XeNLYwcakv/DLlIPppx4CkfGL0ueyNz\n" \
"Xr8B4qrEV1nMUikVW0U+zkRv10UOn2qbVS2BQEo3hvgaet9LCsqOrRZgMcAZZiXZ\n" \
"mSmTHEznOSlEa7kBUR6TAu48u5JaDqTfnVuEg+ipLRd74oa8CJK7QQ==\n" \
"-----END RSA PRIVATE KEY-----\n";

// This key should be fine as is. It is just the root certificate.
const char* rootCA = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n";


void printHeapStatus()
{
	Serial.print("HEAP: ");
  Serial.print( ESP.getFreeHeap() ); 
  Serial.println();
}


void resetJson() {
  jsonDoc.clear();
  jsonDoc.garbageCollect();
  jsonDoc["delay_interval"] = DELAY_INTERVAL;
  oxygen_array = jsonDoc.createNestedArray("oxygen_level_readings");
  heart_rate_array = jsonDoc.createNestedArray("heart_rate_readings");
  //temperature_array = jsonDoc.createNestedArray("temperature_readings");
  accel_x = jsonDoc.createNestedArray("accel_x");
  accel_y = jsonDoc.createNestedArray("accel_y");
  accel_z = jsonDoc.createNestedArray("accel_z");
  magneto_x = jsonDoc.createNestedArray("magneto_x");
  magneto_y = jsonDoc.createNestedArray("magneto_y");
  magneto_z = jsonDoc.createNestedArray("magneto_z");
}

void publishJson(){
  serializeJson(jsonDoc,message);
  //serializeJson(jsonDoc,Serial);
  boolean rc = pubSubClient.publish(topic, message);
  Serial.println("Published ="); 
  Serial.println( (rc ? "OK: " : "FAILED: ") );
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    char charId = pBLERemoteCharacteristic->getUUID().toString().at(7);
    switch(charId) {
      case '2':{
          Serial.println("2");
          sscanf((char*)pData,"%d,%f",&hr,&a_x);
          heart_rate_array.add(hr);
          accel_x.add(a_x);
      }
      break;
      case '3':{
          sscanf((char*)pData,"%f,%f",&a_y,&a_z);
          accel_y.add(a_y);
          accel_z.add(a_z);
      }
      break;
      case '4':{
          /*magneto_z.add(pData);
          Serial.println("Oxygen:"+body.oxygen);
          oxygen_array.add(body.oxygen);
          counter++;
          if(counter == READINGS_LIMIT) {
            publishFlag = true;
            counter = 0;
          }*/
          sscanf((char*)pData,"%f,%f,%f",&m_x,&m_y,&m_z);
          magneto_x.add(m_x);
          magneto_y.add(m_y);
          magneto_z.add(m_z);
          body = bioHub.readBpm();
          int oxygen_level = body.oxygen;
          /*while(oxygen_level==0) {
            body = bioHub.readBpm();
            oxygen_level = body.oxygen;
            Serial.println(oxygen_level);
          }*/
          Serial.println(oxygen_level);
          oxygen_array.add(oxygen_level);
          counter++;
      }
      break;
      default:{
          Serial.println("Undefined characteristic callback invoked");
      }
    }


    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}

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

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {

    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* service = pClient->getService(serviceUUID);
    if (service == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found heart rate service");

    // Heart rate characteristic
    characteristic1 = service->getCharacteristic(characteristic1UUID);
    if (characteristic1 == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(characteristic1UUID.toString().c_str());
      pClient->disconnect();

      return false;
    }
    Serial.println(" - Found characteristic 1");

    characteristic2 = service->getCharacteristic(characteristic2UUID);
    if (characteristic2 == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(characteristic2UUID.toString().c_str());
      pClient->disconnect();

      return false;
    }
    Serial.println(" - Found characteristic 2");

    characteristic3 = service->getCharacteristic(characteristic3UUID);
    if (characteristic3 == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(characteristic3UUID.toString().c_str());
      pClient->disconnect();

      return false;
    }
    Serial.println(" - Found characteristic 3");
  
    if(characteristic1->canNotify())
      characteristic1->registerForNotify(notifyCallback);
    if(characteristic2->canNotify())
      characteristic2->registerForNotify(notifyCallback);
    if(characteristic3->canNotify())
      characteristic3->registerForNotify(notifyCallback);
    
    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEScan *pBLEScan = BLEDevice::getScan();
      pBLEScan->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
      pBLEScan->clearResults();
    } 
  } 
}; 


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
  //delay(2000); 
  body = bioHub.readBpm();
  while(body.status!=3) {
    Serial.println(body.status);
    body = bioHub.readBpm();
  }
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
}




void init_ble() {
  BLEDevice::init("Bangle");
  Serial.println("Initialized BLE");

  BLEScan* pBLEScan = BLEDevice::getScan();
  Serial.println("Scanning");
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void setup(){

  Serial.begin(115200);
  Serial.println();
  Serial.printf("SDK version: %s\n", ESP.getSdkVersion());
  //init_BME680(bme);
  init_MAX30101(bioHub);
  //init_WiFiClient(wiFiClient);
  resetJson();
  //printHeapStatus();
  //connectToBroker();
  init_ble();
}

void loop(){
    if(counter==5) {
      pClient->disconnect();
      BLEDevice::deinit(true);
      //printHeapStatus();
      //delay(1000);
      init_WiFiClient(wiFiClient);
      connectToBroker();
      publishJson();
      Serial.println("Disconnecting pubsub");
      pubSubClient.disconnect();
      Serial.println("Resetting JSON");
      resetJson();
      Serial.println("Disconnecting wifi");
      WiFi.disconnect(true);
      ESP.restart();
      publishFlag = false;
      delay(1000);
      Serial.println("Initializing BT");
      init_ble();
      doConnect = true;
    }
    if (doConnect == true) {
      if (connectToServer()) {
        Serial.println("We are now connected to the BLE Server.");
        } else {
        Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      }
      doConnect = false;
    }
    if (connected) {
      
    }else if(doScan){
      BLEDevice::getScan()->start(0);  
    }
    
    //delay(1000);
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