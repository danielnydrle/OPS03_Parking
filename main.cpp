/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "LCD_DISCO_F469NI.h"
#include "stm32469i_discovery.h"
#include "stm32469i_discovery_lcd.h"
#include "mbed_wait_api.h"
#include <stack>

LCD_DISCO_F469NI lcd;

Mutex mutexParking;
Mutex mutexDisplay;
Semaphore sem(4);
Thread t1;
Thread t2;
Thread t3;
Thread t4;
Thread t5;
Thread t6;

#define WAIT 1000
#define MAX_SPACES 4

int width = lcd.GetXSize();
int height = lcd.GetYSize();

std::stack<int> parkingStack;

struct Auto {
    int color;
    uint32_t timeParked;
    uint32_t timeUnparked;
};

short Reserve() {
    while(true) {
        if (!parkingStack.empty()) {
            mutexParking.lock();
            int spaceIndex = parkingStack.top();
            parkingStack.pop();
            mutexParking.unlock();
            return spaceIndex;
        } else {
            return -1;
        }
    }
}

void Unreserve(short i) {
    mutexParking.lock();
    parkingStack.push(i);
    mutexParking.unlock();
}

void Park(Auto *a) {
    short x = 0;
    short y = 0;
    while(true) {
        ThisThread::sleep_for(a->timeUnparked);
        sem.acquire();
        short parkingIndex = Reserve();
        switch(parkingIndex) {
            case 0:
            x = 0;
            y = 0;
            break;
            case 1:
            x = 0;
            y = 1;
            break;
            case 2:
            x = 1;
            y = 0;
            break;
            case 3:
            x = 1;
            y = 1;
            break;
        }
        mutexDisplay.lock();
        lcd.SetTextColor(a->color);
        lcd.FillRect(x*width/2, y*height/2, width/2, height/2);
        mutexDisplay.unlock();
        ThisThread::sleep_for(a->timeParked);
        Unreserve(parkingIndex);

        mutexDisplay.lock();
        lcd.SetTextColor(LCD_COLOR_BLACK);
        lcd.FillRect(x*width/2, y*height/2, width/2, height/2);
        mutexDisplay.unlock();

        sem.release();
    }
}

int main()
{
    for (int i = MAX_SPACES - 1; i >= 0; --i) {
        parkingStack.push(i);
    }
    Auto auta[6];
    auta[0].color = LCD_COLOR_WHITE;
    auta[1].color = LCD_COLOR_BLUE;
    auta[2].color = LCD_COLOR_BROWN;
    auta[3].color = LCD_COLOR_GRAY;
    auta[4].color = LCD_COLOR_GREEN;
    auta[5].color = LCD_COLOR_RED;
    for (int i = 1; i <= 6; i++) {
        auta[i-1].timeParked = i * WAIT;
        auta[i-1].timeUnparked = 7000 - i * WAIT;
    }
    lcd.Clear(LCD_COLOR_BLACK);
    
    t1.start(callback(Park, &auta[0]));
    t2.start(callback(Park, &auta[1]));
    t3.start(callback(Park, &auta[2]));
    t4.start(callback(Park, &auta[3]));
    t5.start(callback(Park, &auta[4]));
    t6.start(callback(Park, &auta[5]));
    while(true) {
        ThisThread::sleep_for(7000);
    }
}
