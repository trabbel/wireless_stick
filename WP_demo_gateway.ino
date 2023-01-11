#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "heltec.h"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 65;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


uint32_t chipId = 0;
int nodeId;
int counter_0 = 0;
int counter_1 = 0;
int cycles = 0;
int report = 100;


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic){
    std::string value = pCharacteristic->getValue();
    if(value == "0"){
      counter_0++;
    }else if(value == "1"){
      counter_1++;
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

  // -----Init Display-----
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Init");
  Heltec.display->display();



  //Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  //|
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
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
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
  char * output;
  asprintf(&output, "Ctr 0: %d Ctr 1: %d", counter_0, counter_1);
  Heltec.display->clear();
  Heltec.display->drawStringMaxWidth(0, 0, 50, output);
  Heltec.display->display();
  cycles++;
  delete(output);
  if(cycles == report){
    digitalWrite(25, HIGH); 
  char * message;
  asprintf(&message, "C0:%d, C1:%d", counter_0, counter_1);

  // -----LoRa send message-----
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
  delete(message);
  Serial.println("Sent");
  cycles = 0;
  digitalWrite(25, LOW); 
  }
        delay(1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}