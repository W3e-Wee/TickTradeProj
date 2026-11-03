	.file	"replay_engine.c"
	.text
	.def	now_ns;	.scl	3;	.type	32;	.endef
	.seh_proc	now_ns
now_ns:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$64, %rsp
	.seh_stackalloc	64
	.seh_endprologue
	movl	ready.1(%rip), %eax
	testl	%eax, %eax
	jne	.L2
	leaq	freq.0(%rip), %rax
	movq	%rax, %rcx
	movq	__imp_QueryPerformanceFrequency(%rip), %rax
	call	*%rax
	movl	$1, ready.1(%rip)
.L2:
	leaq	-24(%rbp), %rax
	movq	%rax, %rcx
	movq	__imp_QueryPerformanceCounter(%rip), %rax
	call	*%rax
	movq	freq.0(%rip), %rax
	movq	%rax, -8(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	movl	$0, %edx
	divq	-8(%rbp)
	imulq	$1000000000, %rax, %rcx
	movq	-16(%rbp), %rax
	movl	$0, %edx
	divq	-8(%rbp)
	movq	%rdx, %rax
	imulq	$1000000000, %rax, %rax
	movl	$0, %edx
	divq	-8(%rbp)
	addq	%rcx, %rax
	addq	$64, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	sleep_ms;	.scl	3;	.type	32;	.endef
	.seh_proc	sleep_ms
sleep_ms:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$32, %rsp
	.seh_stackalloc	32
	.seh_endprologue
	movl	%ecx, 16(%rbp)
	movl	16(%rbp), %eax
	movl	%eax, %ecx
	movq	__imp_Sleep(%rip), %rax
	call	*%rax
	nop
	addq	$32, %rsp
	popq	%rbp
	ret
	.seh_endproc
.lcomm replay_stop_requested,4,4
	.def	replay_signal_handler;	.scl	3;	.type	32;	.endef
	.seh_proc	replay_signal_handler
replay_signal_handler:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	.seh_endprologue
	movl	%ecx, 16(%rbp)
	movl	$1, replay_stop_requested(%rip)
	nop
	popq	%rbp
	ret
	.seh_endproc
	.def	real_now;	.scl	3;	.type	32;	.endef
	.seh_proc	real_now
real_now:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$32, %rsp
	.seh_stackalloc	32
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	call	now_ns
	addq	$32, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	real_wait;	.scl	3;	.type	32;	.endef
	.seh_proc	real_wait
real_wait:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$64, %rsp
	.seh_stackalloc	64
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	call	now_ns
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	cmpq	16(%rbp), %rax
	jnb	.L13
	movq	16(%rbp), %rax
	subq	-16(%rbp), %rax
	movq	%rax, -24(%rbp)
	cmpq	$3000000, -24(%rbp)
	jbe	.L11
	movq	-24(%rbp), %rax
	movabsq	$4835703278458516699, %rdx
	mulq	%rdx
	movq	%rdx, %rax
	shrq	$18, %rax
	subq	$1, %rax
	movq	%rax, -8(%rbp)
	cmpq	$25, -8(%rbp)
	jbe	.L12
	movq	$25, -8(%rbp)
.L12:
	movq	-8(%rbp), %rax
	movl	%eax, %ecx
	call	sleep_ms
	jmp	.L8
.L11:
	movq	__imp_SwitchToThread(%rip), %rax
	call	*%rax
	jmp	.L8
.L13:
	nop
.L8:
	addq	$64, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.section .rdata,"dr"
.LC0:
	.ascii "False\0"
.LC1:
	.ascii "false\0"
.LC2:
	.ascii "True\0"
.LC3:
	.ascii "true\0"
	.text
	.def	parse_bool;	.scl	3;	.type	32;	.endef
	.seh_proc	parse_bool
parse_bool:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$32, %rsp
	.seh_stackalloc	32
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	leaq	.LC0(%rip), %rdx
	movq	16(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	je	.L15
	leaq	.LC1(%rip), %rdx
	movq	16(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L16
.L15:
	movq	24(%rbp), %rax
	movl	$1, (%rax)
	movl	$1, %eax
	jmp	.L17
.L16:
	leaq	.LC2(%rip), %rdx
	movq	16(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	je	.L18
	leaq	.LC3(%rip), %rdx
	movq	16(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L19
.L18:
	movq	24(%rbp), %rax
	movl	$0, (%rax)
	movl	$1, %eax
	jmp	.L17
.L19:
	movl	$0, %eax
.L17:
	addq	$32, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	trim_line;	.scl	3;	.type	32;	.endef
	.seh_proc	trim_line
trim_line:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$48, %rsp
	.seh_stackalloc	48
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	16(%rbp), %rax
	movq	%rax, %rcx
	call	strlen
	movq	%rax, -8(%rbp)
	jmp	.L21
.L23:
	subq	$1, -8(%rbp)
	movq	16(%rbp), %rdx
	movq	-8(%rbp), %rax
	addq	%rdx, %rax
	movb	$0, (%rax)
.L21:
	cmpq	$0, -8(%rbp)
	je	.L24
	movq	-8(%rbp), %rax
	leaq	-1(%rax), %rdx
	movq	16(%rbp), %rax
	addq	%rdx, %rax
	movzbl	(%rax), %eax
	cmpb	$10, %al
	je	.L23
	movq	-8(%rbp), %rax
	leaq	-1(%rax), %rdx
	movq	16(%rbp), %rax
	addq	%rdx, %rax
	movzbl	(%rax), %eax
	cmpb	$13, %al
	je	.L23
.L24:
	nop
	addq	$48, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.section .rdata,"dr"
.LC4:
	.ascii "trade_id\0"
	.text
	.def	skip_header_or_blank;	.scl	3;	.type	32;	.endef
	.seh_proc	skip_header_or_blank
skip_header_or_blank:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$32, %rsp
	.seh_stackalloc	32
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	jmp	.L26
.L27:
	addq	$1, 16(%rbp)
.L26:
	movq	16(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	movl	%eax, %ecx
	movq	__imp_isspace(%rip), %rax
	call	*%rax
	testl	%eax, %eax
	jne	.L27
	movq	16(%rbp), %rax
	movzbl	(%rax), %eax
	testb	%al, %al
	je	.L28
	leaq	.LC4(%rip), %rdx
	movq	16(%rbp), %rax
	movl	$8, %r8d
	movq	%rax, %rcx
	call	strncmp
	testl	%eax, %eax
	jne	.L29
.L28:
	movl	$1, %eax
	jmp	.L30
.L29:
	movl	$0, %eax
.L30:
	addq	$32, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	parse_integer_strict;	.scl	3;	.type	32;	.endef
	.seh_proc	parse_integer_strict
parse_integer_strict:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$48, %rsp
	.seh_stackalloc	48
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	movq	$0, -16(%rbp)
	movq	__imp__errno(%rip), %rax
	call	*%rax
	movl	$0, (%rax)
	leaq	-16(%rbp), %rdx
	movq	16(%rbp), %rax
	movl	$10, %r8d
	movq	%rax, %rcx
	call	strtoll
	movq	%rax, -8(%rbp)
	movq	__imp__errno(%rip), %rax
	call	*%rax
	movl	(%rax), %eax
	testl	%eax, %eax
	jne	.L33
	movq	-16(%rbp), %rax
	cmpq	%rax, 16(%rbp)
	je	.L33
	movq	-16(%rbp), %rax
	movzbl	(%rax), %eax
	testb	%al, %al
	je	.L34
.L33:
	movl	$0, %eax
	jmp	.L36
.L34:
	movq	24(%rbp), %rax
	movq	-8(%rbp), %rdx
	movq	%rdx, (%rax)
	movl	$1, %eax
.L36:
	addq	$48, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	parse_double_strict;	.scl	3;	.type	32;	.endef
	.seh_proc	parse_double_strict
parse_double_strict:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$48, %rsp
	.seh_stackalloc	48
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	movq	$0, -16(%rbp)
	movq	__imp__errno(%rip), %rax
	call	*%rax
	movl	$0, (%rax)
	leaq	-16(%rbp), %rdx
	movq	16(%rbp), %rax
	movq	%rax, %rcx
	call	strtod
	movq	%xmm0, %rax
	movq	%rax, -8(%rbp)
	movq	__imp__errno(%rip), %rax
	call	*%rax
	movl	(%rax), %eax
	testl	%eax, %eax
	jne	.L38
	movq	-16(%rbp), %rax
	cmpq	%rax, 16(%rbp)
	je	.L38
	movq	-16(%rbp), %rax
	movzbl	(%rax), %eax
	testb	%al, %al
	jne	.L38
	movq	-8(%rbp), %rax
	movq	%rax, %xmm0
	call	__fpclassify
	andl	$256, %eax
	testl	%eax, %eax
	je	.L39
.L38:
	movl	$0, %eax
	jmp	.L41
.L39:
	movq	24(%rbp), %rax
	movsd	-8(%rbp), %xmm0
	movsd	%xmm0, (%rax)
	movl	$1, %eax
.L41:
	addq	$48, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.section .rdata,"dr"
.LC5:
	.ascii ",\0"
	.text
	.def	parse_baseline_line;	.scl	3;	.type	32;	.endef
	.seh_proc	parse_baseline_line
parse_baseline_line:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	addq	$-128, %rsp
	.seh_stackalloc	128
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	pxor	%xmm0, %xmm0
	movups	%xmm0, -80(%rbp)
	movups	%xmm0, -64(%rbp)
	movups	%xmm0, -48(%rbp)
	movq	%xmm0, -32(%rbp)
	movq	$0, -8(%rbp)
	leaq	.LC5(%rip), %rdx
	movq	16(%rbp), %rax
	movq	%rax, %rcx
	movq	__imp_strtok(%rip), %rax
	call	*%rax
	movq	%rax, -16(%rbp)
	jmp	.L43
.L45:
	movq	-8(%rbp), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, -8(%rbp)
	movq	-16(%rbp), %rdx
	movq	%rdx, -80(%rbp,%rax,8)
	leaq	.LC5(%rip), %rax
	movq	%rax, %rdx
	movl	$0, %ecx
	movq	__imp_strtok(%rip), %rax
	call	*%rax
	movq	%rax, -16(%rbp)
.L43:
	cmpq	$0, -16(%rbp)
	je	.L44
	cmpq	$6, -8(%rbp)
	jbe	.L45
.L44:
	pxor	%xmm0, %xmm0
	movsd	%xmm0, -88(%rbp)
	movl	$0, -92(%rbp)
	cmpq	$7, -8(%rbp)
	jne	.L46
	movq	24(%rbp), %rdx
	movq	-80(%rbp), %rax
	movq	%rax, %rcx
	call	parse_integer_strict
	testl	%eax, %eax
	je	.L46
	movq	24(%rbp), %rax
	leaq	16(%rax), %rdx
	movq	-72(%rbp), %rax
	movq	%rax, %rcx
	call	parse_double_strict
	testl	%eax, %eax
	je	.L46
	movq	24(%rbp), %rax
	leaq	32(%rax), %rdx
	movq	-64(%rbp), %rax
	movq	%rax, %rcx
	call	parse_double_strict
	testl	%eax, %eax
	je	.L46
	movq	-56(%rbp), %rax
	leaq	-88(%rbp), %rdx
	movq	%rax, %rcx
	call	parse_double_strict
	testl	%eax, %eax
	je	.L46
	movq	24(%rbp), %rax
	leaq	8(%rax), %rdx
	movq	-48(%rbp), %rax
	movq	%rax, %rcx
	call	parse_integer_strict
	testl	%eax, %eax
	je	.L46
	movq	24(%rbp), %rax
	leaq	40(%rax), %rdx
	movq	-40(%rbp), %rax
	movq	%rax, %rcx
	call	parse_bool
	testl	%eax, %eax
	je	.L46
	movq	-32(%rbp), %rax
	leaq	-92(%rbp), %rdx
	movq	%rax, %rcx
	call	parse_bool
	testl	%eax, %eax
	je	.L46
	movq	24(%rbp), %rax
	movq	(%rax), %rax
	testq	%rax, %rax
	js	.L46
	movq	24(%rbp), %rax
	movq	8(%rax), %rax
	testq	%rax, %rax
	js	.L46
	movq	24(%rbp), %rax
	movsd	16(%rax), %xmm0
	pxor	%xmm1, %xmm1
	comisd	%xmm1, %xmm0
	jbe	.L46
	movq	24(%rbp), %rax
	movsd	32(%rax), %xmm0
	pxor	%xmm1, %xmm1
	comisd	%xmm1, %xmm0
	jbe	.L46
	movsd	-88(%rbp), %xmm0
	pxor	%xmm1, %xmm1
	comisd	%xmm1, %xmm0
	jb	.L46
	movl	$1, %eax
	jmp	.L51
.L46:
	movl	$0, %eax
.L51:
	subq	$-128, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.section .rdata,"dr"
.LC7:
	.ascii "r\0"
	.text
	.def	reader_open;	.scl	3;	.type	32;	.endef
	.seh_proc	reader_open
reader_open:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$32, %rsp
	.seh_stackalloc	32
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	leaq	.LC7(%rip), %rdx
	movq	24(%rbp), %rax
	movq	%rax, %rcx
	call	fopen
	movq	16(%rbp), %rdx
	movq	%rax, (%rdx)
	movq	16(%rbp), %rax
	movq	(%rax), %rax
	testq	%rax, %rax
	jne	.L56
	movq	24(%rbp), %rax
	movq	%rax, %rcx
	call	perror
.L56:
	movq	16(%rbp), %rax
	movq	(%rax), %rax
	testq	%rax, %rax
	setne	%al
	movzbl	%al, %eax
	addq	$32, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	reader_next;	.scl	3;	.type	32;	.endef
	.seh_proc	reader_next
reader_next:
	pushq	%rbp
	.seh_pushreg	%rbp
	subq	$560, %rsp
	.seh_stackalloc	560
	leaq	128(%rsp), %rbp
	.seh_setframe	%rbp, 128
	.seh_endprologue
	movq	%rcx, 448(%rbp)
	movq	%rdx, 456(%rbp)
	jmp	.L59
.L64:
	leaq	-96(%rbp), %rax
	movq	%rax, %rcx
	call	strlen
	movq	%rax, 424(%rbp)
	cmpq	$511, 424(%rbp)
	jne	.L60
	movq	424(%rbp), %rax
	subq	$1, %rax
	movzbl	-96(%rbp,%rax), %eax
	cmpb	$10, %al
	je	.L60
	movq	448(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, %rcx
	call	feof
	testl	%eax, %eax
	jne	.L60
	movl	$-1, %eax
	jmp	.L67
.L60:
	leaq	-96(%rbp), %rax
	movq	%rax, %rcx
	call	trim_line
	leaq	-96(%rbp), %rax
	movq	%rax, %rcx
	call	skip_header_or_blank
	testl	%eax, %eax
	je	.L62
	jmp	.L59
.L62:
	movq	456(%rbp), %rdx
	leaq	-96(%rbp), %rax
	movq	%rax, %rcx
	call	parse_baseline_line
	testl	%eax, %eax
	jne	.L63
	movl	$-1, %eax
	jmp	.L67
.L63:
	movl	$1, %eax
	jmp	.L67
.L59:
	movq	448(%rbp), %rax
	movq	(%rax), %rdx
	leaq	-96(%rbp), %rax
	movq	%rdx, %r8
	movl	$512, %edx
	movq	%rax, %rcx
	call	fgets
	testq	%rax, %rax
	jne	.L64
	movq	448(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, %rcx
	call	ferror
	testl	%eax, %eax
	je	.L65
	movl	$-1, %eax
	jmp	.L67
.L65:
	movl	$0, %eax
.L67:
	addq	$560, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	reader_close;	.scl	3;	.type	32;	.endef
	.seh_proc	reader_close
reader_close:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$48, %rsp
	.seh_stackalloc	48
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	16(%rbp), %rax
	movq	(%rax), %rax
	testq	%rax, %rax
	jne	.L69
	movl	$1, %eax
	jmp	.L70
.L69:
	movq	16(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, %rcx
	call	fclose
	testl	%eax, %eax
	sete	%al
	movzbl	%al, %eax
	movl	%eax, -4(%rbp)
	movq	16(%rbp), %rax
	movq	$0, (%rax)
	movl	-4(%rbp), %eax
.L70:
	addq	$48, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	to_trade;	.scl	3;	.type	32;	.endef
	.seh_proc	to_trade
to_trade:
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$56, %rsp
	.seh_stackalloc	56
	leaq	48(%rsp), %rbp
	.seh_setframe	%rbp, 48
	.seh_endprologue
	movq	%rcx, 32(%rbp)
	movq	%rdx, 40(%rbp)
	pxor	%xmm0, %xmm0
	movups	%xmm0, -48(%rbp)
	movups	%xmm0, -32(%rbp)
	movups	%xmm0, -16(%rbp)
	movq	40(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -48(%rbp)
	movq	40(%rbp), %rax
	movsd	16(%rax), %xmm0
	movsd	%xmm0, -40(%rbp)
	movq	40(%rbp), %rax
	movsd	32(%rax), %xmm0
	movsd	%xmm0, -32(%rbp)
	movq	40(%rbp), %rax
	movsd	16(%rax), %xmm1
	movq	40(%rbp), %rax
	movsd	32(%rax), %xmm0
	mulsd	%xmm1, %xmm0
	movsd	%xmm0, -24(%rbp)
	movq	40(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, -16(%rbp)
	movq	40(%rbp), %rax
	movl	40(%rax), %eax
	movl	%eax, -8(%rbp)
	movq	32(%rbp), %rax
	movq	-48(%rbp), %rcx
	movq	-40(%rbp), %rbx
	movq	%rcx, (%rax)
	movq	%rbx, 8(%rax)
	movq	-32(%rbp), %rcx
	movq	-24(%rbp), %rbx
	movq	%rcx, 16(%rax)
	movq	%rbx, 24(%rax)
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rbx
	movq	%rcx, 32(%rax)
	movq	%rbx, 40(%rax)
	movq	32(%rbp), %rax
	addq	$56, %rsp
	popq	%rbx
	popq	%rbp
	ret
	.seh_endproc
	.def	grow_trade_array;	.scl	3;	.type	32;	.endef
	.seh_proc	grow_trade_array
grow_trade_array:
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$104, %rsp
	.seh_stackalloc	104
	leaq	96(%rsp), %rbp
	.seh_setframe	%rbp, 96
	.seh_endprologue
	movq	%rcx, 32(%rbp)
	movq	%rdx, 40(%rbp)
	movq	%r8, 48(%rbp)
	movq	%r9, 56(%rbp)
	movq	40(%rbp), %rax
	movq	(%rax), %rax
	cmpq	56(%rbp), %rax
	jb	.L74
	movl	$0, %eax
	jmp	.L75
.L74:
	movq	40(%rbp), %rax
	movq	(%rax), %rdx
	movq	48(%rbp), %rax
	movq	(%rax), %rax
	cmpq	%rax, %rdx
	jne	.L76
	movq	48(%rbp), %rax
	movq	(%rax), %rax
	testq	%rax, %rax
	je	.L77
	movq	48(%rbp), %rax
	movq	(%rax), %rax
	addq	%rax, %rax
	movq	%rax, -8(%rbp)
	jmp	.L78
.L77:
	movq	$4096, -8(%rbp)
.L78:
	movq	48(%rbp), %rax
	movq	(%rax), %rax
	cmpq	%rax, -8(%rbp)
	jb	.L79
	movq	-8(%rbp), %rax
	cmpq	%rax, 56(%rbp)
	jnb	.L80
.L79:
	movq	56(%rbp), %rax
	movq	%rax, -8(%rbp)
.L80:
	movq	48(%rbp), %rax
	movq	(%rax), %rax
	cmpq	-8(%rbp), %rax
	jnb	.L81
	movabsq	$384307168202282325, %rax
	cmpq	-8(%rbp), %rax
	jnb	.L82
.L81:
	movl	$0, %eax
	jmp	.L75
.L82:
	movq	-8(%rbp), %rdx
	movq	%rdx, %rax
	addq	%rax, %rax
	addq	%rdx, %rax
	salq	$4, %rax
	movq	%rax, %rdx
	movq	32(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, %rcx
	call	realloc
	movq	%rax, -16(%rbp)
	cmpq	$0, -16(%rbp)
	jne	.L83
	movl	$0, %eax
	jmp	.L75
.L83:
	movq	32(%rbp), %rax
	movq	-16(%rbp), %rdx
	movq	%rdx, (%rax)
	movq	48(%rbp), %rax
	movq	-8(%rbp), %rdx
	movq	%rdx, (%rax)
.L76:
	movq	32(%rbp), %rax
	movq	(%rax), %r8
	movq	40(%rbp), %rax
	movq	(%rax), %rdx
	leaq	1(%rdx), %rcx
	movq	40(%rbp), %rax
	movq	%rcx, (%rax)
	movq	%rdx, %rax
	addq	%rax, %rax
	addq	%rdx, %rax
	salq	$4, %rax
	leaq	(%r8,%rax), %rbx
	leaq	-64(%rbp), %rax
	movq	64(%rbp), %rdx
	movq	%rax, %rcx
	call	to_trade
	movq	-64(%rbp), %rax
	movq	-56(%rbp), %rdx
	movq	%rax, (%rbx)
	movq	%rdx, 8(%rbx)
	movq	-48(%rbp), %rax
	movq	-40(%rbp), %rdx
	movq	%rax, 16(%rbx)
	movq	%rdx, 24(%rbx)
	movq	-32(%rbp), %rax
	movq	-24(%rbp), %rdx
	movq	%rax, 32(%rbx)
	movq	%rdx, 40(%rbx)
	movl	$1, %eax
.L75:
	addq	$104, %rsp
	popq	%rbx
	popq	%rbp
	ret
	.seh_endproc
	.section .rdata,"dr"
	.align 8
.LC8:
	.ascii "cannot open replay loading pass\0"
.LC9:
	.ascii "trade array capacity exceeded\0"
	.align 8
.LC10:
	.ascii "whole-file dynamic allocation/capacity failed\0"
	.align 8
.LC11:
	.ascii "malformed input during loading pass\0"
	.text
	.def	load_replay_storage;	.scl	3;	.type	32;	.endef
	.seh_proc	load_replay_storage
load_replay_storage:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$160, %rsp
	.seh_stackalloc	160
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	movq	%r8, 32(%rbp)
	movq	%r9, 40(%rbp)
	movq	40(%rbp), %rax
	movq	$0, (%rax)
	movq	48(%rbp), %rax
	movq	$0, (%rax)
	movq	32(%rbp), %rax
	movl	$0, (%rax)
	movq	80(%rbp), %rax
	movq	$0, (%rax)
	movq	88(%rbp), %rax
	movq	$0, (%rax)
	call	now_ns
	movq	%rax, -8(%rbp)
	movq	$0, -32(%rbp)
	movq	$0, -40(%rbp)
	movq	$0, -48(%rbp)
	call	now_ns
	movq	%rax, -16(%rbp)
	movq	$0, -64(%rbp)
	movq	$0, -56(%rbp)
	movq	16(%rbp), %rdx
	leaq	-64(%rbp), %rax
	movq	%rax, %rcx
	call	reader_open
	testl	%eax, %eax
	jne	.L87
	movq	-48(%rbp), %rax
	movq	%rax, %rcx
	call	free
	leaq	.LC8(%rip), %rcx
	movq	104(%rbp), %rdx
	movq	96(%rbp), %rax
	movq	%rcx, %r8
	movq	%rax, %rcx
	call	snprintf
	movl	$0, %eax
	jmp	.L92
.L89:
	movq	-32(%rbp), %rax
	cmpq	56(%rbp), %rax
	jb	.L88
	leaq	-64(%rbp), %rax
	movq	%rax, %rcx
	call	reader_close
	movq	-48(%rbp), %rax
	movq	%rax, %rcx
	call	free
	leaq	.LC9(%rip), %rcx
	movq	104(%rbp), %rdx
	movq	96(%rbp), %rax
	movq	%rcx, %r8
	movq	%rax, %rcx
	call	snprintf
	movl	$0, %eax
	jmp	.L92
.L88:
	movq	56(%rbp), %r9
	leaq	-40(%rbp), %r8
	leaq	-32(%rbp), %rdx
	leaq	-48(%rbp), %rax
	leaq	-112(%rbp), %rcx
	movq	%rcx, 32(%rsp)
	movq	%rax, %rcx
	call	grow_trade_array
	testl	%eax, %eax
	jne	.L87
	leaq	-64(%rbp), %rax
	movq	%rax, %rcx
	call	reader_close
	movq	-48(%rbp), %rax
	movq	%rax, %rcx
	call	free
	leaq	.LC10(%rip), %rcx
	movq	104(%rbp), %rdx
	movq	96(%rbp), %rax
	movq	%rcx, %r8
	movq	%rax, %rcx
	call	snprintf
	movl	$0, %eax
	jmp	.L92
.L87:
	leaq	-112(%rbp), %rdx
	leaq	-64(%rbp), %rax
	movq	%rax, %rcx
	call	reader_next
	movl	%eax, -20(%rbp)
	cmpl	$0, -20(%rbp)
	jg	.L89
	leaq	-64(%rbp), %rax
	movq	%rax, %rcx
	call	reader_close
	movl	%eax, -24(%rbp)
	call	now_ns
	subq	-16(%rbp), %rax
	movq	%rax, %rdx
	movq	88(%rbp), %rax
	movq	%rdx, (%rax)
	cmpl	$0, -20(%rbp)
	js	.L90
	cmpl	$0, -24(%rbp)
	jne	.L91
.L90:
	movq	-48(%rbp), %rax
	movq	%rax, %rcx
	call	free
	leaq	.LC11(%rip), %rcx
	movq	104(%rbp), %rdx
	movq	96(%rbp), %rax
	movq	%rcx, %r8
	movq	%rax, %rcx
	call	snprintf
	movl	$0, %eax
	jmp	.L92
.L91:
	movq	-32(%rbp), %rdx
	movq	48(%rbp), %rax
	movq	%rdx, (%rax)
	movq	-48(%rbp), %rdx
	movq	40(%rbp), %rax
	movq	%rdx, (%rax)
	call	now_ns
	subq	-8(%rbp), %rax
	movq	%rax, %rdx
	movq	72(%rbp), %rax
	movq	%rdx, (%rax)
	movl	$1, %eax
.L92:
	addq	$160, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.def	replay_trade_at;	.scl	3;	.type	32;	.endef
	.seh_proc	replay_trade_at
replay_trade_at:
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rbx
	.seh_pushreg	%rbx
	leaq	(%rsp), %rbp
	.seh_setframe	%rbp, 0
	.seh_endprologue
	movq	%rcx, 24(%rbp)
	movq	%rdx, 32(%rbp)
	movq	%r8, 40(%rbp)
	movq	32(%rbp), %rdx
	movq	%rdx, %rax
	addq	%rax, %rax
	addq	%rdx, %rax
	salq	$4, %rax
	movq	%rax, %rdx
	movq	40(%rbp), %rax
	addq	%rax, %rdx
	movq	24(%rbp), %rax
	movq	(%rdx), %rcx
	movq	8(%rdx), %rbx
	movq	%rcx, (%rax)
	movq	%rbx, 8(%rax)
	movq	16(%rdx), %rcx
	movq	24(%rdx), %rbx
	movq	%rcx, 16(%rax)
	movq	%rbx, 24(%rax)
	movq	32(%rdx), %rcx
	movq	40(%rdx), %rbx
	movq	%rcx, 32(%rax)
	movq	%rbx, 40(%rax)
	movq	24(%rbp), %rax
	popq	%rbx
	popq	%rbp
	ret
	.seh_endproc
	.def	crossed;	.scl	3;	.type	32;	.endef
	.seh_proc	crossed
crossed:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	.seh_endprologue
	movl	%ecx, 16(%rbp)
	movl	%edx, 24(%rbp)
	movl	%r8d, 32(%rbp)
	cmpl	$0, 32(%rbp)
	jne	.L96
	movl	$0, %eax
	jmp	.L97
.L96:
	cmpl	$0, 16(%rbp)
	jg	.L98
	cmpl	$0, 24(%rbp)
	jle	.L98
	movl	$1, %eax
	jmp	.L97
.L98:
	cmpl	$0, 16(%rbp)
	js	.L99
	cmpl	$0, 24(%rbp)
	jns	.L99
	movl	$-1, %eax
	jmp	.L97
.L99:
	movl	$0, %eax
.L97:
	popq	%rbp
	ret
	.seh_endproc
	.section .rdata,"dr"
.LC12:
	.ascii "Invalid %s: %s\12\0"
	.text
	.def	parse_unsigned_option;	.scl	3;	.type	32;	.endef
	.seh_proc	parse_unsigned_option
parse_unsigned_option:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$48, %rsp
	.seh_stackalloc	48
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	movq	%r8, 32(%rbp)
	movq	%r9, 40(%rbp)
	movq	$0, -16(%rbp)
	movq	__imp__errno(%rip), %rax
	call	*%rax
	movl	$0, (%rax)
	leaq	-16(%rbp), %rdx
	movq	24(%rbp), %rax
	movl	$10, %r8d
	movq	%rax, %rcx
	call	strtoull
	movq	%rax, -8(%rbp)
	movq	__imp__errno(%rip), %rax
	call	*%rax
	movl	(%rax), %eax
	testl	%eax, %eax
	jne	.L101
	movq	-16(%rbp), %rax
	cmpq	%rax, 24(%rbp)
	je	.L101
	movq	-16(%rbp), %rax
	movzbl	(%rax), %eax
	testb	%al, %al
	jne	.L101
	movq	-8(%rbp), %rax
	cmpq	%rax, 32(%rbp)
	jnb	.L102
.L101:
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rcx
	movq	24(%rbp), %r8
	movq	16(%rbp), %rdx
	leaq	.LC12(%rip), %rax
	movq	%r8, %r9
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	fprintf
	movl	$0, %eax
	jmp	.L104
.L102:
	movq	40(%rbp), %rax
	movq	-8(%rbp), %rdx
	movq	%rdx, (%rax)
	movl	$1, %eax
.L104:
	addq	$48, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.section .rdata,"dr"
.LC13:
	.ascii "--help\0"
.LC14:
	.ascii "-h\0"
.LC15:
	.ascii "Usage: %s <csv> [options]\12\0"
.LC16:
	.ascii "Options:\0"
	.align 8
.LC17:
	.ascii "  --version-id ID              Label the selected replay version\0"
	.align 8
.LC18:
	.ascii "  --speed 1|100|1000           Replay speed (default: 1)\0"
	.align 8
.LC19:
	.ascii "  --short N                    Short SMA window (default: 3)\0"
	.align 8
.LC20:
	.ascii "  --long N                     Long SMA window (default: 5)\0"
	.align 8
.LC21:
	.ascii "  --duration-seconds N         Limit market-time duration from first trade\0"
	.align 8
.LC22:
	.ascii "  --limit N                    Limit processed records\0"
	.align 8
.LC23:
	.ascii "  --capacity N                 Maximum stored trades for array variants\0"
	.align 8
.LC24:
	.ascii "  --output off|console|hardware Signal output mode (default: off)\0"
	.align 8
.LC25:
	.ascii "  --signal-policy primary|legacy-hybrid-shadow\0"
	.align 8
.LC26:
	.ascii "  --latency-csv PATH           Write per-trade timing samples\0"
	.align 8
.LC27:
	.ascii "  --decision-csv PATH          Write per-trade strategy decisions\0"
	.align 8
.LC28:
	.ascii "  --start-at-ns N              Shared monotonic replay start epoch (race use)\0"
	.align 8
.LC29:
	.ascii "  --race-progress              Emit post-decision telemetry for the race launcher\0"
.LC30:
	.ascii "Usage: %s <csv> [--help]\12\0"
.LC31:
	.ascii "replay-unknown\0"
.LC32:
	.ascii "--speed\0"
.LC33:
	.ascii "speed\0"
.LC34:
	.ascii "--version-id\0"
.LC35:
	.ascii "--short\0"
.LC36:
	.ascii "short window\0"
.LC37:
	.ascii "--long\0"
.LC38:
	.ascii "long window\0"
.LC39:
	.ascii "--duration-seconds\0"
.LC40:
	.ascii "duration\0"
.LC41:
	.ascii "--limit\0"
.LC42:
	.ascii "record limit\0"
.LC43:
	.ascii "--capacity\0"
.LC44:
	.ascii "capacity\0"
.LC45:
	.ascii "--output\0"
.LC46:
	.ascii "off\0"
.LC47:
	.ascii "console\0"
.LC48:
	.ascii "hardware\0"
.LC49:
	.ascii "Invalid output mode: %s\12\0"
.LC50:
	.ascii "--signal-policy\0"
.LC51:
	.ascii "primary\0"
.LC52:
	.ascii "legacy-hybrid-shadow\0"
.LC53:
	.ascii "Invalid signal policy: %s\12\0"
.LC54:
	.ascii "--latency-csv\0"
.LC55:
	.ascii "--decision-csv\0"
.LC56:
	.ascii "--start-at-ns\0"
.LC57:
	.ascii "start epoch\0"
.LC58:
	.ascii "--race-progress\0"
.LC59:
	.ascii "Unknown replay option: %s\12\0"
	.align 8
.LC60:
	.ascii "short window must not exceed long window.\12\0"
	.align 8
.LC61:
	.ascii "Hardware LED mode unavailable on Windows.\12\0"
	.align 8
.LC62:
	.ascii "Replay ring allocation failed.\12\0"
.LC63:
	.ascii "Replay preload failed: %s\12\0"
.LC64:
	.ascii "w\0"
	.align 8
.LC65:
	.ascii "trade,timestamp_us,primary_short_sma,primary_long_sma,primary_decision,shadow_short_sma,shadow_long_sma,shadow_decision,effective_decision,policy\12\0"
	.align 8
.LC66:
	.ascii "decreasing timestamp at trade %zu\0"
.LC67:
	.ascii "release time overflow\0"
	.align 8
.LC68:
	.ascii "invalid trade values at trade %zu\0"
	.align 8
.LC69:
	.ascii "latency sample allocation/overflow failed\0"
	.align 8
.LC70:
	.ascii "RACE|%llu|%lld|%.12f|%d|%llu|%llu\12\0"
.LC71:
	.ascii "race progress output failed\0"
	.align 8
.LC72:
	.ascii "%llu,%lld,%.12f,%.12f,%d,%.12f,%.12f,%d,%d,%s\12\0"
.LC73:
	.ascii "decision log write failed\0"
.LC74:
	.ascii "decision log close failed\0"
	.align 8
.LC75:
	.ascii "latency sample file open failed\0"
	.align 8
.LC76:
	.ascii "trade,scheduled_release_ns,processing_start_ns,decision_ns,processing_duration_ns,release_to_decision_latency_ns\12\0"
.LC77:
	.ascii "%zu,%llu,%llu,%llu,%llu,%llu\12\0"
.LC78:
	.ascii "latency sample write failed\0"
.LC79:
	.ascii "latency sample close failed\0"
	.align 8
.LC80:
	.ascii "Replay failed after %llu trades: %s\12\0"
.LC81:
	.ascii "replay_version=%s\12\0"
.LC82:
	.ascii "replay_policy=%s\12\0"
.LC83:
	.ascii "output_mode=%s\12\0"
.LC84:
	.ascii "replay_speed=%ux\12\0"
.LC85:
	.ascii "preload_ns=%llu\12\0"
.LC86:
	.ascii "counting_ns=%llu\12\0"
.LC87:
	.ascii "loading_ns=%llu\12\0"
.LC88:
	.ascii "processed_count=%llu\12\0"
.LC89:
	.ascii "elapsed_ns=%llu\12\0"
	.align 8
.LC90:
	.ascii "lag_definition=max(decision_ns-scheduled_release_ns,0)\0"
.LC91:
	.ascii "max_lag_ns=%llu\12\0"
	.align 8
.LC92:
	.ascii "primary_buy=%llu\12primary_sell=%llu\12\0"
	.align 8
.LC93:
	.ascii "shadow_buy=%llu\12shadow_sell=%llu\12\0"
	.align 8
.LC94:
	.ascii "effective_buy=%llu\12effective_sell=%llu\12\0"
	.align 8
.LC95:
	.ascii "primary_shadow_disagreements=%llu\12\0"
.LC96:
	.ascii "latency_sample_count=%zu\12\0"
.LC97:
	.ascii "latency_sample_bytes=%zu\12\0"
	.align 8
.LC98:
	.ascii "percentile_convention=nearest-rank ceil(p*N), 1-based rank\0"
	.align 8
.LC99:
	.ascii "processing_average_ns=%llu\12processing_worst_ns=%llu\12\0"
	.align 8
.LC100:
	.ascii "release_latency_average_ns=%llu\12release_latency_worst_ns=%llu\12\0"
	.align 8
.LC101:
	.ascii "processing_p50_ns=%llu\12processing_p99_ns=%llu\12\0"
	.align 8
.LC102:
	.ascii "release_latency_p50_ns=%llu\12release_latency_p99_ns=%llu\12\0"
.LC103:
	.ascii "stop_reason=interrupted\0"
.LC104:
	.ascii "stop_reason=record_limit\0"
.LC105:
	.ascii "stop_reason=duration_limit\0"
.LC106:
	.ascii "stop_reason=end_of_file\0"
	.text
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
	.seh_proc	main
main:
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$1256, %rsp
	.seh_stackalloc	1256
	leaq	128(%rsp), %rbp
	.seh_setframe	%rbp, 128
	.seh_endprologue
	movl	%ecx, 1152(%rbp)
	movq	%rdx, 1160(%rbp)
	call	__main
	cmpl	$2, 1152(%rbp)
	jne	.L106
	movq	1160(%rbp), %rax
	addq	$8, %rax
	movq	(%rax), %rax
	leaq	.LC13(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	je	.L107
	movq	1160(%rbp), %rax
	addq	$8, %rax
	movq	(%rax), %rax
	leaq	.LC14(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L106
.L107:
	movq	1160(%rbp), %rax
	movq	(%rax), %rax
	leaq	.LC15(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	leaq	.LC16(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC17(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC18(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC19(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC20(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC21(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC22(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC23(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC24(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC25(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC26(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC27(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC28(%rip), %rax
	movq	%rax, %rcx
	call	puts
	leaq	.LC29(%rip), %rax
	movq	%rax, %rcx
	call	puts
	movl	$0, %eax
	jmp	.L219
.L106:
	cmpl	$1, 1152(%rbp)
	jg	.L109
	movq	1160(%rbp), %rax
	movq	(%rax), %rbx
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rcx
	leaq	.LC30(%rip), %rax
	movq	%rbx, %r8
	movq	%rax, %rdx
	call	fprintf
	movl	$2, %eax
	jmp	.L219
.L109:
	movq	1160(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, 808(%rbp)
	leaq	.LC31(%rip), %rax
	movq	%rax, 1112(%rbp)
	movq	$0, 1104(%rbp)
	movq	$0, 1096(%rbp)
	movl	$1, 1092(%rbp)
	movl	$3, 1088(%rbp)
	movl	$5, 1084(%rbp)
	movq	$0, 616(%rbp)
	movl	$0, 1080(%rbp)
	movq	$0, 608(%rbp)
	movq	$0, 600(%rbp)
	movl	$0, 1076(%rbp)
	movq	$12000000, 1064(%rbp)
	movl	$0, 1060(%rbp)
	movl	$0, 1056(%rbp)
	movl	$2, 1052(%rbp)
	jmp	.L110
.L142:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC32(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L111
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L111
	movq	$0, 104(%rbp)
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	104(%rbp), %rdx
	leaq	.LC33(%rip), %rcx
	movq	%rdx, %r9
	movl	$1000, %r8d
	movq	%rax, %rdx
	call	parse_unsigned_option
	testl	%eax, %eax
	je	.L112
	movq	104(%rbp), %rax
	cmpq	$1, %rax
	je	.L113
	movq	104(%rbp), %rax
	cmpq	$100, %rax
	je	.L113
	movq	104(%rbp), %rax
	cmpq	$1000, %rax
	je	.L113
.L112:
	movl	$2, %eax
	jmp	.L219
.L113:
	movq	104(%rbp), %rax
	movl	%eax, 1092(%rbp)
	jmp	.L114
.L111:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC34(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L115
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L115
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	movq	%rax, 1112(%rbp)
	jmp	.L114
.L115:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC35(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L116
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L116
	movq	$0, 96(%rbp)
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	96(%rbp), %rdx
	leaq	.LC36(%rip), %rcx
	movq	%rdx, %r9
	movl	$100000, %r8d
	movq	%rax, %rdx
	call	parse_unsigned_option
	testl	%eax, %eax
	je	.L117
	movq	96(%rbp), %rax
	testq	%rax, %rax
	jne	.L118
.L117:
	movl	$2, %eax
	jmp	.L219
.L118:
	movq	96(%rbp), %rax
	movl	%eax, 1088(%rbp)
	jmp	.L114
.L116:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC37(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L119
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L119
	movq	$0, 88(%rbp)
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	88(%rbp), %rdx
	leaq	.LC38(%rip), %rcx
	movq	%rdx, %r9
	movl	$100000, %r8d
	movq	%rax, %rdx
	call	parse_unsigned_option
	testl	%eax, %eax
	je	.L120
	movq	88(%rbp), %rax
	testq	%rax, %rax
	jne	.L121
.L120:
	movl	$2, %eax
	jmp	.L219
.L121:
	movq	88(%rbp), %rax
	movl	%eax, 1084(%rbp)
	jmp	.L114
.L119:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC39(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L122
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L122
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	616(%rbp), %rdx
	movabsq	$18446744073709, %r8
	leaq	.LC40(%rip), %rcx
	movq	%rdx, %r9
	movq	%rax, %rdx
	call	parse_unsigned_option
	testl	%eax, %eax
	jne	.L123
	movl	$2, %eax
	jmp	.L219
.L123:
	movl	$1, 1080(%rbp)
	jmp	.L114
.L122:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC41(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L124
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L124
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	608(%rbp), %rdx
	leaq	.LC42(%rip), %rcx
	movq	%rdx, %r9
	movq	$-1, %r8
	movq	%rax, %rdx
	call	parse_unsigned_option
	testl	%eax, %eax
	jne	.L114
	movl	$2, %eax
	jmp	.L219
.L124:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC43(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L126
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L126
	movq	$0, 80(%rbp)
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	80(%rbp), %rdx
	leaq	.LC44(%rip), %rcx
	movq	%rdx, %r9
	movq	$-1, %r8
	movq	%rax, %rdx
	call	parse_unsigned_option
	testl	%eax, %eax
	jne	.L127
	movl	$2, %eax
	jmp	.L219
.L127:
	movq	80(%rbp), %rax
	movq	%rax, 1064(%rbp)
	jmp	.L114
.L126:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC45(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L128
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L128
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	movq	%rax, 632(%rbp)
	leaq	.LC46(%rip), %rdx
	movq	632(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L129
	movl	$0, 1060(%rbp)
	jmp	.L114
.L129:
	leaq	.LC47(%rip), %rdx
	movq	632(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L131
	movl	$1, 1060(%rbp)
	jmp	.L114
.L131:
	leaq	.LC48(%rip), %rdx
	movq	632(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L132
	movl	$2, 1060(%rbp)
	jmp	.L114
.L132:
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rcx
	movq	632(%rbp), %rdx
	leaq	.LC49(%rip), %rax
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	fprintf
	movl	$2, %eax
	jmp	.L219
.L128:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC50(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L133
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L133
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	movq	%rax, 624(%rbp)
	leaq	.LC51(%rip), %rdx
	movq	624(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L134
	movl	$0, 1056(%rbp)
	jmp	.L114
.L134:
	leaq	.LC52(%rip), %rdx
	movq	624(%rbp), %rax
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L136
	movl	$1, 1056(%rbp)
	jmp	.L114
.L136:
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rcx
	movq	624(%rbp), %rdx
	leaq	.LC53(%rip), %rax
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	fprintf
	movl	$2, %eax
	jmp	.L219
.L133:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC54(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L137
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L137
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	movq	%rax, 1104(%rbp)
	jmp	.L114
.L137:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC55(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L138
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L138
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	movq	%rax, 1096(%rbp)
	jmp	.L114
.L138:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC56(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L139
	movl	1052(%rbp), %eax
	addl	$1, %eax
	cmpl	%eax, 1152(%rbp)
	jle	.L139
	addl	$1, 1052(%rbp)
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	600(%rbp), %rdx
	leaq	.LC57(%rip), %rcx
	movq	%rdx, %r9
	movq	$-1, %r8
	movq	%rax, %rdx
	call	parse_unsigned_option
	testl	%eax, %eax
	jne	.L114
	movl	$2, %eax
	jmp	.L219
.L139:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rax
	leaq	.LC58(%rip), %rdx
	movq	%rax, %rcx
	call	strcmp
	testl	%eax, %eax
	jne	.L141
	movl	$1, 1076(%rbp)
	jmp	.L114
.L141:
	movl	1052(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	1160(%rbp), %rax
	addq	%rdx, %rax
	movq	(%rax), %rbx
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rcx
	leaq	.LC59(%rip), %rax
	movq	%rbx, %r8
	movq	%rax, %rdx
	call	fprintf
	movl	$2, %eax
	jmp	.L219
.L114:
	addl	$1, 1052(%rbp)
.L110:
	movl	1052(%rbp), %eax
	cmpl	1152(%rbp), %eax
	jl	.L142
	movl	1088(%rbp), %eax
	cmpl	1084(%rbp), %eax
	jle	.L143
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rdx
	leaq	.LC60(%rip), %rax
	movq	%rdx, %r9
	movl	$42, %r8d
	movl	$1, %edx
	movq	%rax, %rcx
	call	fwrite
	movl	$2, %eax
	jmp	.L219
.L143:
	cmpl	$2, 1060(%rbp)
	jne	.L144
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rdx
	leaq	.LC61(%rip), %rax
	movq	%rdx, %r9
	movl	$42, %r8d
	movl	$1, %edx
	movq	%rax, %rcx
	call	fwrite
	movl	$4, %eax
	jmp	.L219
.L144:
	movq	$0, 588(%rbp)
	movl	$0, 596(%rbp)
	pxor	%xmm0, %xmm0
	movups	%xmm0, 528(%rbp)
	movups	%xmm0, 544(%rbp)
	movups	%xmm0, 560(%rbp)
	movq	%xmm0, 576(%rbp)
	movq	$0, 520(%rbp)
	movq	$0, 496(%rbp)
	movq	$0, 504(%rbp)
	pxor	%xmm0, %xmm0
	movups	%xmm0, 448(%rbp)
	movups	%xmm0, 464(%rbp)
	movups	%xmm0, 480(%rbp)
	movl	$0, 444(%rbp)
	movq	$0, 432(%rbp)
	movq	$0, 424(%rbp)
	movq	$0, 416(%rbp)
	movq	$0, 408(%rbp)
	movq	$0, 144(%rbp)
	movq	$0, 152(%rbp)
	movq	$0, 160(%rbp)
	movq	$0, 168(%rbp)
	movq	$0, 176(%rbp)
	movq	$0, 184(%rbp)
	movq	$0, 192(%rbp)
	movq	$0, 200(%rbp)
	movq	$0, 208(%rbp)
	movq	$0, 216(%rbp)
	movq	$0, 224(%rbp)
	movq	$0, 232(%rbp)
	movq	$0, 240(%rbp)
	movq	$0, 248(%rbp)
	movq	$0, 256(%rbp)
	movq	$0, 264(%rbp)
	movq	$0, 272(%rbp)
	movq	$0, 280(%rbp)
	movq	$0, 288(%rbp)
	movq	$0, 296(%rbp)
	movq	$0, 304(%rbp)
	movq	$0, 312(%rbp)
	movq	$0, 320(%rbp)
	movq	$0, 328(%rbp)
	movq	$0, 336(%rbp)
	movq	$0, 344(%rbp)
	movq	$0, 352(%rbp)
	movq	$0, 360(%rbp)
	movq	$0, 368(%rbp)
	movq	$0, 376(%rbp)
	movq	$0, 384(%rbp)
	movq	$0, 392(%rbp)
	movl	1084(%rbp), %eax
	cltq
	movl	$8, %edx
	movq	%rax, %rcx
	call	calloc
	movq	%rax, 800(%rbp)
	movq	$0, 792(%rbp)
	cmpq	$0, 800(%rbp)
	jne	.L145
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rdx
	leaq	.LC62(%rip), %rax
	movq	%rdx, %r9
	movl	$31, %r8d
	movl	$1, %edx
	movq	%rax, %rcx
	call	fwrite
	movq	800(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	792(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movl	$3, %eax
	jmp	.L219
.L145:
	leaq	520(%rbp), %r9
	leaq	444(%rbp), %r8
	leaq	448(%rbp), %rdx
	movq	808(%rbp), %rax
	movq	$256, 88(%rsp)
	leaq	144(%rbp), %rcx
	movq	%rcx, 80(%rsp)
	leaq	408(%rbp), %rcx
	movq	%rcx, 72(%rsp)
	leaq	416(%rbp), %rcx
	movq	%rcx, 64(%rsp)
	leaq	424(%rbp), %rcx
	movq	%rcx, 56(%rsp)
	leaq	496(%rbp), %rcx
	movq	%rcx, 48(%rsp)
	movq	1064(%rbp), %rcx
	movq	%rcx, 40(%rsp)
	leaq	432(%rbp), %rcx
	movq	%rcx, 32(%rsp)
	movq	%rax, %rcx
	call	load_replay_storage
	testl	%eax, %eax
	jne	.L146
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rcx
	leaq	144(%rbp), %rdx
	leaq	.LC63(%rip), %rax
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	fprintf
	movq	800(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	792(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	520(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movl	$4, %eax
	jmp	.L219
.L146:
	cmpl	$2, 1060(%rbp)
	jne	.L147
	movl	1060(%rbp), %edx
	leaq	588(%rbp), %rax
	movq	%rax, %rcx
	call	replay_output_init
	testl	%eax, %eax
	jne	.L147
	movq	800(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	792(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	520(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movl	$4, %eax
	jmp	.L219
.L147:
	cmpl	$2, 1060(%rbp)
	je	.L148
	movl	1060(%rbp), %edx
	leaq	588(%rbp), %rax
	movq	%rax, %rcx
	call	replay_output_init
	testl	%eax, %eax
	jne	.L148
	movq	800(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	792(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	520(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movl	$4, %eax
	jmp	.L219
.L148:
	movq	$0, 1040(%rbp)
	cmpq	$0, 1096(%rbp)
	je	.L149
	leaq	.LC64(%rip), %rdx
	movq	1096(%rbp), %rax
	movq	%rax, %rcx
	call	fopen
	movq	%rax, 1040(%rbp)
	cmpq	$0, 1040(%rbp)
	jne	.L150
	movq	1096(%rbp), %rax
	movq	%rax, %rcx
	call	perror
	leaq	588(%rbp), %rax
	movq	%rax, %rcx
	call	replay_output_cleanup
	movq	800(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	792(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	520(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movl	$4, %eax
	jmp	.L219
.L150:
	movq	1040(%rbp), %rdx
	leaq	.LC65(%rip), %rax
	movq	%rdx, %r9
	movl	$146, %r8d
	movl	$1, %edx
	movq	%rax, %rcx
	call	fwrite
.L149:
	movl	$0, replay_stop_requested(%rip)
	leaq	replay_signal_handler(%rip), %rax
	movq	%rax, %rdx
	movl	$2, %ecx
	call	signal
	movq	%rax, 784(%rbp)
	leaq	replay_signal_handler(%rip), %rax
	movq	%rax, %rdx
	movl	$15, %ecx
	call	signal
	movq	%rax, 776(%rbp)
	call	now_ns
	movq	%rax, 1032(%rbp)
	movq	$0, 1024(%rbp)
	movl	$0, 1020(%rbp)
	movl	$0, 1016(%rbp)
	movq	$0, 1008(%rbp)
	movq	616(%rbp), %rax
	imulq	$1000000, %rax, %rax
	movq	%rax, 768(%rbp)
	movq	$0, 1000(%rbp)
	movq	$0, 992(%rbp)
	movq	$0, 984(%rbp)
	movq	$0, 976(%rbp)
	movq	$0, 968(%rbp)
	movq	$0, 960(%rbp)
	movq	$0, 952(%rbp)
	movq	$0, 944(%rbp)
	movl	$0, 940(%rbp)
	movl	$0, 936(%rbp)
	movl	$0, 932(%rbp)
	movl	$0, 928(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, 920(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, 912(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, 904(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, 896(%rbp)
	movl	$1, 892(%rbp)
	movq	$0, 880(%rbp)
.L193:
	pxor	%xmm0, %xmm0
	movups	%xmm0, -32(%rbp)
	movups	%xmm0, -16(%rbp)
	movups	%xmm0, 0(%rbp)
	pxor	%xmm0, %xmm0
	movups	%xmm0, 16(%rbp)
	movups	%xmm0, 32(%rbp)
	movups	%xmm0, 48(%rbp)
	movq	432(%rbp), %rax
	cmpq	%rax, 880(%rbp)
	jb	.L151
	movl	$0, 892(%rbp)
	jmp	.L152
.L151:
	movq	520(%rbp), %rcx
	leaq	16(%rbp), %rax
	movq	880(%rbp), %rdx
	movq	%rcx, %r8
	movq	%rax, %rcx
	call	replay_trade_at
	movq	16(%rbp), %rax
	movq	%rax, -32(%rbp)
	movq	48(%rbp), %rax
	movq	%rax, -24(%rbp)
	movsd	24(%rbp), %xmm0
	movsd	%xmm0, -16(%rbp)
	movsd	32(%rbp), %xmm0
	movsd	%xmm0, 0(%rbp)
	movl	56(%rbp), %eax
	movl	%eax, 8(%rbp)
	movl	replay_stop_requested(%rip), %eax
	testl	%eax, %eax
	je	.L153
	movl	$-2, 892(%rbp)
	jmp	.L152
.L153:
	cmpl	$0, 1016(%rbp)
	je	.L154
	movq	-24(%rbp), %rax
	cmpq	%rax, 1008(%rbp)
	jle	.L154
	movl	$-1, 892(%rbp)
	movq	880(%rbp), %rax
	leaq	1(%rax), %rcx
	leaq	.LC66(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rcx, %r9
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
	jmp	.L152
.L154:
	cmpl	$0, 1020(%rbp)
	jne	.L155
	movq	-24(%rbp), %rax
	movq	%rax, 1024(%rbp)
	movl	$1, 1020(%rbp)
	movq	600(%rbp), %rax
	testq	%rax, %rax
	je	.L156
	jmp	.L157
.L159:
	movl	$1, %ecx
	call	sleep_ms
.L157:
	call	now_ns
	movq	%rax, %rdx
	movq	600(%rbp), %rax
	cmpq	%rax, %rdx
	jnb	.L158
	movl	replay_stop_requested(%rip), %eax
	testl	%eax, %eax
	je	.L159
.L158:
	movl	replay_stop_requested(%rip), %eax
	testl	%eax, %eax
	je	.L160
	movl	$-2, 892(%rbp)
	jmp	.L152
.L160:
	movq	600(%rbp), %rax
	movq	%rax, 1032(%rbp)
	jmp	.L155
.L156:
	call	now_ns
	movq	%rax, 1032(%rbp)
.L155:
	movq	608(%rbp), %rax
	testq	%rax, %rax
	je	.L161
	movq	608(%rbp), %rax
	cmpq	%rax, 1000(%rbp)
	jb	.L161
	movl	$2, 892(%rbp)
	jmp	.L152
.L161:
	movq	-24(%rbp), %rax
	subq	1024(%rbp), %rax
	movq	%rax, 760(%rbp)
	cmpl	$0, 1080(%rbp)
	je	.L162
	movq	760(%rbp), %rax
	cmpq	%rax, 768(%rbp)
	jnb	.L162
	movl	$3, 892(%rbp)
	jmp	.L152
.L162:
	movq	$0, 72(%rbp)
	movq	-24(%rbp), %r8
	movl	1092(%rbp), %r9d
	movq	1024(%rbp), %rdx
	movq	1032(%rbp), %rax
	leaq	72(%rbp), %rcx
	movq	%rcx, 32(%rsp)
	movq	%rax, %rcx
	call	replay_schedule_ns
	testl	%eax, %eax
	jne	.L163
	movl	$-1, 892(%rbp)
	leaq	.LC67(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
	jmp	.L152
.L163:
	movq	72(%rbp), %rax
	leaq	real_wait(%rip), %r8
	leaq	real_now(%rip), %rdx
	leaq	replay_stop_requested(%rip), %rcx
	movq	%rcx, 32(%rsp)
	movl	$0, %r9d
	movq	%rax, %rcx
	call	replay_wait_until
	testl	%eax, %eax
	jne	.L164
	movl	$-2, 892(%rbp)
	jmp	.L152
.L164:
	call	now_ns
	movq	%rax, 752(%rbp)
	movq	-16(%rbp), %rax
	movq	%rax, %xmm0
	call	__fpclassify
	andl	$256, %eax
	testl	%eax, %eax
	jne	.L165
	movq	0(%rbp), %rax
	movq	%rax, %xmm0
	call	__fpclassify
	andl	$256, %eax
	testl	%eax, %eax
	jne	.L165
	movsd	-16(%rbp), %xmm1
	pxor	%xmm0, %xmm0
	comisd	%xmm1, %xmm0
	jnb	.L165
	movsd	0(%rbp), %xmm1
	pxor	%xmm0, %xmm0
	comisd	%xmm1, %xmm0
	jb	.L220
.L165:
	movl	$-1, 892(%rbp)
	movq	880(%rbp), %rax
	leaq	1(%rax), %rcx
	leaq	.LC68(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rcx, %r9
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
	jmp	.L152
.L220:
	movl	1084(%rbp), %eax
	movslq	%eax, %rcx
	movq	1000(%rbp), %rax
	movl	$0, %edx
	divq	%rcx
	movq	%rdx, 744(%rbp)
	movl	1088(%rbp), %eax
	cltq
	cmpq	%rax, 1000(%rbp)
	jb	.L168
	movl	1088(%rbp), %eax
	cltq
	movq	1000(%rbp), %rdx
	movq	%rdx, %rcx
	subq	%rax, %rcx
	movl	1084(%rbp), %eax
	movslq	%eax, %rbx
	movq	%rcx, %rax
	movl	$0, %edx
	divq	%rbx
	movq	%rdx, %rcx
	movq	%rcx, %rax
	leaq	0(,%rax,8), %rdx
	movq	800(%rbp), %rax
	addq	%rdx, %rax
	movsd	(%rax), %xmm0
	movsd	%xmm0, 736(%rbp)
	movsd	920(%rbp), %xmm0
	subsd	736(%rbp), %xmm0
	movsd	%xmm0, 920(%rbp)
	movsd	904(%rbp), %xmm0
	subsd	736(%rbp), %xmm0
	movsd	%xmm0, 904(%rbp)
.L168:
	movl	1084(%rbp), %eax
	cltq
	cmpq	%rax, 1000(%rbp)
	jb	.L169
	movq	744(%rbp), %rax
	leaq	0(,%rax,8), %rdx
	movq	800(%rbp), %rax
	addq	%rdx, %rax
	movsd	(%rax), %xmm0
	movsd	%xmm0, 728(%rbp)
	movsd	912(%rbp), %xmm0
	subsd	728(%rbp), %xmm0
	movsd	%xmm0, 912(%rbp)
	movsd	896(%rbp), %xmm0
	subsd	728(%rbp), %xmm0
	movsd	%xmm0, 896(%rbp)
.L169:
	movq	744(%rbp), %rax
	leaq	0(,%rax,8), %rdx
	movq	800(%rbp), %rax
	addq	%rdx, %rax
	movsd	-16(%rbp), %xmm0
	movsd	%xmm0, (%rax)
	movsd	-16(%rbp), %xmm0
	movsd	904(%rbp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 904(%rbp)
	movsd	-16(%rbp), %xmm0
	movsd	896(%rbp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 896(%rbp)
	movsd	-16(%rbp), %xmm0
	movsd	920(%rbp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 920(%rbp)
	movsd	-16(%rbp), %xmm0
	movsd	912(%rbp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 912(%rbp)
	movq	1000(%rbp), %rax
	leaq	1(%rax), %rdx
	movl	1088(%rbp), %eax
	cltq
	cmpq	%rax, %rdx
	setnb	%al
	movzbl	%al, %eax
	movl	%eax, 724(%rbp)
	movq	1000(%rbp), %rax
	leaq	1(%rax), %rdx
	movl	1084(%rbp), %eax
	cltq
	cmpq	%rax, %rdx
	setnb	%al
	movzbl	%al, %eax
	movl	%eax, 720(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, 872(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, 864(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, 856(%rbp)
	pxor	%xmm0, %xmm0
	movsd	%xmm0, 848(%rbp)
	cmpl	$0, 724(%rbp)
	je	.L170
	pxor	%xmm1, %xmm1
	cvtsi2sdl	1088(%rbp), %xmm1
	movsd	920(%rbp), %xmm0
	divsd	%xmm1, %xmm0
	movsd	%xmm0, 872(%rbp)
.L170:
	cmpl	$0, 720(%rbp)
	je	.L171
	pxor	%xmm1, %xmm1
	cvtsi2sdl	1084(%rbp), %xmm1
	movsd	912(%rbp), %xmm0
	divsd	%xmm1, %xmm0
	movsd	%xmm0, 864(%rbp)
.L171:
	cmpl	$0, 724(%rbp)
	je	.L172
	pxor	%xmm1, %xmm1
	cvtsi2sdl	1088(%rbp), %xmm1
	movsd	904(%rbp), %xmm0
	divsd	%xmm1, %xmm0
	movsd	%xmm0, 856(%rbp)
.L172:
	cmpl	$0, 720(%rbp)
	je	.L173
	pxor	%xmm1, %xmm1
	cvtsi2sdl	1084(%rbp), %xmm1
	movsd	896(%rbp), %xmm0
	divsd	%xmm1, %xmm0
	movsd	%xmm0, 848(%rbp)
.L173:
	movl	$0, 844(%rbp)
	movl	$0, 840(%rbp)
	cmpl	$0, 724(%rbp)
	je	.L174
	cmpl	$0, 720(%rbp)
	je	.L174
	movsd	872(%rbp), %xmm0
	subsd	864(%rbp), %xmm0
	movsd	%xmm0, 712(%rbp)
	movsd	712(%rbp), %xmm0
	pxor	%xmm1, %xmm1
	comisd	%xmm1, %xmm0
	seta	%al
	movzbl	%al, %edx
	pxor	%xmm0, %xmm0
	comisd	712(%rbp), %xmm0
	seta	%al
	movzbl	%al, %eax
	subl	%eax, %edx
	movl	%edx, 844(%rbp)
	movsd	856(%rbp), %xmm0
	subsd	848(%rbp), %xmm0
	movsd	%xmm0, 704(%rbp)
	movsd	704(%rbp), %xmm0
	pxor	%xmm1, %xmm1
	comisd	%xmm1, %xmm0
	seta	%al
	movzbl	%al, %edx
	pxor	%xmm0, %xmm0
	comisd	704(%rbp), %xmm0
	seta	%al
	movzbl	%al, %eax
	subl	%eax, %edx
	movl	%edx, 840(%rbp)
.L174:
	movl	936(%rbp), %ecx
	movl	844(%rbp), %edx
	movl	940(%rbp), %eax
	movl	%ecx, %r8d
	movl	%eax, %ecx
	call	crossed
	movl	%eax, 700(%rbp)
	movl	928(%rbp), %ecx
	movl	840(%rbp), %edx
	movl	932(%rbp), %eax
	movl	%ecx, %r8d
	movl	%eax, %ecx
	call	crossed
	movl	%eax, 696(%rbp)
	cmpl	$0, 724(%rbp)
	je	.L175
	cmpl	$0, 720(%rbp)
	je	.L175
	movl	844(%rbp), %eax
	movl	%eax, 940(%rbp)
	movl	840(%rbp), %eax
	movl	%eax, 932(%rbp)
	movl	$1, 936(%rbp)
	movl	$1, 928(%rbp)
.L175:
	cmpl	$0, 1056(%rbp)
	jne	.L176
	movl	700(%rbp), %eax
	movl	%eax, 836(%rbp)
	jmp	.L177
.L176:
	movl	696(%rbp), %eax
	movl	%eax, 836(%rbp)
.L177:
	movl	700(%rbp), %eax
	cmpl	696(%rbp), %eax
	je	.L178
	addq	$1, 944(%rbp)
.L178:
	cmpl	$0, 700(%rbp)
	jle	.L179
	addq	$1, 992(%rbp)
	jmp	.L180
.L179:
	cmpl	$0, 700(%rbp)
	jns	.L180
	addq	$1, 984(%rbp)
.L180:
	cmpl	$0, 696(%rbp)
	jle	.L181
	addq	$1, 976(%rbp)
	jmp	.L182
.L181:
	cmpl	$0, 696(%rbp)
	jns	.L182
	addq	$1, 968(%rbp)
.L182:
	cmpl	$0, 836(%rbp)
	jle	.L183
	addq	$1, 960(%rbp)
	jmp	.L184
.L183:
	cmpl	$0, 836(%rbp)
	jns	.L184
	addq	$1, 952(%rbp)
.L184:
	call	now_ns
	movq	%rax, 688(%rbp)
	movq	72(%rbp), %rdx
	movq	688(%rbp), %r8
	movq	752(%rbp), %rcx
	leaq	528(%rbp), %rax
	movq	%r8, %r9
	movq	%rcx, %r8
	movq	%rax, %rcx
	call	replay_latency_add
	testl	%eax, %eax
	jne	.L185
	movl	$-1, 892(%rbp)
	leaq	.LC69(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
	jmp	.L152
.L185:
	cmpl	$0, 1076(%rbp)
	je	.L186
	cmpq	$0, 1000(%rbp)
	je	.L187
	movq	1000(%rbp), %rax
	leaq	1(%rax), %rcx
	movq	%rcx, %rax
	shrq	$3, %rax
	movabsq	$2361183241434822607, %rdx
	mulq	%rdx
	movq	%rdx, %rax
	shrq	$4, %rax
	imulq	$1000, %rax, %rdx
	movq	%rcx, %rax
	subq	%rdx, %rax
	testq	%rax, %rax
	je	.L187
	cmpl	$0, 836(%rbp)
	je	.L186
.L187:
	movq	688(%rbp), %rax
	subq	752(%rbp), %rax
	movq	%rax, 680(%rbp)
	movq	72(%rbp), %rax
	movq	688(%rbp), %rdx
	subq	%rax, %rdx
	movq	%rdx, 672(%rbp)
	movsd	-16(%rbp), %xmm0
	movq	-24(%rbp), %r8
	movq	1000(%rbp), %rax
	leaq	1(%rax), %rdx
	movapd	%xmm0, %xmm1
	movapd	%xmm1, %xmm0
	movq	%xmm1, %r9
	leaq	.LC70(%rip), %rax
	movq	672(%rbp), %rcx
	movq	%rcx, 48(%rsp)
	movq	680(%rbp), %rcx
	movq	%rcx, 40(%rsp)
	movl	836(%rbp), %ecx
	movl	%ecx, 32(%rsp)
	movapd	%xmm0, %xmm3
	movq	%rax, %rcx
	call	printf
	testl	%eax, %eax
	js	.L188
	movl	$1, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rcx
	call	fflush
	testl	%eax, %eax
	je	.L186
.L188:
	movl	$-1, 892(%rbp)
	leaq	.LC71(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
	jmp	.L152
.L186:
	cmpq	$0, 1040(%rbp)
	je	.L189
	cmpl	$0, 1056(%rbp)
	jne	.L190
	leaq	.LC51(%rip), %rax
	jmp	.L191
.L190:
	leaq	.LC52(%rip), %rax
.L191:
	movq	-24(%rbp), %r8
	movq	1000(%rbp), %rdx
	leaq	1(%rdx), %r10
	leaq	.LC72(%rip), %rdx
	movq	1040(%rbp), %rcx
	movq	%rax, 88(%rsp)
	movl	836(%rbp), %eax
	movl	%eax, 80(%rsp)
	movl	696(%rbp), %eax
	movl	%eax, 72(%rsp)
	movsd	848(%rbp), %xmm0
	movsd	%xmm0, 64(%rsp)
	movsd	856(%rbp), %xmm0
	movsd	%xmm0, 56(%rsp)
	movl	700(%rbp), %eax
	movl	%eax, 48(%rsp)
	movsd	864(%rbp), %xmm0
	movsd	%xmm0, 40(%rsp)
	movsd	872(%rbp), %xmm0
	movsd	%xmm0, 32(%rsp)
	movq	%r8, %r9
	movq	%r10, %r8
	call	fprintf
	testl	%eax, %eax
	jns	.L189
	movl	$-1, 892(%rbp)
	leaq	.LC73(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
	jmp	.L152
.L189:
	cmpl	$0, 836(%rbp)
	je	.L192
	movq	1000(%rbp), %rax
	leaq	1(%rax), %rcx
	movl	836(%rbp), %edx
	leaq	588(%rbp), %rax
	movq	%rcx, %r8
	movq	%rax, %rcx
	call	replay_output_signal
.L192:
	addq	$1, 1000(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, 1008(%rbp)
	movl	$1, 1016(%rbp)
	addq	$1, 880(%rbp)
	jmp	.L193
.L152:
	cmpq	$0, 1040(%rbp)
	je	.L194
	movq	1040(%rbp), %rax
	movq	%rax, %rcx
	call	fclose
	testl	%eax, %eax
	je	.L194
	cmpl	$0, 892(%rbp)
	js	.L194
	movl	$-1, 892(%rbp)
	leaq	.LC74(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
.L194:
	leaq	588(%rbp), %rax
	movq	%rax, %rcx
	call	replay_output_cleanup
	movq	784(%rbp), %rax
	movq	%rax, %rdx
	movl	$2, %ecx
	call	signal
	movq	776(%rbp), %rax
	movq	%rax, %rdx
	movl	$15, %ecx
	call	signal
	cmpl	$0, 1020(%rbp)
	je	.L195
	call	now_ns
	subq	1032(%rbp), %rax
	movq	%rax, 824(%rbp)
	jmp	.L196
.L195:
	movq	$0, 824(%rbp)
.L196:
	cmpq	$0, 1104(%rbp)
	je	.L197
	leaq	.LC64(%rip), %rdx
	movq	1104(%rbp), %rax
	movq	%rax, %rcx
	call	fopen
	movq	%rax, 664(%rbp)
	cmpq	$0, 664(%rbp)
	jne	.L198
	movq	1104(%rbp), %rax
	movq	%rax, %rcx
	call	perror
	movl	$-1, 892(%rbp)
	leaq	.LC75(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
	jmp	.L197
.L198:
	movq	664(%rbp), %rdx
	leaq	.LC76(%rip), %rax
	movq	%rdx, %r9
	movl	$113, %r8d
	movl	$1, %edx
	movq	%rax, %rcx
	call	fwrite
	movq	$0, 816(%rbp)
	jmp	.L199
.L202:
	movq	528(%rbp), %rcx
	movq	816(%rbp), %rdx
	movq	%rdx, %rax
	addq	%rax, %rax
	addq	%rdx, %rax
	salq	$3, %rax
	addq	%rcx, %rax
	movq	%rax, 656(%rbp)
	movq	656(%rbp), %rax
	movq	(%rax), %rdx
	movq	656(%rbp), %rax
	movq	16(%rax), %rax
	subq	%rax, %rdx
	movq	%rdx, 648(%rbp)
	movq	656(%rbp), %rax
	movq	(%rax), %rdx
	movq	656(%rbp), %rax
	movq	8(%rax), %rax
	subq	%rax, %rdx
	movq	%rdx, 640(%rbp)
	movq	656(%rbp), %rax
	movq	16(%rax), %r9
	movq	656(%rbp), %rax
	movq	8(%rax), %r8
	movq	656(%rbp), %rax
	movq	(%rax), %rcx
	movq	816(%rbp), %rax
	leaq	1(%rax), %r11
	movq	648(%rbp), %r10
	leaq	.LC77(%rip), %rdx
	movq	664(%rbp), %rax
	movq	%r9, 56(%rsp)
	movq	%r8, 48(%rsp)
	movq	%rcx, 40(%rsp)
	movq	640(%rbp), %rcx
	movq	%rcx, 32(%rsp)
	movq	%r10, %r9
	movq	%r11, %r8
	movq	%rax, %rcx
	call	fprintf
	testl	%eax, %eax
	jns	.L200
	movl	$-1, 892(%rbp)
	leaq	.LC78(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
	jmp	.L201
.L200:
	addq	$1, 816(%rbp)
.L199:
	movq	536(%rbp), %rax
	cmpq	%rax, 816(%rbp)
	jb	.L202
.L201:
	movq	664(%rbp), %rax
	movq	%rax, %rcx
	call	fclose
	testl	%eax, %eax
	je	.L197
	cmpl	$0, 892(%rbp)
	js	.L197
	movl	$-1, 892(%rbp)
	leaq	.LC79(%rip), %rdx
	leaq	144(%rbp), %rax
	movq	%rdx, %r8
	movl	$256, %edx
	movq	%rax, %rcx
	call	snprintf
.L197:
	cmpl	$0, 892(%rbp)
	jns	.L203
	movl	$2, %ecx
	movq	__imp___acrt_iob_func(%rip), %rax
	call	*%rax
	movq	%rax, %rcx
	leaq	144(%rbp), %r8
	movq	1000(%rbp), %rdx
	leaq	.LC80(%rip), %rax
	movq	%r8, %r9
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	fprintf
.L203:
	movq	1112(%rbp), %rax
	leaq	.LC81(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	cmpl	$0, 1056(%rbp)
	jne	.L204
	leaq	.LC51(%rip), %rax
	jmp	.L205
.L204:
	leaq	.LC52(%rip), %rax
.L205:
	leaq	.LC82(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	cmpl	$0, 1060(%rbp)
	je	.L206
	cmpl	$1, 1060(%rbp)
	jne	.L207
	leaq	.LC47(%rip), %rax
	jmp	.L208
.L207:
	leaq	.LC48(%rip), %rax
	jmp	.L208
.L206:
	leaq	.LC46(%rip), %rax
.L208:
	leaq	.LC83(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	movl	1092(%rbp), %eax
	leaq	.LC84(%rip), %rcx
	movl	%eax, %edx
	call	printf
	movq	424(%rbp), %rax
	leaq	.LC85(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	movq	416(%rbp), %rax
	leaq	.LC86(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	movq	408(%rbp), %rax
	leaq	.LC87(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	movq	1000(%rbp), %rax
	leaq	.LC88(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	movq	824(%rbp), %rax
	leaq	.LC89(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	leaq	.LC90(%rip), %rax
	movq	%rax, %rcx
	call	puts
	movq	576(%rbp), %rax
	leaq	.LC91(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	movq	984(%rbp), %rdx
	movq	992(%rbp), %rax
	leaq	.LC92(%rip), %rcx
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	printf
	movq	968(%rbp), %rdx
	movq	976(%rbp), %rax
	leaq	.LC93(%rip), %rcx
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	printf
	movq	952(%rbp), %rdx
	movq	960(%rbp), %rax
	leaq	.LC94(%rip), %rcx
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	printf
	movq	944(%rbp), %rax
	leaq	.LC95(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	movq	536(%rbp), %rax
	leaq	.LC96(%rip), %rcx
	movq	%rax, %rdx
	call	printf
	movq	536(%rbp), %rdx
	movq	%rdx, %rax
	addq	%rax, %rax
	addq	%rdx, %rax
	salq	$3, %rax
	movq	%rax, %rdx
	leaq	.LC97(%rip), %rax
	movq	%rax, %rcx
	call	printf
	leaq	.LC98(%rip), %rax
	movq	%rax, %rcx
	call	puts
	movq	568(%rbp), %rbx
	leaq	528(%rbp), %rax
	movq	%rax, %rcx
	call	replay_average_processing_ns
	movq	%rax, %rdx
	leaq	.LC99(%rip), %rax
	movq	%rbx, %r8
	movq	%rax, %rcx
	call	printf
	movq	576(%rbp), %rbx
	leaq	528(%rbp), %rax
	movq	%rax, %rcx
	call	replay_average_release_latency_ns
	movq	%rax, %rdx
	leaq	.LC100(%rip), %rax
	movq	%rbx, %r8
	movq	%rax, %rcx
	call	printf
	movq	$0, 136(%rbp)
	movq	$0, 128(%rbp)
	movq	$0, 120(%rbp)
	movq	$0, 112(%rbp)
	movq	536(%rbp), %rax
	testq	%rax, %rax
	je	.L209
	leaq	136(%rbp), %rdx
	leaq	528(%rbp), %rax
	movq	%rdx, %r9
	movl	$50, %r8d
	movl	$0, %edx
	movq	%rax, %rcx
	call	replay_percentile_ns
	testl	%eax, %eax
	je	.L209
	leaq	128(%rbp), %rdx
	leaq	528(%rbp), %rax
	movq	%rdx, %r9
	movl	$99, %r8d
	movl	$0, %edx
	movq	%rax, %rcx
	call	replay_percentile_ns
	testl	%eax, %eax
	je	.L209
	leaq	120(%rbp), %rdx
	leaq	528(%rbp), %rax
	movq	%rdx, %r9
	movl	$50, %r8d
	movl	$1, %edx
	movq	%rax, %rcx
	call	replay_percentile_ns
	testl	%eax, %eax
	je	.L209
	leaq	112(%rbp), %rdx
	leaq	528(%rbp), %rax
	movq	%rdx, %r9
	movl	$99, %r8d
	movl	$1, %edx
	movq	%rax, %rcx
	call	replay_percentile_ns
	testl	%eax, %eax
	je	.L209
	movq	128(%rbp), %rdx
	movq	136(%rbp), %rax
	leaq	.LC101(%rip), %rcx
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	printf
	movq	112(%rbp), %rdx
	movq	120(%rbp), %rax
	leaq	.LC102(%rip), %rcx
	movq	%rdx, %r8
	movq	%rax, %rdx
	call	printf
.L209:
	cmpl	$-2, 892(%rbp)
	jne	.L210
	leaq	.LC103(%rip), %rax
	movq	%rax, %rcx
	call	puts
	jmp	.L211
.L210:
	cmpl	$2, 892(%rbp)
	jne	.L212
	leaq	.LC104(%rip), %rax
	movq	%rax, %rcx
	call	puts
	jmp	.L211
.L212:
	cmpl	$3, 892(%rbp)
	jne	.L213
	leaq	.LC105(%rip), %rax
	movq	%rax, %rcx
	call	puts
	jmp	.L211
.L213:
	cmpl	$0, 892(%rbp)
	jne	.L211
	leaq	.LC106(%rip), %rax
	movq	%rax, %rcx
	call	puts
.L211:
	leaq	528(%rbp), %rax
	movq	%rax, %rcx
	call	replay_latency_destroy
	movq	800(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	792(%rbp), %rax
	movq	%rax, %rcx
	call	free
	movq	520(%rbp), %rax
	movq	%rax, %rcx
	call	free
	cmpl	$0, 892(%rbp)
	jns	.L214
	cmpl	$-2, 892(%rbp)
	jne	.L215
.L214:
	cmpl	$-2, 892(%rbp)
	jne	.L216
	movl	$130, %eax
	jmp	.L218
.L216:
	movl	$0, %eax
	jmp	.L218
.L215:
	movl	$5, %eax
	jmp	.L221
.L218:
.L221:
	nop
.L219:
	addq	$1256, %rsp
	popq	%rbx
	popq	%rbp
	ret
	.seh_endproc
.lcomm ready.1,4,4
.lcomm freq.0,8,8
	.def	__main;	.scl	2;	.type	32;	.endef
	.ident	"GCC: (Rev3, Built by MSYS2 project) 16.2.0"
	.def	strcmp;	.scl	2;	.type	32;	.endef
	.def	strlen;	.scl	2;	.type	32;	.endef
	.def	strncmp;	.scl	2;	.type	32;	.endef
	.def	strtoll;	.scl	2;	.type	32;	.endef
	.def	strtod;	.scl	2;	.type	32;	.endef
	.def	__fpclassify;	.scl	2;	.type	32;	.endef
	.def	fopen;	.scl	2;	.type	32;	.endef
	.def	perror;	.scl	2;	.type	32;	.endef
	.def	feof;	.scl	2;	.type	32;	.endef
	.def	fgets;	.scl	2;	.type	32;	.endef
	.def	ferror;	.scl	2;	.type	32;	.endef
	.def	fclose;	.scl	2;	.type	32;	.endef
	.def	realloc;	.scl	2;	.type	32;	.endef
	.def	free;	.scl	2;	.type	32;	.endef
	.def	snprintf;	.scl	2;	.type	32;	.endef
	.def	strtoull;	.scl	2;	.type	32;	.endef
	.def	fprintf;	.scl	2;	.type	32;	.endef
	.def	printf;	.scl	2;	.type	32;	.endef
	.def	puts;	.scl	2;	.type	32;	.endef
	.def	fwrite;	.scl	2;	.type	32;	.endef
	.def	calloc;	.scl	2;	.type	32;	.endef
	.def	replay_output_init;	.scl	2;	.type	32;	.endef
	.def	replay_output_cleanup;	.scl	2;	.type	32;	.endef
	.def	signal;	.scl	2;	.type	32;	.endef
	.def	replay_schedule_ns;	.scl	2;	.type	32;	.endef
	.def	replay_wait_until;	.scl	2;	.type	32;	.endef
	.def	replay_latency_add;	.scl	2;	.type	32;	.endef
	.def	fflush;	.scl	2;	.type	32;	.endef
	.def	replay_output_signal;	.scl	2;	.type	32;	.endef
	.def	puts;	.scl	2;	.type	32;	.endef
	.def	replay_average_processing_ns;	.scl	2;	.type	32;	.endef
	.def	replay_average_release_latency_ns;	.scl	2;	.type	32;	.endef
	.def	replay_percentile_ns;	.scl	2;	.type	32;	.endef
	.def	replay_latency_destroy;	.scl	2;	.type	32;	.endef
