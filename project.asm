;\ :ts=8 bk=0
;
; project.asm:	3D point projector.
;
; Leo L. Schwab						9206.01
;/
		AREA	project_code, CODE
		ALIGN

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; project
;
; SYNOPSIS
;	project (src, dest, magic, zpull,  cx,   cy,  npoints)
;		 r0    r1    r2     r3    [sp] [sp+4]  [sp+8]
;
;	Vertex	*src, *dest;
;	LONG	magic, zpull;
;	LONG	cx, cy;
;	LONG	npoints;
;
; NOTE
;	'src' and 'dest' are identically formatted.  'zpull' and 'magic' are
;	in fixed-point format.
;
		IMPORT	__rt_sdiv

		EXPORT	project
project		ROUT

		stmdb	sp, {r4-r12,lr}	; STACK POINTER DOESN'T MOVE
		ldr	r8, [sp, #8]	; Fetch npoints from stack
		mov	r4, r0		; Move these parameters up
		mov	r5, r1		;  (division routine destroys them)
		mov	r6, r2, ASL #15
		mov	r7, r3

0		subs	r8, r8, #1	; Decrement counter
		ldmmidb	sp, {r4-r12,pc}	; All done, restore regs, RTS

		ldmia	r4!, {r9-r11}	; Fetch source XYZ
		adds	r11, r11, r7	; Add zpull
		movle	r11, #&80000000	; Behind camera; set invalid point
		ble	writexyz

		mov	r9, r9, ASR #8	; Shift off part of fraction
		mov	r10, r10, ASR #8
		mov	r11, r11, ASR #8

		mov	r0, r11		; z
		mov	r1, r6		; magic
		bl	__rt_sdiv	; r0 == magic / z
		mul	r9, r0, r9	; x = x * r0
		mul	r10, r0, r10	; y = y * r0

		ldmia	sp, {r2,r3}	; Load cx,cy
		add	r9, r2, r9, ASR #15	; x = (x>>SHIFT_DECIMAL) + cx
		add	r10, r3, r10, ASR #15	; y = (y>>SHIFT_DECIMAL) + cy

writexyz	stmia	r5!, {r9-r11}
		b	%0

		END
