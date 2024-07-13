#include <Arduino.h>
#include <exp_setup.h>
#include <unordered_map>
#include <vector>
#include <SD.h> // SD card library but included in M5 library

int UOM_sensorBME(std::unordered_map<int, std::vector<std::pair<unsigned long, uint32_t>>> &UOM_sensorData, std::vector<int> heaterSettings, int heatingTime)
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
            unsigned long timestamp = millis() - startTime;
            UOM_sensorData[setting].push_back(std::make_pair(timestamp, bme.gas_resistance));
        }
        else
        {
            Serial.println("Failed to perform reading.");
        }
    }

    return 0;
}

void sampleBME(void *pvParameters)
{

    for (;;)
    {
        // continuous experiment
        std::unordered_map<int, std::vector<std::pair<unsigned long, uint32_t>>> UOM_sensorData;

        // Wait for a notification to start data acquisition
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Perform data acquisition continuously until notified to save data
        while (true)
        {
            UOM_sensorBME(UOM_sensorData, heaterSettings, heatingTime);
            Serial.print("+");

            // Check if there's a notification to save data
            if (ulTaskNotifyTake(pdTRUE, 0)) // Check without waiting
            {
                break;
            }
        }
        // Save the acquired data
        saveUOMData(UOM_sensorData, setup_tracker, repeat_tracker, channel_tracker, exp_name);
        // Notify the exp_loop task that data saving is complete
        xTaskNotifyGive(expLoopTaskHandle);
    }
}

void BMEsampleTask()
{
    xTaskCreate(
        sampleBME,       /* Task function. */
        "bme_start",     /* String with name of task. */
        10240,           /* Stack size in bytes. */
        NULL,            /* Parameter passed as input of the task */
        1,               /* Priority of the task. */
        &bmeTaskHandle); /* Task handle. */
}

void saveUOMData(std::unordered_map<int, std::vector<std::pair<unsigned long, uint32_t>>> &UOM_sensorData, int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name)
{
    String filename = setupSave(setup_tracker, repeat_tracker, channel_tracker, exp_name);
    if (filename == "")
    {
        Serial.println("Error in setupSave function.");
        return;
    }

    File myFile = SD.open(filename, FILE_WRITE);
    if (myFile)
    {
        myFile.println("Setting,Timestamp,Data");
        for (auto &entry : UOM_sensorData)
        {
            int setting = entry.first;
            for (auto &data : entry.second)
            {
                unsigned long timestamp = data.first; // Extract timestamp
                uint32_t gasResistance = data.second; // Extract gas resistance
                myFile.print(setting);
                myFile.print(",");
                myFile.print(timestamp);
                myFile.print(",");
                myFile.println(gasResistance);
            }
        }
        Serial.println("Data saved to file: " + String(filename));
        myFile.close();
    }
    else
    {
        Serial.println("Error opening file for writing");
    }
    UOM_sensorData.clear();
}