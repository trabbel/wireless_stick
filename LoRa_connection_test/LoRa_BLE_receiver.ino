a#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "heltec.h"
#include <string>

using namespace std;

uint32_t chipId = 0;
int nodeId;
int i;
int RSSI;
int SnR;
bool displayRSSI = true; //True displays RSSI, false displays SnR


BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 65;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

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
    int messageSize = message.length();
    if (messageSize == 3){
      LoRa.setSignalBandwidth(std::atoi(message.c_str())*1000);
    }else{
      LoRa.setSpreadingFactor(std::atoi(message.c_str()));
    }
  }
};


void setup() {
  /* -----Determine ID-----
     When working with multiple sticks it is often useful to easily distinguish between them.
     Each stick has a unique chip id, so we assign ascending numbers to the ids. Change the switch-
     statement to match your ids. */
  for(int i=0; i<17; i=i+8) {
	  chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
  switch(chipId){
    case 11779112: nodeId = 0;
      break;
    case 11779672: nodeId = 1;
      break;
    case 11779684: nodeId = 2;
      break;
    case 11779836: nodeId = 3;
      break;
    default: nodeId = -1;
      break;
  }

  // Start heltec display, LoRa and Serial
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, 868E6 /*long frequency*/);

  // -----Init display-----
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);

 // Create the BLE Device
  BLEDevice::init("LoRa_receiver");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE 
  );

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


  Serial.printf("Setup completed! My ID is %d\n", nodeId);
  Heltec.display->drawStringMaxWidth(0, 0, 64, "Waiting for signal...");
  Heltec.display->display();
}


void loop() {
  // -----LoRa receive messages-----
  int packetSize = LoRa.parsePacket();
  yield();
  // Check for transmission
  if (packetSize) {
    // Activate LED to signal packet reception
    digitalWrite(25, HIGH); 
    char answer[packetSize+1];
    i=0;
    // Read transmission
    while (LoRa.available()) {
      answer[i] = (char)LoRa.read();
      i++;
    }
    answer[i] = '\0';
    // Deactivate LED to signal end of packet reception
    digitalWrite(25, LOW);

    char * output;
    if (displayRSSI==true) {
      RSSI = LoRa.packetRssi();
      asprintf(&output, "%s RSSI: %d", answer, RSSI);
    }else{
      SnR = LoRa.packetSnr();
      asprintf(&output, "%s SnR: %d", answer, SnR);
    }

    // Display message and RSSI or SnR
    Heltec.display->clear();
    Heltec.display->drawStringMaxWidth(0, 0, 64, output);
    Heltec.display->display();
    delete(output);
  }
    // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                  
    pServer->startAdvertising();
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}