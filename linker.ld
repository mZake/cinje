MEMORY
{
    ROM : ORIGIN = BLOB_BEGIN, LENGTH = 32M
}

SECTIONS
{
    .text   : { *(.text) } >ROM
    .rodata : { *(.rodata) } >ROM
    .data   : { *(.data) } >ROM
    .bss    : { *(.bss) *(COMMON) } >ROM
    BLOB_END = .;
}
