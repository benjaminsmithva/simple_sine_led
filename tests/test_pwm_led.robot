*** Variables ***
${ELF}                 @${CURDIR}/../build/firmware.elf
${REPL}                @${CURDIR}/../platforms/stm32f4_discovery_full.repl
${LED}                  sysbus.gpioPortD.UserLED

*** Test Cases ***
Average Duty Cycle Over Full Sine Period
    [Documentation]    Primary, timing-insensitive smoke test. The average of
    ...    sin(x) over one full period is 0, so the average PWM duty cycle
    ...    over one full 3.6s sine cycle (360 steps * 10ms) should be ~50%
    ...    regardless of exact phase alignment between Renode and firmware
    ...    boot timing.
    Execute Command         mach create
    Execute Command         machine LoadPlatformDescription ${REPL}
    Create LED Tester       ${LED}    defaultTimeout=5
    Execute Command         sysbus LoadELF ${ELF}
    Assert LED Duty Cycle    testDuration=3.6    expectedDutyCycle=0.5    tolerance=0.1

Duty Cycle Near Sine Peak
    [Documentation]    Runs to ~0.85s after boot (x approaching 90 degrees,
    ...    sin nearing 1, duty nearing 100%), then samples for a further
    ...    0.1s. Sampled deliberately near the peak, where the duty cycle
    ...    changes slowest (derivative of sine ~= 0), giving a stable window
    ...    to average over.
    Execute Command         mach create
    Execute Command         machine LoadPlatformDescription ${REPL}
    Create LED Tester       ${LED}    defaultTimeout=5
    Execute Command         sysbus LoadELF ${ELF}
    Execute Command         emulation RunFor "0.85s"
    Assert LED Duty Cycle    testDuration=0.1    expectedDutyCycle=1.0    tolerance=0.15

Duty Cycle Near Sine Trough
    [Documentation]    Runs to ~2.65s after boot (x approaching 270 degrees,
    ...    sin nearing -1, duty nearing 0%) — same reasoning as the peak
    ...    case, mirrored at the trough.
    Execute Command         mach create
    Execute Command         machine LoadPlatformDescription ${REPL}
    Create LED Tester       ${LED}    defaultTimeout=5
    Execute Command         sysbus LoadELF ${ELF}
    Execute Command         emulation RunFor "2.65s"
    Assert LED Duty Cycle    testDuration=0.1    expectedDutyCycle=0.0    tolerance=0.15

# NOTE: Assert LED Duty Cycle relies on Renode's STM32 timer model actually
# toggling the connected GPIO pin on PWM compare match (TIM4 CH1 -> PD12 via
# AF2 -> UserLED). This chain is modeled in stock Renode (see
# platforms/cpus/stm32f4.repl's "timer4: 0 -> gpioPortD#12@02") and was
# confirmed working for this firmware by manually reading TIM4->CCR1
# (0x40000834) at 0.05s/0.9s/2.7s and observing ~534/998/0 out of ARR=999 —
# matching the expected sine phase at each point. GitHub issue
# renode/renode#562 reports this PWM-to-GPIO modeling not working for some
# other users/setups, so if these assertions prove flaky in a different
# Renode version, fall back to reading TIM4 registers directly instead:
#   Execute Command    sysbus ReadDoubleWord 0x40000834    # TIM4->CCR1
#   Execute Command    sysbus ReadDoubleWord 0x4000082C    # TIM4->ARR
#   duty = CCR1 / (ARR + 1)
