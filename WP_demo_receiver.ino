#include "heltec.h"

uint32_t chipId = 0;
int nodeId;
int i;
int RSSI;
int SnR;
bool displayRSSI = true; //True displays RSSI, false displays SnR


const int voltageSensor = 4;
const int totalLeds = 6;
int ledPins[totalLeds] = {2,32,33,13,12};
float adjustedRange = 40.0;
float temperature;

// Control 6 LED display
void ledRange(int num, int total) {
    for (int i = 0; i < num; i++) {
        digitalWrite(ledPins[i], HIGH);
    }
    for (int i = num; i < total; i++) {
        digitalWrite(ledPins[i], LOW);
    }
}


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

  // Init display
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();

  // Init LED pins
  for (int i = 0; i <= totalLeds; i++)
  {
    pinMode(ledPins[i], OUTPUT);
  }

  Serial.printf("Setup completed! My ID is %d\n", nodeId);
  Heltec.display->drawStringMaxWidth(0, 0, 64, "Waiting for signal...");
  Heltec.display->display();
}


void loop() {
  // LoRa receive messages
  int packetSize = LoRa.parsePacket();
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
      asprintf(&output, "%s deg C RSSI: %d", answer, RSSI);
    }else{
      SnR = LoRa.packetSnr();
      asprintf(&output, "%s SnR: %d", answer, SnR);
    }
    temperature = std::atof(answer);
    int level = map(temperature, 18, adjustedRange, 0, totalLeds);//4095

    // Activate LED display
    ledRange(level, totalLeds); 

    // Display message and RSSI or SnR
    Heltec.display->clear();
    Heltec.display->drawStringMaxWidth(0, 0, 64, output);
    Heltec.display->display();
    delete(output);
  }
}