#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <qrcode.h>
#include <vector>

#include <WiFi.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "esp_wifi.h"
#include <esp_task_wdt.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// OLED pins [GND, VCC - 3V3, SCL - D22, SDA - D21]
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
// #define BTN_NAV 4
#define BTN_X 34
#define BTN_Y 39
#define BTN_E 36

#define LEFT 75
#define TOP 76
#define RIGHT 77
#define BOTTOM 78

String ssid = "ChiliConLabyu",
       pass = "hotchilli";

// Server side
const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 4, 4); // The default Android DNS
AsyncWebServer server(80);
DNSServer dnsServer;

QRCode qrcode;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int setVals[] = {
    2,
    4,
    2};

int formSubmitDone = 0;

int btnClick(bool valVolts = false);
void wait(bool reverse = false);
void drawStr(String buf, int x, int y, int align = 0);
void drawArrow(int rotation, int x, int y, int size, uint16_t color);
int drawMenu(String header, std::vector<String> items, bool hasBack = true, int align = 1);
int drawDialog(String header, std::vector<String> message, std::vector<String> items, bool hasBack = true, int align = 1);
int drawSetting(String header, std::vector<String> settingsOrder, std::map<String, std::vector<String>> settings, int currentPage, int currentValue, int selectedY = 0, bool hasBack = true);
void drawQrCode(String qrStr, String *lines);
void handleFormSubmit(AsyncWebServerRequest *request);
String getContentType(String filename);
void init_serverFiles();
void setupServerForConnectedWiFi();
int MainMenu();
int Settings(int page, int curVal, int selY = 0);
int ConnectLocal();
int Pair();

/*
  Returns click button value in volts or index
  valVolts set as false by default.

  Values:
    INDEX |  VOLTS  |  DESCRIPTION                       |  INFO
    0     |  0v     |  No buttons are being pressed.     |  Stable
    1     |  1980   |  Select button are being pressed.  |  Slightly stable
    2     |  3300   |  Left button are being pressed.    |  Unstable
    3     |  3840   |  Up button are being pressed.      |  Slightly Stable
    4     |  2610   |  Down button are being pressed.    |  Slightly Stable
    5     |  2270   |  Right button are being pressed.   |  Stable

int btnClick(bool valVolts = false)
{

  int btnV = analogRead(BTN_NAV); // Read in the button value
  if (valVolts)
    return btnV;

  const int adj  = 50; // adjustment
  const int btnVal[]  = {
      1980,
      3300,
      3840,
      2610,
      2270}; // button voltages {select, left, top, bottom, right}

  // Serial.println(btnV);
  if (btnV >= btnVal[0] - adj && btnV <= btnVal[0] + adj)
    return 1;
  else if (btnV >= btnVal[1] - adj && btnV <= btnVal[1] + adj)
    return LEFT;
  else if (btnV >= btnVal[2] - adj && btnV <= btnVal[2] + adj)
    return TOP;
  else if (btnV >= btnVal[3] - adj && btnV <= btnVal[3] + adj)
    return BOTTOM;
  else if (btnV >= btnVal[4] - adj && btnV <= btnVal[4] + adj)
    return RIGHT;
  else
    return 0;
}
*/

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
*/
int btnClick(bool valVolts )
{
  while (true)
  {
    int btnX = analogRead(BTN_X), // Read in the button value
        btnY = analogRead(BTN_Y), // Read in the button value
        btnE = analogRead(BTN_E); // Read in the button value

    /*
    Serial.print(btnX);
    Serial.print(" ");
    Serial.print(btnY);
    Serial.print(" ");
    Serial.println(btnE);
    */

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

void wait(bool reverse )
{
  if (reverse)
    while (btnClick() == 0)
      delay(1);
  else
    while (btnClick() != 0)
      delay(1);
}


// DRAW FUNCTIONS

/*
  Alignment:
    Left = 0
    Center = 1
    Right = 2

*/
void drawStr(String buf, int x, int y, int align )
{
  // SCREEN_WIDTH / CHAR_PIXEL = MAX_LEN
  // 128 / 6 = 21.3333...
  int maxLen = 21;

  if (align == 1)
    x += int(SCREEN_WIDTH / 2 - (buf.length() * 6) / 2);
  else if (align == 2)
    x += maxLen - buf.length();

  display.setCursor(x, y);
  display.print(buf);
}

void drawArrow(int rotation, int x, int y, int size, uint16_t color)
{

  if (rotation == LEFT)
  {
    display.drawLine(x + size, y + size, x, y, color);
    display.drawLine(x + size, y - size, x, y, color);
  }
  else if (rotation == TOP)
  {
  }
  else if (rotation == RIGHT)
  {
    display.drawLine(x, y + size, x + size, y, color);
    display.drawLine(x, y - size, x + size, y, color);
  }
  else if (rotation == BOTTOM)
  {
  }
}

int drawMenu(String header,
             std::vector<String> items,
             bool hasBack ,
             int align )
{
  int selectedY = 0,
      iSz = items.size();
  while (true)
  {
    int itmLen = items[selectedY].length(),
        btnEvent = btnClick(),
        itmPx = (itmLen * 5) + itmLen - 1 + 16,
        selL = (SCREEN_WIDTH - (itmPx)) / 2;

    if (btnEvent == 0){
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      drawStr(header, 0, 7, align);

      if (hasBack)
      {
        drawArrow(LEFT, 11, 10, 3, SSD1306_WHITE);
        if (selectedY == -1)
          display.drawRoundRect(5, 4, 17, 13, 4, SSD1306_WHITE);
      }

      if (selectedY != -1)
        display.drawRoundRect(selL, 21 + 12 * selectedY, itmPx, 13, 4, SSD1306_WHITE);

      for (int i = 0; i < iSz; i++)
        drawStr(items[i], 0, 24 + (12 * i), align);

      display.display();
      wait(1);
    }
    else if (btnEvent == 1)
    {
      if (selectedY == -1 || selectedY == iSz)
      {
        display.fillRoundRect(5, 4, 17, 13, 4, SSD1306_WHITE);
        drawArrow(LEFT, 11, 10, 3, SSD1306_BLACK);
      }
      else
      {
        display.fillRoundRect(selL, 21 + 12 * selectedY, itmPx, 13, 4, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        drawStr(items[selectedY], 0, 24 + (12 * selectedY), align);
      }
      display.display();

      wait(0);
      delay(100);
      return selectedY;
    } else {
      if (btnEvent == TOP)
        selectedY--; // top
      else if (btnEvent == BOTTOM)
        selectedY++; // bottom

      wait(0);
    }
    
    if (hasBack)
    {
      if (selectedY == -2)
        selectedY = 2;
      else if (selectedY == iSz)
        selectedY = -1;
    }
    else
    {
      if (selectedY == -1)
        selectedY = iSz - 1;
      else if (selectedY == iSz)
        selectedY %= iSz;
    }

  }
}

int drawDialog(String header,
               std::vector<String> message,
               std::vector<String> items,
               bool hasBack ,
               int align )
{
  int selectedX = 0,
      iSz = items.size(),
      mSz = message.size();

    int xStart = 0;
    for (int i = 0; i < iSz; i++)
    {
      int iLen = items[i].length(),
          iPx = (iLen * 5) + iLen - 1 + 16;
      xStart += iPx;
    }
    xStart = (SCREEN_WIDTH - (xStart)) / 2;

    int curX;
  while (true)
  {
    int btnEvent = btnClick(),
        itmLen = items[selectedX].length(),
        itmPx = (itmLen * 5) + itmLen - 1 + 16,
        selL = (SCREEN_WIDTH - (itmPx)) / 2;

    if (btnEvent == 0)
    {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      drawStr(header, 0, 7, 1);

      for (int i = 0; i < mSz; i++)
        drawStr(message[i], 0, 21 + (10 * i), align);

      if (hasBack)
      {
        drawArrow(LEFT, 11, 10, 3, SSD1306_WHITE);
        if (selectedX == -1)
          display.drawRoundRect(5, 4, 17, 13, 4, SSD1306_WHITE);
      }
      curX = xStart;
      for (int i = 0; i < iSz; i++)
      {
        int iLen = items[i].length(),
            iPx = (iLen * 5) + iLen - 1 + 16;

        drawStr(items[i], curX + 8, 48, 0);

        if (selectedX == i)
          display.drawRoundRect(curX, 45, iPx, 13, 4, SSD1306_WHITE);

        curX += iPx;
      }

      display.display();
      wait(1);
    }
    else if (btnEvent == 1)
    {
      if (selectedX == -1 || selectedX == iSz)
      {
        display.fillRoundRect(5, 4, 17, 13, 4, SSD1306_WHITE);
        drawArrow(LEFT, 11, 10, 3, SSD1306_BLACK);
      }
      else
      {
        curX = xStart;
        for (int i = 0; i < selectedX; i++)
        {
          int iLen = items[i].length();
          curX += (iLen * 5) + iLen - 1 + 16;
        }

        display.fillRoundRect(curX, 45, itmPx, 13, 4, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        drawStr(items[selectedX], curX + 8, 48, 0);
      }
      display.display();

      wait(0);
      delay(100);
      return selectedX;
    }
    else
    {
      if (btnEvent == LEFT)
        selectedX--; // left
      else if (btnEvent == RIGHT)
        selectedX++; // right
      wait(0);
      break;
    }

    if (hasBack)
    {
      if (selectedX == -2)
        selectedX = 2;
      else if (selectedX == iSz)
        selectedX = -1;
    }
    else
    {
      if (selectedX == -1)
        selectedX = iSz - 1;
      else if (selectedX == iSz)
        selectedX %= iSz;
    }
  }
}

int drawSetting(String header,
                std::vector<String> settingsOrder,
                std::map<String, std::vector<String>> settings,
                const int currentPage,
                const int currentValue,
                int selectedY ,
                bool hasBack )
{

  int selectedX = currentValue,
      sSz = settings.size(),
      iSz = 2;
  while (true)
  {
    int btnEvent = btnClick();

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    drawStr(header, 0, 7, 1);

    auto curr = settingsOrder[currentPage];
    drawStr(curr, 0, 21, 1);

    if (hasBack)
    {
      drawArrow(LEFT, 11, 10, 3, SSD1306_WHITE);
      if (selectedY == -1)
        display.drawRoundRect(5, 4, 17, 13, 4, SSD1306_WHITE);
    }

    if (selectedY == 1)
    {
      display.drawRoundRect(15, 48, 98, 13, 4, SSD1306_WHITE);
    }

    const int topScrl = 54;
    if (currentPage != sSz - 1)
    {
      drawArrow(RIGHT, 103, topScrl, 2, SSD1306_WHITE);
    }
    if (currentPage != 0)
    {
      drawArrow(LEFT, 25, topScrl, 2, SSD1306_WHITE);
    }

    int cSz = settings[curr].size(),
        pos = (SCREEN_WIDTH / cSz);
    for (int k = 0; k < cSz; k++)
    {
      int curChSz = settings[curr][k].length(),
          curChPx = (curChSz * 5) + curChSz - 1,
          curPos = (pos * (k + 1)),
          curLnL = pos * k + (pos - curChPx) / 2,
          curLnR = pos * k + (pos + curChPx) / 2;

      drawStr(settings[curr][k], curLnL, 34, 0);
      if (currentValue == k)
        display.drawLine(curLnL - 1, 44, curLnR , 44, SSD1306_WHITE);
      if (selectedX == k && selectedY == 0)
        display.drawRoundRect(curLnL - 7, 31, curChPx + 14, 17, 4, SSD1306_WHITE);
    }

    const int indcS = (SCREEN_WIDTH - sSz * 10) / 2;
    for (int h = 0; h < sSz; h++)
    {
      if (h == currentPage)
        display.fillCircle(indcS + (10 * h) + 4, topScrl, 1, SSD1306_WHITE);
      else
        display.fillCircle(indcS + (10 * h) + 4, topScrl, 0, SSD1306_WHITE);
    }
    display.display();

    while (btnClick() != 0)
    {
      btnEvent = btnClick();

      if (btnEvent == 1)
      {
        if (selectedY == 0)
        {
          int curChSz = settings[curr][selectedX].length(),
              curChPx = (curChSz * 5) + curChSz - 1,
              curPos = (pos * (selectedX + 1)),
              curLnL = pos * selectedX + (pos - curChPx) / 2,
              curLnR = pos * selectedX + (pos + curChPx) / 2;

          display.fillRoundRect(curLnL - 7, 31, curChPx + 14, 17, 4, SSD1306_WHITE);
          display.setTextColor(SSD1306_BLACK);
          drawStr(settings[curr][selectedX], curLnL, 34, 0);
          display.drawLine(curLnL - 1, 44, curLnR , 44, SSD1306_BLACK);

          display.display();
          wait(0);
          delay(100);
          return selectedX;
        }
        else if (selectedY == -1)
        {
          display.fillRoundRect(5, 4, 17, 13, 4, SSD1306_WHITE);
          drawArrow(LEFT, 11, 10, 3, SSD1306_BLACK);

          display.display();
          wait(0);
          delay(100);
          return -1;
        }
      }
      else
      {
        if (selectedY == 0)
        {
          if (btnEvent == LEFT)
            selectedX--; // left
          else if (btnEvent == RIGHT)
            selectedX++; // right
        }
        else if ((selectedY == 1) && (btnEvent == LEFT || btnEvent == RIGHT))
        {
          display.fillRoundRect(15, 48, 98, 13, 4, SSD1306_WHITE);

          if (currentPage == 0)
          {
            drawArrow(RIGHT, 103, topScrl, 2, SSD1306_BLACK);
            if (btnEvent == LEFT)
              continue;
          }
          else if (currentPage == sSz - 1)
          {
            drawArrow(LEFT, 25, topScrl, 2, SSD1306_BLACK);
            if (btnEvent == RIGHT)
              continue;
          }
          else
          {
            drawArrow(LEFT, 25, topScrl, 2, SSD1306_BLACK);
            drawArrow(RIGHT, 103, topScrl, 2, SSD1306_BLACK);
          }

          for (int h = 0; h < sSz; h++)
          {
            if (h == currentPage)
              display.fillCircle(indcS + 4 + (10 * h), topScrl, 1, SSD1306_BLACK);
            else
              display.fillCircle(indcS + 4 + (10 * h), topScrl, 0, SSD1306_BLACK);
          }

          display.display();
          wait(0);
          delay(100);

          if (btnEvent == LEFT)
            return LEFT; // left
          else if (btnEvent == RIGHT)
            return RIGHT; // right
        }

        if (btnEvent == TOP)
          selectedY--; // top
        else if (btnEvent == BOTTOM)
          selectedY++; // bottom

        wait(0);
        break;
      }
    }

    if (hasBack)
    {
      if (selectedY == -2)
        selectedY = 1;
      else if (selectedY == iSz)
        selectedY = -1;
    }
    else
    {
      if (selectedY == -1)
        selectedY = iSz - 1;
      else
        selectedY %= iSz;
    }
    if (selectedX == -1)
      selectedX = settings[curr].size() - 1;
    else
      selectedX %= settings[curr].size();
  }
}

void drawQrCode(String qrStr, String lines[])
{
  display.fillRect(0, 0, 128, 64, 1);

  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrStr.c_str());

  // Text starting point
  int cursor_start_y = 4;
  int cursor_start_x = 4;
  int font_height = 12;

  // QR Code Starting Point
  int offset_x = 67;
  int offset_y = 3;

  for (int y = 0; y < qrcode.size; y++)
  {
    for (int x = 0; x < qrcode.size; x++)
    {
      int newX = offset_x + (x * 2);
      int newY = offset_y + (y * 2);

      if (qrcode_getModule(&qrcode, x, y))
      {
        display.fillRect(newX, newY, 2, 2, 0);
      }
      else
      {
        display.fillRect(newX, newY, 2, 2, 1);
      }
    }
  }

  display.setTextColor(0, 1);
  for (int i = 0; i < 4; i++)
  {
    display.setCursor(cursor_start_x, cursor_start_y + font_height * i);
    display.println(lines[i]);
  }
  display.display();
}




// SERVER FUNCTIONS
void handleFormSubmit(AsyncWebServerRequest *request)
{
  if (request->hasArg("SSID") && request->hasArg("password"))
  {
    String ssidValue = request->arg("SSID");
    String passwordValue = request->arg("password");

    display.clearDisplay();
    display.display();
    display.setTextColor(1);
    drawStr("Connecting to WiFi", 0, 28, 1);
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
       
        // WiFi.softAPdisconnect(true);
        delay(100);
        formSubmitDone = 1;

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
      Serial.println(filePath); 
    });
  }
}

String MESSAGE_CONFIGURE_WIFI[4] = {"SCAN TO", "CONNECT", "LOCALLY", ""};
String MESSAGE_OPEN_WEBAPP[4] = {"Scan QR", "to open", "the app.", ""};

void setup()
{
  // pinMode(BTN_NAV, INPUT);
  pinMode(BTN_X, INPUT_PULLDOWN);
  pinMode(BTN_Y, INPUT_PULLDOWN);
  pinMode(BTN_E, INPUT_PULLDOWN);

  esp_task_wdt_init(30, false);

  Serial.begin(9600);
  SPIFFS.begin();
  // initialize the OLED object
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.clearDisplay();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  // if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);
  
  init_serverFiles();

  display.setTextColor(1);
  drawStr("Chili Con Labyu", 0, 28, 1);
  display.display();

  // delay(3000);

  // Clear the buffer.
  display.clearDisplay();
  server.begin();

}

int MainMenu()
{
  return drawMenu(
    "SETTINGS", 
    std::vector<String>{
      "CONNECT LOCALLY", 
      "PAIR PHONE", 
      "UI SETTINGS"
      }, 
    false);
}

int Settings(int page, int curVal, int selY)
{
  return drawSetting(
      "UI SETTINGS",
      std::vector <String> {
        "THEME",
        "BRIGHTNESS",
        "SOUND"
      },
      std::map <String, std::vector<String>> {
        {"THEME", std::vector<String>{"WHITE", "BLACK"}},
        {"BRIGHTNESS", std::vector<String>{"1", "2", "3", "4", "5"}},
        {"SOUND", std::vector<String>{"ON", "OFF", "HM"}}
      },
      page, curVal, selY);
}

int serverCreated = 0;
void setupServerForConnectedWiFi()
{
  IPAddress localIP = WiFi.localIP();
  Serial.print("Connected to Wi-Fi. Local IP: ");
  Serial.println(localIP);

  init_serverFiles();
  server.begin();
}

int ConnectLocal(){
  display.clearDisplay();
  display.display();

  display.fillRect(0, 0, 128, 64, 1);
  display.setTextColor(0);

  drawStr("Opening WiFi...", 0, 28, 1);
  display.display();

  IPAddress apIP(8, 8, 4, 4); // The default android DNS

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  drawQrCode(
      ("WIFI:S:" + String(ssid) + ";T:WPA;P:" + String(pass) + ";;").c_str(),
      MESSAGE_CONFIGURE_WIFI);

  int redrawQR = 1, redrawCLIENT = 1;
  while (btnClick() == 0)
  {
    dnsServer.processNextRequest();

    // Get the connected clients' details
    int numberOfClients = WiFi.softAPgetStationNum();
    if (formSubmitDone)
    {
      WiFi.softAPdisconnect(true);
      int res = drawDialog(
          "WiFi Connected",
          std::vector<String>{
              String("SSID: ") + WiFi.SSID(),
              String("IP: ") + WiFi.localIP()},
          std::vector<String>{
              "EXIT",
              "PAIR"},
          false);

    
      formSubmitDone = 0;
      return res;
    }

    int connNum = WiFi.softAPgetStationNum(); 
    if (connNum > 0)
    {
      if (redrawCLIENT)
      {

        display.fillRect(0, 0, 128, 64, 1);
        display.setTextColor(0);
        drawStr("CONNECTED CLIENTS", 0, 7, 1);

        for (int i = 0; i < connNum; i++)
        {
          wifi_sta_list_t stationList;
          uint8_t *mac = stationList.sta[0].mac;
          char macStr[18];
          snprintf(macStr, sizeof(macStr), "%d. %02X:%02X:%02X:%02X:%02X:%02X",
                   i, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
          drawStr(macStr, 0, 20, 1);
        }
      
        drawStr("Local Connection", 0, 45, 1);
        display.display();

        redrawCLIENT = 0;
        redrawQR = 1;
      }
    }
    else
    {
      if (redrawQR)
      {
        drawQrCode(
            ("WIFI:S:" + String(ssid) + ";T:WPA;P:" + String(pass) + ";;").c_str(),
            MESSAGE_CONFIGURE_WIFI);

        redrawQR = 0;
        redrawCLIENT = 1;
      }
    }
  }
  return -1;
}

int Pair(){
  WiFi.mode(WIFI_STA);
  setupServerForConnectedWiFi();
  return 0;
}

int prev_button = -1;
void loop()
{
  dnsServer.processNextRequest();

  if (formSubmitDone)
  {
    Serial.println("Form submit done");
    WiFi.softAPdisconnect(true);
    int res = drawDialog(
        "WiFi Connected",
        std::vector<String>{
            String("SSID: ") + WiFi.SSID(),
            String("IP: ") + WiFi.localIP()},
        std::vector<String>{
            "EXIT",
            "PAIR"},
        false);
    Serial.println("Form submit doddd");

    formSubmitDone = 0;

  } else {
    int mainRes = MainMenu();
    
    if (mainRes == -1)
    {
      display.clearDisplay();
      display.display();
      wait(true);
    }
    else if (mainRes == 0)
    {
      if (ConnectLocal() == 0)
        Pair();
    }
    else if (mainRes == 1)
    {
      display.clearDisplay();
      display.display();
      wait(1);
    }
    else if (mainRes == 2)
    {
      int tmp = 0,
          page = 0,
          selY = 0;
      while (true)
      {
        if (page == -1)
          page = 0;
        else if (page == 3)
          page = 2;

        tmp = Settings(page, setVals[page], selY);

        if (tmp == -1)
          break;
        else if (tmp == LEFT)
        {
          page--;
          selY = 1;
        }
        else if (tmp == RIGHT)
        {
          page++;
          selY = 1;
        }
        else
        {
          setVals[page] = tmp;

          display.ssd1306_command(0x81);
          display.ssd1306_command(1 + 51 * setVals[1]);
          selY = 0;
        }
      }
    }

  }
}