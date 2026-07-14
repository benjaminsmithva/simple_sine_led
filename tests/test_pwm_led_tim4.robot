*** Variables ***
${ELF}                 @${CURDIR}/../build/firmware.elf
${REPL}                @${CURDIR}/../platforms/stm32f4_discovery_full.repl
${TIM4_CCR1}            0x40000834
${TIM4_ARR}             0x4000082C

*** Keywords ***
Setup Machine
    Execute Command         mach create
    Execute Command         machine LoadPlatformDescription ${REPL}
    Execute Command         sysbus LoadELF ${ELF}

Get TIM4 Duty Cycle
    [Documentation]    Reads TIM4->CCR1 and TIM4->ARR directly and computes
    ...    duty = CCR1 / (ARR + 1), as described in the fallback note at the
    ...    bottom of test_pwm_led.robot.
    ${ccr1}=                Execute Command    sysbus ReadDoubleWord ${TIM4_CCR1}
    ${arr}=                 Execute Command    sysbus ReadDoubleWord ${TIM4_ARR}
    ${duty}=                Evaluate    (${ccr1}) / ((${arr}) + 1)
    RETURN    ${duty}

Assert TIM4 Duty Cycle
    [Documentation]    Instantaneous check: reads the duty cycle once, at the
    ...    current point in simulated time.
    [Arguments]    ${expectedDutyCycle}    ${tolerance}
    ${duty}=                 Get TIM4 Duty Cycle
    ${diff}=                 Evaluate    abs(${duty} - ${expectedDutyCycle})
    Should Be True           ${diff} <= ${tolerance}
    ...    Sampled duty cycle ${duty} not within ${tolerance} of expected ${expectedDutyCycle}

Assert TIM4 Average Duty Cycle
    [Documentation]    Register-read analogue of "Assert LED Duty Cycle". A
    ...    register read is an instantaneous snapshot, not a GPIO-toggle time
    ...    integral, so this approximates the time-average by sampling
    ...    TIM4->CCR1/ARR repeatedly across the test window and averaging.
    [Arguments]    ${testDuration}    ${expectedDutyCycle}    ${tolerance}    ${sampleIntervalS}=0.1
    ${numSamples}=           Evaluate    int(${testDuration} / ${sampleIntervalS})
    ${total}=                Set Variable    0
    FOR    ${i}    IN RANGE    ${numSamples}
        Execute Command      emulation RunFor "${sampleIntervalS}s"
        ${duty}=             Get TIM4 Duty Cycle
        ${total}=            Evaluate    ${total} + ${duty}
    END
    ${avg}=                  Evaluate    ${total} / ${numSamples}
    ${diff}=                 Evaluate    abs(${avg} - ${expectedDutyCycle})
    Should Be True           ${diff} <= ${tolerance}
    ...    Average duty cycle ${avg} not within ${tolerance} of expected ${expectedDutyCycle}

*** Test Cases ***
Average Duty Cycle Over Full Sine Period (TIM4 Register Fallback)
    [Documentation]    Fallback for the primary smoke test, per the note at the
    ...    bottom of test_pwm_led.robot: instead of relying on Renode's
    ...    LEDTester to model TIM4 CH1 toggling PD12 on compare match, read
    ...    TIM4->CCR1 and TIM4->ARR directly. Samples every 0.1s across one
    ...    full 3.6s sine period and averages; avg(sin(x)) over a full period
    ...    is 0, so the average duty should be ~50% regardless of phase
    ...    alignment at boot.
    Setup Machine
    Assert TIM4 Average Duty Cycle    testDuration=3.6    expectedDutyCycle=0.5    tolerance=0.1

Duty Cycle Near Sine Peak (TIM4 Register Fallback)
    [Documentation]    Runs to 0.9s after boot (x=90 degrees, sin=1, duty
    ...    nearing 100%) and reads TIM4->CCR1/ARR directly. This is one of
    ...    the reference points manually verified in the comment at the
    ...    bottom of test_pwm_led.robot (observed CCR1=998, ARR=999).
    Setup Machine
    Execute Command          emulation RunFor "0.9s"
    Assert TIM4 Duty Cycle    expectedDutyCycle=1.0    tolerance=0.05

Duty Cycle Near Sine Trough (TIM4 Register Fallback)
    [Documentation]    Runs to 2.7s after boot (x=270 degrees, sin=-1, duty
    ...    nearing 0%) and reads TIM4->CCR1/ARR directly. This is one of the
    ...    reference points manually verified in the comment at the bottom of
    ...    test_pwm_led.robot (observed CCR1=0, ARR=999).
    Setup Machine
    Execute Command          emulation RunFor "2.7s"
    Assert TIM4 Duty Cycle    expectedDutyCycle=0.0    tolerance=0.05

Duty Cycle Near Boot (TIM4 Register Fallback)
    [Documentation]    Runs to 0.05s after boot (x still in the first few
    ...    degrees) and reads TIM4->CCR1/ARR directly. This is the third
    ...    reference point manually verified in the comment at the bottom of
    ...    test_pwm_led.robot (observed CCR1~534, ARR=999, i.e. duty~0.534).
    Setup Machine
    Execute Command          emulation RunFor "0.05s"
    Assert TIM4 Duty Cycle    expectedDutyCycle=0.534    tolerance=0.05
