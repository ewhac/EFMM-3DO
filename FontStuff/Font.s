;	Assembly flags
DEBUG		EQU	1
SOUNDSTUFF	EQU	1
NO_GLASSES	EQU	1
;    Copyright (C) 1992, The 3DO Company, Inc.

		INCLUDE	macros.i
		INCLUDE	structs.i
		INCLUDE	nodes.i
		INCLUDE	mem.i
		INCLUDE	folio.i
		INCLUDE	list.i
		INCLUDE	graphics.i
		INCLUDE	item.i
		INCLUDE	kernelmacros.i
		INCLUDE	kernel.i
		INCLUDE	kernelnodes.i
		INCLUDE	operamath.i
		INCLUDE	registers.i
		INCLUDE	hardware.i
		INCLUDE	task.i

		INCLUDE	stack.i
		INCLUDE	font.i

		IMPORT	GrafBase
		IMPORT	KernelBase

		AREA	code_area,CODE
;
;	Name:
;		FontInit
;	Purpose:
;		Allocates memory and initializes Font
;	Entry:
;		Maximum number of characters per line
;	Exit:
;		None
;	C Prototype:
;		void FontInit (int32 num)
;
FontMaxLetters
		DCD	0
FontListPtr
		DCD	0
FontMemSize
		DCD	0
FontInit	PROC
		ENTER	r4-r5
		str	r0,FontMaxLetters
		subs	r4,r0,#1
		bmi	%99		; invalid number

;	pointer to cels table

		mov	r0,#SizeOfFirstCharacter
		mov	r1,#SizeOfOtherCharacters
		mla	r0,r1,r4,r0
		str	r0,FontMemSize
		mov	r1,#MEMTYPE_CEL
		bl	MyAllocMem
		str	r0,FontListPtr
		cmp	r0,#0
		beq	%99

;	Only init routine so don't worry that this is slow

		ADR	r1,FirstCharacter
		mov	r2,#SizeOfFirstCharacter/4
0
		ldr	r3,[r1],#4
		str	r3,[r0],#4
		subs	r2,r2,#1
		bgt	%0

		cmp	r4,#0			; check if only one character string
		beq	%99
1
		ADR	r1,OtherCharacters
		mov	r2,#SizeOfOtherCharacters/4
2
		ldr	r3,[r1],#4
		str	r3,[r0],#4
		subs	r2,r2,#1
		bgt	%2
		subs	r4,r4,#1
		bgt	%1
99
		EXIT	r4-r5
10
		DCD	KernelBase
FirstCharacter
FirstCharFlags1	EQU	CCB_SPABS+CCB_PPABS+CCB_LDSIZE+CCB_LDPRS
FirstCharFlags2	EQU	CCB_LDPPMP+CCB_LDPLUT+CCB_YOXY+CCB_ACW
FirstCharFlags3	EQU	CCB_ACCW+CCB_ACE+CCB_PACKED
		DCD	FirstCharFlags1+FirstCharFlags2+FirstCharFlags3
		DCD	11*4		; ccb_NextPtr
		DCD	0		; ccb_SourcePtr
		DCD	0		; ccb_PLUTPtr
		DCD	0		; ccb_XPos
		DCD	0		; ccb_YPos
		DCD	0		; ccb_HDX
		DCD	&100000		; ccb_HDY
		DCD	&10000		; ccb_VDX
		DCD	0		; ccb_VDY
		DCD	0		; ccb_HDDX
		DCD	0		; ccb_HDDY
		DCD	&1F001F00	; ccb_PIXC
SizeOfFirstCharacter	EQU	.-FirstCharacter
OtherCharacters
		DCD	CCB_SPABS+CCB_ACW+CCB_ACCW+CCB_ACE+CCB_PACKED
		DCD	1*4	; ccb_NextPtr
		DCD	0	; ccb_SourcePtr
SizeOfOtherCharacters	EQU	.-OtherCharacters

;
;	Name:
;		FontPrint
;	Purpose:
;		Prints a text message using a font.
;		Expects the character set to be packed,
;		rotated 90 degrees to be facing down,
;		and horizontally flipped.
;		The control point is in the top left corner.
;		Accepts strings with CR/LF.
;	Entry:
;		Character set pointer
;		PLUT pointer
;		x Coord
;		y Coord
;	Exit:
;		None
;	C Prototype:
;		void FontPrint (FontStruct* text)
;

FontPrint	PROC
		ENTER	r4-r9
		ldmia	r0!,{r4-r9}
		ldr	r0,[r0]
		ldr	r3,FontListPtr
		cmp	r3,#0
		beq	%99			; error check
		str	r0,[r3,#ccb_PLUTPtr]
		mov	r6,r6,LSL #16
		mov	r7,r7,LSL #16
		mov	r8,r8,LSL #16
		str	r6,[r3,#ccb_XPos]
		str	r7,[r3,#ccb_YPos]
;	New line
0
		ldr	r6,FontMaxLetters
		mov	r7,r3
		ldrb	r0,[r4],#1
		cmp	r0,#0
		beq	%90
		subs	r6,r6,#1
		bmi	%90
		cmp	r0,#&0a				;lf
		beq	%10
		cmp	r0,#&0d				;cr
		beq	%10

		ldr	r0,[r5,r0,LSL #2]
		add	r0,r5,r0
		str	r0,[r3,#ccb_SourcePtr]
	
		add	r3,r3,#SizeOfFirstCharacter
1
		ldrb	r0,[r4],#1
		cmp	r0,#0
		beq	%90
		subs	r6,r6,#1
		bmi	%90
		cmp	r0,#&0a				;lf
		beq	%10
		cmp	r0,#&0d				;cr
		beq	%10

		ldr	r0,[r5,r0,LSL #2]
		add	r0,r5,r0
		str	r0,[r3,#ccb_SourcePtr]
		mov	r7,r3
		add	r3,r3,#SizeOfOtherCharacters
		b	%1
10
		cmp	r3,r7
		beq	%0

		ldr	r0,[r7,#ccb_Flags]
		orr	r0,r0,#CCB_LAST
		str	r0,[r7,#ccb_Flags]

		mov	r0,r9
		ldr	r1,FontListPtr
		LIBCALL	DrawCels

		ldr	r0,[r7,#ccb_Flags]
		bic	r0,r0,#CCB_LAST
		str	r0,[r7,#ccb_Flags]

		ldr	r3,FontListPtr
		ldr	r0,[r3,#ccb_YPos]
		add	r0,r0,r8
		str	r0,[r3,#ccb_YPos]
		b	%0
90
		cmp	r3,r7
		beq	%99

		ldr	r0,[r7,#ccb_Flags]
		orr	r0,r0,#CCB_LAST
		str	r0,[r7,#ccb_Flags]

		mov	r0,r9
		ldr	r1,FontListPtr
		LIBCALL	DrawCels

		ldr	r0,[r7,#ccb_Flags]
		bic	r0,r0,#CCB_LAST
		str	r0,[r7,#ccb_Flags]
99
		EXIT	r4-r9
;
;	Name:
;		FontFree
;	Purpose:
;		Frees memory allocation for characters
;	Entry:
;		None
;	Exit:
;		None
;	C Prototype:
;		void FontFree (void)
;
FontFree	PROC
		ENTER
		ldr	r0,FontListPtr
		ldr	r1,FontMemSize
		bl	MyFreeMem
		EXIT



		END

