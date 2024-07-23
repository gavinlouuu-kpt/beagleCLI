#include <Arduino.h>
#include <exp_setup.h>
#include <unordered_map>
#include <vector>
#include <SD.h> // SD card library but included in M5 library
// #include <Adafruit_ADS1X15.h>
#include <pinConfig.h>

String continuousADS_Header = "Setting,Timestamp,Channel_0";

struct SingleChannel
{
    int setting;
    unsigned long timestamp;
    int16_t channel_0;
};

std::vector<SingleChannel> ADSBuffer1;
std::vector<SingleChannel> ADSBuffer2;
std::vector<SingleChannel> *currentBuffer = &ADSBuffer1;
std::vector<SingleChannel> *saveBuffer = &ADSBuffer2;

constexpr size_t bufferSize = 500;

void switchBuffers()
{
    // Serial.println("Switching buffers.");
    if (currentBuffer == &ADSBuffer1)
    {
        currentBuffer = &ADSBuffer2;
        saveBuffer = &ADSBuffer1;
    }
    else
    {
        currentBuffer = &ADSBuffer1;
        saveBuffer = &ADSBuffer2;
    }
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

void ADS_warm_up(const std::vector<int> &heaterSettings, int heatingTime, int warmDuration)
{
    Serial.println("Warming up the heater.");
    uint32_t warm_up_Start = millis();
    while (millis() - warm_up_Start < warmDuration)
    {
        for (int setting : heaterSettings)
        {
            ledcWrite(PWM_Heater, setting);
            uint32_t heatDuration = millis();
            while (millis() - heatDuration < heatingTime)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
    Serial.println("Warm up complete.");
    // xTaskNotifyGive(expLoopTaskHandle);
}

void ADS_continuous(std::vector<SingleChannel> *buffer, const std::vector<int> &heaterSettings, int heatingTime)
{
    for (int setting : heaterSettings)
    {
        if (!checkAndRecoverSDCard())
        {
            Serial.println("Failed to recover SD card. Cannot save data.");
            return;
        }
        ledcWrite(PWM_Heater, setting);
        unsigned long timestamp = millis();              // Get current timestamp
        int16_t result = ads.getLastConversionResults(); // Get sensor reading

        SingleChannel data;         // Create a struct instance
        data.setting = setting;     // Assign the setting
        data.timestamp = timestamp; // Assign the timestamp
        data.channel_0 = result;    // Assign the sensor result

        buffer->push_back(data); // Push structured data to circular buffer
    }
}

void saveADSDataFromBuffer(const std::vector<SingleChannel> &buffer, const String &filename, String header)
{

    File myFile = SD.open(filename, FILE_APPEND); // Change here to FILE_APPEND
    if (!myFile)
    {
        Serial.println("Error opening file for writing");
        return;
    }
    if (myFile.size() == 0) // The header should only be written if the file was newly created or is empty
    {
        myFile.println(header); // Write header only if file is empty
    }
    for (const auto &data : buffer)
    {
        myFile.printf("%d,%lu,%d\n", data.setting, data.timestamp, data.channel_0);
    }
    myFile.close();
    // Serial.println("Data dumped to: " + filename);
}

String filename;
void dataSavingTask(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification from the data collection task
        saveADSDataFromBuffer(*saveBuffer, filename, continuousADS_Header);
        saveBuffer->clear();
        // xTaskNotifyGive(dataCollectionTaskHandle);// Notify the collection task that saving is complete and buffer is cleared
    }
}

void sampleADScontinuous(void *pvParameters)
{
    const unsigned long saveInterval = 5000; // 5 seconds in milliseconds
    unsigned long lastSaveTime = millis();
    currentBuffer->reserve(bufferSize);
    saveBuffer->reserve(bufferSize);
    TaskHandle_t savingTaskHandle; // Start the saving task
    xTaskCreate(dataSavingTask, "Data Saving Task", 4096, NULL, 1, &savingTaskHandle);

    for (;;)
    {
        // Wait for a notification to start data acquisition
        Serial.println("RUNNER: Waiting for notification to start data acquisition.");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Start of a new experiment
        Serial.println("RUNNER: Start data acquisition.");
        currentBuffer->clear();
        // Global variable version
        filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);

        while (true)
        {
            ADS_continuous(currentBuffer, heaterSettings, heatingTime);
            // Serial.print("*");

            if (millis() - lastSaveTime >= saveInterval || currentBuffer->size() >= bufferSize)
            {
                switchBuffers();
                lastSaveTime = millis();           // Reset the timer after saving
                xTaskNotifyGive(savingTaskHandle); // Notify the saving task to save the data
            }
            if (ulTaskNotifyTake(pdTRUE, 0)) // Check for a stop notification without waiting
            {
                Serial.println("RUNNER: Data saving notification received.");
                break; // Stop data acquisition on notification
            }
        }
        if (!currentBuffer->empty())
        {
            switchBuffers();
            xTaskNotifyGive(savingTaskHandle); // Notify the saving task to save the data
        }

        Serial.println("RUNNER: Data saving complete. Notifying expLoopTask.");
        xTaskNotifyGive(expLoopTaskHandle);
    }
}

// template functions
//-----------------------------------------------------------------------------------------------

#include <functional>

using BufferType = std::vector<SingleChannel>; // Use SingleChannel for the buffer type
using SamplingFunction = void (*)(BufferType *, const std::vector<int> &, int);

void genericSampleADSContinuous(void *pvParameters, SamplingFunction sampleFunc)
{
    const unsigned long saveInterval = 5000; // 5 seconds in milliseconds
    unsigned long lastSaveTime = millis();

    currentBuffer->reserve(bufferSize);
    saveBuffer->reserve(bufferSize);

    TaskHandle_t savingTaskHandle;
    xTaskCreate(dataSavingTask, "Data Saving Task", 4096, NULL, 1, &savingTaskHandle);

    for (;;)
    {
        Serial.println("RUNNER: Waiting for notification to start data acquisition.");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        currentBuffer->clear();
        filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);

        while (true)
        {
            sampleFunc(currentBuffer, heaterSettings, heatingTime);
            Serial.print("*");

            if (millis() - lastSaveTime >= saveInterval || currentBuffer->size() >= bufferSize)
            {
                switchBuffers();
                lastSaveTime = millis();
                xTaskNotifyGive(savingTaskHandle);
            }

            if (ulTaskNotifyTake(pdTRUE, 0))
            {
                Serial.println("RUNNER: Data saving notification received.");
                break;
            }
        }

        if (!currentBuffer->empty())
        {
            switchBuffers();
            xTaskNotifyGive(savingTaskHandle);
        }

        Serial.println("RUNNER: Data saving complete. Notifying expLoopTask.");
        xTaskNotifyGive(expLoopTaskHandle);
    }
}

void temp_ADS(void *pvParameters)
{
    // Call the generic function with the specific sampling function
    genericSampleADSContinuous(nullptr, ADS_continuous);
}

// Multichannel ADS
//-----------------------------------------------------------------------------------------------

struct MultiChannel
{
    int setting;
    unsigned long timestamp;
    int16_t channel_0;
    int16_t channel_1;
    int16_t channel_2;
    int16_t channel_3;
};

std::vector<MultiChannel> m_ADSBuffer1;
std::vector<MultiChannel> m_ADSBuffer2;
std::vector<MultiChannel> *m_currentBuffer = &m_ADSBuffer1;
std::vector<MultiChannel> *m_saveBuffer = &m_ADSBuffer2;

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
                Serial.println("Data saving notification received.");
                break;
            }
        }

        // Save the acquired data
        saveADSData(ADS_sensorData, setup_tracker, repeat_tracker, channel_tracker, exp_name);

        // Notify the exp_loop task that data saving is complete
        xTaskNotifyGive(expLoopTaskHandle);
    }
}

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