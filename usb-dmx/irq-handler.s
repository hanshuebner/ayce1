;;; call at 250 khz
	.section	.text.__vector_21,"ax",@progbits
.global	__vector_21
__vector_21:
	push r0
	in r0,__SREG__
	push r0
	push zl
	push zh
        in gpior1, zl
        in gpior2, zh
        ld r0, z+
        out portd, r0
        ld r0, z+
        out portb, r0
;;; wenn tc0 compare match, beide timer abschalten
        out gpior1, zl
        out gpior2, zh
.done:
/* epilogue start */
	pop zh
	pop zl
	pop r0
	out __SREG__,r0
	pop r0
	reti
