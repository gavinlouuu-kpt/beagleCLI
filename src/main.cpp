#include <Arduino.h>
#include <Wire.h>
// #include <FS.h>
#include <pinConfig.h>
#include <Init.h>
#include <FirebaseJson.h>
#include <WiFi.h>
#include <Update.h>
#include <time.h>
#include <map>
#include <vector>
#include <beagleCLI.h>
#include <Network.h>

// #include <SPI.h>
#include <Firebase_ESP_Client.h>

void setup()
{
  Serial.begin(115200);

  cmdSetup();

  // networkCheck();
}

void loop()
{
  beagleCLI();
}
