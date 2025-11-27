ENTRY(_start)

SECTIONS
{
  . = 1M;

  kernel_start = .;

  .multiboot ALIGN(4) :
  {
    *(.multiboot)
  }

  .text ALIGN(4) :
  {
    *(.text*)
  }

  .rodata ALIGN(4) :
  {
    *(.rodata*)
  }

  .data ALIGN(4) :
  {
    *(.data*)
  }

  .bss ALIGN(4) :
  {
    *(.bss*)
    *(COMMON)
  }

  kernel_end = .;
}
