/* stm32f407.h — Bare-metal peripheral register definitions for STM32F407VG
 *
 * Peripherals covered: RCC, GPIO, USART2, TIM4, SysTick, EXTI, SYSCFG, NVIC
 * All addresses from RM0090 (STM32F40x Reference Manual).
 */

#ifndef STM32F407_H
#define STM32F407_H

#include <stdint.h>

/* ── Base addresses ─────────────────────────────────────────────────────── */
#define PERIPH_BASE         0x40000000UL
#define APB1_BASE           (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE           (PERIPH_BASE + 0x00010000UL)
#define AHB1_BASE           (PERIPH_BASE + 0x00020000UL)

#define GPIOA_BASE          (AHB1_BASE  + 0x0000UL)
#define GPIOB_BASE          (AHB1_BASE  + 0x0400UL)
#define GPIOC_BASE          (AHB1_BASE  + 0x0800UL)
#define GPIOD_BASE          (AHB1_BASE  + 0x0C00UL)
#define RCC_BASE            (AHB1_BASE  + 0x3800UL)

#define SYSCFG_BASE         (APB2_BASE  + 0x3800UL)
#define EXTI_BASE           (APB2_BASE  + 0x3C00UL)
#define USART2_BASE         (APB1_BASE  + 0x4400UL)
#define TIM4_BASE           (APB1_BASE  + 0x0800UL)

#define NVIC_BASE           0xE000E100UL
#define SYSTICK_BASE        0xE000E010UL

/* ── GPIO ───────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 Mode register                  */
    volatile uint32_t OTYPER;   /* 0x04 Output type register           */
    volatile uint32_t OSPEEDR;  /* 0x08 Output speed register          */
    volatile uint32_t PUPDR;    /* 0x0C Pull-up/pull-down register     */
    volatile uint32_t IDR;      /* 0x10 Input data register            */
    volatile uint32_t ODR;      /* 0x14 Output data register           */
    volatile uint32_t BSRR;     /* 0x18 Bit set/reset register         */
    volatile uint32_t LCKR;     /* 0x1C Configuration lock register    */
    volatile uint32_t AFR[2];   /* 0x20 Alternate function registers   */
} GPIO_t;

#define GPIOA   ((GPIO_t *)GPIOA_BASE)
#define GPIOB   ((GPIO_t *)GPIOB_BASE)
#define GPIOD   ((GPIO_t *)GPIOD_BASE)

/* GPIO MODER values (2 bits per pin) */
#define GPIO_MODER_INPUT    0x0U
#define GPIO_MODER_OUTPUT   0x1U
#define GPIO_MODER_AF       0x2U
#define GPIO_MODER_ANALOG   0x3U

/* GPIO PUPDR values */
#define GPIO_PUPDR_NONE     0x0U
#define GPIO_PUPDR_PU       0x1U
#define GPIO_PUPDR_PD       0x2U

/* ── RCC ────────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR;           /* 0x00 */
    volatile uint32_t PLLCFGR;      /* 0x04 */
    volatile uint32_t CFGR;         /* 0x08 */
    volatile uint32_t CIR;          /* 0x0C */
    volatile uint32_t AHB1RSTR;     /* 0x10 */
    volatile uint32_t AHB2RSTR;     /* 0x14 */
    volatile uint32_t AHB3RSTR;     /* 0x18 */
    uint32_t          RESERVED0;    /* 0x1C */
    volatile uint32_t APB1RSTR;     /* 0x20 */
    volatile uint32_t APB2RSTR;     /* 0x24 */
    uint32_t          RESERVED1[2]; /* 0x28-0x2C */
    volatile uint32_t AHB1ENR;      /* 0x30 */
    volatile uint32_t AHB2ENR;      /* 0x34 */
    volatile uint32_t AHB3ENR;      /* 0x38 */
    uint32_t          RESERVED2;    /* 0x3C */
    volatile uint32_t APB1ENR;      /* 0x40 */
    volatile uint32_t APB2ENR;      /* 0x44 */
} RCC_t;

#define RCC     ((RCC_t *)RCC_BASE)

/* RCC AHB1ENR bits */
#define RCC_AHB1ENR_GPIOAEN     (1U << 0)
#define RCC_AHB1ENR_GPIOBEN     (1U << 1)
#define RCC_AHB1ENR_GPIODEN     (1U << 3)

/* RCC APB1ENR bits */
#define RCC_APB1ENR_TIM4EN      (1U << 2)
#define RCC_APB1ENR_USART2EN    (1U << 17)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_SYSCFGEN    (1U << 14)

/* ── USART ──────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t SR;   /* 0x00 Status register        */
    volatile uint32_t DR;   /* 0x04 Data register          */
    volatile uint32_t BRR;  /* 0x08 Baud rate register     */
    volatile uint32_t CR1;  /* 0x0C Control register 1     */
    volatile uint32_t CR2;  /* 0x10 Control register 2     */
    volatile uint32_t CR3;  /* 0x14 Control register 3     */
    volatile uint32_t GTPR; /* 0x18 Guard time/prescaler   */
} USART_t;

#define USART2  ((USART_t *)USART2_BASE)

/* USART SR bits */
#define USART_SR_TXE    (1U << 7)   /* Transmit data register empty */
#define USART_SR_TC     (1U << 6)   /* Transmission complete        */
#define USART_SR_RXNE   (1U << 5)   /* Read data register not empty */

/* USART CR1 bits */
#define USART_CR1_UE    (1U << 13)  /* USART enable       */
#define USART_CR1_TE    (1U << 3)   /* Transmitter enable */
#define USART_CR1_RE    (1U << 2)   /* Receiver enable    */

/* ── TIM4 (general-purpose timer, used here for PWM on CH1/PD12) ──────────── */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 Control register 1               */
    volatile uint32_t CR2;      /* 0x04 Control register 2               */
    volatile uint32_t SMCR;     /* 0x08 Slave mode control register      */
    volatile uint32_t DIER;     /* 0x0C DMA/interrupt enable register    */
    volatile uint32_t SR;       /* 0x10 Status register                  */
    volatile uint32_t EGR;      /* 0x14 Event generation register        */
    volatile uint32_t CCMR1;    /* 0x18 Capture/compare mode register 1  */
    volatile uint32_t CCMR2;    /* 0x1C Capture/compare mode register 2  */
    volatile uint32_t CCER;     /* 0x20 Capture/compare enable register  */
    volatile uint32_t CNT;      /* 0x24 Counter                          */
    volatile uint32_t PSC;      /* 0x28 Prescaler                        */
    volatile uint32_t ARR;      /* 0x2C Auto-reload register             */
    uint32_t          RESERVED0; /* 0x30 (RCR — advanced timers only)    */
    volatile uint32_t CCR1;     /* 0x34 Capture/compare register 1       */
    volatile uint32_t CCR2;     /* 0x38 Capture/compare register 2       */
    volatile uint32_t CCR3;     /* 0x3C Capture/compare register 3       */
    volatile uint32_t CCR4;     /* 0x40 Capture/compare register 4       */
    uint32_t          RESERVED1; /* 0x44 (BDTR — advanced timers only)   */
    volatile uint32_t DCR;      /* 0x48 DMA control register             */
    volatile uint32_t DMAR;     /* 0x4C DMA address for full transfer    */
} TIM_t;

#define TIM4    ((TIM_t *)TIM4_BASE)

/* TIM CR1 bits */
#define TIM_CR1_CEN         (1U << 0)   /* Counter enable            */
#define TIM_CR1_ARPE        (1U << 7)   /* Auto-reload preload enable */

/* TIM CCMR1 bits (channel 1) */
#define TIM_CCMR1_OC1PE     (1U << 3)   /* Output compare 1 preload enable */
#define TIM_CCMR1_OC1M_MSK  (0x7UL << 4)
#define TIM_CCMR1_OC1M_PWM1 (0x6UL << 4) /* PWM mode 1 */

/* TIM CCER bits */
#define TIM_CCER_CC1E       (1U << 0)   /* Capture/compare 1 output enable */

/* TIM EGR bits */
#define TIM_EGR_UG          (1U << 0)   /* Update generation (force reload of shadow regs) */

/* ── EXTI ───────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t IMR;   /* 0x00 Interrupt mask register    */
    volatile uint32_t EMR;   /* 0x04 Event mask register        */
    volatile uint32_t RTSR;  /* 0x08 Rising trigger selection   */
    volatile uint32_t FTSR;  /* 0x0C Falling trigger selection  */
    volatile uint32_t SWIER; /* 0x10 Software interrupt event   */
    volatile uint32_t PR;    /* 0x14 Pending register           */
} EXTI_t;

#define EXTI    ((EXTI_t *)EXTI_BASE)

/* ── SYSCFG ─────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t MEMRMP;       /* 0x00 */
    volatile uint32_t PMC;          /* 0x04 */
    volatile uint32_t EXTICR[4];    /* 0x08-0x14 External interrupt config */
    uint32_t          RESERVED[2];
    volatile uint32_t CMPCR;        /* 0x20 */
} SYSCFG_t;

#define SYSCFG  ((SYSCFG_t *)SYSCFG_BASE)

/* ── NVIC ───────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t ISER[8];  /* 0x000 Interrupt Set Enable Registers   */
    uint32_t          RESERVED0[24];
    volatile uint32_t ICER[8];  /* 0x080 Interrupt Clear Enable Registers */
    uint32_t          RESERVED1[24];
    volatile uint32_t ISPR[8];  /* 0x100 Interrupt Set Pending Registers  */
    uint32_t          RESERVED2[24];
    volatile uint32_t ICPR[8];  /* 0x180 Interrupt Clear Pending          */
    uint32_t          RESERVED3[24];
    volatile uint32_t IABR[8];  /* 0x200 Interrupt Active Bit Registers   */
    uint32_t          RESERVED4[56];
    volatile uint8_t  IP[240];  /* 0x300 Interrupt Priority Registers     */
    uint32_t          RESERVED5[644];
    volatile uint32_t STIR;     /* 0xE00 Software Trigger Interrupt       */
} NVIC_t;

#define NVIC    ((NVIC_t *)NVIC_BASE)

/* STM32F407 IRQ numbers */
#define EXTI0_IRQn      6
#define USART2_IRQn     38

static inline void NVIC_EnableIRQ(int irqn) {
    NVIC->ISER[irqn >> 5] = (1U << (irqn & 0x1F));
}

static inline void NVIC_SetPriority(int irqn, uint8_t priority) {
    NVIC->IP[irqn] = (uint8_t)(priority << 4);
}

/* ── SysTick ────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CTRL;     /* 0x00 Control and status  */
    volatile uint32_t LOAD;     /* 0x04 Reload value        */
    volatile uint32_t VAL;      /* 0x08 Current value       */
    volatile uint32_t CALIB;    /* 0x0C Calibration value   */
} SysTick_t;

#define SYSTICK ((SysTick_t *)SYSTICK_BASE)

#define SYSTICK_CTRL_ENABLE     (1U << 0)
#define SYSTICK_CTRL_TICKINT    (1U << 1)
#define SYSTICK_CTRL_CLKSOURCE  (1U << 2)   /* 1 = processor clock */

#endif /* STM32F407_H */
