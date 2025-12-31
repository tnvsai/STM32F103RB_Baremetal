#ifndef _RTC_H
#define _RTC_H

void RTC_Init(void);
void RTC_SetCounter(uint32_t seconds);
void RTC_GetTime(uint32_t *hours, uint32_t *minutes, uint32_t *seconds);
void RTC_SetAlarm(uint32_t seconds);
uint8_t RTC_AlarmTriggered(void);

#endif // _RTC_H