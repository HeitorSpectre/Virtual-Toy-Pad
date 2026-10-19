/* flat position-independent image: header first, everything contiguous */
ENTRY(h_HandleEvents)
SECTIONS {
  . = 0;
  .hdr    : { KEEP(*(.hdr)) }
  .text   : { *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data   : { *(.data .data.*) }
  .bss    : { *(.bss .bss.* COMMON) }
  _end = .;
  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
