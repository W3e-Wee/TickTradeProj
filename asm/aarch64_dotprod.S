    // uint64_t dot_product_u64(const uint64_t *a, const uint64_t *b,
    //                          size_t count)
    // AAPCS64: x0/x1/x2 carry arguments, x0 returns the result. x3-x6 are
    // caller-saved temporaries; no callee-saved register or stack storage is
    // used, so the 16-byte stack-alignment rule is preserved.
    // LDR supports naturally aligned uint64_t arrays. Products and sum wrap
    // modulo 2^64, matching unsigned C arithmetic. count==0 returns zero.
    .text
    .align 2
    .global dot_product_u64
    .type dot_product_u64, %function

dot_product_u64:
    // x0 = uint64_t *a
    // x1 = uint64_t *b
    // x2 = size_t count
    // x3 = accumulator
    mov x3, #0
    cbz x2, 2f
1:
    ldr x4, [x0], #8
    ldr x5, [x1], #8
    mul x6, x4, x5
    add x3, x3, x6
    sub x2, x2, #1
    cbnz x2, 1b
2:
    mov x0, x3
    ret
    .size dot_product_u64, .-dot_product_u64
