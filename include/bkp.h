#ifndef BKP_H
#define BKP_H

#include "stm32f103xb.h"
#include <stdint.h>

// Total available backup registers
#define BKP_REGISTER_COUNT 10

// BKP Register index type (1..10)
typedef enum {
    BKP_DR1 = 1,
    BKP_DR2,
    BKP_DR3,
    BKP_DR4,
    BKP_DR5,
    BKP_DR6,
    BKP_DR7,
    BKP_DR8,
    BKP_DR9,
    BKP_DR10
} BKP_Reg_t;

// --------- Function Prototypes ----------
void BKP_Init(void);
void BKP_WriteReg(BKP_Reg_t reg, uint16_t data);
uint16_t BKP_ReadReg(BKP_Reg_t reg);
void BKP_ResetDomain(void);

#endif
