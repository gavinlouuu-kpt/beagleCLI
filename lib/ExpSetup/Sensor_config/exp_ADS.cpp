#include <Arduino.h>
#include <exp_setup.h>
#include <unordered_map>
#include <vector>
#include <SD.h> // SD card library but included in M5 library
// #include <Adafruit_ADS1X15.h>
#include <pinConfig.h>
#include <CircularBuffer.hpp>

String continuousADS_Header = "Setting,Timestamp,Channel_0";
String bme_Header = "Timestamp,Temperature,Humidity,Pressure";
// SemaphoreHandle_t sdCardMutex;

struct SingleChannel
{
    int setting;
    unsigned long timestamp;
    int16_t channel_0;
};

struct BME680Data
{
    unsigned long timestamp;
    float temperature;
    float humidity;
    float pressure;
    // uint32_t gas_resistance;
};

constexpr size_t bufferSize = 500;
constexpr size_t bme_bufferSize = 1000;

CircularBuffer<BME680Data, bme_bufferSize> bmeBuffer1;
CircularBuffer<BME680Data, bme_bufferSize> bmeBuffer2;
CircularBuffer<BME680Data, bme_bufferSize> *currentBME_Buffer = &bmeBuffer1;
CircularBuffer<BME680Data, bme_bufferSize> *saveBME_Buffer = &bmeBuffer2;

CircularBuffer<SingleChannel, bufferSize> ADSBuffer1;
CircularBuffer<SingleChannel, bufferSize> ADSBuffer2;
CircularBuffer<SingleChannel, bufferSize> *currentBuffer = &ADSBuffer1;
CircularBuffer<SingleChannel, bufferSize> *saveBuffer = &ADSBuffer2;

void switchBME_Buffers()
{
    // Serial.println("Switching buffers.");
    if (currentBME_Buffer == &bmeBuffer1)
    {
        currentBME_Buffer = &bmeBuffer2;
        saveBME_Buffer = &bmeBuffer1;
    }
    else
    {
        currentBME_Buffer = &bmeBuffer1;
        saveBME_Buffer = &bmeBuffer2;
    }
}

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
TaskHandle_t BME_ENV_taskHandle;
void BME_ENV_SampleTask()
{
    xTaskCreate(
        BME_ENV_loop,          // Function that the task will run
        "BME ENV Sample Task", // Name of the task
        10240,                 // Stack size
        NULL,                  // Parameters to pass (can be modified as needed)
        1,                     // Task priority
        &BME_ENV_taskHandle    // Pointer to handle
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

void ADS_continuous(CircularBuffer<SingleChannel, bufferSize> *buffer, const std::vector<int> &heaterSettings, int heatingTime)
{
    for (int setting : heaterSettings)
    {
        ledcWrite(PWM_Heater, setting);
        unsigned long timestamp = millis();              // Get current timestamp
        int16_t result = ads.getLastConversionResults(); // Get sensor reading

        SingleChannel data;         // Create a struct instance
        data.setting = setting;     // Assign the setting
        data.timestamp = timestamp; // Assign the timestamp
        data.channel_0 = result;    // Assign the sensor result

        buffer->push(data); // Push structured data to circular buffer
    }
}

void saveADSDataFromBuffer(const CircularBuffer<SingleChannel, bufferSize> &buffer, const String &filename, String header)
{
    if (xSemaphoreTake(sdCardMutex, portMAX_DELAY) == pdTRUE)
    {
        File myFile = SD.open(filename, FILE_APPEND); // Change here to FILE_APPEND
        if (!myFile)
        {
            Serial.println("Error opening file for writing");
            xSemaphoreGive(sdCardMutex);
            return;
        }
        if (myFile.size() == 0) // The header should only be written if the file was newly created or is empty
        {
            myFile.println(header); // Write header only if file is empty
        }
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const auto &data = buffer[i];
            myFile.printf("%d,%lu,%d\n", data.setting, data.timestamp, data.channel_0);
        }
        myFile.close();
        // Serial.println("Data dumped to: " + filename);
        xSemaphoreGive(sdCardMutex);
    }

    // File myFile = SD.open(filename, FILE_APPEND); // Change here to FILE_APPEND
    // if (!myFile)
    // {
    //     Serial.println("Error opening file for writing");
    //     return;
    // }
    // if (myFile.size() == 0) // The header should only be written if the file was newly created or is empty
    // {
    //     myFile.println(header); // Write header only if file is empty
    // }
    // for (size_t i = 0; i < buffer.size(); ++i)
    // {
    //     const auto &data = buffer[i];
    //     myFile.printf("%d,%lu,%d\n", data.setting, data.timestamp, data.channel_0);
    // }
    // myFile.close();
    // Serial.println("Data dumped to: " + filename);
}

void BME_sampling(CircularBuffer<BME680Data, bme_bufferSize> *buffer)
{
    if (bme.performReading()) // Perform a reading
    {
        unsigned long timestamp = millis(); // Get current timestamp
        BME680Data data;                    // Create a struct instance
        data.timestamp = timestamp;         // Assign the timestamp
        data.temperature = bme.temperature; // Assign the sensor result
        // Serial.print("BME680: Temperature: " + String(data.temperature));
        data.humidity = bme.humidity; // Assign the sensor result
        // Serial.print(", Humidity: " + String(data.humidity));
        data.pressure = bme.pressure; // Assign the sensor result
        // Serial.print(", Pressure: " + String(data.pressure));
        buffer->push(data); // Push structured data to circular buffer
    }
}
String BMEfilename;
void saveBME_DataFromBuffer(const CircularBuffer<BME680Data, bme_bufferSize> &buffer, const String &filename, String header)
{
    if (xSemaphoreTake(sdCardMutex, portMAX_DELAY) == pdTRUE)
    {
        File myFile = SD.open(filename, FILE_APPEND); // Change here to FILE_APPEND
        if (!myFile)
        {
            Serial.println("Error opening file for writing");
            xSemaphoreGive(sdCardMutex);
            return;
        }
        if (myFile.size() == 0) // The header should only be written if the file was newly created or is empty
        {
            myFile.println(header); // Write header only if file is empty
        }
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const auto &data = buffer[i];
            myFile.printf("%lu,%.2f,%.2f,%.2f\n", data.timestamp, data.temperature, data.humidity, data.pressure);
        }
        myFile.close();
        xSemaphoreGive(sdCardMutex);
    }
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

void BME_ENV_SavingTask(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification from the data collection task
        saveBME_DataFromBuffer(*saveBME_Buffer, BMEfilename, bme_Header);
        saveBME_Buffer->clear();
    }
}

void BME_ENV_loop(void *pvParameters)
{
    const unsigned long saveInterval = 10000; // 5 seconds in milliseconds
    unsigned long lastSaveTime = millis();
    Serial.println("BME_ENV_loop: Starting BME ENV Saving Task");
    TaskHandle_t BMEsavingTaskHandle; // Start the saving task
    xTaskCreate(BME_ENV_SavingTask, "BME ENV Saving Task", 4096, NULL, 1, &BMEsavingTaskHandle);

    for (;;)
    {
        // Wait for a notification to start data acquisition
        Serial.println("BME_ENV_loop: Waiting for notification to start data acquisition.");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        Serial.println("BME_ENV_loop: Start data acquisition.");
        currentBME_Buffer->clear();

        // Perform data acquisition continuously until notified to save data
        while (true)
        {
            BME_sampling(currentBME_Buffer);

            if (millis() - lastSaveTime >= saveInterval || currentBME_Buffer->size() >= bme_bufferSize)
            {
                switchBME_Buffers();
                lastSaveTime = millis();              // Reset the timer after saving
                xTaskNotifyGive(BMEsavingTaskHandle); // Notify the saving task to save the data
            }
            // Check if there's a notification to save data
            if (ulTaskNotifyTake(pdTRUE, 0)) // Check without waiting
            {
                Serial.println("BME_ENV_loop: Data saving notification received.");
                break;
            }
        }
        if (!currentBuffer->isEmpty())
        {
            switchBME_Buffers();
            xTaskNotifyGive(BMEsavingTaskHandle); // Notify the saving task to save the data
        }
        Serial.println("BME_ENV_loop: Data saving complete.");
        xTaskNotifyGive(samplingTask);
    }
}

void sampleADScontinuous(void *pvParameters)
{
    const unsigned long saveInterval = 5000; // 5 seconds in milliseconds
    unsigned long lastSaveTime = millis();
    TaskHandle_t savingTaskHandle; // Start the saving task
    xTaskCreate(dataSavingTask, "Data Saving Task", 4096, NULL, 1, &savingTaskHandle);

    for (;;)
    {
        // Wait for a notification to start data acquisition
        Serial.println("RUNNER: Waiting for notification to start data acquisition.");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Start of a new experiment
        Serial.println("RUNNER: Start data acquisition.");

        Serial.println("RUNNER: Give BME notification to start.");
        xTaskNotifyGive(BME_ENV_taskHandle);

        currentBuffer->clear();
        // Global variable version
        filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);
        BMEfilename = filename.substring(0, filename.length() - 4) + "_BME680.csv";

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
        if (!currentBuffer->isEmpty())
        {
            switchBuffers();
            xTaskNotifyGive(savingTaskHandle); // Notify the saving task to save the data
        }
        Serial.println("RUNNER: Give BME data saving signal");
        xTaskNotifyGive(BME_ENV_taskHandle); // Notify the BME task to save the data
        Serial.println("RUNNER: Waiting for BME data saving to complete.");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for a notification from the BME_ENV task
        Serial.println("RUNNER: Data saving complete. Notifying expLoopTask.");

        xTaskNotifyGive(expLoopTaskHandle);
    }
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