#include "adc.h"

// -----------------------------
// Initialize ADC1
// -----------------------------
void ADC_Init(void) {
    // 1. Enable ADC1 clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // 2. Configure PA0 as analog input (example for channel 0)
    GPIOA->CRL &= ~0xF;  // MODE0=00, CNF0=00 → analog input

    // 3. Power on ADC
    ADC1->CR2 |= ADC_CR2_ADON;
    for (volatile int i = 0; i < 1000; i++); // small delay

    // 4. Calibrate ADC
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL); // wait for calibration
}

// -----------------------------
// Read a single ADC channel (0–15)
// -----------------------------
uint16_t ADC_Read_Single(uint8_t channel) {
    if (channel > 15) return 0; // invalid channel

    // Select the channel
    ADC1->SQR3 = channel;

    // Start conversion
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_ADON; // second write triggers conversion

    // Wait for conversion complete
    while (!(ADC1->SR & ADC_SR_EOC));

    return ADC1->DR;
}
