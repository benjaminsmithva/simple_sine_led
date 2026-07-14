# simple_sine_led

## Objective

A bare-metal STM32F407VG (STM32F4DISCOVERY) demo: an integer loop variable `x`
sweeps through a sine wave forever, and `y = sin(x)` sets the brightness of the
onboard green LED (PD12) via hardware PWM. Since LED brightness can't be
controlled directly, `y` (mapped from `[-1, 1]` to a `[0, 1]` duty cycle) is
written to `TIM4` channel 1's compare register each step, producing a smooth
"breathing" fade rather than a discrete on/off blink.

`x` is an integer loop variable that sweeps `0..359` (interpreted as degrees,
converted to radians before calling `sinf()`) rather than literally `0..6`
(`round(2*pi)`) — the literal interpretation would only produce 7 visibly
chunky brightness steps. At 10ms per step, one full sine cycle takes 3.6
seconds. Both the step count and step interval are `#define`s in `main.c` if
you want to retune the fade speed/smoothness.

PD12/TIM4_CH1 (AF2) was chosen because it's the only onboard LED modeled as a
named `UserLED` peripheral (`sysbus.gpioPortD.UserLED`) in Renode's stock
STM32F4 Discovery platform — see the **Limitation** note at the bottom.

## Files copied from `../first_embedded`

- `startup_stm32f407.s`, `stm32f407.ld`, `syscalls.c` — used unmodified.
- `stm32f407.h` — used as the starting point, then **extended** here with a
  `TIM_t` register struct, `TIM4` base address/pointer, and PWM-relevant bit
  macros (`RCC_APB1ENR_TIM4EN`, `TIM_CR1_*`, `TIM_CCMR1_*`, `TIM_CCER_*`,
  `TIM_EGR_UG`). `first_embedded` never touched a hardware timer (its blink
  demo is purely SysTick + GPIO), so none of that existed before.
- `arm-none-eabi.cmake`, and the overall `CMakeLists.txt` structure (target
  layout, linker flags, `flash`/`debug`/`disasm` custom targets) — copied and
  adapted. One notable change: this project links `-lm` via
  `target_link_libraries` (not embedded in `target_link_options`) because
  `main.c` calls `sinf()` — see the comment in `CMakeLists.txt` for why
  ordering matters there.

## Files copied from `../first_renode`

- `platforms/stm32f4_discovery_full.repl` — used **as-is**, no changes needed.
  It already corrects SysTick frequency (16MHz HSI, matching this firmware's
  clock assumption), adds CCM RAM, and fixes SRAM/flash sizes.
- The `.resc` script structure (`scripts/sine_demo.resc`,
  `scripts/sine_demo_gdb.resc`) mirrors `first_renode/scripts/full_demo*.resc`,
  minus the UART terminal connector (this firmware has no UART).
- The `.robot` test structure (`*** Variables ***` block with `${ELF}`/`${REPL}`,
  no `Library` import) follows the same pattern as `first_renode`'s tests.

## Building and flashing on real hardware

```bash
cmake --preset stm32
cmake --build --preset stm32          # -> build/firmware.elf, build/firmware.bin
cmake --build build --target flash    # program via OpenOCD/ST-LINK
```

Other useful targets: `cmake --build build --target size`, `... --target
disasm`, `... --target debug` (starts an OpenOCD GDB server on `:3333`
without programming — same workflow as `first_embedded`'s README).

## Running under Renode

This assumes you've already gone through `../first_renode`'s README for the
Renode + Robot Framework install (portable Renode at `~/renode_portable`,
`renode-venv` with Robot Framework installed).

Unlike `first_embedded`'s blink demo, this firmware has **no UART**, so there's
no text console to watch — brightness must be observed via the LED's simulated
state.

**Console-only, watch LED state changes:**
```
$ renode --console
(monitor) mach create
(machine-0) machine LoadPlatformDescription @platforms/stm32f4_discovery_full.repl
(machine-0) logLevel -1 sysbus.gpioPortD.UserLED
(machine-0) sysbus LoadELF @build/firmware.elf
(machine-0) start
```
`logLevel -1` prints a `LED state changed to True/False` line on every PWM
edge (~1kHz). Watching the *density* of these lines is a rough textual proxy
for the breathing pattern: they cluster tightly near 50% duty and sparsely
near the 0%/100% extremes of the sine wave.

**Scripted (same as above, one command):**
```bash
renode --console scripts/sine_demo.resc
```

**GDB debugging:**
```
$ renode --console scripts/sine_demo_gdb.resc     # starts halted, GDB server on :3333
```
In a second terminal:
```bash
gdb-multiarch build/firmware.elf
(gdb) target remote :3333
(gdb) break main
(gdb) continue
```

## Robot tests

```bash
source ~/renode_portable/renode-venv/bin/activate
renode-test tests/test_pwm_led.robot
```

Three test cases, all using `Create LED Tester` on `sysbus.gpioPortD.UserLED`
+ `Assert LED Duty Cycle testDuration=<s> expectedDutyCycle=<0..1>
tolerance=<t>`:

1. **`Average Duty Cycle Over Full Sine Period`** — samples over one full
   3.6s sine cycle from boot. The average of `sin(x)` over a full period is
   0, so average duty should be ~50% *regardless of exact phase alignment*
   between Renode and the firmware's SysTick timing — this is the primary,
   timing-insensitive smoke test.
2. **`Duty Cycle Near Sine Peak`** — runs to ~0.85s (x approaching 90°, duty
   approaching 100%) before sampling, deliberately near the peak where duty
   changes slowest (sine's derivative ≈ 0 there), giving a stable window.
3. **`Duty Cycle Near Sine Trough`** — same idea at ~2.65s (x approaching
   270°, duty approaching 0%).

`Assert LED Duty Cycle` depends on Renode's STM32 timer model actually
toggling the GPIO pin on PWM compare match (TIM4 CH1 → PD12 via AF2 →
`UserLED`). This was manually verified working for this firmware/platform
combination by reading `TIM4->CCR1` (`0x40000834`) directly at 0.05s/0.9s/2.7s
and observing ≈534/998/0 out of `ARR=999` — matching the expected sine phase
at each point. GitHub issue `renode/renode#562` reports this PWM-to-GPIO
modeling not working for some other users/Renode versions; if `Assert LED
Duty Cycle` proves flaky in a different setup, fall back to reading TIM4
registers directly (documented in a comment at the bottom of
`tests/test_pwm_led.robot`):
```
Execute Command    sysbus ReadDoubleWord 0x40000834    # TIM4->CCR1
Execute Command    sysbus ReadDoubleWord 0x4000082C    # TIM4->ARR
```
and computing `duty = CCR1 / (ARR + 1)` — this validates firmware intent
independent of the GPIO-propagation path.

## Limitation

Only PD12 (green) is wired to hardware PWM here. The other three onboard
LEDs (PD13/14/15 — orange/red/blue) aren't used, because they aren't modeled
as a named `UserLED` peripheral in Renode's stock STM32F4 Discovery platform
— exercising them in a robot test would require extending
`platforms/stm32f4_discovery_full.repl` with an explicit LED peripheral
declaration for that GPIO line.
