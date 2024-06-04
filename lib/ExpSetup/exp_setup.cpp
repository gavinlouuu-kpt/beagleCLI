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

#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#include <unordered_map>
#include <WiFi.h>

Adafruit_BME680 bme; // I2C

// Declare a mutex
SemaphoreHandle_t expStateMutex;

enum ExpState
{
    EXP_IDLE,
    EXP_WARMING_UP,
    EXP_SAVE,
    EXP_READY,
    EXP_DAQ,

};

ExpState expState = EXP_IDLE;

void expMutexSetup()
{
    // Create the mutex in your setup function
    expStateMutex = xSemaphoreCreateMutex();
}

// Use the mutex when accessing expState
void mutexEdit(ExpState state)
{
    // Take the mutex, waiting indefinitely for it to become available
    xSemaphoreTake(expStateMutex, portMAX_DELAY);
    expState = state;
    Serial.println("Setting expState to state:" + String(expState));
    // Give the mutex back
    xSemaphoreGive(expStateMutex);
}

// Do the same when reading expState
ExpState getExpState()
{
    // Take the mutex, waiting indefinitely for it to become available
    xSemaphoreTake(expStateMutex, portMAX_DELAY);
    ExpState currentState = expState;
    // Give the mutex back
    xSemaphoreGive(expStateMutex);
    return currentState;
}

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
    Serial.println(jsonString);
    json.setJsonData(jsonString);
    Serial.println("Counting Setups");

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

int setup_tracker = 0;
int repeat_tracker = 0;
int channel_tracker = 0;

void exp_loop(FirebaseJson config, int setup_count, int exp_time = 10000)
{
    mutexEdit(EXP_WARMING_UP);
    Serial.println("State = EXP_WARMING_UP" + String(expState));
    std::vector<uint8_t> pump_on = switchCommand(1, 47, 1);
    Serial2.write(pump_on.data(), pump_on.size());
    Serial.println("Pump command: ");
    for (uint8_t i : pump_on)
    {
        Serial.print(i);
    }

    for (int i = 1; i <= setup_count; i++)
    {
        setup_tracker = i;
        int duration = getInt(config, i, "/duration(s)");
        duration = duration * 1000;
        int repeat = getInt(config, i, "/repeat");
        std::vector<int> channels = getArr(config, i, "/channel");
        // only continue with the experiment if all three parameters are not 0
        if (duration == 0 || repeat == 0 || channels.size() == 0)
        {
            // if invalid setup found exit the function
            Serial.println("Invalid setup: " + String(i));
            continue;
        }
        mutexEdit(EXP_READY); // Set experiment state to READY
        Serial.println("State = EXP_READY" + String(expState));

        // for each channel in the array, run the experiment
        Serial.println("Running experiment for setup: " + String(i));
        for (int j = 0; j < repeat; j++)
        {
            repeat_tracker = j;
            Serial.println("Running experiment for setup: " + String(i) + " repeat: " + String(j));
            for (int cur_channel : channels)
            {
                channel_tracker = cur_channel;
                // Only proceed if the state is EXP_READY
                while (expState != EXP_READY)
                {
                    Serial.print("/");
                }
                mutexEdit(EXP_DAQ);
                Serial.println("State = EXP_DAQ" + String(expState));
                // RTOS logic that is driven by events
                // use rtos to collect data and flag to signal start and stop for data saving
                Serial.println("Running experiment for channel: " + String(cur_channel));
                std::vector<uint8_t> on_command = switchCommand(1, cur_channel, 1);

                Serial.println("On command: ");
                for (uint8_t i : on_command)
                {
                    Serial.print(i);
                }
                Serial.println("");

                Serial.println("Turning on channel: " + String(cur_channel));
                Serial2.write(on_command.data(), on_command.size());
                delay(duration);
                // flush serial

                std::vector<uint8_t> off_command = switchCommand(1, cur_channel, 0);
                Serial.println("Off command: ");
                for (uint8_t i : off_command)
                {
                    Serial.print(i);
                }
                Serial.println("");
                Serial.println("Turning off channel: " + String(cur_channel));
                Serial2.write(off_command.data(), off_command.size());
                delay(exp_time);
                mutexEdit(EXP_SAVE);
                Serial.println("State = EXP_SAVE" + String(expState));
                // wait for data to be saved and expState to be set to EXP_READY
            }
        }
    }

    std::vector<uint8_t> pump_off = switchCommand(1, 47, 0);
    Serial2.write(pump_off.data(), pump_off.size());
    Serial.println("Pump off command: ");
    for (uint8_t i : pump_off)
    {
        Serial.print(i);
    }
    Serial.println("END of LOOP");
    while (expState != EXP_READY)
    {
        Serial.print("/");
    }
    mutexEdit(EXP_IDLE);
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
    Serial.println("Number of setups: " + String(setupCount));
    exp_loop(json, setupCount);
}

void exp_start(void *pvParameters)
{
    exp_build();
    vTaskDelete(NULL);
}

void expTask()
{
    xTaskCreate(
        exp_start,   /* Task function. */
        "exp_start", /* String with name of task. */
        10000,       /* Stack size in bytes. */
        NULL,        /* Parameter passed as input of the task */
        1,           /* Priority of the task. */
        NULL);       /* Task handle. */
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
        Serial.println("SD Config data: ");
        Serial.println(configData);
        myFile.close();
    }
    else
    {
        M5.Lcd.println("error opening /hello.txt"); // If the file is not open.
                                                    // 如果文件没有打开
    }
}

int heatingTime = 8;
std::unordered_map<int, std::vector<uint32_t>> UOM_sensorData;
std::vector<int> heaterSettings = {150, 200, 250, 300, 350, 400, 450, 500};

void displayMap(std::unordered_map<int, std::vector<uint32_t>> UOM_sensorData)
{
    for (auto iter = UOM_sensorData.begin(); iter != UOM_sensorData.end(); ++iter)
    {
        int setting = iter->first;                           // Extract the setting
        const std::vector<uint32_t> &dataVec = iter->second; // Extract the vector of data

        Serial.print("Data for setting ");
        Serial.print(setting);
        Serial.println(": ");

        for (auto value : dataVec)
        {
            Serial.println(value);
        }
        Serial.println(); // Adds a new line after the list of data for readability
    }
    Serial.println("All data printed");
}
// save uom data as csv file in SD card
void saveUOMData(std::unordered_map<int, std::vector<uint32_t>> UOM_sensorData, int setup_tracker, int repeat_tracker, int channel_tracker)
{

    std::string macAddressTest = WiFi.macAddress().c_str();
    // obtain date
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    char today[11];                                                    // Buffer to hold the date string
    strftime(today, sizeof(today), "%Y_%m_%d", &timeinfo);             // Format: YYYY-MM-DD
    char currentTime[9];                                               // Buffer to hold the time string
    strftime(currentTime, sizeof(currentTime), "%H_%M_%S", &timeinfo); // Format: HH:MM:SS
    String folderPath = "/" + String(today);
    // Ensuring the folder exists or create if it does not
    if (!SD.exists(folderPath))
    {
        if (!SD.mkdir(folderPath))
        {
            Serial.println("Failed to create today's directory");
            return;
        }
    }

    String uniqueFilename = folderPath + "/" + String(currentTime) + "s" + String(setup_tracker) + "c" + String(channel_tracker) + "r" + String(repeat_tracker) + ".csv";
    const char *filename = uniqueFilename.c_str();

    // Open the file in write mode
    File myFile = SD.open(filename, FILE_WRITE);

    // Check if the file was opened successfully
    if (myFile)
    {
        // Write the header row
        myFile.println("Setting,Data");

        // Write the data
        for (auto iter = UOM_sensorData.begin(); iter != UOM_sensorData.end(); ++iter)
        {
            int setting = iter->first;                           // Extract the setting
            const std::vector<uint32_t> &dataVec = iter->second; // Extract the vector of data

            for (auto value : dataVec)
            {
                myFile.print(setting);
                myFile.print(",");
                myFile.println(value);
            }
        }

        // Close the file
        myFile.close();
    }
    else
    {
        Serial.println("Error opening file for writing");
    }
}

// called as RTOS task
// int UOM_sensor(std::unordered_map<int, std::vector<uint32_t>> UOM_sensorData, std::vector<int> heaterSettings, int heatingTime)
// {
//     // while (expState != EXP_DAQ)
//     // {
//     //     // Serial.println("Waiting for the experiment to start");
//     // }
//     while (expState == EXP_DAQ)
//     {
//         // Serial.println("In the sampling loop");
//         if (!bme.begin())
//         {
//             Serial.println("Could not find a valid BME680 sensor, check wiring!");
//             while (1)
//                 ;
//         }
//         for (int setting : heaterSettings)
//         {
//             Serial.print(".");
//             // Serial.print("Setting the gas heater to ");
//             // Serial.println(setting);
//             bme.setGasHeater(setting, heatingTime);
//             if (bme.performReading())
//             {
//                 UOM_sensorData[setting].push_back(bme.gas_resistance);
//                 // Serial.print("Gas Resistance: ");
//                 // Serial.println(bme.gas_resistance);
//                 // Serial.println(expState);
//             }
//             else
//             {
//                 Serial.println("Failed to perform reading.");
//             }
//         }
//     }
//     if (expState == EXP_SAVE)
//     {
//         Serial.println("Saving data");
//         // save the data
//         // displayMap(UOM_sensorData);
//         saveUOMData(UOM_sensorData, setup_tracker, repeat_tracker, channel_tracker);
//         expState = EXP_READY;
//         return 1;
//     }

//     return 0;
// }

int UOM_sensor(std::unordered_map<int, std::vector<uint32_t>> &UOM_sensorData, std::vector<int> heaterSettings, int heatingTime)
{
    if (!bme.begin())
    {
        Serial.println("Could not find a valid BME680 sensor, check wiring!");
        while (1)
            ;
    }
    for (int setting : heaterSettings)
    {
        Serial.print(".");
        bme.setGasHeater(setting, heatingTime);
        if (bme.performReading())
        {
            UOM_sensorData[setting].push_back(bme.gas_resistance);
        }
        else
        {
            Serial.println("Failed to perform reading.");
        }
    }

    return 0;
}

void sample_start(void *pvParameters)
{
    for (;;)
    {
        // uomTest();
        int currentState = getExpState();
        // Serial.print("-");
        if (currentState == EXP_DAQ) // EXP_DAQ)
        {

            UOM_sensor(UOM_sensorData, heaterSettings, heatingTime);
        }
        else if (currentState == EXP_SAVE)
        {
            // Serial.print("+");
            displayMap(UOM_sensorData);
            saveUOMData(UOM_sensorData, setup_tracker, repeat_tracker, channel_tracker);
            mutexEdit(EXP_READY);
        }
        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void sampleTask()
{
    xTaskCreate(
        sample_start,   /* Task function. */
        "sample_start", /* String with name of task. */
        10000,          /* Stack size in bytes. */
        NULL,           /* Parameter passed as input of the task */
        1,              /* Priority of the task. */
        NULL);          /* Task handle. */
}

void expDAQ()
{
    mutexEdit(EXP_DAQ);
}

void expIDLE()
{
    mutexEdit(EXP_IDLE);
}

void expSAVE()
{
    mutexEdit(EXP_SAVE);
}

void checkState()
{
    Serial.println("Current state: " + String(expState));
}

void readConfigCMD()
{
    commandMap["startRTOS"] = []()
    { expTask(); };
    commandMap["sdTest"] = []()
    { exp_build(); };
    commandMap["sampleTask"] = []()
    { sampleTask(); };
    commandMap["expDAQ"] = []()
    { expDAQ(); };
    commandMap["expIDLE"] = []()
    { expIDLE(); };
    commandMap["expSAVE"] = []()
    { expSAVE(); };
    commandMap["checkState"] = []()
    { checkState(); };
}