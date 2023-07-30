#include <Arduino.h>

#include "server_side.h"



int setVals[] = {
    2,
    4,
    2};


int MainMenu();
int Settings(int page, int curVal, int selY = 0);
int ConnectLocal();
int Pair();

void initDefines()
{
  pinMode(BUTTON, INPUT_PULLDOWN);
  pinMode(MOISTURE1, INPUT);
  pinMode(LIGHT1, INPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(LED, 0);
}

void setup()
{
  initDefines();
  configDisplay();

  Serial.begin(9600);
  SPIFFS.begin();


}

int MainMenu()
{
  return drawMenu(
    "SETTINGS", 
    std::vector<String>{
      "CONNECT LOCALLY", 
      "CONNECT WIFI", 
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

bool formSubmitDone = false;
int ConnectLocal(){
  display.clearDisplay();
  display.display();
  display.setTextColor(1);
  drawStr("Opening WiFi...", 0, 28, 1);
  display.display();

  configLocalServer();

  drawQrCode(
      ("WIFI:S:" + String(ssid) + ";T:WPA;P:" + String(pass) + ";;").c_str(),
      "SCAN QR AND CONNECT");

  int redrawQR = 1, redrawCLIENT = 1;
  while (btnClick() == 0)
  {
    dnsServer.processNextRequest();

    // Get the connected clients' details
    int numberOfClients = WiFi.softAPgetStationNum();

    int connNum = WiFi.softAPgetStationNum(); 
    if (connNum > 0)
    {
      if (redrawCLIENT)
      {

        display.fillRect(0, 0, 128, 64, 0);
        display.setTextColor(1);
        drawStr("CONNECTED CLIENTS", 0, 7, 1);

        /*
        for (int i = 0; i < connNum; i++)
        {
          wifi_sta_list_t stationList;
          uint8_t *mac = stationList.sta[i].mac;
          char macStr[18];
          snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
          drawStr(macStr, 0, 20 + i * 9, 1);
        }
        */

        wifi_sta_list_t stationList;
        uint8_t *mac = stationList.sta[0].mac;
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        drawStr(macStr, 0, 20, 1);

        drawStr("Local Connection", 0, 45, 1);
        display.display();

        redrawCLIENT = 0;
        redrawQR = 1;
      }
    }
    else
    {
      if (WiFi.isConnected())
      {
        drawDialog(
            "WIFI CONNECTED",
            std::vector<String>{"Local mode was", "activated."},
            std::vector<String>{"OK"},
            false);
        return -1;
      }

      if (redrawQR)
      {
        drawQrCode(
            ("WIFI:S:" + String(ssid) + ";T:WPA;P:" + String(pass) + ";;").c_str(),
            "SCAN QR AND CONNECT");

        redrawQR = 0;
        redrawCLIENT = 1;
      }
    }
  }
  if (WiFi.isConnected())
    WiFi.softAPdisconnect(true);
  return -1;
}

int Pair(){
  if (WiFi.isConnected())
  {

  }
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
      "WIFI CONNECTED",
      std::vector<String>{"Local mode was", "activated."},
      std::vector<String>{"OK"},
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
      if (WiFi.isConnected())
      {
        display.clearDisplay();
        drawDialog(
            "WIFI CONNECTED",
            std::vector<String>{"WiFi mode was", "activated."},
            std::vector<String>{"OK"},
            false);
      }
      else if (ConnectLocal() == 0)
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