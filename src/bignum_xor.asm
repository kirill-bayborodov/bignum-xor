; @file    bignum_xor.asm
; @brief   x86-64 System V AMD64 implementation of bignum_xor.
; @version 0.1.0
; @details result = a ^ b. Exact result==a/result==b aliases are supported;
;          partial overlap is rejected before any destination write. The kernel
;          iterates to max(a->len,b->len), treating missing words as zero.
; -----------------------------------------------------------------------------
; SPDX-License-Identifier: MIT
; -----------------------------------------------------------------------------
default rel
section .text
    align 16
    global bignum_xor

BIGNUM_CAPACITY                    equ 32
BIGNUM_WORD_SIZE                   equ 8
BIGNUM_SIZE                        equ 264
BIGNUM_OFFSET_LEN                  equ 256
SUCCESS                            equ 0
ERROR_NULL_PTR                     equ -1
ERROR_CAPACITY_EXCEEDED            equ -2
ERROR_BUFFER_OVERLAP               equ -3

; bignum_xor_status_t bignum_xor(bignum_t *result, const bignum_t *a,
;                              const bignum_t *b)
; rdi = result, rsi = a, rdx = b
; r8/r9 = input lengths, r10 = output length, rcx = word index.
bignum_xor:
    test    rdi, rdi
    jz      .error_null
    test    rsi, rsi
    jz      .error_null
    test    rdx, rdx
    jz      .error_null

    mov     r8, [rsi + BIGNUM_OFFSET_LEN]
    mov     r9, [rdx + BIGNUM_OFFSET_LEN]
    cmp     r8, BIGNUM_CAPACITY
    ja      .error_capacity
    cmp     r9, BIGNUM_CAPACITY
    ja      .error_capacity

    ; Reject partial overlap for every pair; exact aliases are permitted.
    cmp     rdi, rsi
    je      .check_result_b
    lea     rax, [rdi + BIGNUM_SIZE]
    cmp     rax, rsi
    jbe     .check_result_b
    lea     r11, [rsi + BIGNUM_SIZE]
    cmp     r11, rdi
    jbe     .check_result_b
    jmp     .error_overlap

.check_result_b:
    cmp     rdi, rdx
    je      .check_inputs
    lea     rax, [rdi + BIGNUM_SIZE]
    cmp     rax, rdx
    jbe     .check_inputs
    lea     r11, [rdx + BIGNUM_SIZE]
    cmp     r11, rdi
    jbe     .check_inputs
    jmp     .error_overlap

.check_inputs:
    cmp     rsi, rdx
    je      .compute_len
    lea     rax, [rsi + BIGNUM_SIZE]
    cmp     rax, rdx
    jbe     .compute_len
    lea     r11, [rdx + BIGNUM_SIZE]
    cmp     r11, rsi
    jbe     .compute_len
    jmp     .error_overlap

.compute_len:
    mov     r10, r8
    cmp     r9, r10
    cmova   r10, r9                    ; out_len = max(a->len, b->len)
    xor     ecx, ecx

.word_loop:
    cmp     rcx, r10
    jae     .zero_tail

    xor     eax, eax
    cmp     rcx, r8
    jae     .a_missing
    mov     rax, [rsi + rcx * BIGNUM_WORD_SIZE]
.a_missing:
    xor     r11d, r11d
    cmp     rcx, r9
    jae     .b_missing
    mov     r11, [rdx + rcx * BIGNUM_WORD_SIZE]
.b_missing:
    xor      rax, r11
    mov     [rdi + rcx * BIGNUM_WORD_SIZE], rax
    inc     rcx
    jmp     .word_loop

.zero_tail:
    cmp     rcx, BIGNUM_CAPACITY
    jae     .normalize
    xor     eax, eax
.zero_loop:
    mov     [rdi + rcx * BIGNUM_WORD_SIZE], rax
    inc     rcx
    cmp     rcx, BIGNUM_CAPACITY
    jb      .zero_loop

.normalize:
    mov     [rdi + BIGNUM_OFFSET_LEN], r10
.normalize_loop:
    test    r10, r10
    jz      .success
    cmp     qword [rdi + r10 * BIGNUM_WORD_SIZE - BIGNUM_WORD_SIZE], 0
    jne     .success
    dec     r10
    mov     [rdi + BIGNUM_OFFSET_LEN], r10
    jmp     .normalize_loop

.error_null:
    mov     eax, ERROR_NULL_PTR
    ret
.error_capacity:
    mov     eax, ERROR_CAPACITY_EXCEEDED
    ret
.error_overlap:
    mov     eax, ERROR_BUFFER_OVERLAP
    ret
.success:
    xor     eax, eax
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
