#include <chrono>
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

#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#include <Adafruit_ADS1X15.h>

#include <unordered_map>
#include <WiFi.h>

Adafruit_ADS1115 ads;
Adafruit_BME680 bme; // I2C
TaskHandle_t bmeTaskHandle, adsTaskHandle, expLoopTaskHandle;

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

std::vector<int> stringToArray(const std::string &str)
{
    std::vector<int> result;
    std::stringstream ss(str.substr(1, str.size() - 2)); // Remove the brackets
    std::string item;

    while (getline(ss, item, ','))
    {
        result.push_back(stoi(item));
    }

    return result;
}

int getInt(FirebaseJson json, int setup_no, String target)
{
    // get item within a setup
    FirebaseJsonData jsonData;
    String key = "setup_" + String(setup_no);
    String full_key = key + target;
    int result;
    if (json.get(jsonData, full_key))
    {
        json.get(jsonData, full_key);
        result = String(jsonData.intValue).toInt();
    }
    else
    {
        Serial.println("Failed to find key: " + target);
    }
    return result;
}

String getExpName(FirebaseJson json, int setup_no, String target)
{
    // get item within a setup
    FirebaseJsonData jsonData;
    String key = "setup_" + String(setup_no);
    String full_key = key + target;
    String result;
    if (json.get(jsonData, full_key))
    {
        json.get(jsonData, full_key);
        result = jsonData.to<String>().c_str();
    }
    else
    {
        Serial.println("Failed to find key: " + target);
    }
    return result;
}

std::vector<int> getArr(FirebaseJson json, int setup_no, String target)
{
    FirebaseJsonData jsonData;
    String key = "setup_" + String(setup_no);
    String full_key = key + target;
    std::vector<int> result;
    if (json.get(jsonData, full_key))
    {
        json.get(jsonData, full_key);
        result = stringToArray(jsonData.stringValue.c_str());
    }
    else
    {
        Serial.println("Failed to find key: " + target);
    }
    return result;
}

int count_setup(String jsonString)
{
    size_t setupCount = 0;
    FirebaseJson json;
    FirebaseJsonData jsonData;
    // Serial.println(jsonString);
    json.setJsonData(jsonString);
    // Serial.println("Counting Setups");

    // Assuming setups are named 'setup_1', 'setup_2', ..., 'setup_n'
    for (int i = 1; i <= 10; i++)
    { // Adjust the upper limit as needed
        String key = "setup_" + String(i);
        if (json.get(jsonData, key))
        {
            setupCount++;
        }
        // Serial.println("Setup count: " + String(setupCount));
    }

    return setupCount;
}

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
        BMEsampleTask();
        return 1;
    }
    else if (ads.begin(ADSi2c))
    {
        ads.setDataRate(RATE_ADS1115_860SPS);
        ads.setGain(GAIN_ONE);
        Serial.println("ADS1115 sensor found!");
        ADSsampleTask();
        return 2;
    }
    Serial.println("Sensor checking failed!");
    return -1;
}

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

void exp_loop(FirebaseJson config, int setup_count)
{
    int sensorType = sensorCheck();
    purgeAll();

    std::vector<uint8_t> pump_on = switchCommand(1, pump_relay, 1);
    Serial2.write(pump_on.data(), pump_on.size());
    ledcWrite(PWM_Vin, 255); // sensor voltage pwm

    for (int i = 1; i <= setup_count; i++)
    {
        setup_tracker = i;
        exp_name = getExpName(config, i, "/exp_name");
        heaterSettings = getArr(config, i, "/heater_settings");
        heatingTime = getInt(config, i, "/heating_time(ms)");
        int exp_time = getInt(config, i, "/exp_time(ms)");
        int duration = getInt(config, i, "/duration(s)") * 1000;
        int repeat = getInt(config, i, "/repeat");
        std::vector<int> channels = getArr(config, i, "/channel");

        if (duration == 0 || repeat == 0 || channels.size() == 0)
        {
            Serial.println("Invalid setup: " + String(i));
            continue;
        }

        for (int j = 0; j < repeat; j++)
        {
            repeat_tracker = j;
            Serial.println("Running experiment for setup: " + String(i) + " repeat: " + String(j));

            for (int cur_channel : channels)
            {
                purgeOff();
                startTime = millis();
                channel_tracker = cur_channel;

                // Notify the ADS sampling task to start data acquisition
                xTaskNotifyGive(adsTaskHandle);

                Serial.println("Running experiment for channel: " + String(cur_channel));
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
                xTaskNotify(adsTaskHandle, 0, eIncrement);

                // Wait for the ADS sampling task to signal completion
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            }
        }
    }

    if (sensorType == 1)
    {
        vTaskDelete(bmeTaskHandle);
    }
    else if (sensorType == 2)
    {
        vTaskDelete(adsTaskHandle);
    }
    relay_off();

    Serial.println("END of LOOP");
    startTime = 0;
    last_setup_tracker = -1;
}

void exp_build()
{
    // File name for the JSON configuration
    const char *filename = "/expSetup.json";
    String configData;
    M5_SD_JSON(filename, configData);
    FirebaseJson json;
    json.setJsonData(configData);
    int setupCount = count_setup(configData);
    // Serial.println("Number of setups: " + String(setupCount));
    exp_loop(json, setupCount);
}

void exp_start(void *pvParameters)
{
    expLoopTaskHandle = xTaskGetCurrentTaskHandle();
    exp_build();
    vTaskDelete(NULL);
}

void expTask()
{
    xTaskCreate(
        exp_start,           /* Task function. */
        "exp_start",         /* String with name of task. */
        10240,               /* Stack size in bytes. */
        NULL,                /* Parameter passed as input of the task */
        1,                   /* Priority of the task. */
        &expLoopTaskHandle); /* Task handle. */
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
        M5.Lcd.println("with cmd /expSetup.json Content:");
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

void displayMap(std::unordered_map<int, std::vector<std::pair<unsigned long, uint32_t>>> UOM_sensorData)
{
    Serial.println("Setting,Timestamp,Data");
    for (auto &entry : UOM_sensorData)
    {
        int setting = entry.first;
        for (auto &data : entry.second)
        {
            unsigned long timestamp = data.first; // Extract timestamp
            uint32_t gasResistance = data.second; // Extract gas resistance
            Serial.print(setting);
            Serial.print(",");
            Serial.print(timestamp);
            Serial.print(",");
            Serial.println(gasResistance);
        }
    }
    Serial.println("Data in object printed");
}

bool ensureDirectoryExists(String path)
{
    if (!SD.exists(path))
    {
        if (SD.mkdir(path.c_str()))
        {
            Serial.println("Directory created: " + path);
            return true;
        }
        else
        {
            Serial.println("Failed to create directory: " + path);
            return false;
        }
    }
    return true;
}

String incrementFolder(String folderPath)
{
    int underscoreIndex = folderPath.lastIndexOf('_');
    int lastNumber = 0;
    String basePart = folderPath;

    // Check if the substring after the last underscore can be converted to a number
    if (underscoreIndex != -1 && underscoreIndex < folderPath.length() - 1)
    {
        String numPart = folderPath.substring(underscoreIndex + 1);
        if (numPart.toInt() != 0 || numPart == "0")
        { // Checks if conversion was successful or is "0"
            lastNumber = numPart.toInt();
            basePart = folderPath.substring(0, underscoreIndex); // Adjust the base part to exclude the number
        }
    }

    // Increment and create a new folder name with the updated number
    String newPath;
    do
    {
        newPath = basePart + "_" + (++lastNumber);
    } while (SD.exists(newPath));

    if (SD.mkdir(newPath.c_str()))
    {
        Serial.println("New directory created: " + newPath);
    }
    else
    {
        Serial.println("Failed to create directory: " + newPath);
    }
    return newPath;
}

String createOrIncrementFolder(String folderPath)
{
    if (!ensureDirectoryExists(folderPath))
    {
        Serial.println("Base directory does not exist and could not be created.");
        return "";
    }
    return incrementFolder(folderPath);
}

String setupSave(int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return "";
    }
    char today[11];
    strftime(today, sizeof(today), "%Y_%m_%d", &timeinfo);
    char currentTime[9];
    strftime(currentTime, sizeof(currentTime), "%H_%M_%S", &timeinfo);

    String baseDir = "/" + String(today);
    String expDir = baseDir + "/" + exp_name;

    if (!ensureDirectoryExists(baseDir) || !ensureDirectoryExists(expDir))
    {
        Serial.println("Failed to ensure directories exist.");
        return "";
    }

    if (setup_tracker != last_setup_tracker)
    {
        currentPath = createOrIncrementFolder(expDir);
        last_setup_tracker = setup_tracker;
    }

    String uniqueFilename = currentPath + "/" + String(currentTime) + "s" + String(setup_tracker) + "c" + String(channel_tracker) + "r" + String(repeat_tracker) + ".csv";
    return uniqueFilename;
}

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
    commandMap["start"] = []()
    { expTask(); };
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