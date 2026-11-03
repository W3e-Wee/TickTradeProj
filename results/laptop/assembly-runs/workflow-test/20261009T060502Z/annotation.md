# Automatic assembly annotation

> Generated from the actual assembly text. Categories and explanations are heuristic; they have not been manually reviewed. Check every interpretation against the source, compiler, and target ABI.

- Version: `v1_baseline_strtok_running_double_heap_O2`
- Assembly: `C:\Users\siamu\Desktop\SIT Trimester 1\Mini Project\Grp05\results\laptop\assembly-runs\workflow-test\20261009T060502Z\assembly\v1_baseline_strtok_running_double_heap_O2.s`
- SHA256: `574ec403cf8b861addcbfcf1062fdf0c2d7d0e5f78bc4911adb6a5c8c4eabc19`
- Manifest: `C:\Users\siamu\Desktop\SIT Trimester 1\Mini Project\Grp05\results\laptop\assembly-runs\workflow-test\20261009T060502Z\assembly_manifest.json`
- Compiler: `C:\msys64\ucrt64\bin\gcc.EXE` (gcc.EXE (Rev3, Built by MSYS2 project) 16.2.0)
- Flags: `-std=c11 -Wall -Wextra -Wpedantic -O2 -DNDEBUG -Iinclude`
- Host / architecture: `Windows-11-10.0.26200-SP0` / `AMD64`

## Functions and labels

Functions below are recognized from compiler `.type` declarations (or non-local labels when those declarations are absent). Instruction classification is automatic.

### `parse_bool_value.constprop.0`

Instruction lines recognized: 35.

**Stores** (2 recognized):
- Line 17: `pushq	%rsi` — stores data to memory.
- Line 19: `pushq	%rbx` — stores data to memory.

**Integer / address arithmetic** (3 recognized):
- Line 21: `subq	$40, %rsp` — updates values, addresses, or condition operands.
- Line 46: `addq	$40, %rsp` — updates values, addresses, or condition operands.
- Line 56: `addq	$40, %rsp` — updates values, addresses, or condition operands.

**Review-needed** (17 recognized):
- Line 24: `movq	%rdx, %rbx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 25: `leaq	.LC0(%rip), %rdx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 26: `movq	%rcx, %rsi` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 28: `testl	%eax, %eax` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 30: `leaq	.LC1(%rip), %rdx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 31: `movq	%rsi, %rcx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 33: `testl	%eax, %eax` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 35: `leaq	.LC2(%rip), %rdx` — instruction was not confidently classified by the annotator. **[review-needed]**
- … 9 additional matching instruction line(s) omitted.

**Function call** (4 recognized):
- Line 27: `call	strcmp` — calls another function or indirect call target.
- Line 32: `call	strcmp` — calls another function or indirect call target.
- Line 37: `call	strcmp` — calls another function or indirect call target.
- Line 42: `call	strcmp` — calls another function or indirect call target.

**Branches** (3 recognized):
- Line 29: `je	.L2` — changes control flow, often for a conditional or loop edge.
- Line 34: `je	.L2` — changes control flow, often for a conditional or loop edge.
- Line 39: `je	.L4` — changes control flow, often for a conditional or loop edge.

**Loads** (4 recognized):
- Line 47: `popq	%rbx` — loads data from memory or a register stack slot.
- Line 48: `popq	%rsi` — loads data from memory or a register stack slot.
- Line 57: `popq	%rbx` — loads data from memory or a register stack slot.
- Line 58: `popq	%rsi` — loads data from memory or a register stack slot.

**Return** (2 recognized):
- Line 49: `ret` — returns from the current function.
- Line 59: `ret` — returns from the current function.

### `parse_trade_line`

Instruction lines recognized: 202.

**Stores** (6 recognized):
- Line 72: `pushq	%r15` — stores data to memory.
- Line 74: `pushq	%r14` — stores data to memory.
- Line 76: `pushq	%rbp` — stores data to memory.
- Line 78: `pushq	%rdi` — stores data to memory.
- Line 80: `pushq	%rsi` — stores data to memory.
- Line 82: `pushq	%rbx` — stores data to memory.

**Integer / address arithmetic** (6 recognized):
- Line 84: `subq	$664, %rsp` — updates values, addresses, or condition operands.
- Line 117: `addq	$1, %rdx` — updates values, addresses, or condition operands.
- Line 128: `addq	$1, %rcx` — updates values, addresses, or condition operands.
- Line 156: `addq	$1, %rdi` — updates values, addresses, or condition operands.
- Line 158: `addq	$8, %rsi` — updates values, addresses, or condition operands.
- Line 263: `addq	$664, %rsp` — updates values, addresses, or condition operands.

**Review-needed** (125 recognized):
- Line 86: `movups	%xmm6, 624(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 88: `movups	%xmm7, 640(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 91: `pxor	%xmm0, %xmm0` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 92: `movq	$0, 96(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 93: `movq	%rcx, %rsi` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 94: `movq	%rdx, %rbx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 95: `movups	%xmm0, 48(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 96: `movups	%xmm0, 64(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- … 117 additional matching instruction line(s) omitted.

**Function call** (18 recognized):
- Line 98: `call	strlen` — calls another function or indirect call target.
- Line 104: `call	memcpy` — calls another function or indirect call target.
- Line 139: `call	strncmp` — calls another function or indirect call target.
- Line 146: `call	*%rbp` — calls another function or indirect call target.
- Line 157: `call	*%rbp` — calls another function or indirect call target.
- Line 166: `call	*%rsi` — calls another function or indirect call target.
- Line 172: `call	strtoll` — calls another function or indirect call target.
- Line 174: `call	*%rsi` — calls another function or indirect call target.
- … 10 additional matching instruction line(s) omitted.

**Branches** (40 recognized):
- Line 100: `ja	.L36` — changes control flow, often for a conditional or loop edge.
- Line 107: `je	.L13` — changes control flow, often for a conditional or loop edge.
- Line 113: `je	.L40` — changes control flow, often for a conditional or loop edge.
- Line 115: `je	.L40` — changes control flow, often for a conditional or loop edge.
- Line 119: `jne	.L16` — changes control flow, often for a conditional or loop edge.
- Line 123: `jmp	.L17` — changes control flow, often for a conditional or loop edge.
- Line 133: `ja	.L18` — changes control flow, often for a conditional or loop edge.
- Line 135: `jc	.L19` — changes control flow, often for a conditional or loop edge.
- … 32 additional matching instruction line(s) omitted.

**Loads** (6 recognized):
- Line 264: `popq	%rbx` — loads data from memory or a register stack slot.
- Line 265: `popq	%rsi` — loads data from memory or a register stack slot.
- Line 266: `popq	%rdi` — loads data from memory or a register stack slot.
- Line 267: `popq	%rbp` — loads data from memory or a register stack slot.
- Line 268: `popq	%r14` — loads data from memory or a register stack slot.
- Line 269: `popq	%r15` — loads data from memory or a register stack slot.

**Return** (1 recognized):
- Line 270: `ret` — returns from the current function.

### `parse_trade_file`

Instruction lines recognized: 111.

**Stores** (8 recognized):
- Line 332: `pushq	%r15` — stores data to memory.
- Line 334: `pushq	%r14` — stores data to memory.
- Line 336: `pushq	%r13` — stores data to memory.
- Line 338: `pushq	%r12` — stores data to memory.
- Line 340: `pushq	%rbp` — stores data to memory.
- Line 342: `pushq	%rdi` — stores data to memory.
- Line 344: `pushq	%rsi` — stores data to memory.
- Line 346: `pushq	%rbx` — stores data to memory.

**Integer / address arithmetic** (3 recognized):
- Line 348: `subq	$648, %rsp` — updates values, addresses, or condition operands.
- Line 392: `addq	$1, %rbx` — updates values, addresses, or condition operands.
- Line 435: `addq	$648, %rsp` — updates values, addresses, or condition operands.

**Review-needed** (67 recognized):
- Line 350: `movups	%xmm6, 624(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 353: `movq	%rdx, %r15` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 354: `leaq	.LC8(%rip), %rdx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 355: `movq	%r8, 736(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 356: `movq	%rcx, %r13` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 358: `movq	%rax, %rbp` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 359: `testq	%rax, %rax` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 361: `xorl	%esi, %esi` — instruction was not confidently classified by the annotator. **[review-needed]**
- … 59 additional matching instruction line(s) omitted.

**Function call** (11 recognized):
- Line 357: `call	fopen` — calls another function or indirect call target.
- Line 372: `call	fgets` — calls another function or indirect call target.
- Line 382: `call	parse_trade_line` — calls another function or indirect call target.
- Line 412: `call	realloc` — calls another function or indirect call target.
- Line 428: `call	fclose` — calls another function or indirect call target.
- Line 450: `call	*__imp___acrt_iob_func(%rip)` — calls another function or indirect call target.
- Line 457: `call	fprintf` — calls another function or indirect call target.
- Line 459: `call	free` — calls another function or indirect call target.
- … 3 additional matching instruction line(s) omitted.

**Branches** (13 recognized):
- Line 360: `je	.L65` — changes control flow, often for a conditional or loop edge.
- Line 374: `je	.L70` — changes control flow, often for a conditional or loop edge.
- Line 384: `je	.L57` — changes control flow, often for a conditional or loop edge.
- Line 386: `jne	.L71` — changes control flow, often for a conditional or loop edge.
- Line 388: `je	.L72` — changes control flow, often for a conditional or loop edge.
- Line 399: `jmp	.L57` — changes control flow, often for a conditional or loop edge.
- Line 404: `je	.L66` — changes control flow, often for a conditional or loop edge.
- Line 414: `je	.L73` — changes control flow, often for a conditional or loop edge.
- … 5 additional matching instruction line(s) omitted.

**Loads** (8 recognized):
- Line 436: `popq	%rbx` — loads data from memory or a register stack slot.
- Line 437: `popq	%rsi` — loads data from memory or a register stack slot.
- Line 438: `popq	%rdi` — loads data from memory or a register stack slot.
- Line 439: `popq	%rbp` — loads data from memory or a register stack slot.
- Line 440: `popq	%r12` — loads data from memory or a register stack slot.
- Line 441: `popq	%r13` — loads data from memory or a register stack slot.
- Line 442: `popq	%r14` — loads data from memory or a register stack slot.
- Line 443: `popq	%r15` — loads data from memory or a register stack slot.

**Return** (1 recognized):
- Line 444: `ret` — returns from the current function.

### `compute_market_summary`

Instruction lines recognized: 530.

**Stores** (8 recognized):
- Line 500: `pushq	%r15` — stores data to memory.
- Line 502: `pushq	%r14` — stores data to memory.
- Line 504: `pushq	%r13` — stores data to memory.
- Line 506: `pushq	%r12` — stores data to memory.
- Line 508: `pushq	%rbp` — stores data to memory.
- Line 510: `pushq	%rdi` — stores data to memory.
- Line 512: `pushq	%rsi` — stores data to memory.
- Line 514: `pushq	%rbx` — stores data to memory.

**Integer / address arithmetic** (16 recognized):
- Line 516: `subq	$392, %rsp` — updates values, addresses, or condition operands.
- Line 585: `addq	%rcx, %rax` — updates values, addresses, or condition operands.
- Line 586: `subq	%rbx, %rax` — updates values, addresses, or condition operands.
- Line 640: `subq	%rdx, %rax` — updates values, addresses, or condition operands.
- Line 646: `subq	%rdx, %rax` — updates values, addresses, or condition operands.
- Line 740: `addq	$48, %rbx` — updates values, addresses, or condition operands.
- Line 744: `addq	$1, %rdi` — updates values, addresses, or condition operands.
- Line 784: `subq	%r8, %rsi` — updates values, addresses, or condition operands.
- … 8 additional matching instruction line(s) omitted.

**Review-needed** (415 recognized):
- Line 518: `movups	%xmm6, 224(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 520: `movups	%xmm7, 240(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 522: `movups	%xmm8, 256(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 524: `movups	%xmm9, 272(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 526: `movups	%xmm10, 288(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 528: `movups	%xmm11, 304(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 530: `movups	%xmm12, 320(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 532: `movups	%xmm13, 336(%rsp)` — instruction was not confidently classified by the annotator. **[review-needed]**
- … 407 additional matching instruction line(s) omitted.

**Branches** (57 recognized):
- Line 547: `je	.L147` — changes control flow, often for a conditional or loop edge.
- Line 569: `jne	.L148` — changes control flow, often for a conditional or loop edge.
- Line 571: `je	.L149` — changes control flow, often for a conditional or loop edge.
- Line 591: `je	.L150` — changes control flow, often for a conditional or loop edge.
- Line 597: `jne	.L130` — changes control flow, often for a conditional or loop edge.
- Line 599: `jle	.L130` — changes control flow, often for a conditional or loop edge.
- Line 602: `je	.L151` — changes control flow, often for a conditional or loop edge.
- Line 649: `jmp	.L113` — changes control flow, often for a conditional or loop edge.
- … 49 additional matching instruction line(s) omitted.

**Function call** (25 recognized):
- Line 552: `call	*%rax` — calls another function or indirect call target.
- Line 565: `call	parse_trade_file` — calls another function or indirect call target.
- Line 574: `call	*120(%rsp)` — calls another function or indirect call target.
- Line 617: `call	*120(%rsp)` — calls another function or indirect call target.
- Line 695: `call	printf` — calls another function or indirect call target.
- Line 728: `call	printf` — calls another function or indirect call target.
- Line 831: `call	printf` — calls another function or indirect call target.
- Line 837: `call	printf` — calls another function or indirect call target.
- … 17 additional matching instruction line(s) omitted.

**Loads** (8 recognized):
- Line 955: `popq	%rbx` — loads data from memory or a register stack slot.
- Line 956: `popq	%rsi` — loads data from memory or a register stack slot.
- Line 957: `popq	%rdi` — loads data from memory or a register stack slot.
- Line 958: `popq	%rbp` — loads data from memory or a register stack slot.
- Line 959: `popq	%r12` — loads data from memory or a register stack slot.
- Line 960: `popq	%r13` — loads data from memory or a register stack slot.
- Line 961: `popq	%r14` — loads data from memory or a register stack slot.
- Line 962: `popq	%r15` — loads data from memory or a register stack slot.

**Return** (1 recognized):
- Line 963: `ret` — returns from the current function.

### `main`

Instruction lines recognized: 218.

**Stores** (7 recognized):
- Line 1181: `pushq	%r15` — stores data to memory.
- Line 1183: `pushq	%r14` — stores data to memory.
- Line 1185: `pushq	%r12` — stores data to memory.
- Line 1187: `pushq	%rbp` — stores data to memory.
- Line 1189: `pushq	%rdi` — stores data to memory.
- Line 1191: `pushq	%rsi` — stores data to memory.
- Line 1193: `pushq	%rbx` — stores data to memory.

**Integer / address arithmetic** (4 recognized):
- Line 1195: `subq	$224, %rsp` — updates values, addresses, or condition operands.
- Line 1212: `addq	$224, %rsp` — updates values, addresses, or condition operands.
- Line 1252: `addq	$8, %rbx` — updates values, addresses, or condition operands.
- Line 1263: `addq	$8, %rbx` — updates values, addresses, or condition operands.

**Review-needed** (143 recognized):
- Line 1198: `movl	%ecx, %esi` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 1199: `movq	%rdx, %rdi` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 1201: `cmpl	$1, %esi` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 1203: `movq	(%rdi), %rbx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 1204: `movl	$2, %ecx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 1206: `leaq	.LC18(%rip), %rdx` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 1207: `movq	%rbx, %r8` — instruction was not confidently classified by the annotator. **[review-needed]**
- Line 1208: `movq	%rax, %rcx` — instruction was not confidently classified by the annotator. **[review-needed]**
- … 135 additional matching instruction line(s) omitted.

**Function call** (27 recognized):
- Line 1200: `call	__main` — calls another function or indirect call target.
- Line 1205: `call	*__imp___acrt_iob_func(%rip)` — calls another function or indirect call target.
- Line 1209: `call	fprintf` — calls another function or indirect call target.
- Line 1237: `call	strcmp` — calls another function or indirect call target.
- Line 1242: `call	strcmp` — calls another function or indirect call target.
- Line 1247: `call	strcmp` — calls another function or indirect call target.
- Line 1259: `call	strcmp` — calls another function or indirect call target.
- Line 1292: `call	compute_market_summary` — calls another function or indirect call target.
- … 19 additional matching instruction line(s) omitted.

**Branches** (29 recognized):
- Line 1202: `jg	.L162` — changes control flow, often for a conditional or loop edge.
- Line 1224: `je	.L179` — changes control flow, often for a conditional or loop edge.
- Line 1231: `jmp	.L173` — changes control flow, often for a conditional or loop edge.
- Line 1239: `je	.L189` — changes control flow, often for a conditional or loop edge.
- Line 1244: `je	.L190` — changes control flow, often for a conditional or loop edge.
- Line 1249: `jne	.L170` — changes control flow, often for a conditional or loop edge.
- Line 1254: `je	.L164` — changes control flow, often for a conditional or loop edge.
- Line 1261: `jne	.L165` — changes control flow, often for a conditional or loop edge.
- … 21 additional matching instruction line(s) omitted.

**Loads** (7 recognized):
- Line 1213: `popq	%rbx` — loads data from memory or a register stack slot.
- Line 1214: `popq	%rsi` — loads data from memory or a register stack slot.
- Line 1215: `popq	%rdi` — loads data from memory or a register stack slot.
- Line 1216: `popq	%rbp` — loads data from memory or a register stack slot.
- Line 1217: `popq	%r12` — loads data from memory or a register stack slot.
- Line 1218: `popq	%r14` — loads data from memory or a register stack slot.
- Line 1219: `popq	%r15` — loads data from memory or a register stack slot.

**Return** (1 recognized):
- Line 1220: `ret` — returns from the current function.

## Code and report annotation checklist

- [ ] Timing helper: trace the clock source, units, conversion, and call overhead.
- [ ] Replay scheduling: identify deadline calculation, wait/sleep path, and late-trade handling.
- [ ] Signal decision path: follow input through indicator updates and BUY/SELL decision branches.
- [ ] Latency capture: identify start/end timestamp reads and stored processing/release latency fields.
- [ ] Relevant loops: mark dataset/replay loops and their exit conditions; distinguish loop branches from error branches.
- [ ] `-O2` layout: compare with the matching `-O0` build; optimization can inline helpers, fold constants, remove redundant loads/stores, schedule instructions, and change register/stack usage. Confirm each change in both listings.

## Review-needed instructions

Any instruction marked `[review-needed]` needs a human to confirm its exact semantics and role. An unmarked category is still an automatic classification, not proof of manual review.
