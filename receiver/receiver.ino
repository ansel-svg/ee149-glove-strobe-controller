/*
	Will be used to setup the Arduino sketch for the receiver itself.
	This is the wireless connection setup for the entire device.

	Input: none

	Output: "mac_address": string, printed to Serial (115200 baud)

	Interfaces with: WiFi.h
*/
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  WiFi.mode(WIFI_STA);
}

void loop() {
  Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
  delay(1000);
}