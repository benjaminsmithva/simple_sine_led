/*
 * stm32f407.ld — Linker script for STM32F407VG (Cortex-M4)
 *
 * Memory map:
 *   FLASH:  1 MB  at 0x08000000  (code, rodata, init values)
 *   RAM:  128 KB  at 0x20000000  (stack, .data, .bss, heap)
 */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

ENTRY(Reset_Handler)

SECTIONS
{
  /* Vector table must be first in flash at 0x08000000 */
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  /* Code and read-only data */
  .text :
  {
    *(.text*)
    *(.rodata*)
  } > FLASH

  /*
   * ARM exception unwinding tables — required by libgcc even with
   * -fno-exceptions; omitting these causes undefined __exidx_start/end.
   */
  .ARM.extab :
  {
    *(.ARM.extab*)
    *(.gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    *(.gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH

  /* C constructor/destructor tables — iterated by __libc_init_array */
  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  /*
   * Initialized data — stored in FLASH (LMA), copied to RAM (VMA) by
   * Reset_Handler before main(). .got MUST come before .data* so the
   * Global Offset Table is covered by the startup copy loop.
   */
  .data :
  {
    _sdata = .;
    *(.got)
    *(.got.plt)
    *(.data*)
    _edata = .;
  } > RAM AT > FLASH

  /* Flash address of .data load image — used by Reset_Handler copy loop */
  _sidata = LOADADDR(.data);

  /* Zero-initialized data — zeroed by Reset_Handler before main() */
  .bss :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM

  /* Stack grows downward from top of RAM */
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
