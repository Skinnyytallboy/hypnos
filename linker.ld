ENTRY(_start)

SECTIONS
{
  /* Link address 1 MiB (typical for Multiboot OS) */
  . = 1M;

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
}
