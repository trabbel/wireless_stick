#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "heltec.h"
#include <string>

using namespace std;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 65;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


uint32_t chipId = 0;
int nodeId;
const int sensor = 39;
int sensorValue = 0;
float temperature;
float averageTemp;
float correction;

int counter_0 = 0;
int counter_1 = 0;
int cycles = 0;
const int report = 10;
int messageSize;

float data_0[report] = {};
float data_1[report] = {};
float data_self[report] = {};


class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    string message = pCharacteristic->getValue();
    messageSize = message.length();
    float temp = std::atof(message.substr(2, messageSize - 2).c_str());
    if (message[0] == '0') {
      data_0[counter_0] = temp;
      counter_0++;
      if (counter_0 == report){
        counter_0 = 0;
      }
    } else if (message[0] == '1') {
      data_1[counter_1] = temp;
      counter_1++;
      if (counter_1 == report){
        counter_1 = 0;
      }
    }
    //Serial.print("Counter 0: ");
    //Serial.print(counter_0);
    //Serial.print(", Counter 1: ");
    //Serial.println(counter_1);
  }
};



void setup() {
  /* -----Determine ID-----
     When working with multiple sticks it is often useful to easily distinguish between them.
     Each stick has a unique chip id, so we assign ascending numbers to the ids. Change the switch-
     statement to match your ids. */
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  switch (chipId) {
    case 11779112:
      nodeId = 0; correction = 0;
      break;
    case 11779672:
      nodeId = 1; correction = 0;
      break;
    case 11779684:
      nodeId = 2; correction = 0;
      break;
    case 11779836:
      nodeId = 3; correction = 0;
      break;
    default:
      nodeId = -1;
      break;
  }

  // Start heltec display, LoRa and Serial
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, 868E6 /*long frequency*/);

  // -----Init Display-----
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Init");
  Heltec.display->display();

  pinMode(sensor, INPUT);
  analogReadResolution(12);

  //Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE  //|
    //BLECharacteristic::PROPERTY_NOTIFY |
    //BLECharacteristic::PROPERTY_INDICATE |
    //BLECharacteristic::PROPERTY_BROADCAST
  );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->setCallbacks(new MyCallback());

  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection to notify...");
  pCharacteristic->setValue((uint8_t*)&value, 4);
  pCharacteristic->notify();
}

void loop() {
  // notify changed value
  //if (deviceConnected) {
  // pCharacteristic->setValue((uint8_t*)&value, 4);
  // pCharacteristic->notify();
  // value++;
  //}

  sensorValue = analogRead(sensor);
  float voltageOut = (sensorValue * 3300) / 4095;

  // calculate temperature for LM35 (LM35DZ)
  temperature = (voltageOut / 10) - 273;
  temperature += correction;

  data_self[cycles] = temperature;
   //Serial.println(temperature);
   
  averageTemp = 0;
  for (int i = 0; i<report;i++){
    averageTemp += (data_0[i] + data_1[i] + data_self[i]);
    //Serial.println(data_self[i]);
  }
  averageTemp /= (3*report);

  char* output;
  asprintf(&output, "Own:%.2fAve:%.2f",temperature,  averageTemp);
  Heltec.display->clear();
  Heltec.display->drawStringMaxWidth(0, 0, 50, output);
  Heltec.display->display();
  cycles++;
  delete (output);
  if (cycles == report) {
    digitalWrite(25, HIGH);
    char* message;
    asprintf(&message, "%.2f", averageTemp);

    // -----LoRa send message-----
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
    delete (message);
    Serial.println("Sending \"" + String(averageTemp) + "\" over LoRa");
    cycles = 0;
    digitalWrite(25, LOW);
  }
  delay(1000);  // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}