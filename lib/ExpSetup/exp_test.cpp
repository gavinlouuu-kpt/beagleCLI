#include <exp_setup.h>
#include <zsrelay.h>
#include <pinConfig.h>
#include <Arduino.h>
#include <beagleCLI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Adafruit_ADS1X15.h>

Adafruit_BME680 bme; // I2C
Adafruit_ADS1115 ads;

uint8_t ADSi2c = 0x48;

void ads_pwm_test()
{
    ads.begin(ADSi2c);
    // ledcWrite(PWM_H_CH, 150);
    for (int i = 0; i < 256; i = i + 50)
    {
        ledcWrite(PWM_V_CH, i);
        // ledcWrite(PWM_H_CH, i);
        unsigned long time = millis();
        while (millis() - time < 20000)
        {
            Serial.print(">PWM_Heater:");
            Serial.println(i);
            // Serial.print(",");
            Serial.print(">time:");
            Serial.println(millis() - time);
            // Serial.print(",");
            Serial.print(">Channel_0:");
            Serial.println(ads.readADC_SingleEnded(0));
            // Serial.print(",");
            Serial.print(">Channel_1:");
            Serial.println(ads.readADC_SingleEnded(1));
            // Serial.print(",");
            Serial.print(">Channel_2:");
            Serial.println(ads.readADC_SingleEnded(2));
            // Serial.print(",");
            Serial.print(">Channel_3:");
            Serial.println(ads.readADC_SingleEnded(3));
        }
    }
    ledcWrite(PWM_V_CH, 0);
    // ledcWrite(PWM_H_CH, 0);
}

void ads_pwm_control()
{
    ads.begin(ADSi2c);
    // ledcWrite(PWM_H_CH, 150);
    for (int i = 0; i < 256; i = i + 50)
    {
        ledcWrite(PWM_V_CH, 255);
        // ledcWrite(PWM_H_CH, i);
        unsigned long time = millis();
        while (millis() - time < 20000)
        {
            Serial.print(">PWM_Heater:");
            Serial.println(i);
            // Serial.print(",");
            Serial.print(">time:");
            Serial.println(millis() - time);
            // Serial.print(",");
            Serial.print(">Channel_0:");
            Serial.println(ads.readADC_SingleEnded(0));
            // Serial.print(",");
            Serial.print(">Channel_1:");
            Serial.println(ads.readADC_SingleEnded(1));
            // Serial.print(",");
            Serial.print(">Channel_2:");
            Serial.println(ads.readADC_SingleEnded(2));
            // Serial.print(",");
            Serial.print(">Channel_3:");
            Serial.println(ads.readADC_SingleEnded(3));
        }
    }
    ledcWrite(PWM_V_CH, 0);
    // ledcWrite(PWM_H_CH, 0);
}

void ads_pwm_test_rev()
{
    ads.begin(ADSi2c);
    // ledcWrite(PWM_H_CH, 150);
    for (int i = 255; i > 0; i = i - 50)
    {
        ledcWrite(PWM_V_CH, i);
        // ledcWrite(PWM_H_CH, i);
        unsigned long time = millis();
        while (millis() - time < 20000)
        {
            Serial.print(">PWM_Heater:");
            Serial.println(i);
            // Serial.print(",");
            Serial.print(">time:");
            Serial.println(millis() - time);
            // Serial.print(",");
            Serial.print(">Channel_0:");
            Serial.println(ads.readADC_SingleEnded(0));
            // Serial.print(",");
            Serial.print(">Channel_1:");
            Serial.println(ads.readADC_SingleEnded(1));
            // Serial.print(",");
            Serial.print(">Channel_2:");
            Serial.println(ads.readADC_SingleEnded(2));
            // Serial.print(",");
            Serial.print(">Channel_3:");
            Serial.println(ads.readADC_SingleEnded(3));
        }
    }
    ledcWrite(PWM_V_CH, 0);
    // ledcWrite(PWM_H_CH, 0);
}

void exp_test_CMD()
{
    // commandMap["start"] = []()
    // { expTask(); };
    // commandMap["sample"] = []()
    // { BMEsampleTask(); };
    // commandMap["expDAQ"] = []()
    // { expDAQ(); };
    // commandMap["expIDLE"] = []()
    // { expIDLE(); };
    // commandMap["expSAVE"] = []()
    // { expSAVE(); };
    // commandMap["checkState"] = []()
    // { checkState(); };
    commandMap["adsPWM"] = []()
    { ads_pwm_test(); };
    commandMap["adsPWMrev"] = []()
    { ads_pwm_test_rev(); };
    commandMap["adsPWMcon"] = []()
    { ads_pwm_control(); };
    // commandMap["pumpOn"] = []()
    // { onPump(); };
    // commandMap["pumpOff"] = []()
    // { offPump(); };
    // relayCMD();
}