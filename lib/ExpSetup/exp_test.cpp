#include <exp_setup.h>
#include <zsrelay.h>
#include <pinConfig.h>
#include <Arduino.h>
#include <beagleCLI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Adafruit_ADS1X15.h>

void sector_1_on()
{
    relayRange(0, 7, 1);
}

void sector_1_off()
{
    relayRange(0, 7, 0);
}

void sector_2_on()
{
    relayRange(8, 15, 1);
}

void sector_2_off()
{
    relayRange(8, 15, 0);
}

void sector_3_on()
{
    relayRange(16, 23, 1);
}

void sector_3_off()
{
    relayRange(16, 23, 0);
}

void sector_4_on()
{
    relayRange(24, 31, 1);
}

void sector_4_off()
{
    relayRange(24, 31, 0);
}

void sector_5_on()
{
    relayRange(32, 39, 1);
}

void sector_5_off()
{
    relayRange(32, 39, 0);
}

void s1pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_1r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s1pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_1r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s2pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_2r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s2pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_2r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s3pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_3r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s3pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_3r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s4pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_4r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s4pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_4r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s5pOn()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_5r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void s5pOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_5r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void test_CMD()
{
    commandMap["sector1On"] = []()
    { sector_1_on(); };
    commandMap["sector1Off"] = []()
    { sector_1_off(); };
    commandMap["sector2On"] = []()
    { sector_2_on(); };
    commandMap["sector2Off"] = []()
    { sector_2_off(); };
    commandMap["sector3On"] = []()
    { sector_3_on(); };
    commandMap["sector3Off"] = []()
    { sector_3_off(); };
    commandMap["sector4On"] = []()
    { sector_4_on(); };
    commandMap["sector4Off"] = []()
    { sector_4_off(); };
    commandMap["sector5On"] = []()
    { sector_5_on(); };
    commandMap["sector5Off"] = []()
    { sector_5_off(); };
    commandMap["s1pOn"] = []()
    { s1pOn(); };
    commandMap["s1pOff"] = []()
    { s1pOff(); };
    commandMap["s2pOn"] = []()
    { s2pOn(); };
    commandMap["s2pOff"] = []()
    { s2pOff(); };
    commandMap["s3pOn"] = []()
    { s3pOn(); };
    commandMap["s3pOff"] = []()
    { s3pOff(); };
    commandMap["s4pOn"] = []()
    { s4pOn(); };
    commandMap["s4pOff"] = []()
    { s4pOff(); };
    commandMap["s5pOn"] = []()
    { s5pOn(); };
    commandMap["s5pOff"] = []()
    { s5pOff(); };
}

void ADStest()
{
    if (!ads.begin(ADSi2c))
    {
        Serial.println("Could not find a valid ADS1115, check wiring!");
        while (1)
            ;
    }
    for (int i = 0; i < 10; i++)
    {
        Serial.println(ads.readADC_SingleEnded(0));
        delay(1000);
    }
}

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
    commandMap["adsPWM"] = []()
    { ads_pwm_test(); };
    commandMap["adsPWMrev"] = []()
    { ads_pwm_test_rev(); };
    commandMap["adsPWMcon"] = []()
    { ads_pwm_control(); };
}