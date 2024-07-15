#ifndef EXP_SETUP_H
#define EXP_SETUP_H

#include <Arduino.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <FirebaseJson.h>

void readConfigCMD();
// void read_number_of_setups();
// void load_sd_json(const char *filename, String &configData);
void M5_SD_JSON(const char *filename, String &configData);
void BMEsampleTask();
void ADSsampleTask();
int sensorCheck();
void relayRange(int start, int end, int state);

extern uint8_t ADSi2c;

// Exp trackers
extern volatile int last_setup_tracker;
extern volatile int setup_tracker;
extern volatile int repeat_tracker;
extern volatile int channel_tracker;
extern volatile unsigned long startTime;
extern String exp_name;
extern String currentPath;

extern volatile int heatingTime;
extern std::vector<int> heaterSettings;

enum class SamplingType
{
    ADS_DETAIL,
    ADS,
    BME680
};

extern SamplingType samplingType;

// RTOS task handles
extern TaskHandle_t bmeTaskHandle, adsTaskHandle, expLoopTaskHandle;
void expTask();
void startExperimentTask(SamplingType samplingType);

// File system functions
std::vector<int> stringToArray(const std::string &str);
int getInt(FirebaseJson json, int setup_no, String target);
String getExpName(FirebaseJson json, int setup_no, String target);
std::vector<int> getArr(FirebaseJson json, int setup_no, String target);
int count_setup(String jsonString);
bool ensureDirectoryExists(String path);
String incrementFolder(String folderPath);
String createOrIncrementFolder(String folderPath);
String setupSave(int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name);

// ADS experiment functions
extern Adafruit_ADS1115 ads;
int UOM_sensorADS(std::unordered_map<int, std::vector<std::pair<unsigned long, std::array<int16_t, 4>>>> &ADS_sensorData, std::vector<int> heaterSettings, int heatingTime);
int UOM_sensorADS(std::unordered_map<int, std::vector<std::pair<unsigned long, std::array<int16_t, 4>>>> &ADS_sensorData, std::vector<int> heaterSettings, int heatingTime);
void sampleADS(void *pvParameters);
void sampleADScontinuous(void *pvParameters);
void saveADScontinuous(std::unordered_map<int, std::vector<std::pair<unsigned long, int16_t>>> &ADS_continuous, int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name);
void ADSsampleTask();
void saveADSData(std::unordered_map<int, std::vector<std::pair<unsigned long, std::array<int16_t, 4>>>> &ADS_sensorData, int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name);

// BME experiment functions
extern Adafruit_BME680 bme; // I2C

int UOM_sensorBME(std::unordered_map<int, std::vector<std::pair<unsigned long, uint32_t>>> &UOM_sensorData, std::vector<int> heaterSettings, int heatingTime);
void sampleBME(void *pvParameters);
void BMEsampleTask();
void saveUOMData(std::unordered_map<int, std::vector<std::pair<unsigned long, uint32_t>>> &UOM_sensorData, int setup_tracker, int repeat_tracker, int channel_tracker, String exp_name);

#endif // EXP_SETUP_H
