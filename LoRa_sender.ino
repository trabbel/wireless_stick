/* Example file to send and display data with LoRa and the Heltec Wireless Stick
   For more ressources about LoRa, see:
   https://github.com/HelTecAutomation/Heltec_ESP32/blob/master/src/lora/API.md
   For more ressources about the OLED display, see:
   https://github.com/HelTecAutomation/Heltec_ESP32/blob/master/src/oled/API.md
*/


// Library for LoRa and the display
#include "heltec.h"


// Declare variables
int msgCnt = 0;
uint32_t chipId = 0;
int nodeId;


// Code in the setup method is automatically called (once) after booting.
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
  Heltec.display->drawString(0, 0, "Sending ping:");
  Heltec.display->display();

  // -----Init LoRa-----
  // -----LoRa options-----

  /* Transmission power of the radio, 2-17dB. */
  //LoRa.setTxPower(14, RF_PACONFIG_PASELECT_PABOOST);

  /* Change the frequency in Hz, wireless stick supports 868E6 - 915E6, so the general 
     region is Europe and North America. Croatia is restricted to EU863-870. */
  //LoRa.setFrequency(868E6);

  /* Spreading factor between 6 and 12, represents number of bits per symbol.
     High SF leads to more noise resistance, but at the cost of a lower data rate. */
  //LoRa.setSpreadingFactor(7);

  /* Bandwidth of the chirp signal. Higher bandwidth has higher data rates, 
     but less communication range. */
  //LoRa.setSignalBandwidth(125E3);

  /* Coding rate between 5 and 8, corresponds to the ratio of data to parity bits.
     CR 5 means 4 out of 5 bits are data bits, CR 8 means 4 out of 8 bits are data bits.
     Higher CR imrpoves the transmission quality, but comes with an overhead. */
  //LoRa.setCodingRate4(5);

  /* Change the preamble length between 6 and 65535. A higher preamble increases the 
     needed time to send data, but can improve the transmission. */
  //LoRa.setPreambleLength(8);

  /* The sync word marks the end of the header and the start of data. */
  //LoRa.setSyncWord(0x34);

  /* LoRa can use Cyclic Redundancy Check, default is disabled. */
  //LoRa.enableCrc();
  //LoRa.disableCrc();

  /* LoRa can use inverted IQ to distinguish between sender and receiver messages,
     default is disabled. */
  //LoRa.disableInvertIQ();
  //LoRa.enableInvertIQ();
  //LoRa.enableTxInvertIQ();
  //LoRa.enableRxInvertIQ();

  Serial.printf("Setup completed! My ID is %d\n", nodeId);
}


// Code in this method will loop after setup
void loop() {
  // Display node id and packet number
  char * output;
  asprintf(&output, "ID: %d Packet: %d", nodeId, msgCnt);
  Heltec.display->clear();
  Heltec.display->drawStringMaxWidth(0, 0, 64, output);
  Heltec.display->display();

  // Format message string
  char * message;
  asprintf(&message, "S:%d, C:%d", nodeId, msgCnt);

  // -----LoRa send message-----
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
  
  delete(output);
  delete(message);
  msgCnt++;
  delay(1000);
}