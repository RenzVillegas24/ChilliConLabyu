#include <Arduino.h>

#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <qrcode.h>
#include <vector>
#include <map>

#include "tools.h"
#include "defines.h"

QRCode qrcode;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
/*
void configDisplay();
void drawStr(String, int, int, int = 0);
void drawArrow(int, int, int, int, uint16_t);
int drawMenu(String, std::vector<String>, bool = true, int = 1);
int drawDialog(String, std::vector<String>, std::vector<String>, bool = true, int = 1);
int drawSetting(String, std::vector<String>, std::map<String, std::vector<String>>, int, int, int = 0, bool = true);
void drawQrCode(String, String*);
*/

void configDisplay()
{
    // initialize the OLED object
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    display.clearDisplay();

    /*
    display.setTextColor(1);
    drawStr("Chili Con Labyu", 0, 28, 1);
    display.display();

    delay(3000);

    display.clearDisplay();
    */
}

// DRAW FUNCTIONS

/*
  Alignment:
    Left = 0
    Center = 1
    Right = 2

*/
void drawStr(String buf, int x, int y, int align = 0)
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
             bool hasBack = true,
             int align = 1)
{
    int selectedY = 0,
        iSz = items.size();
    while (true)
    {
        int itmLen = items[selectedY].length(),
            btnEvent = btnClick(),
            itmPx = (itmLen * 5) + itmLen - 1 + 16,
            selL = (SCREEN_WIDTH - (itmPx)) / 2;

        if (btnEvent == 0)
        {
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
        }
        else
        {
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
               bool hasBack = true,
               int align = 1)
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
            // wait(0);
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
    return -1;
}

int drawSetting(String header,
                std::vector<String> settingsOrder,
                std::map<String, std::vector<String>> settings,
                const int currentPage,
                const int currentValue,
                int selectedY = 0,
                bool hasBack = true)
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
                display.drawLine(curLnL - 1, 44, curLnR, 44, SSD1306_WHITE);
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
                    display.drawLine(curLnL - 1, 44, curLnR, 44, SSD1306_BLACK);

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

void drawQrCode(String qrStr, String str)
{
    display.clearDisplay();
    display.fillRoundRect(47, 22, 35, 35, 4, 1);

    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrStr.c_str());

    // Text starting point
    int cursor_start_y = 4;
    int cursor_start_x = 4;
    int font_height = 12;

    // QR Code Starting Point
    int offset_x = 50;
    int offset_y = 25;

    for (int y = 0; y < qrcode.size; y++)
    {
        for (int x = 0; x < qrcode.size; x++)
        {
            int newX = offset_x + (x * 1);
            int newY = offset_y + (y * 1);

            if (qrcode_getModule(&qrcode, x, y))
            {
                display.fillRect(newX, newY, 1, 1, 0);
            }
            else
            {
                display.fillRect(newX, newY, 1, 1, 1);
            }
        }
    }

    display.setTextColor(1);
    drawStr(str, 0, 7, 1);

    display.display();
}
