#include <Arduino.h>
#include <exp_setup.h>
#include <unordered_map>
#include <vector>
#include <SD.h> // SD card library but included in M5 library
// #include <Adafruit_ADS1X15.h>
#include <pinConfig.h>
#include <CircularBuffer.hpp>

constexpr size_t bufferSize = 5000;
// CircularBuffer<std::pair<unsigned long, int16_t>, bufferSize> ADSBuffer;

struct SettingData
{
    int setting;
    unsigned long timestamp;
    int16_t value;
};

CircularBuffer<SettingData, bufferSize> ADSBuffer;

int UOM_sensorADS(std::unordered_map<int, std::vector<std::pair<unsigned long, std::array<int16_t, 4>>>> &ADS_sensorData, std::vector<int> heaterSettings, int heatingTime)
{

    for (int setting : heaterSettings)
    {
        // Serial.print("+");
        ledcWrite(PWM_Heater, setting);

        unsigned long timestamp = millis(); // relative timestamp has a resetting bug that needs to be fixed
        std::array<int16_t, 4> ADSreadings = {ads.readADC_SingleEnded(0), ads.readADC_SingleEnded(1), ads.readADC_SingleEnded(2), ads.readADC_SingleEnded(3)};
        // ADS_sensorData[setting].push_back(std::make_pair(timestamp, ADSreadings));
        ADS_sensorData[setting].emplace_back(timestamp, ADSreadings);
    }

    return 0;
}

void UOM_ADS_continuous(CircularBuffer<SettingData, bufferSize> &buffer, const std::vector<int> &heaterSettings, int heatingTime)
{
    for (int setting : heaterSettings)
    {
        ledcWrite(PWM_Heater, setting);
        unsigned long timestamp = millis();              // Get current timestamp
        int16_t result = ads.getLastConversionResults(); // Get sensor reading

        SettingData data;           // Create a struct instance
        data.setting = setting;     // Assign the setting
        data.timestamp = timestamp; // Assign the timestamp
        data.value = result;        // Assign the sensor result

        buffer.push(data); // Push structured data to circular buffer
    }
}
String continuousADS_Header = "Setting,Timestamp,Value";
void saveADSDataFromBuffer(const CircularBuffer<SettingData, bufferSize> &buffer, const String &filename)
{
    File myFile = SD.open(filename, FILE_APPEND); // Change here to FILE_APPEND
    if (!myFile)
    {
        Serial.println("Error opening file for writing");
        return;
    }

    // The header should only be written if the file was newly created or is empty
    if (myFile.size() == 0)
    {
        myFile.println(continuousADS_Header); // Write header only if file is empty
    }

    // Dump all buffer data to file
    for (size_t i = 0; i < buffer.size(); i++)
    {
        const SettingData &data = buffer[i];
        myFile.printf("%d,%lu,%d\n", data.setting, data.timestamp, data.value);
    }

    myFile.close();
    Serial.println("Data dumped to: " + filename);
}

// void UOM_ADS_continuous(std::unordered_map<int, std::vector<std::pair<unsigned long, int16_t>>> &ADS_continuous, std::vector<int> heaterSettings, int heatingTime)
// {
//     // implement circular buffer
//     for (int setting : heaterSettings)
//     {
//         ledcWrite(PWM_Heater, setting);

//         unsigned long timestamp = millis(); // relative timestamp has a resetting bug that needs to be fixed
//         int16_t results = ads.getLastConversionResults();
//         ADS_continuous[setting].emplace_back(timestamp, results);
//     }
// }

void sampleADS(void *pvParameters)
{
    for (;;)
    {
        std::unordered_map<int, std::vector<std::pair<unsigned long, std::array<int16_t, 4>>>> ADS_sensorData;

        // Wait for a notification to start data acquisition
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Perform data acquisition continuously until notified to save data
        while (true)
        {
            UOM_sensorADS(ADS_sensorData, heaterSettings, heatingTime);
            Serial.print("-");

            // Check if there's a notification to save data
            if (ulTaskNotifyTake(pdTRUE, 0)) // Check without waiting
            {
                break;
            }
        }

        // Save the acquired data
        saveADSData(ADS_sensorData, setup_tracker, repeat_tracker, channel_tracker, exp_name);

        // Notify the exp_loop task that data saving is complete
        xTaskNotifyGive(expLoopTaskHandle);
    }
}

// void sampleADScontinuous(void *pvParameters)
// {
//     for (;;)
//     {
//         // continuous experiment
//         std::unordered_map<int, std::vector<std::pair<unsigned long, int16_t>>> ADS_continuous;

//         // Wait for a notification to start data acquisition
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//         // Perform data acquisition continuously until notified to save data
//         while (true)
//         {

//             // ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, true); // Continuous mode
//             UOM_ADS_continuous(ADS_continuous, heaterSettings, heatingTime);
//             Serial.print("*");

//             // Check if there's a notification to save data
//             if (ulTaskNotifyTake(pdTRUE, 0)) // Check without waiting
//             {
//                 break;
//             }
//         }

//         // continuous exp
//         saveADScontinuous(ADS_continuous, setup_tracker, repeat_tracker, channel_tracker, exp_name);

//         // Notify the exp_loop task that data saving is complete
//         xTaskNotifyGive(expLoopTaskHandle);
//     }
// }

// const unsigned long saveInterval = 5000;
void sampleADScontinuous(void *pvParameters)
{
    const unsigned long saveInterval = 300000; // 5 minutes in milliseconds
    unsigned long lastSaveTime = millis();

    for (;;)
    {
        // Wait for a notification to start data acquisition
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Start of a new experiment
        ADSBuffer.clear();
        String filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);

        while (true)
        {
            // Perform data acquisition
            // std::array<int16_t, 4> results;  // Simulate results
            // int currentSetting = getCurrentSetting();  // Get the current setting
            UOM_ADS_continuous(ADSBuffer, heaterSettings, heatingTime);
            Serial.print("*");

            // Check if it's time to save data
            if (millis() - lastSaveTime >= saveInterval || ADSBuffer.isFull())
            {

                saveADSDataFromBuffer(ADSBuffer, filename);
                lastSaveTime = millis(); // Reset the timer after saving
                ADSBuffer.clear();       // Optionally clear buffer after saving if appropriate
            }

            // Check for a stop notification without waiting
            if (ulTaskNotifyTake(pdTRUE, 0))
            {
                break; // Stop data acquisition on notification
            }
        }

        // Optionally perform a final save if there's data left in the buffer
        if (!ADSBuffer.isEmpty())
        {
            // String filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);
            saveADSDataFromBuffer(ADSBuffer, filename);
            ADSBuffer.clear(); // Clear buffer after final saving
        }

        // Notify that this round of data collection is complete
        xTaskNotifyGive(expLoopTaskHandle);
        // Block until the next run or termination
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

// void saveADScontinuous(std::unordered_map<int, std::vector<std::pair<unsigned long, int16_t>>> &ADS_continuous, int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name)
// {

//     String filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);
//     if (filename == "")
//     {
//         Serial.println("Error in setupSave function.");
//         return;
//     }

//     File myFile = SD.open(filename, FILE_WRITE);
//     if (!myFile)
//     {
//         Serial.println("Error opening file for writing");
//         return;
//     }

//     // Start with the CSV header
//     String buffer = "Setting,Timestamp,Value\n";

//     // Accumulate data into the buffer
//     for (auto &entry : ADS_continuous)
//     {
//         int setting = entry.first;
//         for (auto &data : entry.second)
//         {
//             buffer += String(setting) + ",";
//             buffer += String(data.first) + ",";   // Timestamp
//             buffer += String(data.second) + "\n"; // Single value for this timestamp

//             // Check if buffer needs to be flushed
//             if (buffer.length() >= 1024)
//             { // Choose a suitable size for flushing
//                 myFile.print(buffer);
//                 buffer = ""; // Clear the buffer
//             }
//         }
//     }

//     // Flush remaining data in the buffer
//     if (buffer.length() > 0)
//     {
//         myFile.print(buffer);
//     }

//     Serial.println("Data saved to file: " + filename);
//     myFile.close();
//     ADS_continuous.clear(); // Clear the data after saving
// }

// void ADSsampleTask()
// {
//     xTaskCreate(
//         sampleADS,       /* Task function. */
//         "ads_start",     /* String with name of task. */
//         20480,           /* Stack size in bytes. */
//         NULL,            /* Parameter passed as input of the task */
//         1,               /* Priority of the task. */
//         &adsTaskHandle); /* Task handle. */
// }

// void adsFastSampleTask()
// {
//     xTaskCreate(
//         sampleADScontinuous, /* Task function. */
//         "ads_start",         /* String with name of task. */
//         20480,               /* Stack size in bytes. */
//         NULL,                /* Parameter passed as input of the task */
//         1,                   /* Priority of the task. */
//         &adsFastTaskHandle); /* Task handle. */
// }

void ADSsampleTask(TaskHandle_t *taskHandle)
{
    xTaskCreate(
        sampleADS,         // Function that the task will run
        "ADS Sample Task", // Name of the task
        10240,             // Stack size
        NULL,              // Parameters to pass (can be modified as needed)
        1,                 // Task priority
        taskHandle         // Pointer to handle
    );
}

void adsFastSampleTask(TaskHandle_t *taskHandle)
{
    xTaskCreate(
        sampleADScontinuous,    // Function that the task will run
        "ADS Fast Sample Task", // Name of the task
        20240,                  // Stack size
        NULL,                   // Parameters to pass (can be modified as needed)
        1,                      // Task priority
        taskHandle              // Pointer to handle
    );
}

// buffer based saving method
void saveADSData(std::unordered_map<int, std::vector<std::pair<unsigned long, std::array<int16_t, 4>>>> &ADS_sensorData, int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name)
{
    String filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);
    if (filename == "")
    {
        Serial.println("Error in setupSave function.");
        return;
    }

    File myFile = SD.open(filename, FILE_WRITE);
    if (!myFile)
    {
        Serial.println("Error opening file for writing");
        return;
    }

    // Start with the CSV header
    String buffer = "Setting,Timestamp,Channel_0,Channel_1,Channel_2,Channel_3\n";

    // Accumulate data into the buffer
    for (auto &entry : ADS_sensorData)
    {
        int setting = entry.first;
        for (auto &data : entry.second)
        {
            buffer += String(setting) + ",";
            buffer += String(data.first) + ","; // Timestamp
            for (auto value : data.second)
            {
                buffer += String(value) + ",";
            }
            buffer += "\n";

            // Check if buffer needs to be flushed
            if (buffer.length() >= 1024) // Choose a suitable size for flushing
            {
                myFile.print(buffer);
                buffer = ""; // Clear the buffer
            }
        }
    }

    // Flush remaining data in the buffer
    if (buffer.length() > 0)
    {
        myFile.print(buffer);
    }

    Serial.println("Data saved to file: " + filename);
    myFile.close();
    ADS_sensorData.clear(); // Ensure to clear the correct data structure
}
