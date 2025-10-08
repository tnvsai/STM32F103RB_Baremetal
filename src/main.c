#include "stm32f103xb.h"
#include "systick.h"
#include "utility.h"

int main(void)
{
    SysTick_Init(1000); // 1ms tick

    while (1)
    {
        LED_Toggle();
        SysTick_DelayMs(1000);
    }
}
