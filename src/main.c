#include "stm32f103xb.h"
#include "uart.h"
#include "bkp.h"
#include <stdio.h>
#include "utility.h"
#include "rtc.h"

// Simple blocking delay using RTC
void DelaySeconds(uint32_t sec)
{
    uint32_t start = ((uint32_t)RTC->CNTH << 16) | RTC->CNTL;
    while ((((uint32_t)RTC->CNTH << 16) | RTC->CNTL) - start < sec);
}

int main(void)
{
    RTC_Init();

    // Only set counter once if needed
    // RTC_SetCounter(0); // optional, skip if you want RTC to continue after reset

    RTC_SetAlarm(10); // Alarm at 10 seconds

    uint32_t h, m, s;

    while(1)
    {
        // Get current time
        RTC_GetTime(&h, &m, &s);

        // Print time every 2 seconds with manual zero-padding
        mini_printf("Time: %c%u:%c%u:%c%u\r\n",
            (h < 10) ? '0' : '0' + (h / 10), h % 10,
            (m < 10) ? '0' : '0' + (m / 10), m % 10,
            (s < 10) ? '0' : '0' + (s / 10), s % 10);

        DelaySeconds(2);

        // Check if alarm triggered
        if(RTC_AlarmTriggered())
        {
            mini_printf(">>> RTC Alarm Triggered at %c%u:%c%u:%c%u <<<\r\n",
                (h < 10) ? '0' : '0' + (h / 10), h % 10,
                (m < 10) ? '0' : '0' + (m / 10), m % 10,
                (s < 10) ? '0' : '0' + (s / 10), s % 10);
        }
    }
}