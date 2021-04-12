/* memcmp/wmemcmp optimized with AVX2.
   Copyright (C) 2017-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

//AT&T风格代码 op src, dest
//寻址 section:disp(base, index, scale) -> disp + base + index * scale

/*
cmp{bwlqto}	 	reg[8|16|32], r/m[8|16|32]	[1|2|4|8|10|16]Bytes
cmp{bwlqto} 		r/m[8|16|32], reg[8|16|32]      [1|2|4|8|10|16]Bytes
cmp{bwlqto}	 	imm[8|16|32], r/m[8|16|32]      [1|2|4|8|10|16]Bytes
*/

/* memcmp/wmemcmp is implemented as:
   1. For size from 2 to 7 bytes, load as big endian with movbe and bswap
      to avoid branches.
   2. Use overlapping compare to avoid branch.
   3. Use vector compare when size >= 4 bytes for memcmp or size >= 8
      bytes for wmemcmp.
   4. If size is 8 * VEC_SIZE or less, unroll the loop.
   5. Compare 4 * VEC_SIZE at a time with the aligned first memory
      area.
   6. Use 2 vector compares when size is 2 * VEC_SIZE or less.
   7. Use 4 vector compares when size is 4 * VEC_SIZE or less.
   8. Use 8 vector compares when size is 8 * VEC_SIZE or less.  */

#  define MEMCMP	memcmp_avx2_asm
#  define VPCMPEQ	vpcmpeqb	//word

# ifndef VZEROUPPER
#  define VZEROUPPER	vzeroupper
# endif

# define VEC_SIZE 32
# define VEC_MASK ((1 << VEC_SIZE) - 1)

# define RDX_LP rdx
# define L(name) name

/* Warning!
           wmemcmp has to use SIGNED comparison for elements.
           memcmp has to use UNSIGNED comparison for elemnts.
*/

	.section .text.avx,"ax",@progbits

.global 	memcmp_avx2_asm
.type  	memcmp_avx2_asm, @function
memcmp_avx2_asm:
# if defined __ILP32__
	/* Clear the upper 32 bits.  */
	movl	%edx, %edx
# endif
	cmp	$VEC_SIZE, %RDX_LP //根据 %RDX_LP-$VEC_SIZE设置flag
	jb	L(less_vec)	/*相当于jl，b是below的意思，RDX_LP(比较长度)内容小于VEC_SIZE的时候就跳转*/

	/* From VEC to 2 * VEC.  No branch when size == VEC_SIZE.  */
	vmovdqu	(%rsi), %ymm2		//ymm是32byte 
	VPCMPEQ (%rdi), %ymm2, %ymm2
	vpmovmskb %ymm2, %eax		//这里是32位的掩码
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	cmpq	$(VEC_SIZE * 2), %rdx
	jbe	L(last_vec)		//比较完了前面的32byte，后面小于32  VEC-2*VEC

	VPCMPEQ	%ymm0, %ymm0, %ymm0	//把ymm都设置成1
	/* More than 2 * VEC.  */
	cmpq	$(VEC_SIZE * 8), %rdx
	ja	L(more_8x_vec)
	cmpq	$(VEC_SIZE * 4), %rdx
	jb	L(last_4x_vec)		//2*VEC-4*VEC

	/* From 4 * VEC to 8 * VEC, inclusively. */
	vmovdqu	(%rsi), %ymm1		//4路比较，结果分别放在ymm1-4中
	VPCMPEQ (%rdi), %ymm1, %ymm1	//相等就都设为1

	vmovdqu	VEC_SIZE(%rsi), %ymm2
	VPCMPEQ VEC_SIZE(%rdi), %ymm2, %ymm2

	vmovdqu	(VEC_SIZE * 2)(%rsi), %ymm3
	VPCMPEQ (VEC_SIZE * 2)(%rdi), %ymm3, %ymm3

	vmovdqu	(VEC_SIZE * 3)(%rsi), %ymm4
	VPCMPEQ (VEC_SIZE * 3)(%rdi), %ymm4, %ymm4

	vpand	%ymm1, %ymm2, %ymm5	//与
	vpand	%ymm3, %ymm4, %ymm6
	vpand	%ymm5, %ymm6, %ymm5	//4路结果的与放在ymm5中

	vptest	%ymm0, %ymm5		//如果ymm0&(~ymm5)全是0，设置CF（也就是ymm5全是1,也就是是全相等）
	jnc	L(l4x_vec_end)		//前4×32里有至少一个不一样的情况

	leaq	-(4 * VEC_SIZE)(%rdi, %rdx), %rdi
	leaq	-(4 * VEC_SIZE)(%rsi, %rdx), %rsi
	vmovdqu	(%rsi), %ymm1
	VPCMPEQ (%rdi), %ymm1, %ymm1

	vmovdqu	VEC_SIZE(%rsi), %ymm2
	VPCMPEQ VEC_SIZE(%rdi), %ymm2, %ymm2
	vpand	%ymm2, %ymm1, %ymm5

	vmovdqu	(VEC_SIZE * 2)(%rsi), %ymm3
	VPCMPEQ (VEC_SIZE * 2)(%rdi), %ymm3, %ymm3
	vpand	%ymm3, %ymm5, %ymm5

	vmovdqu	(VEC_SIZE * 3)(%rsi), %ymm4
	VPCMPEQ (VEC_SIZE * 3)(%rdi), %ymm4, %ymm4
	vpand	%ymm4, %ymm5, %ymm5

	vptest	%ymm0, %ymm5
	jnc	L(l4x_vec_end)
	xorl	%eax, %eax
	VZEROUPPER
	ret		//这里一定返回0,不用管

	.p2align 4
L(last_2x_vec):
	/* From VEC to 2 * VEC.  No branch when size == VEC_SIZE.  */
	vmovdqu	(%rsi), %ymm2
	VPCMPEQ (%rdi), %ymm2, %ymm2
	vpmovmskb %ymm2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

L(last_vec):
	/* Use overlapping loads to avoid branches.  */
	leaq	-VEC_SIZE(%rdi, %rdx), %rdi	//结尾减32
	leaq	-VEC_SIZE(%rsi, %rdx), %rsi
	vmovdqu	(%rsi), %ymm2
	VPCMPEQ (%rdi), %ymm2, %ymm2
	vpmovmskb %ymm2, %eax		//32位的掩码
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)	//不相等就跳转,这里为什么不直接返回呢,因为不能确定返回大于还是小于0的数
	VZEROUPPER	//这里都要返回了，为什么还要清零ymm的高位呢？
	ret		//这里也返回0

	.p2align 4
L(first_vec):
	/* A byte or int32 is different within 16 or 32 bytes.  */
//	tzcntl	%eax, %ecx	//把%eax结尾的（least sinificant bits）0的个数存在%ecx,也就是相同的长度
//	movzbl	(%rdi, %rcx), %eax	//zero extend,byte到long,把不同的地方的字符分别移到eax和edx
//	movzbl	(%rsi, %rcx), %edx
	tzcntl	%eax, %r8d	//把%eax结尾的（least sinificant bits）0的个数存在%ecx,也就是相同的长度
	movzbl	(%rdi, %r8), %eax	//zero extend,byte到long,把不同的地方的字符分别移到eax和edx
	movzbl	(%rsi, %r8), %edx
	sub	%edx, %eax		//比较eax和dex不同的地方
	leaq	(%rdi, %r8), %rdx	//把(第一个参数的)第一个不同的地址移到第四个参数指向的内存
	movq	%rdx, (%rcx)		//把(第一个参数的)第一个不同的地址移到第四个参数指向的内存
	VZEROUPPER	//清空128及以上的高位
	ret		//note

	.p2align 4
L(between_4_7):
	/* Load as big endian with overlapping movbe to avoid branches.  */
	movbe	(%rdi), %eax	//从rdi拷贝并且做字节交换，32位，4byte
	movbe	(%rsi), %r8d
	shlq	$32, %rax
//	shlq	$32, %rcx
	shlq	$32, %r8
//	movq	%rdi, %r9
//	movbe	-4(%rdi, %rdx), %edi	//把末尾-4的字节用大端法写到edi（rdi，第一个参数的低32位）
	movbe	-4(%rdi, %rdx), %r10d	//把末尾-4的字节用大端法写到edi（rdi，第一个参数的低32位）
	orq	%r10, %rax	//OR第一个参数的前4和后4个byte,哦，拼接，前4byte已经到高32位了,从高位到低位分别是原来的第0,1,2,3,tail-3,tail-2,tail-1,tail字节
//	movbe	-4(%rsi, %rdx), %esi
	movbe	-4(%rsi, %rdx), %r10d
//	orq	%rsi, %rcx
//	subq	%rcx, %rax
	orq		%r10, %r8
	subq	%r8, %rax
	je	L(exit)
//added by xzjin
	movq	%rax, %r10
	movw	(%rdi), %ax
	movw	(%rsi), %r8w
	movw	%ax, %r9w
	subw	%r8w, %ax
	jz		L(later3diff4_7)
	subb	%r8b, %r9b
	jnz 		L(_1diff4_7)
	leaq	1(%rdi), %rdi	//start+len-1
	movq	%rdi, (%rcx)
	ret		//note
L(_1diff4_7):
	movq	%rdi, (%rcx)
	ret		//note

L(later3diff4_7):
	movw	2(%rdi), %ax
	movw	2(%rsi), %r8w
	movw	%ax, %r9w
	subw	%r8w, %ax
	jz		L(later5diff4_7)
	subb	%r8b, %r9b
	jnz 		L(_3diff4_7)
	leaq	3(%rdi), %rdi	//start+len-1
	movq	%rdi, (%rcx)
	ret
L(_3diff4_7):
	leaq	2(%rdi), %rdi
	movq	%rdi, (%rcx)
	ret		//note

L(later5diff4_7):
	movw	4(%rdi), %ax
	movw	4(%rsi), %r8w
	movw	%ax, %r9w
	subw	%r8w, %ax
	jz		L(later6diff4_7)
	subb	%r8b, %r9b
	jnz 		L(_5diff4_7)
	leaq	5(%rdi), %rdi	//start+len-1
	movq	%rdi, (%rcx)
	ret
L(_5diff4_7):
	leaq	4(%rdi), %rdi
	movq	%rdi, (%rcx)
	ret		//note

L(later6diff4_7):
	movq	%r10, %rax
	leaq	6(%rdi), %rdi
	movq	%rdi, (%rcx)
	ret		//note

/*
	lzcntq	%rax, %r8	//开始(leading)0的个数,by bits
	shrq	$3, %r8
	cmp	$4, %r8
	jb	L(sameLess4)	//相当于jl，b是below的意思，R8(相同长度)内容小于4的时候就跳转
	leaq	-8(%r9, %rdx), %rdx	//把(第一个参数的)第一个不同的地址移到第四个参数指向的内存,start+len-8
	addq	%r8, %rdx	//把(第一个参数的)第一个不同的地址移到第四个参数指向的内存,start+len-8+idx
	movq	%rdx, (%rcx)		//把(第一个参数的)第一个不同的地址移到第四个参数指向的内存
	sbbl	%eax, %eax
	orl	$1, %eax
	ret		//note
*/

L(sameLess4):
	leaq	(%r9, %r8), %rdx	//把(第一个参数的)第一个不同的地址移到第四个参数指向的内存
	movq	%rdx, (%rcx)		//把(第一个参数的)第一个不同的地址移到第四个参数指向的内存
	sbbl	%eax, %eax
	orl	$1, %eax
	ret		//note

	.p2align 4
L(exit):
	ret		//返回0

	.p2align 4
L(between_2_3):
	/* Load as big endian to avoid branches.  */
	movzwl	(%rdi), %eax	//把16位扩展成32位
//	movzwl	(%rsi), %ecx
	movzwl	(%rsi), %r8d
	movl	%eax, %r9d	//把%eax复制到%r9d，先比较前两位
	subl	%r8d, %r9d	//比较前两位
	jnz		L(sameLess2)

/*
	shll	$8, %eax
//	shll	$8, %ecx
	shll	$8, %r8d
	bswap	%eax			//交换大小端
//	bswap	%ecx
	bswap	%r8d
*/
	movb	-1(%rdi, %rdx), %al		//读最后一位到低8位
//	movb	-1(%rsi, %rdx), %cl		//从前到后分别是0...第0,1,1/2位
	movb	-1(%rsi, %rdx), %r8b		//从前到后分别是0...第0,1,1/2位
	/* Subtraction is okay because the upper 8 bits are zero.  */
//	subl	%ecx, %eax	//这个应该是%eax-%ecx存到%eax里面
//可以直接减，因为前面两byte一定是相同的
	subb	%r8b, %al	//这个应该是%eax-%ecx存到%eax里面
/*
	lzcntq	%rax, %r8	//开始(leading)0的个数,by bits
	shrq	$3, %r8
	cmp	$7, %r8
	jb	L(sameLess2)	//相当于jl，b是below的意思，R8(相同长度)内容小于7的时候就跳转
*/
	leaq	-1(%rdx, %rdi), %rdi	//start+len-1
	movq	%rdi, (%rcx)
	ret		//note

/*
L(sameLess2):
	leaq	-5(%r8, %rdi), %rdx		//start+idx-5
	movq	%rdx, (%rcx)
	ret		//note
*/

L(sameLess2):
	subb	%r8b, %al	//这个应该是%eax-%ecx存到%eax里面
	jnz		L(firstDiff2_3)
	leaq	1(%rdi), %r8
	movq	%r8, (%rcx)
	movl	%r9d, %eax
	ret

L(firstDiff2_3):
	movl	%r9d, %eax
	movq	%rdi, (%rcx)
	ret		//note

	.p2align 4
L(le1):
	movzbl	(%rdi), %eax	//这里为什么用的movzbl？l是32位，高位会不会有影响
//	movzbl	(%rsi), %ecx
//	subl	%ecx, %eax
	movzbl	(%rsi), %r8d
	subl	%r8d, %eax
	movq	%rdi, (%rcx)
	ret		//note

	.p2align 4
L(zero):
	xorl	%eax, %eax
	ret		//这里返回0

	.p2align 4
L(less_vec):
	cmpb	$1, %dl
	je	L(le1)	//长度为1
	jb	L(zero)	//比较长度小于1就返回0
	cmpb	$4, %dl
	jb	L(between_2_3)
	cmpb	$8, %dl
	jb	L(between_4_7)
	cmpb	$16, %dl
	jae	L(between_16_31)	//大于等于就是16-31位
	/* It is between 8 and 15 bytes.  */
	vmovq	(%rdi), %xmm1	//xmm是128bits，16byte,为什么用16byte呢
	vmovq	(%rsi), %xmm2
	VPCMPEQ %xmm1, %xmm2, %xmm2		//相等就都设为1,否则就都是0	
	vpmovmskb %xmm2, %eax	//搜集每一byte的最高位组成掩码到%eax的低位
	subl    $0xffff, %eax	//0xffff(后面跳转之后用eax末尾0的个数表示相同的bytes),16位,正好和%eax的长度一样
	jnz	L(first_vec)		//前8byte有不相等的就跳转
	/* Use overlapping loads to avoid branches.  */
	leaq	-8(%rdi, %rdx), %rdi	//结尾减8
	leaq	-8(%rsi, %rdx), %rsi
	vmovq	(%rdi), %xmm1
	vmovq	(%rsi), %xmm2
	VPCMPEQ %xmm1, %xmm2, %xmm2
	vpmovmskb %xmm2, %eax
	subl    $0xffff, %eax
	jnz	L(first_vec)	//这里如果是比较13位，第14位不相同的话跳转过去会怎么样？
	ret		//返回0

	.p2align 4
L(between_16_31):
	/* From 16 to 31 bytes.  No branch when size == 16.  */
	vmovdqu	(%rsi), %xmm2	//xmm 是ymm的低128bits
	VPCMPEQ (%rdi), %xmm2, %xmm2
	vpmovmskb %xmm2, %eax
	subl    $0xffff, %eax
	jnz	L(first_vec)

	/* Use overlapping loads to avoid branches.  */
	leaq	-16(%rdi, %rdx), %rdi
	leaq	-16(%rsi, %rdx), %rsi
	vmovdqu	(%rsi), %xmm2
	VPCMPEQ (%rdi), %xmm2, %xmm2
	vpmovmskb %xmm2, %eax
	subl    $0xffff, %eax
	jnz	L(first_vec)
	ret		//返回0

	.p2align 4
L(more_8x_vec):
	/* More than 8 * VEC.  Check the first VEC.  */
	vmovdqu	(%rsi), %ymm2
	VPCMPEQ (%rdi), %ymm2, %ymm2
	vpmovmskb %ymm2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)
	//没看懂
	/* Align the first memory area for aligned loads in the loop.
	   Compute how much the first memory area is misaligned.  */
//	movq	%rdi, %rcx	//rdi,第一个参数，rsi，第二个参数,rdx,第三个参数
//	andl	$(VEC_SIZE - 1), %ecx	//只保留末尾的5位,rcx前面的内容都清零了吗
//	/* Get the negative of offset for alignment.  */
//	subq	$VEC_SIZE, %rcx		//这里减是什么意思，会有溢出吗
//	/* Adjust the second memory area.  */
//	subq	%rcx, %rsi	//这里相当于是到了原本rsi的下一个32byte的开始吗？gdb试一下
//	/* Adjust the first memory area which should be aligned now.  */
//	subq	%rcx, %rdi
//	/* Adjust length.  */
//	addq	%rcx, %rdx
	movq	%rdi, %r8	//rdi,第一个参数，rsi，第二个参数,rdx,第三个参数
	andl	$(VEC_SIZE - 1), %r8d	//只保留末尾的5位,rcx前面的内容都清零了吗
	/* Get the negative of offset for alignment.  */
	subq	$VEC_SIZE, %r8		//这里减是什么意思，会有溢出吗
	/* Adjust the second memory area.  */
	subq	%r8, %rsi	//这里相当于是到了原本rsi的下一个32byte的开始吗？gdb试一下
	/* Adjust the first memory area which should be aligned now.  */
	subq	%r8, %rdi
	/* Adjust length.  */
	addq	%r8, %rdx

L(loop_4x_vec):
	/* Compare 4 * VEC at a time forward.  */
	vmovdqu	(%rsi), %ymm1
	VPCMPEQ (%rdi), %ymm1, %ymm1

	vmovdqu	VEC_SIZE(%rsi), %ymm2
	VPCMPEQ VEC_SIZE(%rdi), %ymm2, %ymm2
	vpand	%ymm2, %ymm1, %ymm5

	vmovdqu	(VEC_SIZE * 2)(%rsi), %ymm3
	VPCMPEQ (VEC_SIZE * 2)(%rdi), %ymm3, %ymm3
	vpand	%ymm3, %ymm5, %ymm5

	vmovdqu	(VEC_SIZE * 3)(%rsi), %ymm4
	VPCMPEQ (VEC_SIZE * 3)(%rdi), %ymm4, %ymm4
	vpand	%ymm4, %ymm5, %ymm5

	vptest	%ymm0, %ymm5
	jnc	L(l4x_vec_end)

	addq	$(VEC_SIZE * 4), %rdi
	addq	$(VEC_SIZE * 4), %rsi

	subq	$(VEC_SIZE * 4), %rdx
	cmpq	$(VEC_SIZE * 4), %rdx
	jae	L(loop_4x_vec)

	/* Less than 4 * VEC.  */
	cmpq	$VEC_SIZE, %rdx
	jbe	L(last_vec)
	cmpq	$(VEC_SIZE * 2), %rdx
	jbe	L(last_2x_vec)

L(last_4x_vec):
	/* From 2 * VEC to 4 * VEC. */
	vmovdqu	(%rsi), %ymm2
	VPCMPEQ (%rdi), %ymm2, %ymm2
	vpmovmskb %ymm2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	addq	$VEC_SIZE, %rdi
	addq	$VEC_SIZE, %rsi
	vmovdqu	(%rsi), %ymm2
	VPCMPEQ (%rdi), %ymm2, %ymm2
	vpmovmskb %ymm2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	/* Use overlapping loads to avoid branches.  */
	leaq	-(3 * VEC_SIZE)(%rdi, %rdx), %rdi	//因为前面加了VEC，这里实际是比较的末尾-2×VEC
	leaq	-(3 * VEC_SIZE)(%rsi, %rdx), %rsi
	vmovdqu	(%rsi), %ymm2
	VPCMPEQ (%rdi), %ymm2, %ymm2
	vpmovmskb %ymm2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)

	addq	$VEC_SIZE, %rdi		//这里实际是结尾的最后一个VEC,我们可以在这情况直接用比较的长度（%rdx）减相同的长度
	addq	$VEC_SIZE, %rsi
	vmovdqu	(%rsi), %ymm2
	VPCMPEQ (%rdi), %ymm2, %ymm2
	vpmovmskb %ymm2, %eax
	subl    $VEC_MASK, %eax
	jnz	L(first_vec)
	VZEROUPPER
	ret		//返回0

	.p2align 4
L(l4x_vec_end):
	vpmovmskb %ymm1, %eax		//32位的掩码
	subl	$VEC_MASK, %eax
	jnz	L(first_vec)
	vpmovmskb %ymm2, %eax
	subl	$VEC_MASK, %eax
	jnz	L(first_vec_x1)
	vpmovmskb %ymm3, %eax
	subl	$VEC_MASK, %eax
	jnz	L(first_vec_x2)
	vpmovmskb %ymm4, %eax
	subl	$VEC_MASK, %eax
//	tzcntl	%eax, %ecx	//结尾0的长度,也就是相等的长度
//	movzbl	(VEC_SIZE * 3)(%rdi, %rcx), %eax	//见过这种语法，跳转到最后的不一样的地方，没太看懂
//	movzbl	(VEC_SIZE * 3)(%rsi, %rcx), %edx
	tzcntl	%eax, %r8d	//结尾0的长度,也就是相等的长度
	movzbl	(VEC_SIZE * 3)(%rdi, %r8), %eax	//0扩展，byte到long,注意这里是字节级的比较，VEC_SIZE是32byte级别的，r8是32byte内部的偏移
	movzbl	(VEC_SIZE * 3)(%rsi, %r8), %edx
	sub	%edx, %eax
	leaq	(VEC_SIZE * 3)(%rdi, %r8), %rdx
	movq	%rdx, (%rcx)
	VZEROUPPER
	ret		//note
//比较同一个vec×4（128byte）里面第1/2个（从0开始）32byte
	.p2align 4
L(first_vec_x1):
//	tzcntl	%eax, %ecx
//	movzbl	VEC_SIZE(%rdi, %rcx), %eax
//	movzbl	VEC_SIZE(%rsi, %rcx), %edx
	tzcntl	%eax, %r8d
	movzbl	VEC_SIZE(%rdi, %r8), %eax
	movzbl	VEC_SIZE(%rsi, %r8), %edx
	sub	%edx, %eax
	leaq	VEC_SIZE(%rdi, %r8), %rdx
	movq	%rdx, (%rcx)
	VZEROUPPER
	ret		//note

	.p2align 4
L(first_vec_x2):
//	tzcntl	%eax, %ecx
//	movzbl	(VEC_SIZE * 2)(%rdi, %rcx), %eax
//	movzbl	(VEC_SIZE * 2)(%rsi, %rcx), %edx
	tzcntl	%eax, %r8d
	movzbl	(VEC_SIZE * 2)(%rdi, %r8), %eax
	movzbl	(VEC_SIZE * 2)(%rsi, %r8), %edx
	sub	%edx, %eax
	leaq	(VEC_SIZE * 2)(%rdi, %r8), %rdx
	movq	%rdx, (%rcx)
	VZEROUPPER
	ret		//note
