#include <Arduino.h>
#include <exp_setup.h>
#include <unordered_map>
#include <vector>
#include <SD.h> // SD card library but included in M5 library
// #include <Adafruit_ADS1X15.h>
#include <pinConfig.h>

String continuousADS_Header = "Setting,Timestamp,Value";

struct SettingData
{
    int setting;
    unsigned long timestamp;
    int16_t value;
};

std::vector<SettingData> ADSBuffer1;
std::vector<SettingData> ADSBuffer2;
std::vector<SettingData> *currentBuffer = &ADSBuffer1;
std::vector<SettingData> *saveBuffer = &ADSBuffer2;

// std::vector<SettingData> ADSBuffer;
constexpr size_t bufferSize = 5000;

void switchBuffers()
{
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

void UOM_ADS_continuous(std::vector<SettingData> *buffer, const std::vector<int> &heaterSettings, int heatingTime)
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

        buffer->push_back(data); // Push structured data to circular buffer
    }
}

void saveADSDataFromBuffer(const std::vector<SettingData> &buffer, const String &filename, String header)
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
        myFile.println(header); // Write header only if file is empty
    }

    for (const auto &data : buffer)
    {
        myFile.printf("%d,%lu,%d\n", data.setting, data.timestamp, data.value);
    }

    myFile.close();
    Serial.println("Data dumped to: " + filename);
}

void sampleADScontinuous(void *pvParameters)
{
    const unsigned long saveInterval = 5000; // 5 seconds in milliseconds
    unsigned long lastSaveTime = millis();
    // ADSBuffer.reserve(bufferSize); // Reserve memory for buffer only within a function
    currentBuffer->reserve(2500);
    saveBuffer->reserve(2500);

    for (;;)
    {
        // Wait for a notification to start data acquisition
        Serial.println("RUNNER: Waiting for notification to start data acquisition.");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Start of a new experiment
        Serial.println("RUNNER: Start data acquisition.");
        // ADSBuffer.clear();
        currentBuffer->clear();

        String filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);

        while (true)
        {

            // UOM_ADS_continuous(ADSBuffer, heaterSettings, heatingTime);
            UOM_ADS_continuous(currentBuffer, heaterSettings, heatingTime);
            Serial.print("*");

            // Check if it's time to save data
            if (millis() - lastSaveTime >= saveInterval || currentBuffer->size() >= bufferSize)
            {
                switchBuffers();
                saveADSDataFromBuffer(*saveBuffer, filename, continuousADS_Header);
                saveBuffer->clear();     // Clear the buffer after saving
                lastSaveTime = millis(); // Reset the timer after saving
                // ADSBuffer.clear();       // Optionally clear buffer after saving if appropriate
            }

            // Check for a stop notification without waiting
            if (ulTaskNotifyTake(pdTRUE, 0))
            {
                Serial.println("RUNNER: Data saving notification received.");
                break; // Stop data acquisition on notification
            }
        }

        // Optionally perform a final save if there's data left in the buffer
        // if (!ADSBuffer.empty())
        if (!currentBuffer->empty())
        {
            // String filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);
            switchBuffers();
            // saveADSDataFromBuffer(ADSBuffer, filename, continuousADS_Header);
            saveADSDataFromBuffer(*saveBuffer, filename, continuousADS_Header);
            // ADSBuffer.clear(); // Clear buffer after final saving
            saveBuffer->clear(); // Clear buffer after final saving
        }

        // Notify that this round of data collection is complete
        Serial.println("RUNNER: Data saving complete. Notifying expLoopTask.");
        xTaskNotifyGive(expLoopTaskHandle);
    }
}

//-----------------------------------------------------------------------------------------------

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