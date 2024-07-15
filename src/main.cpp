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
// #include <Firebase_ESP_Client.h>
#include <M5Stack.h>
#include <exp_setup.h>
#include <zsrelay.h>

void SD_check()
{
  if (!SD.begin())
  {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("SD Card Error");
    return;
  }
  else
  {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("SD Card OK");
    if (!SD.exists("/wifiCredentials.json"))
    {
      Serial.println("wifiCredentials.json does not exist");
      // return;
    }
    networkCheck();
    return;
  }
}

void ADS_check()
{
  if (!ads.begin(ADSi2c))
  {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("ADS1115 Error");
    return;
  }
  else
  {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("ADS1115 OK");
    return;
  }
}

void setup()
{

  Serial.begin(115200);
  Serial2.begin(38400, SERIAL_8N1, U2_RX, U2_TX);
  M5.begin(1, 1, 1, 1);
  M5.Power.begin();

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(
      WHITE);
  M5.Lcd.setTextFont(4);

  // Set the font color to white.  设置字体颜色为白色
  Wire.setClock(400000); // Increase I2C clock speed to 400 kHz
  pinSetup();
  pwmSetup();
  relay_off();
  cmdSetup();

  SD_check();
  ADS_check();
}

void loop()
{
  beagleCLI();
}
