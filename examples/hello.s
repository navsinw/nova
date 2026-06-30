; NOVA-8 assembly demo: arithmetic, registers, a RAM store, a small loop
.code
start:
    push 10
    push 20
    add
    st 0          ; r0 = 30
    bank 1
    push 5        ; addr
    push 42       ; val
    stm
    push 0
    st 1          ; r1 = i = 0
loop:
    ld 1
    push 4
    cmp
    push 1
    add
    jnz done      ; while (i < 4)
    ld 1
    push 1
    add
    st 1
    jmp loop
done:
    halt

.data
    .byte 1, 2, 3, 4, 5
    .word 0x1000
    .space 8
