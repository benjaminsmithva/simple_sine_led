cmake_minimum_required(VERSION 3.20)
project(sine_led C ASM)

# ── Compiler flags ─────────────────────────────────────────────────────────
# CPU flags are also set by the toolchain file; repeating here ensures they
# apply when -DCMAKE_TOOLCHAIN_FILE is not passed (e.g. manual invocation).
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

set(CMAKE_C_FLAGS
    "${CPU_FLAGS} -Os -g -ffunction-sections -fdata-sections -Wall -Wextra -ffreestanding"
    CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS
    "${CPU_FLAGS}"
    CACHE STRING "" FORCE)

# ── Firmware target ────────────────────────────────────────────────────────
add_executable(firmware.elf
    startup_stm32f407.s
    syscalls.c
    main.c
)

# Header search path: project root (for stm32f407.h)
target_include_directories(firmware.elf PRIVATE ${CMAKE_SOURCE_DIR})

# ── Linker flags ───────────────────────────────────────────────────────────
target_link_options(firmware.elf PRIVATE
    -nostartfiles                           # exclude crti.o/crtn.o; we provide _init/_fini stubs
    -specs=nano.specs                       # reduced-size newlib
    -T${CMAKE_SOURCE_DIR}/stm32f407.ld     # custom linker script
    -Wl,--gc-sections                       # strip unused code/data
    -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map
)

# Libraries must be linked via target_link_libraries (not embedded as -l flags
# in target_link_options) so CMake places them after the object files on the
# link line — main.c calls sinf(), which otherwise goes unresolved because the
# linker only pulls symbols from a library while scanning left-to-right.
target_link_libraries(firmware.elf PRIVATE c m gcc)

# ── Post-build: binary image and size report ───────────────────────────────
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary firmware.elf firmware.bin
    COMMENT "Generating firmware.bin"
)
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND arm-none-eabi-size firmware.elf
    COMMENT "Firmware size:"
)

# ── Helper targets ─────────────────────────────────────────────────────────

# flash — program firmware.bin and immediately start the chip running
add_custom_target(flash
    COMMAND openocd
        -f interface/stlink.cfg
        -f target/stm32f4x.cfg
        -c "transport select hla_swd"
        -c "program ${CMAKE_BINARY_DIR}/firmware.bin 0x08000000 verify reset exit"
    DEPENDS firmware.elf
    COMMENT "Flashing firmware via OpenOCD (chip runs after)"
    USES_TERMINAL
)

# debug — start OpenOCD as a GDB server; chip is halted, waiting on port 3333
#         In a second terminal run: arm-none-eabi-gdb firmware.elf
#                                   (gdb) target remote :3333
add_custom_target(debug
    COMMAND openocd
        -f interface/stlink.cfg
        -f target/stm32f4x.cfg
        -c "transport select hla_swd"
    DEPENDS firmware.elf
    COMMENT "Starting OpenOCD GDB server on :3333 (Ctrl-C to stop)"
    USES_TERMINAL
)

# disasm — interleaved disassembly with source
add_custom_target(disasm
    COMMAND arm-none-eabi-objdump -d -S ${CMAKE_BINARY_DIR}/firmware.elf
    DEPENDS firmware.elf
    COMMENT "Disassembly:"
    USES_TERMINAL
)
