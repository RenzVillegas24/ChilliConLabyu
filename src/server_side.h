#include <Arduino.h>

#include <WiFi.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "esp_wifi.h"
#include <esp_task_wdt.h>


#include "display.h"
#include "defines.h"

String ssid = "ChiliConLabyu",
       pass = "hotchilli";

// Server side
const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 4, 4); // The default Android DNS
AsyncWebServer server(80);
DNSServer dnsServer;

// Server side functions
/*
void configServerSide();
void handleFormSubmit(AsyncWebServerRequest*);
String getContentType(String);
void init_serverFiles();
*/


// SERVER FUNCTIONS
void handleFormSubmit(AsyncWebServerRequest *request)
{
    if (request->hasArg("SSID") && request->hasArg("password"))
    {
        String ssidValue = request->arg("SSID");
        String passwordValue = request->arg("password");

        display.clearDisplay();
        display.setTextColor(1);
        drawStr("Connecting to WiFi", 0, 28, 1);
        display.display();

        Serial.println("Connecting to WiFi...");

        // Connect to WiFi using the captured SSID and password
        WiFi.begin(ssidValue.c_str(), passwordValue.c_str());

        // Wait for the connection to be established
        unsigned long startTime = millis();
        while (1)
        {
            Serial.print(".");
            delay(200);

            if (WiFi.status() == WL_CONNECTED)
            {

                Serial.println();
                Serial.println("WiFi connected!");
                Serial.print("SSID: ");
                Serial.println(WiFi.SSID());
                Serial.print("IP: ");
                Serial.println(WiFi.localIP());

                request->send(200); // Respond with 200 OK

                delay(1000);

                WiFi.softAPdisconnect(true);
                server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request)
                {
                    int moistureValue = analogRead(MOISTURE1);
                    int lightValue = analogRead(LIGHT1);

                    String response = "Moisture level: " + String(moistureValue) + "\n";
                    response += "Light intensity: " + String(lightValue);

                    request->send(200, "text/plain", response); 
                });

                break;
            }
            else if (WiFi.status() == WL_NO_SSID_AVAIL)
            {
                request->send(400, "text/plain", "SSID not found. Please enter a valid SSID.");
                break;
            }
            else if (WiFi.status() == WL_DISCONNECTED)
            {
                if (millis() - startTime > 5000)
                {
                    request->send(401, "text/plain", "Incorrect password. Please try again.");
                    break;
                }
            }
        }
    }
    else
    {
        request->send(404, "text/plain", "Bad Request");
    }
}

String getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    if (filename.endsWith(".png"))
        return "image/png";
    if (filename.endsWith(".gif"))
        return "image/gif";
    if (filename.endsWith(".jpg"))
        return "image/jpeg";
    if (filename.endsWith(".ico"))
        return "image/x-icon";
    if (filename.endsWith(".xml"))
        return "text/xml";
    if (filename.endsWith(".pdf"))
        return "application/x-pdf";
    if (filename.endsWith(".zip"))
        return "application/x-zip";
    if (filename.endsWith(".gz"))
        return "application/x-gzip";
    return "text/plain";
}

void init_serverFiles()
{
    server.onNotFound(
        [](AsyncWebServerRequest *request)
        {
            request->redirect("/setup/wificonnect.html");
        });
    server.on("/setup/wificonnect.html", HTTP_POST, handleFormSubmit);

    std::vector<String> fileList;
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
            fileList.push_back(file.path());

        Serial.println(file.path());
        file = root.openNextFile();
    }

    // Set up static routes for all files in the list
    for (const String &filePath : fileList)
    {
        const char *path = filePath.c_str();
        String contentType = getContentType(filePath);

        if (filePath == "/index.html")
            path = "/";

        server.on(path, HTTP_GET, [filePath](AsyncWebServerRequest *request)
                  {
      String contentType = getContentType(filePath);
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filePath, contentType);
      request->send(response);
      Serial.println(filePath); });
    }
}

void configLocalServer()
{
    dnsServer.processNextRequest();

    esp_task_wdt_init(30, false);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, pass);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    // if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);

    init_serverFiles();

    server.begin();
}

/*
  JOYSTICK VERSION
  Returns click button value in volts or index
  valVolts set as false by default.

  Values:
    INDEX |  VOLTS  |  DESCRIPTION                       |  INFO
    0     |  0v     |  No buttons are being pressed.     |  Stable
    1     |  1980   |  Select button are being pressed.  |  Slightly stable
    2     |  3300   |  Left button are being pressed.    |  Unstable
    3     |  3840   |  Up button are being pressed.      |  Slightly Stable

int btnClick(bool valVolts)
{
    while (true)
    {
        int btnX = analogRead(BTN_X), // Read in the button value
            btnY = analogRead(BTN_Y), // Read in the button value
            btnE = analogRead(BTN_E); // Read in the button value


        int btn = 0;
        if (btnE == 0)
            return 1;
        else if (btnX < 95)
            return LEFT;
        else if (4000 < btnX)
            return RIGHT;
        else if (btnY < 95)
            return TOP;
        else if (4000 < btnY)
            return BOTTOM;

        dnsServer.processNextRequest();

        return 0;
    }
}
*/

/*
  Returns click button value in volts or index
  valVolts set as false by default.

  Values:
    INDEX |  VOLTS  |  DESCRIPTION                       |  INFO
    0     |  4095   |  No buttons are being pressed.     |  Stable
    1     |  50     |  Select button are being pressed.  |  Stable
    2     |  520    |  Left button are being pressed.    |  Stable
    3     |  3220   |  Up button are being pressed.      |  Stable
    4     |  1220   |  Down button are being pressed.    |  Stable
    5     |  3750   |  Right button are being pressed.   |  Stable

*/
int btnClick(bool valVolts)
{

    while (true)
    {
        int btnV = analogRead(BUTTON); // Read in the button value
        if (valVolts)
            return btnV;

        const int adj = 100; // adjustment
        const int btnVal[] = {
            50,
            520,
            3220,
            1220,
            3750}; // button voltages {select, left, top, right, bottom}

        Serial.print("btnV: ");
        Serial.println(btnV);

        if (btnV >= btnVal[0] - adj && btnV <= btnVal[0] + adj)
            return 1;
        else if (btnV >= btnVal[1] - adj && btnV <= btnVal[1] + adj)
            return LEFT;
        else if (btnV >= btnVal[2] - adj && btnV <= btnVal[2] + adj)
            return TOP;
        else if (btnV >= btnVal[3] - adj && btnV <= btnVal[3] + adj)
            return RIGHT;
        else if (btnV >= btnVal[4] - adj && btnV <= btnVal[4] + adj)
            return BOTTOM;

        dnsServer.processNextRequest();
        return 0;
    }
}
