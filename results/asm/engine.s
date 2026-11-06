	.file	"engine.c"
	.text
	.p2align 4
	.def	engine_fixed_sma_push;	.scl	3;	.type	32;	.endef
	.seh_proc	engine_fixed_sma_push
engine_fixed_sma_push:
	pushq	%rsi
	.seh_pushreg	%rsi
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$72, %rsp
	.seh_stackalloc	72
	.seh_endprologue
	movq	128(%rsp), %rax
	movq	%rcx, %r10
	movslq	%edx, %rsi
	movq	(%rax), %rcx
	xorl	%edx, %edx
	movq	%r8, %rax
	movq	%r8, %rbx
	divq	%rsi
	movq	%rcx, 48(%rsp)
	leaq	(%r10,%rdx,8), %r10
	cmpq	%rsi, %r8
	jb	.L2
	movq	(%r10), %rdx
	leaq	56(%rsp), %r8
	movq	%r9, 120(%rsp)
	movq	%r10, 40(%rsp)
	negq	%rdx
	call	fp_checked_add
	movq	40(%rsp), %r10
	movq	120(%rsp), %r9
	testl	%eax, %eax
	je	.L5
	movq	56(%rsp), %rcx
	movq	%rcx, 48(%rsp)
.L2:
	movq	%r9, (%r10)
	leaq	48(%rsp), %r8
	movq	%r9, %rdx
	call	fp_checked_add
	testl	%eax, %eax
	je	.L5
	movq	48(%rsp), %rax
	movq	128(%rsp), %rcx
	addq	$1, %rbx
	movq	%rax, (%rcx)
	xorl	%eax, %eax
	cmpq	%rsi, %rbx
	setnb	%al
	addq	$72, %rsp
	popq	%rbx
	popq	%rsi
	ret
	.p2align 4,,10
	.p2align 3
.L5:
	movl	$-1, %eax
	addq	$72, %rsp
	popq	%rbx
	popq	%rsi
	ret
	.seh_endproc
	.p2align 4
	.def	engine_scale_value.constprop.0;	.scl	3;	.type	32;	.endef
	.seh_proc	engine_scale_value.constprop.0
engine_scale_value.constprop.0:
	subq	$40, %rsp
	.seh_stackalloc	40
	.seh_endprologue
	movq	%rdx, %r8
	movq	%xmm0, %rdx
	movq	%rdx, %rax
	shrq	$32, %rax
	movl	%eax, %ecx
	andl	$2147483647, %ecx
	orl	%edx, %ecx
	sete	%dl
	andl	$2146435072, %eax
	sete	%cl
	orb	%cl, %dl
	jne	.L11
	cmpl	$2146435072, %eax
	je	.L13
.L11:
	pxor	%xmm1, %xmm1
	comisd	%xmm0, %xmm1
	jnb	.L13
	mulsd	.LC1(%rip), %xmm0
	movq	%xmm0, %rdx
	movq	%rdx, %rax
	shrq	$32, %rax
	movl	%eax, %ecx
	andl	$2147483647, %ecx
	orl	%edx, %ecx
	sete	%dl
	andl	$2146435072, %eax
	sete	%cl
	orb	%cl, %dl
	jne	.L15
	cmpl	$2146435072, %eax
	je	.L13
.L15:
	movsd	.LC2(%rip), %xmm1
	comisd	%xmm0, %xmm1
	ja	.L13
	comisd	.LC3(%rip), %xmm0
	jb	.L25
.L13:
	xorl	%eax, %eax
	addq	$40, %rsp
	ret
	.p2align 4,,10
	.p2align 3
.L25:
	movq	%r8, 56(%rsp)
	call	llround
	movq	56(%rsp), %r8
	movq	%rax, (%r8)
	movl	$1, %eax
	addq	$40, %rsp
	ret
	.seh_endproc
	.p2align 4
	.def	engine_step.isra.0;	.scl	3;	.type	32;	.endef
	.seh_proc	engine_step.isra.0
engine_step.isra.0:
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
	subq	$152, %rsp
	.seh_stackalloc	152
	.seh_endprologue
	movsd	264(%rsp), %xmm0
	movq	96(%rdx), %r12
	movq	%rdx, %rbx
	leaq	88(%rsp), %rdx
	movq	%rcx, %rbp
	movl	%r8d, %edi
	movq	$0x000000000, 72(%rsp)
	movl	%r9d, %esi
	movq	$0x000000000, 80(%rsp)
	call	engine_scale_value.constprop.0
	movl	%eax, %r14d
	testl	%eax, %eax
	jne	.L78
.L26:
	movl	%r14d, %eax
	addq	$152, %rsp
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
.L78:
	leaq	240(%rbx), %rax
	movq	88(%rsp), %r13
	movq	%r12, %r8
	movl	%edi, %edx
	movq	224(%rbx), %rcx
	movq	%rax, 32(%rsp)
	movq	%r13, %r9
	call	engine_fixed_sma_push
	movq	%r13, %r9
	movq	%r12, %r8
	movl	%esi, %edx
	movl	%eax, %r15d
	leaq	248(%rbx), %rax
	movq	232(%rbx), %rcx
	movq	%rax, 32(%rsp)
	call	engine_fixed_sma_push
	movl	%eax, %r10d
	cmpl	$-1, %r15d
	je	.L37
	cmpl	$-1, %eax
	je	.L37
	movl	%r15d, %eax
	andl	%r10d, %eax
	movl	%eax, 56(%rsp)
	jne	.L79
.L31:
	movl	16(%rbp), %edx
	testl	%edx, %edx
	jne	.L80
	movsd	272(%rsp), %xmm2
	movq	%rbx, %rcx
	movl	%r10d, 60(%rsp)
	movsd	264(%rsp), %xmm1
	call	ind_vwap_push
	movl	12(%rbp), %eax
	movl	60(%rsp), %r10d
	testl	%eax, %eax
	jne	.L81
	movslq	%esi, %r8
	movq	%r12, %rax
	xorl	%edx, %edx
	movq	104(%rbx), %r9
	divq	%r8
	movsd	264(%rsp), %xmm3
	movsd	%xmm3, (%r9,%rdx,8)
	testl	%r15d, %r15d
	je	.L39
	movsd	72(%rsp), %xmm0
	testl	%edi, %edi
	je	.L40
	movl	%edi, %r11d
	leaq	1(%r8), %rcx
	subq	%r11, %rcx
	addq	%r12, %rcx
	addq	%rcx, %r11
	.p2align 5
	.p2align 4
	.p2align 3
.L41:
	movq	%rcx, %rax
	xorl	%edx, %edx
	addq	$1, %rcx
	divq	%r8
	addsd	(%r9,%rdx,8), %xmm0
	movsd	%xmm0, 72(%rsp)
	cmpq	%rcx, %r11
	jne	.L41
.L40:
	pxor	%xmm1, %xmm1
	cvtsi2sdl	%edi, %xmm1
	divsd	%xmm1, %xmm0
	movsd	%xmm0, 72(%rsp)
.L39:
	leaq	96(%rsp), %rdi
	testl	%r10d, %r10d
	je	.L36
	movsd	80(%rsp), %xmm0
	testl	%esi, %esi
	jle	.L42
	leaq	1(%r12), %rcx
	leaq	1(%r12,%r8), %r10
	.p2align 5
	.p2align 4
	.p2align 3
.L43:
	movq	%rcx, %rax
	xorl	%edx, %edx
	addq	$1, %rcx
	divq	%r8
	addsd	(%r9,%rdx,8), %xmm0
	movsd	%xmm0, 80(%rsp)
	cmpq	%r10, %rcx
	jne	.L43
.L42:
	pxor	%xmm1, %xmm1
	leaq	96(%rsp), %rdi
	cvtsi2sdl	%esi, %xmm1
	divsd	%xmm1, %xmm0
	movsd	%xmm0, 80(%rsp)
.L36:
	cmpl	$1, 56(%rsp)
	je	.L82
	cmpl	$-1, 56(%rsp)
	je	.L83
.L45:
	movsd	264(%rsp), %xmm2
	movq	256(%rsp), %rdx
	leaq	16(%rbx), %rcx
	movq	%rdi, %r9
	call	ind_ohlc_push
	testl	%eax, %eax
	je	.L46
	addq	$1, 64(%rbx)
.L46:
	addq	$1, 96(%rbx)
	jmp	.L26
	.p2align 4,,10
	.p2align 3
.L80:
	movsd	272(%rsp), %xmm0
	leaq	96(%rsp), %rdi
	movq	%rdi, %rdx
	call	engine_scale_value.constprop.0
	testl	%eax, %eax
	je	.L37
	movq	96(%rsp), %rsi
	leaq	264(%rbx), %rcx
	movq	%r13, %rdx
	movq	%rsi, %r8
	call	fp_acc_add_product
	testl	%eax, %eax
	jne	.L84
	.p2align 4
	.p2align 3
.L37:
	xorl	%r14d, %r14d
	jmp	.L26
	.p2align 4,,10
	.p2align 3
.L79:
	movq	240(%rbx), %rcx
	movq	248(%rbx), %r8
	movslq	%esi, %rdx
	movl	%edi, %r9d
	movl	%r10d, 60(%rsp)
	call	fp_compare_products
	movl	300(%rbx), %ecx
	movl	60(%rsp), %r10d
	testl	%ecx, %ecx
	je	.L47
	movl	296(%rbx), %edx
	testl	%edx, %edx
	jle	.L85
.L33:
	cltd
	movl	%edx, 56(%rsp)
	jmp	.L32
	.p2align 4,,10
	.p2align 3
.L81:
	movsd	264(%rsp), %xmm1
	leaq	72(%rsp), %r8
	leaq	96(%rsp), %rdi
	leaq	128(%rbx), %rcx
	call	ind_sma_push
	leaq	176(%rbx), %rcx
	leaq	80(%rsp), %r8
	movsd	264(%rsp), %xmm1
	call	ind_sma_push
	jmp	.L36
	.p2align 4,,10
	.p2align 3
.L47:
	movl	$0, 56(%rsp)
.L32:
	movl	%eax, 296(%rbx)
	movl	$1, 300(%rbx)
	jmp	.L31
	.p2align 4,,10
	.p2align 3
.L84:
	movq	256(%rbx), %rcx
	leaq	256(%rbx), %r8
	movq	%rsi, %rdx
	call	fp_checked_add
	testl	%eax, %eax
	jne	.L36
	jmp	.L37
	.p2align 4,,10
	.p2align 3
.L85:
	testl	%eax, %eax
	jg	.L32
	movl	$0, 56(%rsp)
	testl	%edx, %edx
	jne	.L32
	jmp	.L33
	.p2align 4,,10
	.p2align 3
.L83:
	addq	$1, 80(%rbx)
	movq	88(%rbx), %rcx
	leaq	1(%r12), %rdx
	movl	$-1, %r8d
	call	result_hash_signal
	movq	%rax, 88(%rbx)
	jmp	.L45
	.p2align 4,,10
	.p2align 3
.L82:
	addq	$1, 72(%rbx)
	movq	88(%rbx), %rcx
	leaq	1(%r12), %rdx
	movl	$1, %r8d
	call	result_hash_signal
	movq	%rax, 88(%rbx)
	jmp	.L45
	.seh_endproc
	.section .rdata,"dr"
.LC4:
	.ascii "rb\0"
	.text
	.p2align 4
	.globl	engine_execute
	.def	engine_execute;	.scl	2;	.type	32;	.endef
	.seh_proc	engine_execute
engine_execute:
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
	subq	$584, %rsp
	.seh_stackalloc	584
	movups	%xmm6, 528(%rsp)
	.seh_savexmm	%xmm6, 528
	movups	%xmm7, 544(%rsp)
	.seh_savexmm	%xmm7, 544
	movups	%xmm8, 560(%rsp)
	.seh_savexmm	%xmm8, 560
	.seh_endprologue
	movq	%rcx, %r12
	movl	%edx, %esi
	movl	%r8d, %edi
	xorl	%edx, %edx
	movl	$304, %r8d
	leaq	224(%rsp), %rcx
	movq	%r9, %rbx
	call	memset
	movl	ready.1(%rip), %eax
	testl	%eax, %eax
	je	.L184
.L87:
	movq	__imp_QueryPerformanceCounter(%rip), %rbp
	leaq	176(%rsp), %rcx
	call	*%rbp
	movq	freq.0(%rip), %rax
	movq	%rax, 64(%rsp)
	movq	176(%rsp), %rax
	movq	%rax, 72(%rsp)
	testq	%r12, %r12
	je	.L88
	testq	%rbx, %rbx
	je	.L88
	cmpq	$0, 688(%rsp)
	je	.L88
	testl	%esi, %esi
	jle	.L88
	cmpl	%edi, %esi
	jg	.L88
	cmpl	$100000, %edi
	jg	.L88
	movabsq	$-3750763034362895579, %rax
	movl	$8, %edx
	movl	%esi, %ecx
	movl	%edi, %r15d
	movq	%rax, 312(%rsp)
	call	calloc
	movl	$8, %edx
	movq	%r15, %rcx
	movq	%rax, 448(%rsp)
	movq	%rax, 80(%rsp)
	call	calloc
	movq	%rax, 456(%rsp)
	cmpq	$0, 80(%rsp)
	je	.L140
	testq	%rax, %rax
	je	.L140
	movl	16(%rbx), %eax
	testl	%eax, %eax
	je	.L185
.L90:
	movl	20(%rbx), %r14d
	testl	%r14d, %r14d
	je	.L95
	movl	ready.1(%rip), %r13d
	testl	%r13d, %r13d
	je	.L186
.L96:
	leaq	176(%rsp), %rcx
	call	*%rbp
	movq	freq.0(%rip), %rax
	movq	%rax, 120(%rsp)
	movq	176(%rsp), %rax
	movq	%rax, 128(%rsp)
	cmpl	$2, 20(%rbx)
	je	.L187
.L140:
	xorl	%r13d, %r13d
.L94:
	movq	328(%rsp), %rcx
	call	free
	movq	448(%rsp), %rcx
	call	free
	movq	456(%rsp), %rcx
	call	free
	movq	336(%rsp), %rcx
	call	free
	movq	344(%rsp), %rcx
	call	free
	cmpl	$2, 20(%rbx)
	je	.L188
	.p2align 4
	.p2align 3
.L88:
	xorl	%eax, %eax
.L86:
	movups	528(%rsp), %xmm6
	movups	544(%rsp), %xmm7
	movups	560(%rsp), %xmm8
	addq	$584, %rsp
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
.L184:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L87
	.p2align 4,,10
	.p2align 3
.L185:
	movl	12(%rbx), %eax
	movl	$8, %edx
	testl	%eax, %eax
	je	.L91
	movl	%esi, %ecx
	call	calloc
	movl	$8, %edx
	movq	%r15, %rcx
	movq	%rax, 336(%rsp)
	movq	%rax, 80(%rsp)
	call	calloc
	movq	80(%rsp), %r9
	movq	%rax, 344(%rsp)
	testq	%r9, %r9
	je	.L140
	testq	%rax, %rax
	je	.L140
	leaq	352(%rsp), %rcx
	movl	%esi, %r8d
	movq	%r9, %rdx
	call	ind_sma_init
	testl	%eax, %eax
	je	.L140
	movq	344(%rsp), %rdx
	leaq	400(%rsp), %rcx
	movq	%r15, %r8
	call	ind_sma_init
	testl	%eax, %eax
	jne	.L90
	jmp	.L140
	.p2align 4,,10
	.p2align 3
.L95:
	leaq	.LC4(%rip), %rdx
	movq	%r12, %rcx
	call	fopen
	movq	%rax, %r14
	testq	%rax, %rax
	je	.L140
	movq	$0, 104(%rsp)
	xorl	%r15d, %r15d
	xorl	%r13d, %r13d
.L116:
	movl	ready.1(%rip), %r10d
	testl	%r10d, %r10d
	je	.L189
.L117:
	leaq	176(%rsp), %rcx
	call	*%rbp
	movq	freq.0(%rip), %rax
	movl	20(%rbx), %r9d
	movq	%rax, 88(%rsp)
	movq	176(%rsp), %rax
	movq	%rax, 96(%rsp)
	testl	%r9d, %r9d
	jne	.L180
	leaq	168(%rsp), %rax
	movq	%r12, 656(%rsp)
	movq	%rbp, %r15
	movl	%edi, %ebp
	movl	%esi, %r12d
	movq	%rax, 80(%rsp)
	leaq	156(%rsp), %rsi
	leaq	160(%rsp), %rdi
	jmp	.L118
	.p2align 4,,10
	.p2align 3
.L190:
	call	fast_read_trade
.L122:
	testl	%eax, %eax
	je	.L123
	js	.L126
	movsd	176(%rsp), %xmm0
	movl	%ebp, %r9d
	movl	%r12d, %r8d
	movq	%rbx, %rcx
	movq	160(%rsp), %rax
	leaq	224(%rsp), %rdx
	movsd	%xmm0, 48(%rsp)
	movsd	168(%rsp), %xmm0
	movq	%rax, 32(%rsp)
	movsd	%xmm0, 40(%rsp)
	call	engine_step.isra.0
	testl	%eax, %eax
	je	.L126
.L118:
	movl	8(%rbx), %r8d
	movq	%rdi, %rdx
	movq	%rsi, 32(%rsp)
	movq	%r14, %rcx
	leaq	176(%rsp), %r9
	testl	%r8d, %r8d
	movq	80(%rsp), %r8
	jne	.L190
	call	read_trade
	jmp	.L122
	.p2align 4,,10
	.p2align 3
.L180:
	testq	%r15, %r15
	je	.L183
	movq	%r15, %rax
	movq	%r12, 656(%rsp)
	movq	%rbp, %r15
	movl	%edi, %ebp
	salq	$5, %rax
	movl	%esi, %edi
	movq	%r13, %rsi
	leaq	0(%r13,%rax), %r12
	jmp	.L129
	.p2align 4,,10
	.p2align 3
.L192:
	addq	$32, %rsi
	cmpq	%rsi, %r12
	je	.L191
.L129:
	movq	16(%rbx), %rax
	movdqu	(%rbx), %xmm2
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	8(%rsi), %xmm0
	cvtsi2sdq	16(%rsi), %xmm1
	movl	%ebp, %r9d
	movl	%edi, %r8d
	movq	%rax, 192(%rsp)
	divsd	.LC1(%rip), %xmm0
	divsd	.LC1(%rip), %xmm1
	leaq	224(%rsp), %rdx
	movsd	%xmm1, 48(%rsp)
	leaq	176(%rsp), %rcx
	movsd	%xmm0, 40(%rsp)
	movq	(%rsi), %rax
	movl	$1, 192(%rsp)
	movq	%rax, 32(%rsp)
	movups	%xmm2, 176(%rsp)
	call	engine_step.isra.0
	testl	%eax, %eax
	jne	.L192
.L126:
	movq	%r14, %rcx
	testq	%r14, %r14
	je	.L94
	call	fclose
	jmp	.L94
	.p2align 4,,10
	.p2align 3
.L91:
	movq	%r15, %rcx
	call	calloc
	movq	%rax, 328(%rsp)
	movq	%rax, %r13
	testq	%rax, %rax
	jne	.L90
	jmp	.L94
	.p2align 4,,10
	.p2align 3
.L188:
	movq	%r13, %rcx
	call	free
	jmp	.L88
	.p2align 4,,10
	.p2align 3
.L189:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L117
	.p2align 4,,10
	.p2align 3
.L191:
	movq	656(%rsp), %r12
	movl	%edi, %esi
	movl	%ebp, %edi
	movq	%r15, %rbp
.L183:
	leaq	168(%rsp), %rax
	movq	%rax, 80(%rsp)
.L120:
	movq	64(%rsp), %rcx
	movq	72(%rsp), %rax
	xorl	%edx, %edx
	divq	%rcx
	movq	%rax, 72(%rsp)
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%rcx
	movq	88(%rsp), %rcx
	xorl	%edx, %edx
	movq	%rax, 112(%rsp)
	movq	96(%rsp), %rax
	divq	%rcx
	movq	%rax, %r15
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%rcx
	movl	ready.1(%rip), %ecx
	movq	%rax, %r14
	testl	%ecx, %ecx
	je	.L193
.L130:
	leaq	176(%rsp), %rcx
	call	*%rbp
	movq	freq.0(%rip), %r8
	movq	176(%rsp), %rax
	xorl	%edx, %edx
	divq	%r8
	subq	%r15, %rax
	imulq	$1000000000, %rax, %rcx
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	subq	%r14, %rcx
	divq	%r8
	leaq	176(%rsp), %rdx
	addq	%rcx, %rax
	leaq	240(%rsp), %rcx
	movq	%rax, 64(%rsp)
	call	ind_ohlc_flush
	testl	%eax, %eax
	je	.L131
	addq	$1, 288(%rsp)
.L131:
	movl	16(%rbx), %edx
	movq	(%rbx), %r15
	movq	320(%rsp), %r14
	testl	%edx, %edx
	je	.L132
	pxor	%xmm6, %xmm6
	cmpq	$0, 480(%rsp)
	jne	.L194
.L133:
	movdqu	288(%rsp), %xmm8
	movl	ready.1(%rip), %eax
	movdqu	304(%rsp), %xmm7
	testl	%eax, %eax
	je	.L195
.L134:
	movq	80(%rsp), %rcx
	call	*%rbp
	movq	freq.0(%rip), %rcx
	movq	168(%rsp), %rax
	xorl	%edx, %edx
	divq	%rcx
	subq	72(%rsp), %rax
	imulq	$1000000000, %rax, %rbp
	subq	112(%rsp), %rbp
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%rcx
	addq	%rax, %rbp
	call	memusage_peak_kb
	movq	688(%rsp), %rdx
	movq	328(%rsp), %rcx
	movl	%esi, 24(%rdx)
	movq	104(%rsp), %rsi
	movq	%r15, (%rdx)
	movq	%rsi, 72(%rdx)
	movq	64(%rsp), %rsi
	movq	%r12, 8(%rdx)
	movq	%r14, 16(%rdx)
	movl	%edi, 28(%rdx)
	movq	%rsi, 80(%rdx)
	movq	%rbp, 88(%rdx)
	movq	%rax, 96(%rdx)
	movsd	%xmm6, 32(%rdx)
	movups	%xmm8, 40(%rdx)
	movups	%xmm7, 56(%rdx)
	call	free
	movq	448(%rsp), %rcx
	call	free
	movq	456(%rsp), %rcx
	call	free
	movq	336(%rsp), %rcx
	call	free
	movq	344(%rsp), %rcx
	call	free
	cmpl	$2, 20(%rbx)
	je	.L196
.L135:
	xorl	%eax, %eax
	cmpq	$0, 320(%rsp)
	setne	%al
	jmp	.L86
	.p2align 4,,10
	.p2align 3
.L187:
	leaq	.LC4(%rip), %rdx
	movq	%r12, %rcx
	call	fopen
	movq	%rax, %r13
	testq	%rax, %rax
	je	.L94
	leaq	168(%rsp), %rdx
	leaq	160(%rsp), %rcx
	movq	%r12, 656(%rsp)
	movq	%rbp, %r14
	leaq	156(%rsp), %r11
	movq	%rdx, 80(%rsp)
	movq	%rax, %r12
	movq	%rdx, %rbp
	movl	%esi, 664(%rsp)
	movq	%r11, %rsi
	movl	%edi, 672(%rsp)
	movq	%rcx, %rdi
	movq	%rbx, 680(%rsp)
	xorl	%ebx, %ebx
	movq	%rcx, 104(%rsp)
	movq	%r11, 96(%rsp)
	jmp	.L100
	.p2align 4,,10
	.p2align 3
.L197:
	js	.L142
	cmpq	$-1, %rbx
	je	.L142
	addq	$1, %rbx
.L100:
	movq	%rsi, 32(%rsp)
	movq	%rbp, %r8
	movq	%rdi, %rdx
	movq	%r12, %rcx
	leaq	176(%rsp), %r9
	call	fast_read_trade
	testl	%eax, %eax
	jne	.L197
	movq	%r13, %rcx
	movq	%rbx, 112(%rsp)
	movq	%rbx, %r15
	movq	%r14, %rbp
	movq	656(%rsp), %r12
	movl	664(%rsp), %esi
	movl	672(%rsp), %edi
	movq	680(%rsp), %rbx
	call	ferror
	movq	%r13, %rcx
	movl	%eax, %r13d
	call	fclose
	orl	%r13d, %eax
	jne	.L140
	movq	%r15, %rax
	shrq	$59, %rax
	jne	.L140
	testq	%r15, %r15
	je	.L138
	salq	$5, %r15
	movq	%r15, %rcx
	call	malloc
	movq	%rax, %r13
	testq	%rax, %rax
	je	.L94
.L101:
	leaq	.LC4(%rip), %rdx
	movq	%r12, %rcx
	call	fopen
	movq	%rax, %r14
	testq	%rax, %rax
	je	.L102
	movl	%esi, 664(%rsp)
	movq	%r13, %rsi
	movl	%edi, 672(%rsp)
	xorl	%edi, %edi
	movq	%rbp, 136(%rsp)
	movq	%rbx, %rbp
	jmp	.L103
	.p2align 4,,10
	.p2align 3
.L199:
	movsd	.LC1(%rip), %xmm0
	mulsd	168(%rsp), %xmm0
	movq	%xmm0, %rdx
	movq	%rdx, %rax
	shrq	$32, %rax
	movl	%eax, %ecx
	andl	$2147483647, %ecx
	orl	%edx, %ecx
	sete	%dl
	andl	$2146435072, %eax
	sete	%cl
	orb	%cl, %dl
	jne	.L143
	cmpl	$2146435072, %eax
	je	.L106
.L143:
	movsd	.LC1(%rip), %xmm1
	mulsd	176(%rsp), %xmm1
	movq	%xmm1, %rdx
	movq	%rdx, %rax
	shrq	$32, %rax
	movl	%eax, %ecx
	andl	$2147483647, %ecx
	orl	%edx, %ecx
	sete	%dl
	andl	$2146435072, %eax
	sete	%cl
	orb	%cl, %dl
	jne	.L144
	cmpl	$2146435072, %eax
	je	.L106
.L144:
	movsd	.LC2(%rip), %xmm5
	comisd	%xmm0, %xmm5
	ja	.L106
	comisd	%xmm1, %xmm5
	ja	.L106
	comisd	.LC3(%rip), %xmm0
	ja	.L106
	comisd	.LC3(%rip), %xmm1
	ja	.L106
	movq	160(%rsp), %rax
	movl	156(%rsp), %ebx
	addq	$1, %rdi
	addq	$32, %rsi
	movsd	%xmm1, 88(%rsp)
	movq	%rax, -32(%rsi)
	call	llround
	movsd	88(%rsp), %xmm0
	movq	%rax, -24(%rsi)
	call	llround
	movl	%ebx, -8(%rsi)
	movq	%rax, -16(%rsi)
.L103:
	movq	96(%rsp), %rax
	movq	80(%rsp), %r8
	movq	%r14, %rcx
	leaq	176(%rsp), %r9
	movq	104(%rsp), %rdx
	movq	%rax, 32(%rsp)
	call	fast_read_trade
	testl	%eax, %eax
	jle	.L198
	cmpl	$1, 20(%rbp)
	movl	$10000000, %eax
	cmovne	112(%rsp), %rax
	cmpq	%rax, %rdi
	jnb	.L106
	testq	%rsi, %rsi
	jne	.L199
	.p2align 4
	.p2align 3
.L106:
	movq	%rdi, %r15
	movq	%rbp, %rbx
	movl	$-1, %edx
	movl	664(%rsp), %esi
	movl	672(%rsp), %edi
	movq	136(%rsp), %rbp
.L112:
	movq	%r14, %rcx
	movl	%edx, 88(%rsp)
	call	ferror
	movq	%r14, %rcx
	movl	%eax, 80(%rsp)
	call	fclose
	movl	ready.1(%rip), %r11d
	movl	88(%rsp), %edx
	movl	%eax, %r14d
	testl	%r11d, %r11d
	je	.L200
.L109:
	movl	%edx, 88(%rsp)
	leaq	176(%rsp), %rcx
	call	*%rbp
	movl	80(%rsp), %r10d
	orl	88(%rsp), %r10d
	movl	%r10d, %edx
	movq	freq.0(%rip), %r8
	movq	176(%rsp), %rcx
	orl	%r14d, %edx
	movl	20(%rbx), %eax
	je	.L201
	cmpl	$2, %eax
	jne	.L140
.L114:
	movq	%r13, %rcx
	call	free
	jmp	.L140
	.p2align 4,,10
	.p2align 3
.L186:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L96
	.p2align 4,,10
	.p2align 3
.L123:
	movq	%r14, %rcx
	movl	%r12d, %esi
	movq	656(%rsp), %r12
	movl	%ebp, %edi
	call	ferror
	movq	%r14, %rcx
	movq	%r15, %rbp
	movl	%eax, %r15d
	call	fclose
	orl	%r15d, %eax
	je	.L120
	jmp	.L94
	.p2align 4,,10
	.p2align 3
.L193:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L130
	.p2align 4,,10
	.p2align 3
.L195:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	$1, ready.1(%rip)
	jmp	.L134
	.p2align 4,,10
	.p2align 3
.L132:
	leaq	224(%rsp), %rcx
	call	ind_vwap_value
	movapd	%xmm0, %xmm6
	jmp	.L133
	.p2align 4,,10
	.p2align 3
.L142:
	movq	%r13, %rcx
	movq	680(%rsp), %rbx
	call	fclose
	jmp	.L140
	.p2align 4,,10
	.p2align 3
.L194:
	leaq	488(%rsp), %rcx
	call	fp_acc_to_double
	pxor	%xmm1, %xmm1
	cvtsi2sdq	480(%rsp), %xmm1
	divsd	%xmm1, %xmm0
	divsd	.LC1(%rip), %xmm0
	movapd	%xmm0, %xmm6
	jmp	.L133
.L196:
	movq	%r13, %rcx
	call	free
	jmp	.L135
.L201:
	cmpl	$2, %eax
	jne	.L145
	cmpq	112(%rsp), %r15
	jne	.L114
.L145:
	movq	120(%rsp), %r11
	movq	128(%rsp), %rax
	xorl	%edx, %edx
	xorl	%r14d, %r14d
	divq	%r11
	movq	%rax, %r10
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	divq	%r11
	xorl	%edx, %edx
	movq	%rax, %r9
	movq	%rcx, %rax
	divq	%r8
	movq	%rax, %rcx
	imulq	$1000000000, %rdx, %rax
	xorl	%edx, %edx
	subq	%r10, %rcx
	imulq	$1000000000, %rcx, %rcx
	divq	%r8
	subq	%r9, %rcx
	addq	%rcx, %rax
	movq	%rax, 104(%rsp)
	jmp	.L116
.L198:
	movq	%rdi, %r15
	movq	%rbp, %rbx
	movl	664(%rsp), %esi
	movl	%eax, %edx
	movl	672(%rsp), %edi
	movq	136(%rsp), %rbp
	jmp	.L112
.L200:
	leaq	freq.0(%rip), %rcx
	call	*__imp_QueryPerformanceFrequency(%rip)
	movl	88(%rsp), %edx
	movl	$1, ready.1(%rip)
	jmp	.L109
.L138:
	xorl	%r13d, %r13d
	jmp	.L101
.L102:
	cmpl	$2, 20(%rbx)
	jne	.L140
	jmp	.L114
	.seh_endproc
	.section .rdata,"dr"
.LC5:
	.ascii "--help\0"
	.align 8
.LC6:
	.ascii "Usage: %s --file CSV [--short N] [--long N]\12\0"
.LC7:
	.ascii "--file\0"
.LC8:
	.ascii "--short\0"
.LC9:
	.ascii "--long\0"
	.align 8
.LC10:
	.ascii "Tick-to-Trade engine %s\12file=%s short=%d long=%d\12\0"
.LC11:
	.ascii "PASS\0"
	.text
	.p2align 4
	.globl	engine_program_main
	.def	engine_program_main;	.scl	2;	.type	32;	.endef
	.seh_proc	engine_program_main
engine_program_main:
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
	subq	$184, %rsp
	.seh_stackalloc	184
	.seh_endprologue
	movl	$10, 56(%rsp)
	movq	%rdx, %r13
	movq	%r8, %r14
	movl	$60, 60(%rsp)
	cmpl	$1, %ecx
	jle	.L206
	movl	%ecx, %eax
	leal	-1(%rcx), %edi
	movq	%rdx, %rbx
	xorl	%r12d, %r12d
	shrl	%eax
	shrl	%edi
	subl	$1, %eax
	salq	$4, %rdi
	salq	$4, %rax
	addq	%rdx, %rdi
	leaq	16(%rdx,%rax), %rbp
	jmp	.L210
	.p2align 4,,10
	.p2align 3
.L231:
	leaq	.LC8(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	leaq	56(%rsp), %r8
	testl	%eax, %eax
	je	.L228
	leaq	.LC9(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L206
	leaq	60(%rsp), %r8
.L228:
	movl	$100000, %edx
	movq	%r15, %rcx
	call	cli_parse_positive_int
	testl	%eax, %eax
	je	.L206
	addq	$16, %rbx
	cmpq	%rbp, %rbx
	je	.L229
.L210:
	movq	8(%rbx), %rsi
	leaq	.LC5(%rip), %rdx
	movq	%rsi, %rcx
	call	strcmp
	movl	%eax, %r15d
	testl	%eax, %eax
	je	.L230
	cmpq	%rdi, %rbx
	je	.L206
	leaq	.LC7(%rip), %rdx
	movq	%rsi, %rcx
	movq	16(%rbx), %r15
	call	strcmp
	testl	%eax, %eax
	jne	.L231
	addq	$16, %rbx
	movq	%r15, %r12
	cmpq	%rbp, %rbx
	jne	.L210
.L229:
	testq	%r12, %r12
	je	.L206
	movl	56(%rsp), %r9d
	movl	60(%rsp), %eax
	cmpl	%eax, %r9d
	jg	.L206
	movq	(%r14), %rdx
	movl	%eax, 32(%rsp)
	movq	%r12, %r8
	leaq	.LC10(%rip), %rcx
	leaq	64(%rsp), %rbx
	call	printf
	movq	%rbx, 32(%rsp)
	movq	%r14, %r9
	movq	%r12, %rcx
	movl	60(%rsp), %r8d
	movl	56(%rsp), %edx
	call	engine_execute
	testl	%eax, %eax
	je	.L212
	movl	$1, %ecx
	call	*__imp___acrt_iob_func(%rip)
	movq	%rbx, %rdx
	movq	%rax, %rcx
	call	result_print
	testl	%eax, %eax
	je	.L212
	leaq	.LC11(%rip), %rcx
	xorl	%r15d, %r15d
	call	puts
	jmp	.L202
	.p2align 4,,10
	.p2align 3
.L206:
	movl	$2, %r15d
.L202:
	movl	%r15d, %eax
	addq	$184, %rsp
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
.L230:
	movq	0(%r13), %rdx
	leaq	.LC6(%rip), %rcx
	call	printf
	jmp	.L202
.L212:
	movl	$6, %r15d
	jmp	.L202
	.seh_endproc
.lcomm freq.0,8,8
.lcomm ready.1,4,4
	.section .rdata,"dr"
	.align 8
.LC1:
	.long	0
	.long	1100470148
	.align 8
.LC2:
	.long	0
	.long	1071644672
	.align 8
.LC3:
	.long	0
	.long	1138753536
	.ident	"GCC: (Rev5, Built by MSYS2 project) 16.1.0"
	.def	fp_checked_add;	.scl	2;	.type	32;	.endef
	.def	llround;	.scl	2;	.type	32;	.endef
	.def	ind_vwap_push;	.scl	2;	.type	32;	.endef
	.def	ind_ohlc_push;	.scl	2;	.type	32;	.endef
	.def	fp_acc_add_product;	.scl	2;	.type	32;	.endef
	.def	fp_compare_products;	.scl	2;	.type	32;	.endef
	.def	ind_sma_push;	.scl	2;	.type	32;	.endef
	.def	result_hash_signal;	.scl	2;	.type	32;	.endef
	.def	memset;	.scl	2;	.type	32;	.endef
	.def	calloc;	.scl	2;	.type	32;	.endef
	.def	free;	.scl	2;	.type	32;	.endef
	.def	ind_sma_init;	.scl	2;	.type	32;	.endef
	.def	fopen;	.scl	2;	.type	32;	.endef
	.def	fast_read_trade;	.scl	2;	.type	32;	.endef
	.def	read_trade;	.scl	2;	.type	32;	.endef
	.def	fclose;	.scl	2;	.type	32;	.endef
	.def	ind_ohlc_flush;	.scl	2;	.type	32;	.endef
	.def	memusage_peak_kb;	.scl	2;	.type	32;	.endef
	.def	ferror;	.scl	2;	.type	32;	.endef
	.def	malloc;	.scl	2;	.type	32;	.endef
	.def	ind_vwap_value;	.scl	2;	.type	32;	.endef
	.def	fp_acc_to_double;	.scl	2;	.type	32;	.endef
	.def	strcmp;	.scl	2;	.type	32;	.endef
	.def	cli_parse_positive_int;	.scl	2;	.type	32;	.endef
	.def	printf;	.scl	2;	.type	32;	.endef
	.def	result_print;	.scl	2;	.type	32;	.endef
	.def	puts;	.scl	2;	.type	32;	.endef
