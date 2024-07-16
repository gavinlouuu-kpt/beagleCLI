// #include <chrono>
#include <exp_setup.h>
#include <zsrelay.h>
#include <FirebaseJson.h>
#include <beagleCLI.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <M5Stack.h>
#include <Arduino.h>
#include <pinConfig.h>
#include <EXP_CONFIG_KEY.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#include <Adafruit_ADS1X15.h>

#include <unordered_map>
#include <WiFi.h>

Adafruit_ADS1115 ads;
Adafruit_BME680 bme; // I2C
TaskHandle_t bmeTaskHandle, adsTaskHandle, adsFastTaskHandle, expLoopTaskHandle;

volatile int heatingTime;
std::vector<int> heaterSettings;

uint8_t ADSi2c = 0x48;
volatile int last_setup_tracker = -1;
volatile int setup_tracker = 0;
volatile int repeat_tracker = 0;
volatile int channel_tracker = 0;
volatile unsigned long startTime = 0;

String exp_name = "";
String currentPath = "";

SamplingType samplingType;

int sensorCheck()
{
    if (!bme.begin() && !ads.begin(ADSi2c))
    {
        Serial.println("No sensor found!");
        return 0;
    }
    if (bme.begin())
    {
        Serial.println("BME680 sensor found!");
        // BMEsampleTask();
        return 1;
    }
    else if (ads.begin(ADSi2c))
    {
        // ads.setDataRate(RATE_ADS1115_860SPS);
        // ads.setGain(GAIN_ONE);
        Serial.println("ADS1115 sensor found!");
        // ADSsampleTask();
        return 2;
    }
    Serial.println("Sensor checking failed!");
    return -1;
}

void deleteTask(TaskHandle_t *taskHandle)
{
    if (taskHandle != NULL && *taskHandle != NULL)
    {
        vTaskDelete(*taskHandle);
        *taskHandle = NULL; // Set the handle to NULL after deleting the task
    }
}

void deleteTaskBasedOnType(SamplingType samplingType)
{
    switch (samplingType)
    {
    case SamplingType::ADS_DETAIL:
        if (adsTaskHandle != NULL)
        {
            vTaskDelete(adsTaskHandle);
            adsTaskHandle = NULL; // Clear the handle after deletion
        }
        break;

    case SamplingType::ADS:
        if (adsTaskHandle != NULL)
        {
            vTaskDelete(adsTaskHandle);
            adsTaskHandle = NULL; // Clear the handle after deletion
        }
        break;

    case SamplingType::BME680:
        if (bmeTaskHandle != NULL)
        {
            vTaskDelete(bmeTaskHandle);
            bmeTaskHandle = NULL; // Clear the handle after deletion
        }
        break;

    default:
        Serial.println("Unknown sampling type for deletion!");
        break;
    }
}

TaskHandle_t samplingMethod(SamplingType samplingType)
{
    TaskHandle_t taskHandle = NULL;

    if (samplingType == SamplingType::ADS_DETAIL)
    {
        ads.begin(ADSi2c);
        ads.setDataRate(RATE_ADS1115_860SPS);
        ads.setGain(GAIN_ONE);
        ADSsampleTask(&taskHandle); // Pass the address of the taskHandle
    }
    else if (samplingType == SamplingType::ADS)
    {
        ads.begin(ADSi2c);
        ads.setDataRate(RATE_ADS1115_860SPS);
        ads.setGain(GAIN_ONE);
        ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, true); // Continuous mode
        adsFastSampleTask(&taskHandle);                             // Pass the address of the taskHandle
    }

    return taskHandle; // Will be NULL if neither condition is met
}

// TaskHandle_t samplingMethod(SamplingType samplingType)
// {
//     if (samplingType == SamplingType::ADS_DETAIL)
//     {
//         ads.begin(ADSi2c);
//         ads.setDataRate(RATE_ADS1115_860SPS);
//         ads.setGain(GAIN_ONE);
//         ADSsampleTask();

//         return adsTaskHandle;
//     }
//     else if (samplingType == SamplingType::ADS)
//     {
//         ads.begin(ADSi2c);
//         ads.setDataRate(RATE_ADS1115_860SPS);
//         ads.setGain(GAIN_ONE);
//         adsFastSampleTask();

//         return adsFastTaskHandle;
//     }
//     return NULL;
// }

int serial_delay = 15;

void purgeAll()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_1r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
    pump_on = switchCommand(1, clean_2r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
    pump_on = switchCommand(1, clean_3r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
    pump_on = switchCommand(1, clean_4r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
    pump_on = switchCommand(1, clean_5r, 1);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
}

void purgeOff()
{
    std::vector<uint8_t> pump_on = switchCommand(1, clean_1r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
    pump_on = switchCommand(1, clean_2r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
    pump_on = switchCommand(1, clean_3r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
    pump_on = switchCommand(1, clean_4r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
    pump_on = switchCommand(1, clean_5r, 0);
    Serial2.write(pump_on.data(), pump_on.size());
    delay(serial_delay);
}

void relayRange(int x, int y, int state)
{
    for (int i = x; i <= y; i++)
    {
        std::vector<uint8_t> pump_on = switchCommand(1, i, state);
        Serial2.write(pump_on.data(), pump_on.size());
        delay(100);
    }
}

int purgeChannel(int channel)
{
    if (channel >= 0 && channel < 8)
    {
        return clean_1r;
    }
    else if (channel >= 8 && channel < 16)
    {
        return clean_2r;
    }
    else if (channel >= 16 && channel < 24)
    {
        return clean_3r;
    }
    else if (channel >= 24 && channel < 32)
    {
        return clean_4r;
    }
    else if (channel >= 32 && channel < 40)
    {
        return clean_5r;
    }
    else
    {
        return -1;
    }
}

void exp_loop(FirebaseJson config, int setup_count, SamplingType samplingType)
{
    TaskHandle_t samplingTask = samplingMethod(samplingType);

    // int sensorType = sensorCheck();

    // split function to accomodate fast and full ADS
    // now task type is decided by TaskHandle instead of dynamic sensorType detection

    purgeAll();

    std::vector<uint8_t> pump_on = switchCommand(1, pump_relay, 1);
    Serial2.write(pump_on.data(), pump_on.size());
    ledcWrite(PWM_Vin, 255); // sensor voltage pwm

    for (int i = 1; i <= setup_count; i++)
    {
        setup_tracker = i;
        exp_name = getExpName(config, i, EXP_NAME);
        heaterSettings = getArr(config, i, HEATER_SETTING);
        heatingTime = getInt(config, i, HEATING_TIME);
        int exp_time = getInt(config, i, EXP_TIME);
        int duration = getInt(config, i, EXPOSE_DURATION) * 1000;
        int repeat = getInt(config, i, REPEAT);
        std::vector<int> channels = getArr(config, i, CHANNEL);

        if (duration == 0 || repeat == 0 || channels.size() == 0)
        {
            Serial.println("Invalid setup: " + String(i));
            continue;
        }

        for (int j = 0; j < repeat; j++)
        {
            repeat_tracker = j;
            Serial.println("SETUP: Running experiment for setup: " + String(i) + " repeat: " + String(j));

            for (int cur_channel : channels)
            {
                purgeOff();
                startTime = millis();
                channel_tracker = cur_channel;

                // Notify the sampling task to start data acquisition
                // xTaskNotifyGive(adsTaskHandle);

                if (samplingTask == NULL)
                {
                    Serial.println("SETUP: Sampling task handle is NULL");
                }
                else
                {
                    xTaskNotifyGive(samplingTask);
                    Serial.println("SETUP: Sampling task notified");
                }
                // xTaskNotifyGive(samplingTask);

                Serial.println("SETUP: Running experiment for channel: " + String(cur_channel));
                std::vector<uint8_t> on_command = switchCommand(1, cur_channel, 1);
                Serial2.write(on_command.data(), on_command.size());

                // Non-blocking delay for duration
                uint32_t startDuration = millis();
                while (millis() - startDuration < duration)
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                std::vector<uint8_t> off_command = switchCommand(1, cur_channel, 0);
                std::vector<uint8_t> selected_purge_cmd = switchCommand(1, purgeChannel(cur_channel), 1);
                Serial2.write(selected_purge_cmd.data(), selected_purge_cmd.size());

                // Non-blocking delay for purge time
                uint32_t startPurge = millis();
                while (millis() - startPurge < serial_delay)
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                Serial2.write(off_command.data(), on_command.size());

                // Non-blocking delay for elution time
                uint32_t startElution = millis();
                while (millis() - startElution < exp_time)
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                // Notify the ADS sampling task to save data
                // xTaskNotify(adsTaskHandle, 0, eIncrement);
                xTaskNotify(samplingTask, 0, eIncrement);
                Serial.println("SETUP: Sampling task notified for saving data");

                // Wait for the ADS sampling task to signal completion
                Serial.println("SETUP: Waiting for sampling task to complete");
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            }
        }
    }
    // deleteTaskBasedOnType(samplingType);
    Serial.println("SETUP: Deleting task");
    deleteTask(&samplingTask);
    relay_off();

    Serial.println("SETUP: END of LOOP");
    startTime = 0;
    last_setup_tracker = -1;
}

void exp_build(SamplingType samplingType)
{
    // File name for the JSON configuration
    const char *filename = SETUP_NAME;
    String configData;
    M5_SD_JSON(filename, configData);
    FirebaseJson json;
    json.setJsonData(configData);
    int setupCount = count_setup(configData);
    // Serial.println("Number of setups: " + String(setupCount));
    exp_loop(json, setupCount, samplingType);
}

// void exp_start(void *pvParameters)
// {
//     // adsTaskHandle = all ads channels
//     xTaskCreate(
//         exp_start,           /* Task function. */
//         "exp_start",         /* String with name of task. */
//         10240,               /* Stack size in bytes. */
//         NULL,                /* Parameter passed as input of the task */
//         1,                   /* Priority of the task. */
//         &expLoopTaskHandle); /* Task handle. */
//     expLoopTaskHandle = xTaskGetCurrentTaskHandle();
//     exp_build(SamplingType::ADS_DETAIL);
//     vTaskDelete(NULL); // should i delete by task handle?
// }

// void exp_fast(void *pvParameters)
// {
//     expLoopTaskHandle = xTaskGetCurrentTaskHandle();
//     exp_build(SamplingType::ADS);
//     vTaskDelete(NULL); // should i delete by task handle?
// }

// void expTask()
// {
//     xTaskCreate(
//         exp_start,           /* Task function. */
//         "exp_start",         /* String with name of task. */
//         10240,               /* Stack size in bytes. */
//         NULL,                /* Parameter passed as input of the task */
//         1,                   /* Priority of the task. */
//         &expLoopTaskHandle); /* Task handle. */
// }

void exp_generic(void *pvParameters)
{
    // Cast the passed parameter back to SamplingType
    SamplingType samplingType = *(SamplingType *)pvParameters;

    // Obtain and update the global task handle
    expLoopTaskHandle = xTaskGetCurrentTaskHandle();
    Serial.print("Task handle obtained: ");
    Serial.println((uintptr_t)expLoopTaskHandle, HEX);

    // Build experiment based on the sampling type
    exp_build(samplingType);

    // Delete the current task
    vTaskDelete(NULL);
}

void startExperimentTask(SamplingType samplingType)
{
    // Allocate memory for passing the sampling type to the new task
    SamplingType *taskParam = new SamplingType(samplingType);

    // Create a task and pass the sampling type as a parameter
    xTaskCreate(
        exp_generic,       // Task function
        "Experiment Task", // Name of the task
        10240,             // Stack size in bytes
        (void *)taskParam, // Parameter passed as input to the task
        1,                 // Priority of the task
        &expLoopTaskHandle // Task handle updated globally
    );
}

void M5_SD_JSON(const char *filename, String &configData)
{
    if (!SD.begin())
    { // Initialize the SD card. 初始化SD卡
        M5.Lcd.println(
            "Card failed, or not present");
        while (1)
            ;
    }

    File myFile = SD.open(filename,
                          FILE_READ); // Open the file "/hello.txt" in read mode.
                                      // 以读取模式打开文件"/hello.txt"
    if (myFile)
    {
        // M5.Lcd.println("with cmd /expSetup.json Content:");
        while (myFile.available())
        {
            // M5.Lcd.write(myFile.read());
            // or
            configData += char(myFile.read());
            // DO NOT TURN ON BOTH AT THE SAME TIME
        }
        // Serial.println("SD Config data: ");
        // Serial.println(configData);
        myFile.close();
    }
    else
    {
        M5.Lcd.println("error opening /hello.txt"); // If the file is not open.
                                                    // 如果文件没有打开
    }
}

// void displayMap(std::unordered_map<int, std::vector<std::pair<unsigned long, uint32_t>>> UOM_sensorData)
// {
//     Serial.println("Setting,Timestamp,Data");
//     for (auto &entry : UOM_sensorData)
//     {
//         int setting = entry.first;
//         for (auto &data : entry.second)
//         {
//             unsigned long timestamp = data.first; // Extract timestamp
//             uint32_t gasResistance = data.second; // Extract gas resistance
//             Serial.print(setting);
//             Serial.print(",");
//             Serial.print(timestamp);
//             Serial.print(",");
//             Serial.println(gasResistance);
//         }
//     }
//     Serial.println("Data in object printed");
// }

void onPump()
{
    std::vector<uint8_t> pump_on = switchCommand(1, pump_relay, 1);
    Serial2.write(pump_on.data(), pump_on.size());
}

void offPump()
{
    std::vector<uint8_t> pump_on = switchCommand(1, pump_relay, 0);
    Serial2.write(pump_on.data(), pump_on.size());
}

void readConfigCMD()
{
    // commandMap["start"] = []()
    // { expTask(); };
    commandMap["sample"] = []()
    { BMEsampleTask(); };
    commandMap["pumpOn"] = []()
    { onPump(); };
    commandMap["pumpOff"] = []()
    { offPump(); };
    commandMap["purgeAll"] = []()
    { purgeAll(); };
    commandMap["purgeOff"] = []()
    { purgeOff(); };
}