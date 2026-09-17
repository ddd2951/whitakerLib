// Embedded payload with an invalid image magic for the malformed-image test.
.section .rodata.whitaker_layout,"a",@progbits
.balign 8
.global whitaker_layout_image_start
.global whitaker_layout_image_end
whitaker_layout_image_start:
.ascii "NOTWHIT3"
.fill 312, 1, 0
whitaker_layout_image_end:
.section .note.GNU-stack,"",@progbits
