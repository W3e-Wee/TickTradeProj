	.file	"market_baseline.c"
	.text
	.section .rdata,"dr"
.LC0:
	.ascii "True\0"
.LC1:
	.ascii "true\0"
.LC2:
	.ascii "False\0"
.LC3:
	.ascii "false\0"
	.text
	.p2align 4
	.def	parse_bool_value.constprop.0;	.scl	3;	.type	32;	.endef
	.seh_proc	parse_bool_value.constprop.0
parse_bool_value.constprop.0:
	pushq	%rsi
	.seh_pushreg	%rsi
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$40, %rsp
	.seh_stackalloc	40
	.seh_endprologue
	movq	%rdx, %rbx
	leaq	.LC0(%rip), %rdx
	movq	%rcx, %rsi
	call	strcmp
	testl	%eax, %eax
	je	.L2
	leaq	.LC1(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	testl	%eax, %eax
	je	.L2
	leaq	.LC2(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	testl	%eax, %eax
	je	.L4
	leaq	.LC3(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	testl	%eax, %eax
	sete	%al
	movzbl	%al, %eax
	addq	$40, %rsp
	popq	%rbx
	popq	%rsi
	ret
	.p2align 4,,10
	.p2align 3
.L2:
	movl	$1, (%rbx)
.L4:
	movl	$1, %eax
	addq	$40, %rsp
	popq	%rbx
	popq	%rsi
	ret
	.seh_endproc
	.section .rdata,"dr"
.LC4:
	.ascii "trade_id\0"
.LC5:
	.ascii ",\0"
	.text
	.p2align 4
	.globl	parse_trade_line
	.def	parse_trade_line;	.scl	2;	.type	32;	.endef
	.seh_proc	parse_trade_line
parse_trade_line:
	pushq	%r15
	.seh_pushreg	%r15
	pushq	%r14
	.seh_pushreg	%r14
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rdi
	.seh_pushreg	%rdi
	pushq	%rsi
	.seh_pushreg	%rsi
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$664, %rsp
	.seh_stackalloc	664
	movups	%xmm6, 624(%rsp)
	.seh_savexmm	%xmm6, 624
	movups	%xmm7, 640(%rsp)
	.seh_savexmm	%xmm7, 640
	.seh_endprologue
	pxor	%xmm0, %xmm0
	movq	$0, 96(%rsp)
	movq	%rcx, %rsi
	movq	%rdx, %rbx
	movups	%xmm0, 48(%rsp)
	movups	%xmm0, 64(%rsp)
	movups	%xmm0, 80(%rsp)
	call	strlen
	cmpq	$511, %rax
	ja	.L36
	leaq	1(%rax), %r8
	movq	%rsi, %rdx
	leaq	112(%rsp), %rcx
	call	memcpy
	movzbl	112(%rsp), %eax
	testb	%al, %al
	je	.L13
	leaq	112(%rsp), %rdx
	.p2align 4
	.p2align 3
.L16:
	cmpb	$13, %al
	je	.L40
	cmpb	$10, %al
	je	.L40
	movzbl	1(%rdx), %eax
	addq	$1, %rdx
	testb	%al, %al
	jne	.L16
.L13:
	leaq	112(%rsp), %rcx
	movl	$8388627, %r8d
	jmp	.L17
	.p2align 5
	.p2align 4,,10
	.p2align 3
.L19:
	addq	$1, %rcx
.L17:
	movzbl	(%rcx), %edx
	leal	-9(%rdx), %eax
	cmpb	$23, %al
	ja	.L18
	btq	%rax, %r8
	jc	.L19
.L20:
	movl	$8, %r8d
	leaq	.LC4(%rip), %rdx
	call	strncmp
	testl	%eax, %eax
	je	.L21
	movq	__imp_strtok(%rip), %rbp
	leaq	.LC5(%rip), %rdx
	leaq	112(%rsp), %rcx
	xorl	%edi, %edi
	call	*%rbp
	leaq	48(%rsp), %rsi
	testq	%rax, %rax
	je	.L25
	.p2align 4
	.p2align 3
.L23:
	movq	%rax, (%rsi)
	leaq	.LC5(%rip), %rdx
	xorl	%ecx, %ecx
	addq	$1, %rdi
	call	*%rbp
	addq	$8, %rsi
	testq	%rax, %rax
	je	.L41
	cmpq	$7, %rdi
	jne	.L23
.L54:
	movq	$0, 40(%rsp)
	movq	__imp__errno(%rip), %rsi
	call	*%rsi
	movq	48(%rsp), %rdi
	movl	$10, %r8d
	leaq	40(%rsp), %rdx
	movl	$0, (%rax)
	movq	%rdi, %rcx
	call	strtoll
	movq	%rax, %r14
	call	*%rsi
	movl	(%rax), %r10d
	testl	%r10d, %r10d
	jne	.L27
	movq	40(%rsp), %rax
	cmpq	%rax, %rdi
	je	.L27
	cmpb	$0, (%rax)
	jne	.L27
	movq	56(%rsp), %rbp
	leaq	40(%rsp), %rdx
	movq	%rbp, %rcx
	call	strtod
	movapd	%xmm0, %xmm6
	cmpq	40(%rsp), %rbp
	je	.L29
	call	*%rsi
	movl	(%rax), %r9d
	testl	%r9d, %r9d
	jne	.L29
	movq	64(%rsp), %rbp
	leaq	40(%rsp), %rdx
	movq	%rbp, %rcx
	call	strtod
	movapd	%xmm0, %xmm7
	cmpq	40(%rsp), %rbp
	je	.L31
	call	*%rsi
	movl	(%rax), %r8d
	testl	%r8d, %r8d
	jne	.L31
	movq	72(%rsp), %rbp
	leaq	40(%rsp), %rdx
	movq	%rbp, %rcx
	call	strtod
	movq	%xmm0, %r15
	cmpq	40(%rsp), %rbp
	je	.L33
	call	*%rsi
	movl	(%rax), %ecx
	testl	%ecx, %ecx
	jne	.L33
	movq	80(%rsp), %rbp
	movl	$10, %r8d
	leaq	40(%rsp), %rdx
	movq	%rbp, %rcx
	call	strtoll
	movq	%rax, %rdi
	call	*%rsi
	movl	(%rax), %esi
	testl	%esi, %esi
	jne	.L35
	movq	40(%rsp), %rax
	cmpq	%rax, %rbp
	je	.L35
	cmpb	$0, (%rax)
	jne	.L35
	movq	88(%rsp), %rcx
	leaq	32(%rsp), %rdx
	movl	$0, 32(%rsp)
	call	parse_bool_value.constprop.0
	testl	%eax, %eax
	je	.L37
	movq	96(%rsp), %rcx
	leaq	36(%rsp), %rdx
	movl	$0, 36(%rsp)
	call	parse_bool_value.constprop.0
	testl	%eax, %eax
	je	.L38
	pxor	%xmm0, %xmm0
	comisd	%xmm7, %xmm0
	jnb	.L39
	movl	32(%rsp), %edx
	xorl	%eax, %eax
	movapd	%xmm6, %xmm0
	movq	%r14, (%rbx)
	movq	%r15, 24(%rbx)
	unpcklpd	%xmm7, %xmm0
	testl	%edx, %edx
	movq	%rdi, 32(%rbx)
	sete	%al
	movups	%xmm0, 8(%rbx)
	movl	%eax, 40(%rbx)
	movl	36(%rsp), %eax
	movl	%eax, 44(%rbx)
.L11:
	movups	624(%rsp), %xmm6
	movups	640(%rsp), %xmm7
	movl	%esi, %eax
	addq	$664, %rsp
	popq	%rbx
	popq	%rsi
	popq	%rdi
	popq	%rbp
	popq	%r14
	popq	%r15
	ret
	.p2align 4,,10
	.p2align 3
.L18:
	testb	%dl, %dl
	jne	.L20
.L21:
	movl	$1, %esi
	jmp	.L11
	.p2align 4,,10
	.p2align 3
.L40:
	movb	$0, (%rdx)
	jmp	.L13
	.p2align 4,,10
	.p2align 3
.L41:
	cmpq	$7, %rdi
	je	.L54
.L25:
	movl	$-3, %esi
	jmp	.L11
.L36:
	movl	$-2, %esi
	jmp	.L11
.L27:
	movl	$-4, %esi
	jmp	.L11
.L35:
	movl	$-8, %esi
	jmp	.L11
.L31:
	movl	$-6, %esi
	jmp	.L11
.L33:
	movl	$-7, %esi
	jmp	.L11
.L29:
	movl	$-5, %esi
	jmp	.L11
.L37:
	movl	$-9, %esi
	jmp	.L11
.L38:
	movl	$-10, %esi
	jmp	.L11
.L39:
	movl	$-11, %esi
	jmp	.L11
	.seh_endproc
	.section .rdata,"dr"
.LC8:
	.ascii "r\0"
	.align 8
.LC9:
	.ascii "parse error on %s line %d: code=%d\12\0"
	.text
	.p2align 4
	.globl	parse_trade_file
	.def	parse_trade_file;	.scl	2;	.type	32;	.endef
	.seh_proc	parse_trade_file
parse_trade_file:
	pushq	%r15
	.seh_pushreg	%r15
	pushq	%r14
	.seh_pushreg	%r14
	pushq	%r13
	.seh_pushreg	%r13
	pushq	%r12
	.seh_pushreg	%r12
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rdi
	.seh_pushreg	%rdi
	pushq	%rsi
	.seh_pushreg	%rsi
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$648, %rsp
	.seh_stackalloc	648
	movups	%xmm6, 624(%rsp)
	.seh_savexmm	%xmm6, 624
	.seh_endprologue
	movq	%rdx, %r15
	leaq	.LC8(%rip), %rdx
	movq	%r8, 736(%rsp)
	movq	%rcx, %r13
	call	fopen
	movq	%rax, %rbp
	testq	%rax, %rax
	je	.L65
	xorl	%esi, %esi
	xorl	%r12d, %r12d
	xorl	%ebx, %ebx
	xorl	%edi, %edi
	pxor	%xmm6, %xmm6
	.p2align 4
	.p2align 3
.L57:
	movq	%rbp, %r8
	movl	$512, %edx
	leaq	112(%rsp), %rcx
	call	fgets
	testq	%rax, %rax
	je	.L70
	addl	$1, %esi
	leaq	64(%rsp), %rdx
	leaq	112(%rsp), %rcx
	movups	%xmm6, 64(%rsp)
	movl	%esi, %r8d
	movups	%xmm6, 80(%rsp)
	movups	%xmm6, 96(%rsp)
	call	parse_trade_line
	cmpl	$1, %eax
	je	.L57
	testl	%eax, %eax
	jne	.L71
	cmpq	%r12, %rbx
	je	.L72
.L61:
	movdqu	64(%rsp), %xmm0
	leaq	(%rbx,%rbx,2), %rax
	addq	$1, %rbx
	salq	$4, %rax
	movups	%xmm0, (%rdi,%rax)
	movdqu	80(%rsp), %xmm0
	movups	%xmm0, 16(%rdi,%rax)
	movdqu	96(%rsp), %xmm0
	movups	%xmm0, 32(%rdi,%rax)
	jmp	.L57
	.p2align 4,,10
	.p2align 3
.L72:
	testq	%rbx, %rbx
	je	.L66
	leaq	(%rbx,%rbx,2), %rdx
	leaq	(%rbx,%rbx), %r12
	salq	$5, %rdx
	movq	%rdx, %r14
.L62:
	movq	%r14, %rdx
	movq	%rdi, %rcx
	call	realloc
	testq	%rax, %rax
	je	.L73
	movq	%r14, phase_logical_allocated_bytes(%rip)
	movq	%rax, %rdi
	jmp	.L61
	.p2align 4,,10
	.p2align 3
.L66:
	movl	$196608, %r14d
	movl	$4096, %r12d
	jmp	.L62
	.p2align 4,,10
	.p2align 3
.L70:
	movq	%rbp, %rcx
	call	fclose
	movq	736(%rsp), %rax
	movq	%rdi, (%r15)
	movq	%rbx, (%rax)
	xorl	%eax, %eax
.L55:
	movups	624(%rsp), %xmm6
	addq	$648, %rsp
	popq	%rbx
	popq	%rsi
	popq	%rdi
	popq	%rbp
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	ret
	.p2align 4,,10
	.p2align 3
.L71:
	movl	$2, %ecx
	movl	%eax, 60(%rsp)
	call	*__imp___acrt_iob_func(%rip)
	movl	60(%rsp), %edx
	movl	%esi, %r9d
	movq	%r13, %r8
	movq	%rax, %rcx
	movl	%edx, 32(%rsp)
	leaq	.LC9(%rip), %rdx
	call	fprintf
	movq	%rdi, %rcx
	call	free
	movq	%rbp, %rcx
	call	fclose
	movl	$-2, %eax
	jmp	.L55
.L65:
	movl	$-1, %eax
	jmp	.L55
.L73:
	movq	%rdi, %rcx
	call	free
	movq	%rbp, %rcx
	call	fclose
	movl	$-3, %eax
	jmp	.L55
	.seh_endproc
	.section .rdata,"dr"
.LC10:
	.ascii "BUY\0"
.LC11:
	.ascii "SELL\0"
.LC12:
	.ascii "-\0"
	.align 8
.LC13:
	.ascii "TRACE_BAR|%lld|%.8f|%.8f|%.8f|%.8f|%d\12\0"
.LC14:
	.ascii "SIGNAL|%zu|%s|%.8f|%.8f\12\0"
	.align 8
.LC15:
	.ascii "TRACE_ROW|%zu|%.8f|%.8f|%d|%.8f|\0"
.LC16:
	.ascii "%.8f\0"
.LC17:
	.ascii "|%s\12\0"
	.text
	.p2align 4
	.globl	compute_market_summary
	.def	compute_market_summary;	.scl	2;	.type	32;	.endef
	.seh_proc	compute_market_summary
compute_market_summary:
	pushq	%r15
	.seh_pushreg	%r15
	pushq	%r14
	.seh_pushreg	%r14
	pushq	%r13
	.seh_pushreg	%r13
	pushq	%r12
	.seh_pushreg	%r12
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rdi
	.seh_pushreg	%rdi
	pushq	%rsi
	.seh_pushreg	%rsi
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$392, %rsp
	.seh_stackalloc	392
	movups	%xmm6, 224(%rsp)
	.seh_savexmm	%xmm6, 224
	movups	%xmm7, 240(%rsp)
	.seh_savexmm	%xmm7, 240
	movups	%xmm8, 256(%rsp)
	.seh_savexmm	%xmm8, 256
	movups	%xmm9, 272(%rsp)
	.seh_savexmm	%xmm9, 272
	movups	%xmm10, 288(%rsp)
	.seh_savexmm	%xmm10, 288
	movups	%xmm11, 304(%rsp)
	.seh_savexmm	%xmm11, 304
	movups	%xmm12, 320(%rsp)
	.seh_savexmm	%xmm12, 320
	movups	%xmm13, 336(%rsp)
	.seh_savexmm	%xmm13, 336
	movups	%xmm14, 352(%rsp)
	.seh_savexmm	%xmm14, 352
	movups	%xmm15, 368(%rsp)
	.seh_savexmm	%xmm15, 368
	.seh_endprologue
	movl	ready.1(%rip), %r11d
	movq	%r9, 488(%rsp)
	movq	%rcx, %rsi
	movl	%edx, %ebp
	movl	%r8d, %r12d
	movq	$0, 200(%rsp)
	movq	$0, 208(%rsp)
	testl	%r11d, %r11d
	je	.L147
.L75:
	movq	__imp_QueryPerformanceCounter(%rip), %rax
	leaq	216(%rsp), %rcx
	movq	%rax, 120(%rsp)
	call	*%rax
	movq	freq.0(%rip), %r8
	movq	216(%rsp), %rax
	xorl	%edx, %edx
	divq	%r8
	imulq	$1000000000, %rax, %rcx
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%r8
	leaq	200(%rsp), %rdx
	leaq	208(%rsp), %r8
	leaq	(%rax,%rcx), %rbx
	movq	%rsi, %rcx
	call	parse_trade_file
	testl	%eax, %eax
	movl	%eax, 172(%rsp)
	movl	ready.1(%rip), %eax
	jne	.L148
	testl	%eax, %eax
	je	.L149
.L79:
	leaq	216(%rsp), %rcx
	call	*120(%rsp)
	movq	freq.0(%rip), %r8
	xorl	%edx, %edx
	movq	216(%rsp), %rax
	movq	200(%rsp), %rdi
	divq	%r8
	movq	%rdi, 144(%rsp)
	imulq	$1000000000, %rax, %rcx
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%r8
	addq	%rcx, %rax
	subq	%rbx, %rax
	movq	%rax, phase_loading_ns(%rip)
	movq	208(%rsp), %rax
	movq	%rax, 80(%rsp)
	testq	%rax, %rax
	je	.L150
	testl	%ebp, %ebp
	setle	%al
	cmpl	%r12d, %ebp
	setg	%dl
	orb	%dl, %al
	jne	.L130
	testl	%r12d, %r12d
	jle	.L130
	movl	ready.1(%rip), %r10d
	testl	%r10d, %r10d
	je	.L151
.L83:
	pxor	%xmm12, %xmm12
	xorl	%r15d, %r15d
	xorl	%r14d, %r14d
	xorl	%r13d, %r13d
	movl	$1, %edi
	movapd	%xmm12, %xmm11
	movapd	%xmm12, %xmm14
	movapd	%xmm12, %xmm13
	movapd	%xmm12, %xmm15
	movapd	%xmm12, %xmm7
	leaq	216(%rsp), %rcx
	movapd	%xmm12, %xmm9
	movapd	%xmm12, %xmm8
	call	*120(%rsp)
	movq	freq.0(%rip), %rcx
	xorl	%edx, %edx
	movq	216(%rsp), %rax
	movl	%r12d, 480(%rsp)
	divq	%rcx
	movl	$0, 112(%rsp)
	movl	$0, 116(%rsp)
	movq	$0x000000000, 72(%rsp)
	movl	$-1, 168(%rsp)
	movl	$-1, 140(%rsp)
	movq	$0, 160(%rsp)
	movq	$0, 152(%rsp)
	movq	%rax, 176(%rsp)
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%rcx
	xorl	%ecx, %ecx
	movq	%rax, 184(%rsp)
	movq	144(%rsp), %rax
	leaq	8(%rax), %rbx
	movl	%ebp, %eax
	leaq	0(,%rax,4), %rdx
	subq	%rdx, %rax
	salq	$4, %rax
	movq	%rax, 96(%rsp)
	movl	%r12d, %eax
	xorl	%r12d, %r12d
	leaq	0(,%rax,4), %rdx
	subq	%rdx, %rax
	salq	$4, %rax
	movq	%rax, 104(%rsp)
	jmp	.L113
	.p2align 4,,10
	.p2align 3
.L156:
	movapd	%xmm15, %xmm10
	pxor	%xmm2, %xmm2
	movapd	%xmm1, %xmm4
	cvtsi2sdl	480(%rsp), %xmm2
	divsd	%xmm2, %xmm10
	subsd	%xmm10, %xmm4
	testl	%r15d, %r15d
	je	.L88
	pxor	%xmm2, %xmm2
	comisd	%xmm14, %xmm2
	jnb	.L152
.L89:
	pxor	%xmm2, %xmm2
	comisd	%xmm2, %xmm14
	jb	.L146
	comisd	%xmm4, %xmm2
	ja	.L97
.L146:
	testl	%r12d, %r12d
	je	.L119
	xorl	%r15d, %r15d
.L94:
	cmpq	%rcx, %rsi
	je	.L153
.L100:
	movl	trace_enabled(%rip), %eax
	orl	bars_enabled(%rip), %eax
	je	.L104
	movl	112(%rsp), %eax
	movsd	72(%rsp), %xmm5
	movq	%rcx, %rdx
	movsd	%xmm13, 32(%rsp)
	movapd	%xmm11, %xmm3
	movq	%xmm11, %r9
	movapd	%xmm12, %xmm2
	movl	%r11d, 136(%rsp)
	movl	%eax, 48(%rsp)
	movq	%xmm12, %r8
	leaq	.LC13(%rip), %rcx
	movsd	%xmm5, 40(%rsp)
	movsd	%xmm4, 128(%rsp)
	movsd	%xmm1, 88(%rsp)
	call	printf
	movsd	(%rbx), %xmm6
	movl	136(%rsp), %r11d
	movsd	128(%rsp), %xmm4
	movsd	88(%rsp), %xmm1
.L104:
	movl	signals_enabled(%rip), %ecx
	addl	$1, 116(%rsp)
	movapd	%xmm6, %xmm13
	movapd	%xmm6, %xmm11
	movl	$1, 112(%rsp)
	movsd	%xmm6, 72(%rsp)
	testl	%ecx, %ecx
	je	.L106
.L105:
	testl	%r15d, %r15d
	je	.L106
	cmpl	$1, %r15d
	movl	$-1, %eax
	leaq	.LC10(%rip), %r8
	cmovne	%eax, %r15d
	leaq	.LC11(%rip), %rax
	cmovne	%rax, %r8
.L99:
	movapd	%xmm1, %xmm3
	movq	%xmm1, %r9
	movq	%rdi, %rdx
	movsd	%xmm10, 32(%rsp)
	leaq	.LC14(%rip), %rcx
	movsd	%xmm1, 88(%rsp)
	movapd	%xmm6, %xmm12
	movl	%r11d, 136(%rsp)
	movsd	%xmm4, 128(%rsp)
	call	printf
	movl	%r15d, %eax
	movl	%r12d, %r15d
	movsd	128(%rsp), %xmm14
	movsd	88(%rsp), %xmm1
	movl	136(%rsp), %r11d
	movl	%eax, %r12d
.L96:
	movl	trace_enabled(%rip), %edx
	testl	%edx, %edx
	jne	.L154
.L107:
	addq	$48, %rbx
	movl	$1, %r12d
	cmpq	%rdi, 80(%rsp)
	je	.L155
	addq	$1, %rdi
	movq	%rsi, %rcx
.L113:
	movsd	(%rbx), %xmm6
	movsd	8(%rbx), %xmm1
	addl	$1, %r13d
	movapd	%xmm6, %xmm2
	addsd	%xmm1, %xmm9
	addsd	%xmm6, %xmm7
	mulsd	%xmm1, %xmm2
	addsd	%xmm2, %xmm8
	cmpl	%r13d, %ebp
	jge	.L84
	movq	96(%rsp), %rax
	movl	%ebp, %r13d
	subsd	(%rbx,%rax), %xmm7
.L84:
	addsd	%xmm6, %xmm15
	addl	$1, %r14d
	cmpl	%r14d, 480(%rsp)
	jge	.L85
	movq	104(%rsp), %rax
	movl	480(%rsp), %r14d
	subsd	(%rbx,%rax), %xmm15
.L85:
	movl	%edi, %r11d
	pxor	%xmm1, %xmm1
	cmpl	%edi, %ebp
	jg	.L86
	pxor	%xmm2, %xmm2
	movapd	%xmm7, %xmm1
	cvtsi2sdl	%ebp, %xmm2
	divsd	%xmm2, %xmm1
.L86:
	movabsq	$4835703278458516699, %rax
	movq	24(%rbx), %r8
	imulq	%r8
	sarq	$63, %r8
	movq	%rdx, %rsi
	sarq	$18, %rsi
	subq	%r8, %rsi
	cmpl	%r11d, 480(%rsp)
	jle	.L156
	testl	%r12d, %r12d
	je	.L129
	movl	%r15d, %r12d
	movapd	%xmm14, %xmm4
	pxor	%xmm10, %xmm10
	xorl	%r15d, %r15d
	cmpq	%rcx, %rsi
	jne	.L100
.L153:
	movapd	%xmm6, %xmm0
	movapd	%xmm6, %xmm2
	addl	$1, 112(%rsp)
	movl	signals_enabled(%rip), %ecx
	maxsd	%xmm11, %xmm0
	minsd	%xmm13, %xmm2
	movsd	%xmm6, 72(%rsp)
	movapd	%xmm12, %xmm6
	movapd	%xmm0, %xmm11
	movapd	%xmm2, %xmm13
	testl	%ecx, %ecx
	jne	.L105
.L106:
	movl	trace_enabled(%rip), %edx
	movl	%r15d, %eax
	movapd	%xmm6, %xmm12
	movl	%r12d, %r15d
	movapd	%xmm4, %xmm14
	movl	%eax, %r12d
	testl	%edx, %edx
	je	.L107
.L154:
	movapd	%xmm8, %xmm2
	movq	8(%rbx), %r9
	movq	(%rbx), %r8
	movq	%rdi, %rdx
	divsd	%xmm9, %xmm2
	leaq	.LC15(%rip), %rcx
	movl	%r11d, 88(%rsp)
	movsd	%xmm1, 128(%rsp)
	movq	%r9, %xmm3
	movsd	%xmm2, 40(%rsp)
	movl	32(%rbx), %eax
	movq	%r8, %xmm2
	movl	%eax, 32(%rsp)
	call	printf
	cmpl	88(%rsp), %ebp
	jg	.L108
	movsd	128(%rsp), %xmm1
	leaq	.LC16(%rip), %rcx
	movq	%xmm1, %rdx
	call	printf
	movl	$124, %ecx
	call	putchar
	movl	88(%rsp), %r11d
	cmpl	%r11d, 480(%rsp)
	jg	.L110
	movapd	%xmm10, %xmm1
	movq	%xmm10, %rdx
	leaq	.LC16(%rip), %rcx
	call	printf
.L111:
	leaq	.LC10(%rip), %rdx
	cmpl	$1, %r12d
	je	.L112
	leaq	.LC12(%rip), %rdx
	cmpl	$-1, %r12d
	leaq	.LC11(%rip), %rax
	cmove	%rax, %rdx
.L112:
	leaq	.LC17(%rip), %rcx
	call	printf
	jmp	.L107
	.p2align 4,,10
	.p2align 3
.L129:
	movapd	%xmm6, %xmm12
	movapd	%xmm6, %xmm13
	movapd	%xmm6, %xmm11
	movl	$1, 112(%rsp)
	movsd	%xmm6, 72(%rsp)
	pxor	%xmm10, %xmm10
	jmp	.L96
	.p2align 4,,10
	.p2align 3
.L88:
	testl	%r12d, %r12d
	jne	.L94
	movapd	%xmm6, %xmm12
	movapd	%xmm4, %xmm14
	movapd	%xmm6, %xmm13
	movl	$1, 112(%rsp)
	movsd	%xmm6, 72(%rsp)
	movapd	%xmm6, %xmm11
	movl	$1, %r15d
	jmp	.L96
	.p2align 4,,10
	.p2align 3
.L108:
	movl	$45, %ecx
	call	putchar
	movl	$124, %ecx
	call	putchar
.L110:
	movl	$45, %ecx
	call	putchar
	jmp	.L111
	.p2align 4,,10
	.p2align 3
.L155:
	movl	trace_enabled(%rip), %eax
	orl	bars_enabled(%rip), %eax
	jne	.L157
.L114:
	movl	116(%rsp), %ebx
	movl	ready.1(%rip), %eax
	addl	$1, %ebx
	testl	%eax, %eax
	je	.L158
.L115:
	leaq	216(%rsp), %rcx
	call	*120(%rsp)
	movq	freq.0(%rip), %r8
	divsd	%xmm9, %xmm8
	xorl	%edx, %edx
	movq	216(%rsp), %rax
	movq	152(%rsp), %rdi
	divq	%r8
	subq	176(%rsp), %rax
	imulq	$1000000000, %rax, %rcx
	subq	184(%rsp), %rcx
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%r8
	addq	%rax, %rcx
	movq	488(%rsp), %rax
	movq	%rcx, phase_compute_ns(%rip)
	movq	144(%rsp), %rcx
	movsd	%xmm8, (%rax)
	movq	496(%rsp), %rax
	movq	%rdi, (%rax)
	movq	504(%rsp), %rax
	movq	160(%rsp), %rdi
	movq	%rdi, (%rax)
	movq	512(%rsp), %rax
	movq	80(%rsp), %rdi
	movq	%rdi, (%rax)
	movq	520(%rsp), %rax
	movl	140(%rsp), %edi
	movl	%edi, (%rax)
	movq	528(%rsp), %rax
	movl	168(%rsp), %edi
	movl	%edi, (%rax)
	movq	536(%rsp), %rax
	movl	%ebx, (%rax)
	call	free
.L74:
	movl	172(%rsp), %eax
	movups	224(%rsp), %xmm6
	movups	240(%rsp), %xmm7
	movups	256(%rsp), %xmm8
	movups	272(%rsp), %xmm9
	movups	288(%rsp), %xmm10
	movups	304(%rsp), %xmm11
	movups	320(%rsp), %xmm12
	movups	336(%rsp), %xmm13
	movups	352(%rsp), %xmm14
	movups	368(%rsp), %xmm15
	addq	$392, %rsp
	popq	%rbx
	popq	%rsi
	popq	%rdi
	popq	%rbp
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	ret
	.p2align 4,,10
	.p2align 3
.L152:
	comisd	%xmm2, %xmm4
	jbe	.L89
	movl	140(%rsp), %eax
	cmpl	$-1, %eax
	cmove	%r11d, %eax
	addq	$1, 152(%rsp)
	movl	%eax, 140(%rsp)
	testl	%r12d, %r12d
	je	.L93
	movl	%r11d, 168(%rsp)
	movl	%r12d, %r15d
	jmp	.L94
	.p2align 4,,10
	.p2align 3
.L119:
	movl	%r15d, 112(%rsp)
	movapd	%xmm6, %xmm12
	movapd	%xmm4, %xmm14
	movapd	%xmm6, %xmm13
	movsd	%xmm6, 72(%rsp)
	movapd	%xmm6, %xmm11
	jmp	.L96
	.p2align 4,,10
	.p2align 3
.L157:
	movl	112(%rsp), %eax
	movapd	%xmm11, %xmm3
	movapd	%xmm6, %xmm2
	movq	%rsi, %rdx
	movsd	72(%rsp), %xmm5
	movq	%xmm11, %r9
	movsd	%xmm13, 32(%rsp)
	movq	%xmm6, %r8
	movl	%eax, 48(%rsp)
	leaq	.LC13(%rip), %rcx
	movsd	%xmm5, 40(%rsp)
	call	printf
	jmp	.L114
	.p2align 4,,10
	.p2align 3
.L97:
	movl	140(%rsp), %eax
	cmpl	$-1, %eax
	cmove	%r11d, %eax
	addq	$1, 160(%rsp)
	movl	%eax, 140(%rsp)
	testl	%r12d, %r12d
	jne	.L121
	movl	signals_enabled(%rip), %r9d
	movl	%r15d, 112(%rsp)
	testl	%r9d, %r9d
	jne	.L122
	movapd	%xmm6, %xmm12
	movapd	%xmm4, %xmm14
	movapd	%xmm6, %xmm13
	movl	%r11d, 168(%rsp)
	movsd	%xmm6, 72(%rsp)
	movapd	%xmm6, %xmm11
	movl	$-1, %r12d
	jmp	.L96
	.p2align 4,,10
	.p2align 3
.L147:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L75
	.p2align 4,,10
	.p2align 3
.L149:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L79
	.p2align 4,,10
	.p2align 3
.L158:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L115
	.p2align 4,,10
	.p2align 3
.L151:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L83
	.p2align 4,,10
	.p2align 3
.L93:
	movl	signals_enabled(%rip), %r8d
	movl	%r15d, 112(%rsp)
	testl	%r8d, %r8d
	je	.L159
	movl	%r11d, 168(%rsp)
	movl	%r15d, %r12d
	movapd	%xmm6, %xmm13
	movapd	%xmm6, %xmm11
	movsd	%xmm6, 72(%rsp)
	leaq	.LC10(%rip), %r8
	jmp	.L99
	.p2align 4,,10
	.p2align 3
.L121:
	movl	%r11d, 168(%rsp)
	movl	$-1, %r15d
	jmp	.L94
	.p2align 4,,10
	.p2align 3
.L159:
	movl	%r11d, 168(%rsp)
	movapd	%xmm6, %xmm12
	movl	%r15d, %r12d
	movapd	%xmm4, %xmm14
	movapd	%xmm6, %xmm13
	movsd	%xmm6, 72(%rsp)
	movapd	%xmm6, %xmm11
	jmp	.L96
	.p2align 4,,10
	.p2align 3
.L122:
	movl	%r15d, %r12d
	movapd	%xmm6, %xmm13
	movapd	%xmm6, %xmm11
	movl	%r11d, 168(%rsp)
	movsd	%xmm6, 72(%rsp)
	movl	$-1, %r15d
	leaq	.LC11(%rip), %r8
	jmp	.L99
.L130:
	movq	144(%rsp), %rcx
	call	free
	movl	$-3, 172(%rsp)
	jmp	.L74
.L148:
	testl	%eax, %eax
	je	.L160
.L77:
	leaq	216(%rsp), %rcx
	call	*120(%rsp)
	movq	freq.0(%rip), %r8
	movq	216(%rsp), %rax
	xorl	%edx, %edx
	movl	$-1, 172(%rsp)
	divq	%r8
	imulq	$1000000000, %rax, %rcx
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%r8
	addq	%rcx, %rax
	subq	%rbx, %rax
	movq	%rax, phase_loading_ns(%rip)
	jmp	.L74
.L150:
	movq	%rdi, %rcx
	call	free
	movl	$-2, 172(%rsp)
	jmp	.L74
.L160:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L77
	.seh_endproc
	.section .rdata,"dr"
	.align 8
.LC18:
	.ascii "Usage: %s <csv-file> [short_window] [long_window]\12\0"
.LC19:
	.ascii "--trace\0"
.LC20:
	.ascii "--signals-only\0"
.LC21:
	.ascii "--bars-only\0"
.LC22:
	.ascii "--bench-phases\0"
	.align 8
.LC23:
	.ascii "Invalid window or argument: %s\12\0"
	.align 8
.LC24:
	.ascii "Failed to compute summary for %s\12\0"
.LC25:
	.ascii "file=%s\12\0"
.LC26:
	.ascii "trades=%lld\12\0"
.LC27:
	.ascii "final_vwap=%.8f\12\0"
.LC28:
	.ascii "buy_signals=%lld\12\0"
.LC29:
	.ascii "sell_signals=%lld\12\0"
.LC30:
	.ascii "first_signal=%d\12\0"
.LC31:
	.ascii "last_signal=%d\12\0"
.LC32:
	.ascii "ohlc_bars=%d\12\0"
.LC33:
	.ascii "psapi.dll\0"
.LC34:
	.ascii "GetProcessMemoryInfo\0"
	.align 8
.LC35:
	.ascii "PHASES|counting_ns=0|loading_ns=%llu|compute_ns=%llu|logical_allocated_bytes=%llu|logical_used_bytes=%llu|process_peak_memory_bytes=%llu|process_memory_metric=%s|compute_scope=compute_only\12\0"
.LC36:
	.ascii "peak_working_set\0"
	.section	.text.startup,"x"
	.p2align 4
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
	.seh_proc	main
main:
	pushq	%r15
	.seh_pushreg	%r15
	pushq	%r14
	.seh_pushreg	%r14
	pushq	%r12
	.seh_pushreg	%r12
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rdi
	.seh_pushreg	%rdi
	pushq	%rsi
	.seh_pushreg	%rsi
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$224, %rsp
	.seh_stackalloc	224
	.seh_endprologue
	movl	%ecx, %esi
	movq	%rdx, %rdi
	call	__main
	cmpl	$1, %esi
	jg	.L162
	movq	(%rdi), %rbx
	movl	$2, %ecx
	call	*__imp___acrt_iob_func(%rip)
	leaq	.LC18(%rip), %rdx
	movq	%rbx, %r8
	movq	%rax, %rcx
	call	fprintf
	movl	$1, %eax
.L161:
	addq	$224, %rsp
	popq	%rbx
	popq	%rsi
	popq	%rdi
	popq	%rbp
	popq	%r12
	popq	%r14
	popq	%r15
	ret
.L162:
	movq	8(%rdi), %r12
	cmpl	$2, %esi
	je	.L179
	leal	-3(%rsi), %eax
	leaq	16(%rdi), %rbx
	xorl	%ebp, %ebp
	movl	$5, %r15d
	leaq	24(%rdi,%rax,8), %rdi
	movl	$3, %r14d
	jmp	.L173
	.p2align 4,,10
	.p2align 3
.L165:
	leaq	.LC20(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	testl	%eax, %eax
	je	.L189
	leaq	.LC21(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	testl	%eax, %eax
	je	.L190
	leaq	.LC22(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L170
	movl	$1, phases_enabled(%rip)
.L168:
	addq	$8, %rbx
	cmpq	%rdi, %rbx
	je	.L164
.L173:
	movq	(%rbx), %rsi
	leaq	.LC19(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L165
	movl	$1, trace_enabled(%rip)
	addq	$8, %rbx
	cmpq	%rdi, %rbx
	jne	.L173
.L164:
	xorl	%eax, %eax
	xorl	%edx, %edx
	leaq	112(%rsp), %r9
	movl	%r15d, %r8d
	movq	%rax, 120(%rsp)
	movq	%r12, %rcx
	movq	%rax, 128(%rsp)
	movq	%rax, 136(%rsp)
	leaq	108(%rsp), %rax
	movq	%rax, 72(%rsp)
	leaq	104(%rsp), %rax
	movq	%rax, 64(%rsp)
	leaq	100(%rsp), %rax
	movq	%rax, 56(%rsp)
	leaq	136(%rsp), %rax
	movq	%rax, 48(%rsp)
	leaq	128(%rsp), %rax
	movq	%rax, 40(%rsp)
	leaq	120(%rsp), %rax
	movq	%rax, 32(%rsp)
	movl	%edx, 108(%rsp)
	movl	%r14d, %edx
	movq	$0x000000000, 112(%rsp)
	movl	$-1, 100(%rsp)
	movl	$-1, 104(%rsp)
	call	compute_market_summary
	movl	%eax, %ebx
	testl	%eax, %eax
	jne	.L191
	movq	%r12, %rdx
	leaq	.LC25(%rip), %rcx
	call	printf
	movq	136(%rsp), %rsi
	leaq	.LC26(%rip), %rcx
	movq	%rsi, %rdx
	call	printf
	movsd	112(%rsp), %xmm1
	leaq	.LC27(%rip), %rcx
	movq	%xmm1, %rdx
	call	printf
	movq	120(%rsp), %rdx
	leaq	.LC28(%rip), %rcx
	call	printf
	movq	128(%rsp), %rdx
	leaq	.LC29(%rip), %rcx
	call	printf
	movl	100(%rsp), %edx
	leaq	.LC30(%rip), %rcx
	call	printf
	movl	104(%rsp), %edx
	leaq	.LC31(%rip), %rcx
	call	printf
	movl	108(%rsp), %edx
	leaq	.LC32(%rip), %rcx
	call	printf
	movl	phases_enabled(%rip), %eax
	testl	%eax, %eax
	je	.L161
	imulq	$48, %rsi, %rsi
	leaq	.LC33(%rip), %rcx
	call	*__imp_LoadLibraryA(%rip)
	movq	%rax, %rbp
	testq	%rax, %rax
	je	.L181
	movq	%rax, %rcx
	leaq	.LC34(%rip), %rdx
	leaq	148(%rsp), %rdi
	call	*__imp_GetProcAddress(%rip)
	movl	$19, %ecx
	movq	%rax, %r9
	movl	%ebx, %eax
	rep stosl
	movl	$80, 144(%rsp)
	testq	%r9, %r9
	je	.L176
	movq	%r9, 88(%rsp)
	call	*__imp_GetCurrentProcess(%rip)
	leaq	144(%rsp), %rdx
	movl	$80, %r8d
	movq	%rax, %rcx
	call	*88(%rsp)
	testl	%eax, %eax
	je	.L176
	movq	152(%rsp), %rbx
.L178:
	movq	%rbp, %rcx
	call	*__imp_FreeLibrary(%rip)
.L175:
	leaq	.LC36(%rip), %rax
	movq	%rbx, 40(%rsp)
	movq	phase_logical_allocated_bytes(%rip), %r9
	leaq	.LC35(%rip), %rcx
	movq	%rax, 48(%rsp)
	movq	phase_compute_ns(%rip), %r8
	movq	%rsi, 32(%rsp)
	movq	phase_loading_ns(%rip), %rdx
	call	printf
	xorl	%eax, %eax
	jmp	.L161
	.p2align 4,,10
	.p2align 3
.L189:
	movl	$1, signals_enabled(%rip)
	jmp	.L168
	.p2align 4,,10
	.p2align 3
.L170:
	leaq	144(%rsp), %rdx
	movq	%rsi, %rcx
	movl	$10, %r8d
	movq	$0, 144(%rsp)
	call	strtol
	movq	(%rbx), %rsi
	movl	%eax, %edx
	movq	144(%rsp), %rax
	cmpq	%rax, %rsi
	je	.L171
	cmpb	$0, (%rax)
	jne	.L171
	leal	-1(%rdx), %eax
	cmpl	$99999, %eax
	ja	.L171
	cmpl	$1, %ebp
	jg	.L171
	testl	%ebp, %ebp
	jne	.L180
	movl	%edx, %r14d
	movl	$1, %ebp
	jmp	.L168
	.p2align 4,,10
	.p2align 3
.L190:
	movl	$1, bars_enabled(%rip)
	jmp	.L168
.L180:
	movl	%edx, %r15d
	movl	$2, %ebp
	jmp	.L168
.L171:
	movl	$2, %ecx
	call	*__imp___acrt_iob_func(%rip)
	movq	%rsi, %r8
	leaq	.LC23(%rip), %rdx
	movq	%rax, %rcx
	call	fprintf
	movl	$2, %eax
	jmp	.L161
.L191:
	movl	$2, %ecx
	call	*__imp___acrt_iob_func(%rip)
	movq	%r12, %r8
	leaq	.LC24(%rip), %rdx
	movq	%rax, %rcx
	call	fprintf
	movl	$4, %eax
	jmp	.L161
.L181:
	xorl	%ebx, %ebx
	jmp	.L175
.L176:
	xorl	%ebx, %ebx
	jmp	.L178
.L179:
	movl	$5, %r15d
	movl	$3, %r14d
	jmp	.L164
	.seh_endproc
.lcomm freq.0,8,8
.lcomm ready.1,4,4
.lcomm phase_logical_allocated_bytes,8,8
.lcomm phase_compute_ns,8,8
.lcomm phase_loading_ns,8,8
.lcomm phases_enabled,4,4
.lcomm bars_enabled,4,4
.lcomm signals_enabled,4,4
.lcomm trace_enabled,4,4
	.def	__main;	.scl	2;	.type	32;	.endef
	.ident	"GCC: (Rev3, Built by MSYS2 project) 16.2.0"
	.def	strcmp;	.scl	2;	.type	32;	.endef
	.def	strlen;	.scl	2;	.type	32;	.endef
	.def	memcpy;	.scl	2;	.type	32;	.endef
	.def	strncmp;	.scl	2;	.type	32;	.endef
	.def	strtoll;	.scl	2;	.type	32;	.endef
	.def	strtod;	.scl	2;	.type	32;	.endef
	.def	fopen;	.scl	2;	.type	32;	.endef
	.def	fgets;	.scl	2;	.type	32;	.endef
	.def	realloc;	.scl	2;	.type	32;	.endef
	.def	fclose;	.scl	2;	.type	32;	.endef
	.def	fprintf;	.scl	2;	.type	32;	.endef
	.def	free;	.scl	2;	.type	32;	.endef
	.def	printf;	.scl	2;	.type	32;	.endef
	.def	putchar;	.scl	2;	.type	32;	.endef
	.def	strtol;	.scl	2;	.type	32;	.endef
