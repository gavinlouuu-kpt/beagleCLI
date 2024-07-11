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
#include <Hardware.h>
#include <beagleCLI.h>
#include <Network.h>
// #include <SPI.h>
#include <Firebase_ESP_Client.h>
#include <M5Stack.h>
#include <exp_setup.h>
#include <zsrelay.h>

// #include <SensorData.h>
// #include <SensorDataFactory.h>
// #include <SensorDataFactory.cpp>

void setup()
{

  Serial.begin(115200);
  Serial2.begin(38400, SERIAL_8N1, U2_RX, U2_TX);
  M5.begin(1, 1, 1, 1);
  Wire.setClock(400000); // Increase I2C clock speed to 400 kHz
  //  the setups are not not needed now
  pinSetup(); // something in pin setup is causing sd card to not initialize properly
  // delay(100);

  pwmSetup(); // something in pwm setup is causing sd card to not initialize properly
  // delay(100);
  // ledcWrite(PWM_H_CH, 150);
  expMutexSetup();
  relay_off();

  cmdSetup();
  if (!SD.exists("/wifiCredentials.json"))
  {
    Serial.println("wifiCredentials.json does not exist");
    return;
  }

  networkCheck();

  // sampleTask();
  // M5_example();
  // const char *filename = "/expSetup.json";
  // String configData;
  // M5_SD_JSON(filename, configData);

  // sensorCheck();
}

void loop()
{
  beagleCLI();
}
