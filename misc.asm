*\  :ts=8 bk=0
*
* misc.asm:	Helpful stuff.
*
* Leo L. Schwab						9305.20
*/
		AREA	misc, CODE
		ALIGN

****************************************************************************
* resetlinebuf
*
* SYNOPSIS
*	resetlinebuf (buf)
*		      r0
*
*	int32	*buf;
*
* DESCRIPTION
*	Sets the contents of buf to contain ~0 (all ones).  The size of
*	buf is assumed to be 40 bytes.
*
		EXPORT	resetlinebuf
resetlinebuf	ROUT

		mvn	r1, #0
		mvn	r2, #0
		mvn	r3, #0

		stmia	r0!, {r1-r3}
		stmia	r0!, {r1-r3}
		stmia	r0!, {r1-r3}
		str	r1, [r0]

		mov	pc, lr		; RTS


****************************************************************************
* islinefull
*
* SYNOPSIS
*	int islinefull (buf)
*		        r0
*
*	int32	*buf;
*
* DESCRIPTION
*	Tests the contents of buf.  Returns TRUE if all zero; FALSE
*	otherwise.
*
		EXPORT	islinefull
islinefull	ROUT

		ldmia	r0!, {r1-r3}	; Check for any nonzero contents
		orr	r1, r1, r2
		orrs	r1, r1, r3
		bne	notfull

		ldmia	r0!, {r1-r3}
		orr	r1, r1, r2
		orrs	r1, r1, r3
		bne	notfull

		ldmia	r0!, {r1-r3}
		orr	r1, r1, r2
		orrs	r1, r1, r3
		bne	notfull

		ldr	r1, [r0]
		cmp	r1, #0

notfull		movne	r0, #0		; FALSE if any bits nonzero
		moveq	r0, #1		; TRUE if all zero

		mov	pc, lr		; RTS


****************************************************************************
* testmarklinebuf
*
* SYNOPSIS
*	int testmarklinebuf (buf, lx, rx)
*			     r0   r1  r2
*
*	int32	*buf;
*	int32	lx, rx;
*
* DESCRIPTION
*	Clears the bits in buf from lx to rx, inclusive, while testing them
*	to see if any were set in the first place.  Returns TRUE if any bits
*	in the range were set, FALSE otherwise.
*
		EXPORT	testmarklinebuf
testmarklinebuf	ROUT

		stmdb	sp, {r4-r6}
		mov	r6, #0		; FALSE

	;------	Are start and end indicies the same?  If so, don't bother.
		cmp	r1, r2		; if (lx == rx)
		beq	exitFALSE	;	return (FALSE)

	;------	Factor in MapCel() artifact.
	;------	If left greater than right, swap values.
		sub	r2, r2, #1	; rx--
		cmp	r1, r2		; if (lx > rx)
		eorgt	r1, r1, r2	;	swap (lx, rx)
		eorgt	r2, r2, r1
		eorgt	r1, r1, r2

	;------	If beyond edges of screen, don't bother.
		cmp	r1, #320	; if (lx >= 320  ||
		bge	exitFALSE
		cmp	r2, #0		;     rx < 0)
		blt	exitFALSE	;	return (FALSE)

	;------	Truncate to edges of screen.
		cmp	r1, #0		; if (lx < 0)
		movlt	r1, #0		;	lx = 0;
		cmp	r2, #320	; if (rx >= 320)
		movgt	r2, #320	;	rx = 319;
		subgt	r2, r2, #1

	;------	Get first and last longword masks.
		mov	r3, r1		; lm = leftmasks[lx & 31];
		mov	r4, r2		; rm = rightmasks[rx & 31];
		and	r3, r3, #31
		and	r4, r4, #31
		ldr	r5, =leftmasks
		ldr	r3, [r5, +r3, LSL #2]
		ldr	r5, =rightmasks
		ldr	r4, [r5, +r4, LSL #2]

	;------	Shift down to get indicies.
		mov	r1, r1, ASR #5	; lx >>= 5
		mov	r2, r2, ASR #5	; rx >>= 5

		cmp	r1, r2
		bne	domany

	;------	We begin and end within the same longword.  Fold masks
	;------	and test relevant longword.
		and	r3, r3, r4	; lm &= rm;
		ldr	r5, [r0, +r1, LSL #2]	; tmp = buf[lx]
		tst	r5, r3		; if (tmp & lm)
		bicne	r5, r5, r3	;	tmp &= ~lm
		strne	r5, [r0, +r1, LSL #2]	; buf[lx] = tmp
		movne	r6, #1		; 	TRUE

exitFALSE	mov	r0, r6
		ldmdb	sp, {r4-r6}
		mov	pc, lr		; RTS


	;------	We span multiple longwords.  Scan and clear relevant bits.
domany		ldr	r5, [r0, +r1, LSL #2]	; tmp = buf[lx]
		tst	r5, r3		; if (tmp & lm)
		movne	r6, #1
		bic	r5, r5, r3	; tmp &= ~lm
		str	r5, [r0, +r1, LSL #2]
		mvn	r3, #0		; lm = ~0

maskloop	add	r1, r1, #1	; lx++
		cmp	r1, r2		; if (lx == rx)
		moveq	r3, r4		;	lm = rm;
		ldr	r5, [r0, +r1, LSL #2]
		tst	r5, r3		; if (tmp & lm)
		movne	r6, #1		;	result = TRUE;
		bic	r5, r5, r3
		str	r5, [r0, +r1, LSL #2]
		cmp	r1, r2
		blt	maskloop	; do { } while (lx < rx)

		mov	r0, r6
		ldmdb	sp, {r4-r6}
		mov	pc, lr		; RTS


	;------	Masking data.
leftmasks
		DCD	&FFFFFFFF
		DCD	&7FFFFFFF
		DCD	&3FFFFFFF
		DCD	&1FFFFFFF
		DCD	&0FFFFFFF
		DCD	&07FFFFFF
		DCD	&03FFFFFF
		DCD	&01FFFFFF
		DCD	&00FFFFFF
		DCD	&007FFFFF
		DCD	&003FFFFF
		DCD	&001FFFFF
		DCD	&000FFFFF
		DCD	&0007FFFF
		DCD	&0003FFFF
		DCD	&0001FFFF
		DCD	&0000FFFF
		DCD	&00007FFF
		DCD	&00003FFF
		DCD	&00001FFF
		DCD	&00000FFF
		DCD	&000007FF
		DCD	&000003FF
		DCD	&000001FF
		DCD	&000000FF
		DCD	&0000007F
		DCD	&0000003F
		DCD	&0000001F
		DCD	&0000000F
		DCD	&00000007
		DCD	&00000003
		DCD	&00000001

rightmasks
		DCD	&80000000
		DCD	&C0000000
		DCD	&E0000000
		DCD	&F0000000
		DCD	&F8000000
		DCD	&FC000000
		DCD	&FE000000
		DCD	&FF000000
		DCD	&FF800000
		DCD	&FFC00000
		DCD	&FFE00000
		DCD	&FFF00000
		DCD	&FFF80000
		DCD	&FFFC0000
		DCD	&FFFE0000
		DCD	&FFFF0000
		DCD	&FFFF8000
		DCD	&FFFFC000
		DCD	&FFFFE000
		DCD	&FFFFF000
		DCD	&FFFFF800
		DCD	&FFFFFC00
		DCD	&FFFFFE00
		DCD	&FFFFFF00
		DCD	&FFFFFF80
		DCD	&FFFFFFC0
		DCD	&FFFFFFE0
		DCD	&FFFFFFF0
		DCD	&FFFFFFF8
		DCD	&FFFFFFFC
		DCD	&FFFFFFFE
		DCD	&FFFFFFFF


****************************************************************************
* mkVertPtrs
*
* SYNOPSIS
*	mkVertPtrs (verts, ptrArry, idx0, idx1, idx2, idx3)
*		     r0      r1      r2    r3   [sp] [sp+4]
*
*	Vertex	*verts;
*	Vertex	**ptrArry;
*	int32	idx0, idx1, idx2, idx3
*
* DESCRIPTION
*	Convert the indicies into pointers to verticies.
*
		EXPORT	mkVertPtrs
mkVertPtrs	ROUT

		stmdb	sp, {r4-r5}

		ldmia	sp, {r4, r5}

		add	r2, r2, r2, LSL #1	; n *= 3
		add	r3, r3, r3, LSL #1
		add	r4, r4, r4, LSL #1
		add	r5, r5, r5, LSL #1

		add	r2, r0, r2, LSL #2	; n = verts + (n * 4)
		add	r3, r0, r3, LSL #2
		add	r4, r0, r4, LSL #2
		add	r5, r0, r5, LSL #2

		stmia	r1, {r2-r5}

		ldmdb	sp, {r4-r5}
		mov	pc, lr		; RTS


		END
