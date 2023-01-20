# Wireless Stick

This repository contains code for the [Heltec Wireless Stick](https://heltec.org/project/wireless-stick/). Wireless Sticks can be used for [LoRa/LoRaWAN](https://lora-alliance.org/), [Bluetooth Low Energy](https://www.bluetooth.com/bluetooth-resources/intro-to-bluetooth-low-energy/) or WiFi.

## Setup/Requirements

The code was written in Arduino IDE. To compile and upload it, the following is needed:
- [Arduino IDE](https://www.arduino.cc/en/software)
- [Heltec development environment](https://docs.heltec.org/en/node/esp32/quick_start.html)

## LoRa_example

Contains a simple example of a sender and receiver with display usage.

## LoRa_connection_test

The range of LoRa connections depends on the spreading factor(SF). By default, LoRa uses SF=7, but it can be increased up to 12. To determine the needed spreading factor during field experiments, it can be changed via a smartphone without the need to connect the Wireless Stick to a computer and recompile the code with different settings. Install a app for BLE communication like [nRF Connect](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&gl=US), start the Wireless sticks and connect to *LoRa_receiver* respectively *LoRa_sender*. New SF or bandwidth settings can now be uploaded to the writeable characteristic in text format:
- To change the spreading factor, send a number between 7 and 12 as UTF-8 encoded text
- To change the bandwidth, send either 125 or 250 as UTF-8 encoded text

## WP_demo

Demonstration for WatchPlant general assembly in January 2023. Three sensor nodes collect and share temperature data via BLE and send it in periodic intervals via LoRa to a remote receiver that is further away.
