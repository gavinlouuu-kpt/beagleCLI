#include <exp_setup.h>
#include <Arduino.h>
#include <FirebaseJson.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <SD.h>
#include <M5Stack.h>

const int maxRetries = 10;
int errorCount = 0;

bool initializeSDCard()
{
    SD.end();   // Stop SPI communication before reinitializing
    delay(100); // Short delay to ensure SPI is stopped
    if (!SD.begin(TFCARD_CS_PIN, SPI, 40000000))
    {
        Serial.println("SD card initialization failed!");
        return false;
    }
    Serial.println("SD card initialization done.");
    return true;
}

bool checkAndRecoverSDCard()
{
    static int retryCount = 0;

    if (SD.exists("/"))
    {
        return true; // SD card is available
    }
    else
    {
        // Increment the error count
        retryCount++;

        // Try to reinitialize SD card if opening file fails
        if (retryCount < maxRetries)
        {
            Serial.print("Retrying SD card initialization. Attempt: ");
            Serial.println(retryCount);
            if (initializeSDCard())
            {
                retryCount = 0; // Reset retry count after successful initialization
                return true;
            }
        }
        else
        {
            Serial.println("Max retries reached. SD card operation failed.");
            return false;
        }
    }

    return false;
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
    char currentTime[20];
    strftime(currentTime, sizeof(currentTime), "%Y%m%d%H%M%S", &timeinfo);

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

String setupSaveJSON(int setup_tracker, String exp_name)
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

    return currentPath;
}

void saveJSON(String filename, FirebaseJson json)
{
    filename = filename + "/expSetup.json";
    // Ensure the directory exists
    int lastSlashIndex = filename.lastIndexOf('/');
    if (lastSlashIndex != -1)
    {
        String directory = filename.substring(0, lastSlashIndex);
        if (!SD.exists(directory))
        {
            Serial.println("Directory does not exist, creating: " + directory);
            if (!SD.mkdir(directory))
            {
                Serial.println("Failed to create directory: " + directory);
                return;
            }
        }
    }

    File file = SD.open(filename.c_str(), FILE_WRITE);
    Serial.println("Saving to: " + filename);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }

    String jsonString;
    json.toString(jsonString);
    file.println(jsonString);
    file.close();
    Serial.println("File saved successfully");
}