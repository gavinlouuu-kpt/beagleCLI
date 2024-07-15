#include <Arduino.h>
#include <Network.h>
#include <WiFi.h>
#include <time.h>
#include <Init.h>
// #include <LittleFS.h>
#include <FirebaseJson.h>
#include <map>
#include <beagleCLI.h>
// #include <SD.h>
#include <ArduinoOTA.h>

#include <M5Stack.h>
#include <esp32_html.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// #include <WebServer.h>

TaskHandle_t ntCheckTaskHandler;

AsyncWebServer server(80);

String urlEncode(const String &str)
{
    String encodedString = "";
    char c;
    char code0;
    char code1;
    for (unsigned int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);
        if (c == ' ')
        {
            encodedString += '+';
        }
        else if (isalnum(c))
        {
            encodedString += c;
        }
        else
        {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9)
            {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9)
            {
                code0 = c - 10 + 'A';
            }
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }
    }
    return encodedString;
}

String urlDecode(const String &str)
{
    String decodedString = "";
    char c;
    char code0;
    char code1;
    for (unsigned int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);
        if (c == '+')
        {
            decodedString += ' ';
        }
        else if (c == '%')
        {
            i++;
            code0 = str.charAt(i);
            i++;
            code1 = str.charAt(i);
            if (code0 >= '0' && code0 <= '9')
            {
                code0 = code0 - '0';
            }
            else if (code0 >= 'A' && code0 <= 'F')
            {
                code0 = code0 - 'A' + 10;
            }
            if (code1 >= '0' && code1 <= '9')
            {
                code1 = code1 - '0';
            }
            else if (code1 >= 'A' && code1 <= 'F')
            {
                code1 = code1 - 'A' + 10;
            }
            decodedString += static_cast<char>(code0 * 16 + code1);
        }
        else
        {
            decodedString += c;
        }
    }
    return decodedString;
}

void handleHomePage(AsyncWebServerRequest *request)
{
    Serial.println("Home page requested");
    request->send_P(200, "text/html", homePage);
}

void listSDdir(AsyncWebServerRequest *request, const String &rawPath)
{
    String path = urlDecode(rawPath); // Decode the path first
    Serial.println(path);

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory())
    {
        request->send(404, "text/plain", "Directory not found");
        return;
    }

    String html = "<html><body><h1>Directory: " + path + "</h1><ul>";
    File file = dir.openNextFile();
    while (file)
    {
        String fileName = file.name();
        String filePath = path + (path.endsWith("/") ? "" : "/") + fileName;
        if (file.isDirectory())
        {
            html += "<li><a href='/?dir=" + urlEncode(filePath) + "'>[Dir] " + fileName + "</a></li>";
        }
        else
        {
            html += "<li><a href='/download?file=" + urlEncode(filePath) + "'>" + fileName + "</a></li>";
        }
        file = dir.openNextFile();
    }
    html += "</ul><a href='/?dir=/'>Home</a></body></html>";
    request->send(200, "text/html", html);
}

void handleFS(AsyncWebServerRequest *request)
{
    if (request->hasParam("dir"))
    {
        String path = request->getParam("dir")->value();
        Serial.println(path);
        listSDdir(request, path); // Pass the raw path directly
    }
    else
    {
        listSDdir(request, "/"); // Start from the root directory
    }
}

// void handleFileRead()
// {
//     String path = server.uri();
//     File file = SD.open(path, FILE_READ);
//     if (!file)
//     {
//         server.send(404, "text/plain", "File not found");
//         return;
//     }
//     if (path.endsWith(".json"))
//     {
//         String fileContent = "";
//         while (file.available())
//         {
//             fileContent += char(file.read());
//         }
//         // Adjust the form action and ensure correct path handling
//         String html = "<form action='/edit' method='POST'>";
//         html += "<input type='hidden' name='path' value='" + path + "'>"; // Ensure this carries the correct path
//         html += "<textarea name='content' style='width:100%;height:200px;'>" + fileContent + "</textarea>";
//         html += "<button type='submit'>Save</button>";
//         html += "</form>";
//         server.send(200, "text/html", html);
//     }
//     else
//     {
//         server.send(200, "text/plain", "File format not supported for editing.");
//     }
//     file.close();
// }

// void handleFileRead(AsyncWebServerRequest *request)
// {
//     if (!request->hasParam("file"))
//     {
//         request->send(400, "text/plain", "Request does not contain file parameter.");
//         return;
//     }

//     AsyncWebParameter *p = request->getParam("file");
//     String path = p->value(); // Get the file path from request parameters

//     File file = SD.open(path, FILE_READ);
//     if (!file)
//     {
//         request->send(404, "text/plain", "File not found");
//         file.close();
//         return;
//     }

//     if (path.endsWith(".json"))
//     {
//         String fileContent = "";
//         while (file.available())
//         {
//             fileContent += char(file.read());
//         }

//         // Adjust the form action and ensure correct path handling
//         String html = "<form action='/edit' method='POST'>";
//         html += "<input type='hidden' name='path' value='" + urlEncode(path) + "'>"; // URL encode the path to ensure it is correctly interpreted by the browser
//         html += "<textarea name='content' style='width:100%;height:200px;'>" + fileContent + "</textarea>";
//         html += "<button type='submit'>Save</button>";
//         html += "</form>";
//         request->send(200, "text/html", html);
//     }
//     else
//     {
//         request->send(200, "text/plain", "File format not supported for editing.");
//     }
//     file.close();
// }

// void handleNotFound()
// {
//     // You can add custom logic to handle different cases of not found
//     String path = server.uri();

//     // Attempt to handle file reading if the path looks like a file request
//     if (path.startsWith("/sd/") || path.endsWith(".json"))
//     {                     // Customize this condition based on your file structure
//         handleFileRead(); // Attempt to handle as file read
//     }
//     else
//     {
//         // If it does not seem to be a file access attempt, handle as a general 404 not found
//         Serial.print("Unmatched request: ");
//         Serial.println(path);
//         server.send(404, "text/plain", "404: Not Found");
//     }
// }

// void handleEdit()
// {
//     String path = server.arg("path"); // This should give the correct path e.g., "/sd/expSetup.json"
//     File file = SD.open(path, FILE_WRITE);
//     if (!file)
//     {
//         server.send(500, "text/plain", "Failed to open file for writing");
//         return;
//     }
//     String content = server.arg("content");
//     file.println(content);
//     file.close();
//     server.send(200, "text/plain", "File saved successfully");
// }

void handleFileAccess(AsyncWebServerRequest *request)
{
    if (!request->hasParam("file"))
    {
        request->send(400, "text/plain", "Bad Request - No file specified");
        return;
    }

    AsyncWebParameter *p = request->getParam("file");
    String filePath = p->value();

    if (filePath.endsWith(".json"))
    {
        // Handle JSON file by reading and preparing for editing
        File file = SD.open(filePath, FILE_READ);
        if (!file)
        {
            request->send(404, "text/plain", "File not found");
            return;
        }

        String fileContent = "";
        while (file.available())
        {
            fileContent += char(file.read());
        }
        file.close();

        String html = "<!DOCTYPE html><html><body>"
                      "<h1>Edit JSON File</h1>"
                      "<form action='/save' method='post'>"
                      "<textarea name='content' rows='20' cols='80'>" +
                      fileContent + "</textarea><br>"
                                    "<input type='hidden' name='path' value='" +
                      filePath + "'>"
                                 "<input type='submit' value='Save Changes'>"
                                 "</form>"
                                 "</body></html>";
        request->send(200, "text/html", html);
    }
    else
    {
        // Handle other file types by offering a download
        File file = SD.open(filePath, FILE_READ);
        if (!file)
        {
            request->send(404, "text/plain", "File not found");
            return;
        }

        request->send(SD, filePath, "application/octet-stream", true);
        file.close();
    }
}

void handleEdit(AsyncWebServerRequest *request)
{
    // Check if both required parameters are present
    if (!request->hasParam("path", true) || !request->hasParam("content", true))
    {
        request->send(400, "text/plain", "Missing data");
        return;
    }

    AsyncWebParameter *pPath = request->getParam("path", true);
    AsyncWebParameter *pContent = request->getParam("content", true);

    String path = pPath->value();
    String content = pContent->value();

    // Open the file for writing
    File file = SD.open(path, FILE_WRITE);
    if (!file)
    {
        request->send(500, "text/plain", "Failed to open file for writing");
        return;
    }

    // Write the content to the file
    file.print(content); // Using print instead of println to avoid extra new line at the end
    file.close();

    String html = "<!DOCTYPE html><html><head><title>Save Successful</title></head><body>"
                  "<h1>File saved successfully</h1>"
                  "<p><a href='/files'>Return to File List</a></p>" // Link back to the file list
                  "</body></html>";

    // Send a response back to the client
    request->send(200, "text/plain", html);
}

// void handleFileDownload()
// {
//     if (server.hasArg("file"))
//     {
//         String path = server.arg("file");
//         File file = SD.open(path, FILE_READ);
//         if (!file)
//         {
//             server.send(404, "text/plain", "File not found");
//             return;
//         }

//         if (file.isDirectory())
//         {
//             file.close();
//             server.send(500, "text/plain", "Cannot download a directory");
//             return;
//         }

//         String fileName = path.substring(path.lastIndexOf("/") + 1);

//         // Check if the file is a JSON file
//         if (fileName.endsWith(".json"))
//         {
//             String fileContent = "";
//             while (file.available())
//             {
//                 fileContent += (char)file.read();
//             }
//             file.close();

//             String html = "<!DOCTYPE html><html><body>"
//                           "<h1>Edit JSON File</h1>"
//                           "<form action='/save' method='post'>"
//                           "<textarea name='content' rows='20' cols='80'>" +
//                           fileContent + "</textarea><br>"
//                                         "<input type='hidden' name='path' value='" +
//                           path + "'>"
//                                  "<input type='submit' value='Save Changes'>"
//                                  "</form>"
//                                  "</body></html>";
//             server.send(200, "text/html", html);
//         }
//         else
//         {
//             // Normal file download process
//             server.setContentLength(file.size());
//             server.sendHeader("Content-Type", "application/octet-stream");
//             server.sendHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
//             server.send(200, "application/octet-stream", ""); // Sends headers
//             WiFiClient client = server.client();

//             byte buffer[128];
//             while (file.available())
//             {
//                 int l = file.read(buffer, 128);
//                 client.write(buffer, l);
//             }
//             file.close();
//         }
//     }
//     else
//     {
//         server.send(400, "text/plain", "Bad Request");
//     }
// }

void serverSetup()
{

    server.on("/home", HTTP_GET, handleHomePage);
    server.on("/", HTTP_GET, handleFS);
    // server.on("/readFile", HTTP_GET, handleFileRead);
    // server.on("/edit", HTTP_POST, handleEdit);
    server.on("/save", HTTP_POST, handleEdit);
    server.on("/download", HTTP_GET, handleFileAccess);
    // server.onNotFound(handleNotFound);
    server.begin();
}

void saveWIFICredentialsToSD(const char *ssid, const char *password)
{
    FirebaseJson json;

    // Assuming you want to overwrite existing credentials with new ones
    json.set("/" + String(ssid), String(password));

    // Open file for writing
    File file = SD.open("/wifiCredentials.json", "w");
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }

    // Serialize JSON to file
    String jsonData;
    json.toString(jsonData, true);
    file.print(jsonData);

    // Close the file
    file.close();
}

String loadWIFICredentialsFromSD(String ssid)
{
    if (!SD.begin())
    {
        Serial.println("An Error has occurred while mounting SD");
        return "";
    }

    File file = SD.open("/wifiCredentials.json", "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return "";
    }

    String jsonData = file.readString();
    file.close();

    FirebaseJson json;
    json.setJsonData(jsonData);

    FirebaseJsonData jsonDataObj; // Object to store the retrieved data
    String path = "/" + ssid;     // Construct the path dynamically based on the SSID

    // Use the correct method signature for get()
    if (json.get(jsonDataObj, path.c_str()))
    { // Make sure to use path.c_str() to pass a const char* type
        if (jsonDataObj.typeNum == FirebaseJson::JSON_OBJECT || jsonDataObj.typeNum == FirebaseJson::JSON_STRING)
        {
            // Assuming password is stored as a plain string
            String password = jsonDataObj.stringValue;
            Serial.println("SSID: " + ssid + ", Password: " + password);
            return password;
            // Optionally, connect to WiFi here or handle as needed
        }
        else
        {
            Serial.println("Invalid format for password");
            return "";
        }
    }
    else
    {
        // Serial.println("SSID not found in stored credentials");
        return "";
    }
}

void arduinoOTAsetup()
{

    ArduinoOTA
        .onStart([]()
                 {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
        .onEnd([]()
               { Serial.println("\nEnd"); })
        .onProgress([](unsigned int progress, unsigned int total)
                    { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
        .onError([](ota_error_t error)
                 {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      } });

    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

void backgroundWIFI()
{
    // Ensure Wi-Fi is on and set to STA mode
    WiFi.mode(WIFI_STA);

    // Start scanning for networks
    Serial.println("Scanning for Wi-Fi networks...");
    int networkCount = WiFi.scanNetworks();
    if (networkCount == 0)
    {
        Serial.println("No networks found.");
        return;
    }

    // Variables to keep track of the best network
    String bestSSID;
    int bestRSSI = -1000; // Start with a very low RSSI

    // Iterate over found networks
    for (int i = 0; i < networkCount; i++)
    {
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);

        // Load credentials for the current SSID
        String password = loadWIFICredentialsFromSD(ssid);
        if (password != "" && rssi > bestRSSI)
        {
            // Found a better candidate, update tracking variables
            bestSSID = ssid;
            bestRSSI = rssi;
        }
    }

    // If we found a suitable network, attempt to connect
    if (bestSSID != "")
    {
        Serial.println("Connecting to " + bestSSID);
        WiFi.begin(bestSSID.c_str(), loadWIFICredentialsFromSD(bestSSID).c_str());

        // Wait for connection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20)
        { // 20 attempts, adjust as needed
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            // knownWIFI = true;
            Serial.println("\nConnected to " + bestSSID);
            Serial.println("IP address: " + WiFi.localIP().toString());
            configTime(gmtOffset_sec, daylightOffset_sec, "time.nist.gov", "hk.pool.ntp.org", "asia.pool.ntp.org");
            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.println("SSID:" + bestSSID);
            M5.Lcd.print("IP address: ");
            M5.Lcd.println(WiFi.localIP());
            arduinoOTAsetup();
            serverSetup();

            //   firebaseSetup();
        }
        else
        {
            Serial.println("\nFailed to connect to any known network");
        }
    }
    else
    {
        Serial.println("No known networks found.");
        // knownWIFI = false;
    }

    // Clean up after scanning
    WiFi.scanDelete();
}

// bool knownWIFI = false;

void wifiCheckTask(void *pvParameters)
{
    for (;;)
    {
        // check if wifiCredentials.json exists
        if (SD.exists("/wifiCredentials.json"))
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                ArduinoOTA.handle();

                // server.handleClient(); // conventional webserver

                //   updateLocalTime(); // Update time every minute if background WIFI managed
                //   fbKeepAlive();
            }
            else
            {
                backgroundWIFI();
                // should move backgroundWIFI to a separate function so it is not looped
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // Check every 10 seconds
    }
}

void networkCheck()
{
    xTaskCreate(
        wifiCheckTask,        /* Task function */
        "WiFiCheckTask",      /* Name of the task */
        10240,                /* Stack size */
        NULL,                 /* Task input parameter */
        1,                    /* Priority of the task */
        &ntCheckTaskHandler); /* Task handle */
}

WiFiManager::WiFiManager() {}

void WiFiManager::ManageWIFI()
{
    this->scanNetworks();

    if (WiFi.status() != WL_CONNECTED)
    {
        String ssid = this->selectNetwork();
        if (ssid != "")
        {
            String password = this->inputPassword();
            if (this->connectToWiFi(ssid, password))
            {
                saveWIFICredentialsToSD(ssid.c_str(), password.c_str());
            }
            else
            {
                Serial.println("Failed to connect to WiFi.");
            }
        }
    }
    else
    {
        Serial.println("Already connected to a WiFi network.");
    }
}

void WiFiManager::scanNetworks()
{
    Serial.println("Scanning WiFi networks...");
    M5.Lcd.println("Scanning WiFi networks...");
    int networkCount = WiFi.scanNetworks();
    if (networkCount == 0)
    {
        Serial.println("No networks found. Try again.");
        return; // Early return if no networks are found
    }
    else
    {
        Serial.print(networkCount);
        Serial.println(" networks found:");
        for (int i = 0; i < networkCount; i++)
        {
            // Print details of each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(" dBm) ");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "[OPEN]" : "[SECURE]");
        }
    }
}

String WiFiManager::selectNetwork()
{
    int attempt = 0;
    int maxAttempts = 3; // Maximum number of attempts to find the network
    while (attempt < maxAttempts)
    {
        Serial.println("Enter the number of the network you want to connect to (or 0 to rescan):");
        while (!Serial.available())
            ;
        int networkNum = Serial.parseInt();

        if (networkNum == 0)
        {
            scanNetworks(); // Rescan networks
            attempt++;
            continue;
        }

        if (networkNum < 0 || networkNum > WiFi.scanNetworks())
        {
            Serial.println("Invalid network number. Please try again.");
        }
        else
        {
            return WiFi.SSID(networkNum - 1); // Return the selected SSID
        }
    }
    Serial.println("Maximum attempts reached. Please restart the process.");
    return ""; // Return empty string if max attempts reached without selection
}

String WiFiManager::inputPassword()
{
    Serial.println("Enter password:");
    while (!Serial.available())
        ;
    String password = Serial.readStringUntil('\n');
    password.trim(); // Remove any whitespace or newline characters
    return password;
}

bool WiFiManager::connectToWiFi(const String &ssid, const String &password)
{
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();  // Get the current time
    const unsigned long timeout = 30000; // Set timeout duration (e.g., 30000 milliseconds or 30 seconds)

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if (millis() - startTime >= timeout)
        { // Check if the timeout has been reached
            Serial.println("\nConnection Timeout!");
            return false; // Return false if the timeout is reached
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nConnected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        long hkGMToffset = 28800;
        long daylightOffset_sec = 0;
        configTime(hkGMToffset, daylightOffset_sec, "hk.pool.ntp.org", "asia.pool.ntp.org", "time.nist.gov"); // Initialize NTP
        return true;                                                                                          // Return true if connected
    }
    else
    {
        Serial.println("\nFailed to connect. Please try again.");
        return false; // Return false if not connected
    }
}

void networkState()
{
    const char *hostname = "www.google.com";
    IPAddress resolvedIP;

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi connection lost.");
    }
    else
    {
        Serial.println("WiFi connected.");
        if (WiFi.hostByName(hostname, resolvedIP))
        {
            Serial.print("IP Address for ");
            Serial.print(hostname);
            Serial.print(" is: ");
            Serial.println(resolvedIP);
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.print("DNS Failed for ");
            Serial.println(hostname);
        }
    }
}

void networkCMD()
{
    commandMap["net"] = []()
    { networkState(); };
    commandMap["wifi"] = []()
    { WiFiManager wifiManager; wifiManager.ManageWIFI(); };
}