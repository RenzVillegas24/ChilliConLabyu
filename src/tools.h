#include <Arduino.h>

#include "defines.h"
/*
int btnClick(bool = false);
void wait(bool = false);
*/



int btnClick(bool valVolts = false);

void wait(bool reverse = false)
{
    if (reverse)
        while (btnClick() == 0)
            delay(1);
    else
        while (btnClick() != 0)
            delay(1);
}