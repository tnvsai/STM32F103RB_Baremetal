#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "stm32f103xb.h"

// ---------------- IWDG ----------------
void IWDG_Init(uint16_t timeout_ms);
void IWDG_Refresh(void);

// ---------------- WWDG ----------------
void WWDG_Init(uint8_t window);
void WWDG_Refresh(void);

#endif
