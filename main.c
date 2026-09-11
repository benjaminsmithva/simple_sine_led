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

#define SYSCLK_HZ        16000000UL   /* HSI, no PLL */
#define PWM_PSC          15U          /* 16MHz / (15+1) = 1MHz counter clock */
#define PWM_ARR          999U         /* 1MHz / 1000 = 1kHz PWM carrier, 1000-step duty */
#define STEP_INTERVAL_MS 10U          /* x steps every 10ms -> 360*10ms = 3.6s full sine cycle */
#define SINE_STEPS       360U         /* x loops 0..359 (degrees) */
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

static void led_pwm_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;               /* read-back before touching GPIOD */
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    (void)RCC->APB1ENR;               /* read-back before touching TIM4 */

    /* PD12 -> AF2 (TIM4_CH1) */
    GPIOD->MODER  &= ~(0x3UL << (12 * 2));
    GPIOD->MODER  |=  (GPIO_MODER_AF << (12 * 2));
    GPIOD->AFR[1] &= ~(0xFUL << ((12 - 8) * 4));
    GPIOD->AFR[1] |=  (0x2UL << ((12 - 8) * 4));   /* AF2 = TIM3/4/5 */

    TIM4->PSC   = PWM_PSC;
    TIM4->ARR   = PWM_ARR;
    TIM4->CCR1  = 0;
    TIM4->CCMR1 = (TIM4->CCMR1 & ~TIM_CCMR1_OC1M_MSK)
                | TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CR1  |= TIM_CR1_ARPE;
    TIM4->EGR   = TIM_EGR_UG;          /* force update: load PSC/ARR/CCR shadow regs */
    TIM4->CR1  |= TIM_CR1_CEN;
}

int main(void) {
    systick_init();
    led_pwm_init();

    uint32_t last = 0;
    uint32_t x = 0;   /* integer loop variable, degrees, wraps 0..359 forever */

    while (1) {
        uint32_t now = g_ticks;
        if ((now - last) >= STEP_INTERVAL_MS) {
            last = now;
            float angle = (float)x * (2.0f * SINE_PI / (float)SINE_STEPS);
            float y     = sinf(angle);              /* -1..1 */
            float duty  = (y + 1.0f) * 0.5f;        /* 0..1  */
            TIM4->CCR1  = (uint32_t)(duty * (float)PWM_ARR);
            x++;
            if (x >= SINE_STEPS) x = 0;
        }
    }
}
