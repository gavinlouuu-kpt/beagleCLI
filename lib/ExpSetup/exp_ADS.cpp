#include <Arduino.h>
#include <exp_setup.h>
#include <unordered_map>
#include <vector>
#include <SD.h> // SD card library but included in M5 library
// #include <Adafruit_ADS1X15.h>
#include <pinConfig.h>

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

void UOM_ADS_continuous(std::unordered_map<int, std::vector<std::pair<unsigned long, int16_t>>> &ADS_continuous, std::vector<int> heaterSettings, int heatingTime)
{
    // set continuous mode
    for (int setting : heaterSettings)
    {
        ledcWrite(PWM_Heater, setting);

        unsigned long timestamp = millis(); // relative timestamp has a resetting bug that needs to be fixed
        int16_t results = ads.getLastConversionResults();
        ADS_continuous[setting].emplace_back(timestamp, results);
    }
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

            ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, true); // Continuous mode
            // UOM_ADS_continuous(ADS_continuous, heaterSettings, heatingTime);

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

void sampleADScontinuous(void *pvParameters)
{
    for (;;)
    {
        // continuous experiment
        std::unordered_map<int, std::vector<std::pair<unsigned long, int16_t>>> ADS_continuous;

        // Wait for a notification to start data acquisition
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Perform data acquisition continuously until notified to save data
        while (true)
        {

            ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, true); // Continuous mode
            UOM_ADS_continuous(ADS_continuous, heaterSettings, heatingTime);
            Serial.print("*");

            // Check if there's a notification to save data
            if (ulTaskNotifyTake(pdTRUE, 0)) // Check without waiting
            {
                break;
            }
        }

        // continuous exp
        saveADScontinuous(ADS_continuous, setup_tracker, repeat_tracker, channel_tracker, exp_name);

        // Notify the exp_loop task that data saving is complete
        xTaskNotifyGive(expLoopTaskHandle);
    }
}

void saveADScontinuous(std::unordered_map<int, std::vector<std::pair<unsigned long, int16_t>>> &ADS_continuous, int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name)
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
    String buffer = "Setting,Timestamp,Value\n";

    // Accumulate data into the buffer
    for (auto &entry : ADS_continuous)
    {
        int setting = entry.first;
        for (auto &data : entry.second)
        {
            buffer += String(setting) + ",";
            buffer += String(data.first) + ",";   // Timestamp
            buffer += String(data.second) + "\n"; // Single value for this timestamp

            // Check if buffer needs to be flushed
            if (buffer.length() >= 1024)
            { // Choose a suitable size for flushing
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
    ADS_continuous.clear(); // Clear the data after saving
}

void ADSsampleTask()
{
    xTaskCreate(
        sampleADS,       /* Task function. */
        "ads_start",     /* String with name of task. */
        20480,           /* Stack size in bytes. */
        NULL,            /* Parameter passed as input of the task */
        1,               /* Priority of the task. */
        &adsTaskHandle); /* Task handle. */
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
