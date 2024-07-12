#include <Arduino.h>
#include <Wire.h>
#include <pinConfig.h>
#include <Init.h>
#include <FirebaseJson.h>
#include <WiFi.h>
#include <Update.h>
#include <time.h>
#include <map>
#include <vector>
#include <Hardware.h>
#include <beagleCLI.h>
#include <Network.h>
#include <Firebase_ESP_Client.h>
#include <M5Stack.h>
#include <exp_setup.h>
#include <zsrelay.h>

void setup()
{

  Serial.begin(115200);
  Serial2.begin(38400, SERIAL_8N1, U2_RX, U2_TX);
  M5.begin(1, 1, 1, 1);
  Wire.setClock(400000); // Increase I2C clock speed to 400 kHz

  pinSetup();
  pwmSetup();
  relay_off();
  cmdSetup();
  if (!SD.exists("/wifiCredentials.json"))
  {
    Serial.println("wifiCredentials.json does not exist");
    return;
  }
  networkCheck();
}

void loop()
{
  beagleCLI();
}
