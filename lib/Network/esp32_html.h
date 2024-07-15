#ifndef ESP32_HTML_H
#define ESP32_HTML_H

const char *homePage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Home Page</title>
    <style>
        body { font-family: 'Arial', sans-serif; margin: 20px; }
        h1 { color: #333; }
    </style>
</head>
<body>
    <h1>Welcome to ESP32 Web Server</h1>
    <p>This is the home page of your ESP32. Use the links below to navigate:</p>
    <ul>
        <li><a href="/files">View Files</a></li>
    </ul>
</body>
</html>
)rawliteral";

#endif // ESP32_HTML_H