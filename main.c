/* main.c — sine-wave PWM LED brightness demo
 *
 * An integer loop variable x sweeps 0..359 (degrees) forever. Each step,
 * y = sin(x) is mapped from [-1,1] to a [0,1] duty cycle and written to
 * TIM4 CH1, which drives the onboard green LED (PD12) via hardware PWM
 * (AF2), producing a smooth "breathing" brightness effect.
 */

 // Test Change


#include "stm32f407.h"
#include <math.h>
#include <uart.c>

#define SYSCLK_HZ        16000000UL   /* HSI, no PLL */
#define STEP_INTERVAL_MS 10U          /* x steps every 10ms -> 360*10ms = 3.6s full sine cycle */
#define TIM4_ARR_VAL     1024    /* 1000 steps, 0..999 */
#define SINE_PI          3.14159265358979323846f


static volatile uint32_t g_ticks = 0;

void SysTick_Handler(void) {
    g_ticks++;
}

static void systick_init(void) {
    SYSTICK->LOAD = SYSCLK_HZ / 1000U - 1U;  /* 1ms tick at 16MHz */
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

static void setRegisterBits(uint32_t *reg, uint32_t LSB, uint32_t width, uint32_t value)
{
    uint32_t mask = ((1U << width) - 1U) << LSB;

    *reg = (*reg & ~mask) | ((value << LSB) & mask);
}

static void led_pin_init(void) {

    //Enable perfipherals
    setRegisterBits(&RCC->AHB1ENR, 3, 1, 0b1);  // ENABLE GPIOD
    setRegisterBits(&RCC->APB1ENR, 2, 1, 0b1);  // ENABLE TIM4

    //Configure PD12
    setRegisterBits(&GPIOD->AFR[1], 16, 4, 0b0010);  // Directs PD12 to TIM4_CH1
    setRegisterBits(&GPIOD->MODER, 24, 2, 0b10);  // Set PD12 to GPIO AFR mode
    setRegisterBits(&GPIOD->OTYPER, 12, 1, 0b0);  // Set PD12 to push-pull
    setRegisterBits(&GPIOD->OSPEEDR, 24, 2, 0b11); // Set PD12 to high speed

    //Configure TIM4
    TIM4->CR1 = 0b0000000000000001; // Enable TIM4, set ARPE
    TIM4->PSC = 1; // Timer prescaler
    TIM4->ARR = TIM4_ARR_VAL; // Auto-reload register value
    TIM4->CCMR1 = 0b00000001110000; // Set output compare mode to PWM mode 1 for channel 1
    TIM4->CCER = 0b0000000000000001; // Enable output for channel 1


}

int main(void) {
    systick_init();
    //led_pwm_init();
    led_pin_init();
    uart_init();

    uint32_t x = 0;   /* integer loop variable, degrees, wraps 0..359 forever */
    uint32_t next = 0;

    while (1) {
        if (g_ticks > next) 
        {
            next = g_ticks + STEP_INTERVAL_MS;

            if (x++ > 359) x = 0;
            float angle = (float)x * 0.0174533f;
            float y     = 0.5f * sinf(angle) + 0.5;              /* -1..1 */
            //uart_put_float(y);
            TIM4->CCR1 = (uint16_t)((float)TIM4_ARR_VAL * y);
        }
    }
}
