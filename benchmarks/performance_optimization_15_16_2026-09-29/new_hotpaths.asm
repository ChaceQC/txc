
E:\Project\other\Compilation\tx_build\performance_15_16\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

000000014002fa60 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE>:
   14002fa60:	push   %r13
   14002fa62:	push   %r12
   14002fa64:	push   %rbp
   14002fa65:	push   %rdi
   14002fa66:	push   %rsi
   14002fa67:	push   %rbx
   14002fa68:	sub    $0x78,%rsp
   14002fa6c:	mov    %rdx,%rdi
   14002fa6f:	mov    (%rdx),%rdx
   14002fa72:	mov    %rcx,%rbx
   14002fa75:	mov    0x8(%rdx),%rax
   14002fa79:	sub    (%rdx),%rax
   14002fa7c:	shr    $0x3e,%rax
   14002fa80:	jne    14002fbd6 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x176>
   14002fa86:	movb   $0x0,0x10(%rcx)
   14002fa8a:	mov    (%rdi),%rax
   14002fa8d:	lea    0x10(%rcx),%rsi
   14002fa91:	mov    %rsi,(%rcx)
   14002fa94:	mov    0x8(%rax),%r10
   14002fa98:	mov    (%rax),%r11
   14002fa9b:	movq   $0x0,0x8(%rcx)
   14002faa3:	mov    %r10,%rbp
   14002faa6:	sub    %r11,%rbp
   14002faa9:	add    %rbp,%rbp
   14002faac:	cmp    $0xf,%rbp
   14002fab0:	ja     14002fb28 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0xc8>
   14002fab2:	cmp    %r10,%r11
   14002fab5:	je     14002fb92 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x132>
   14002fabb:	sub    %r11,%r10
   14002fabe:	xor    %eax,%eax
   14002fac0:	lea    0x5c6a9(%rip),%r9        # 14008c170 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE>
   14002fac7:	nopw   0x0(%rax,%rax,1)
   14002fad0:	movzbl (%r11,%rax,1),%edx
   14002fad5:	xor    %ecx,%ecx
   14002fad7:	mov    %edx,%r8d
   14002fada:	and    $0xf,%edx
   14002fadd:	shr    $0x4,%r8b
   14002fae1:	movzbl (%r9,%rdx,1),%edx
   14002fae6:	and    $0xf,%r8d
   14002faea:	mov    (%r9,%r8,1),%cl
   14002faee:	mov    %dl,%ch
   14002faf0:	mov    %cx,(%rsi,%rax,2)
   14002faf4:	add    $0x1,%rax
   14002faf8:	cmp    %rax,%r10
   14002fafb:	jne    14002fad0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x70>
   14002fafd:	mov    (%rdi),%rdx
   14002fb00:	mov    0x8(%rdx),%rax
   14002fb04:	sub    (%rdx),%rax
   14002fb07:	add    %rax,%rax
   14002fb0a:	mov    (%rbx),%rdx
   14002fb0d:	mov    %rax,0x8(%rbx)
   14002fb11:	movb   $0x0,(%rdx,%rax,1)
   14002fb15:	mov    %rbx,%rax
   14002fb18:	add    $0x78,%rsp
   14002fb1c:	pop    %rbx
   14002fb1d:	pop    %rsi
   14002fb1e:	pop    %rdi
   14002fb1f:	pop    %rbp
   14002fb20:	pop    %r12
   14002fb22:	pop    %r13
   14002fb24:	ret
   14002fb25:	nopl   (%rax)
   14002fb28:	test   %rbp,%rbp
   14002fb2b:	js     14002fbb7 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x157>
   14002fb31:	lea    0x1(%rbp),%rcx
   14002fb35:	cmp    $0x1d,%rbp
   14002fb39:	jbe    14002fbab <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x14b>
   14002fb3b:	call   14006cfd0 <_Znwy>
   14002fb40:	mov    0x8(%rbx),%r8
   14002fb44:	mov    (%rbx),%r13
   14002fb47:	mov    %rax,%r12
   14002fb4a:	cmp    $0x1,%r8
   14002fb4e:	je     14002fba0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x140>
   14002fb50:	test   %r8,%r8
   14002fb53:	je     14002fb60 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x100>
   14002fb55:	mov    %r13,%rdx
   14002fb58:	mov    %rax,%rcx
   14002fb5b:	call   140074c68 <memcpy>
   14002fb60:	cmp    %r13,%rsi
   14002fb63:	je     14002fb75 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x115>
   14002fb65:	mov    0x10(%rbx),%rax
   14002fb69:	mov    %r13,%rcx
   14002fb6c:	lea    0x1(%rax),%rdx
   14002fb70:	call   14006cfd8 <_ZdlPvy>
   14002fb75:	mov    %rbp,0x10(%rbx)
   14002fb79:	mov    (%rdi),%rax
   14002fb7c:	mov    %r12,%rsi
   14002fb7f:	mov    %r12,(%rbx)
   14002fb82:	mov    (%rax),%r11
   14002fb85:	mov    0x8(%rax),%r10
   14002fb89:	cmp    %r10,%r11
   14002fb8c:	jne    14002fabb <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x5b>
   14002fb92:	xor    %eax,%eax
   14002fb94:	jmp    14002fb0a <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0xaa>
   14002fb99:	nopl   0x0(%rax)
   14002fba0:	movzbl 0x0(%r13),%eax
   14002fba5:	mov    %al,(%r12)
   14002fba9:	jmp    14002fb60 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x100>
   14002fbab:	mov    $0x1e,%ebp
   14002fbb0:	mov    $0x1f,%ecx
   14002fbb5:	jmp    14002fb3b <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0xdb>
   14002fbb7:	lea    0x5bf40(%rip),%rcx        # 14008bafe <.rdata+0xbe>
   14002fbbe:	call   14006d010 <_ZSt20__throw_length_errorPKc>
   14002fbc3:	mov    %rax,%rsi
   14002fbc6:	mov    %rbx,%rcx
   14002fbc9:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fbce:	mov    %rsi,%rcx
   14002fbd1:	call   14006e8e8 <_Unwind_Resume>
   14002fbd6:	mov    $0x58,%ecx
   14002fbdb:	lea    0x24(%rsp),%rdi
   14002fbe0:	call   14006cfc8 <__cxa_allocate_exception>
   14002fbe5:	mov    $0x11,%ecx
   14002fbea:	lea    0x5be92(%rip),%rdx        # 14008ba83 <.rdata+0x43>
   14002fbf1:	movl   $0x1,0x20(%rsp)
   14002fbf9:	mov    %rax,%rsi
   14002fbfc:	xor    %eax,%eax
   14002fbfe:	rep stos %eax,%es:(%rdi)
   14002fc00:	lea    0x28(%rsp),%rdi
   14002fc05:	mov    %rdi,%rcx
   14002fc08:	call   140085670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   14002fc0d:	lea    0x48(%rsp),%rbp
   14002fc12:	lea    0x5befd(%rip),%rdx        # 14008bb16 <.rdata+0xd6>
   14002fc19:	mov    %rbp,%rcx
   14002fc1c:	call   140085670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   14002fc21:	lea    0x20(%rsp),%rdx
   14002fc26:	mov    %rsi,%rcx
   14002fc29:	call   14007b030 <_ZN12tx_generated15runtime_failureC1ENS_10error_infoE>
   14002fc2e:	mov    %rbp,%rcx
   14002fc31:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fc36:	mov    %rdi,%rcx
   14002fc39:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fc3e:	lea    0x4b67b(%rip),%r8        # 14007b2c0 <_ZN12tx_generated15runtime_failureD1Ev>
   14002fc45:	lea    0x5ed24(%rip),%rdx        # 14008e970 <_ZTIN12tx_generated15runtime_failureE>
   14002fc4c:	mov    %rsi,%rcx
   14002fc4f:	call   14006cf88 <__cxa_throw>
   14002fc54:	mov    %rax,%rbx
   14002fc57:	jmp    14002fc6c <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x20c>
   14002fc59:	mov    %rbp,%rcx
   14002fc5c:	mov    %rax,%rbx
   14002fc5f:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fc64:	mov    %rdi,%rcx
   14002fc67:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fc6c:	mov    %rsi,%rcx
   14002fc6f:	call   14006cfb0 <__cxa_free_exception>
   14002fc74:	mov    %rbx,%rcx
   14002fc77:	call   14006e8e8 <_Unwind_Resume>
   14002fc7c:	mov    %rdi,%rcx
   14002fc7f:	mov    %rax,%rbx
   14002fc82:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fc87:	jmp    14002fc6c <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x20c>
   14002fc89:	nopl   0x0(%rax)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140032280 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE>:
   140032280:	push   %r15
   140032282:	push   %r14
   140032284:	push   %r13
   140032286:	push   %r12
   140032288:	push   %rbp
   140032289:	push   %rdi
   14003228a:	push   %rsi
   14003228b:	push   %rbx
   14003228c:	sub    $0x2b8,%rsp
   140032293:	movups %xmm6,0x270(%rsp)
   14003229b:	movups %xmm7,0x280(%rsp)
   1400322a3:	movups %xmm8,0x290(%rsp)
   1400322ac:	movups %xmm9,0x2a0(%rsp)
   1400322b5:	mov    0x8(%rdx),%rsi
   1400322b9:	mov    %rcx,%r15
   1400322bc:	mov    (%rdx),%rcx
   1400322bf:	mov    %rcx,%rdi
   1400322c2:	and    $0x1,%edi
   1400322c5:	jne    140032e77 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xbf7>
   1400322cb:	mov    %rcx,%r13
   1400322ce:	lea    (%rsi,%rcx,1),%r8
   1400322d2:	shr    %r13
   1400322d5:	cmp    %r8,%rsi
   1400322d8:	je     140032898 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x618>
   1400322de:	lea    -0x1(%rcx),%rax
   1400322e2:	cmp    $0xe,%rax
   1400322e6:	jbe    140032d5c <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xadc>
   1400322ec:	mov    %rcx,%r11
   1400322ef:	pxor   %xmm3,%xmm3
   1400322f3:	mov    %rsi,%rax
   1400322f6:	movdqu 0x59e91(%rip),%xmm9        # 14008c190 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x20>
   1400322ff:	and    $0xfffffffffffffff0,%r11
   140032303:	movdqu 0x59ea5(%rip),%xmm7        # 14008c1b0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x40>
   14003230b:	movdqu 0x59e8c(%rip),%xmm8        # 14008c1a0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x30>
   140032314:	movdqa %xmm3,%xmm2
   140032318:	movdqu 0x59ea0(%rip),%xmm6        # 14008c1c0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x50>
   140032320:	movdqu 0x59ea8(%rip),%xmm5        # 14008c1d0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x60>
   140032328:	lea    (%r11,%rsi,1),%rdx
   14003232c:	movdqu 0x59eac(%rip),%xmm4        # 14008c1e0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x70>
   140032334:	nopl   0x0(%rax)
   140032338:	movdqu (%rax),%xmm0
   14003233c:	movdqu (%rax),%xmm1
   140032340:	add    $0x10,%rax
   140032344:	por    %xmm9,%xmm0
   140032349:	paddb  %xmm6,%xmm1
   14003234d:	paddb  %xmm8,%xmm0
   140032352:	psubusb %xmm5,%xmm1
   140032356:	psubusb %xmm7,%xmm0
   14003235a:	pcmpeqb %xmm2,%xmm1
   14003235e:	pcmpeqb %xmm2,%xmm0
   140032362:	pcmpeqb %xmm2,%xmm1
   140032366:	pcmpeqb %xmm2,%xmm0
   14003236a:	pand   %xmm1,%xmm0
   14003236e:	pand   %xmm4,%xmm0
   140032372:	por    %xmm0,%xmm3
   140032376:	cmp    %rax,%rdx
   140032379:	jne    140032338 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xb8>
   14003237b:	movdqa %xmm3,%xmm0
   14003237f:	movhlps %xmm3,%xmm4
   140032382:	psrldq $0x8,%xmm0
   140032387:	por    %xmm3,%xmm4
   14003238b:	por    %xmm3,%xmm0
   14003238f:	movdqa %xmm0,%xmm1
   140032393:	psrldq $0x4,%xmm1
   140032398:	por    %xmm1,%xmm0
   14003239c:	movdqa %xmm0,%xmm1
   1400323a0:	psrldq $0x2,%xmm1
   1400323a5:	por    %xmm1,%xmm0
   1400323a9:	movdqa %xmm0,%xmm1
   1400323ad:	psrldq $0x1,%xmm1
   1400323b2:	por    %xmm1,%xmm0
   1400323b6:	movd   %xmm0,%r10d
   1400323bb:	mov    %r10d,%eax
   1400323be:	cmp    %r11,%rcx
   1400323c1:	je     140032d6d <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xaed>
   1400323c7:	sub    %r11,%rcx
   1400323ca:	lea    -0x1(%rcx),%r10
   1400323ce:	cmp    $0x6,%r10
   1400323d2:	jbe    140032498 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x218>
   1400323d8:	movq   0x59db0(%rip),%xmm0        # 14008c190 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x20>
   1400323e0:	movq   (%rsi,%r11,1),%xmm1
   1400323e6:	movq   0x59db2(%rip),%xmm2        # 14008c1a0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x30>
   1400323ee:	movq   0x59dca(%rip),%xmm3        # 14008c1c0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x50>
   1400323f6:	por    %xmm1,%xmm0
   1400323fa:	paddb  %xmm2,%xmm0
   1400323fe:	paddb  %xmm3,%xmm1
   140032402:	movq   0x59da6(%rip),%xmm2        # 14008c1b0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x40>
   14003240a:	movq   0x59dbe(%rip),%xmm3        # 14008c1d0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x60>
   140032412:	psubusb %xmm2,%xmm0
   140032416:	psubusb %xmm3,%xmm1
   14003241a:	pxor   %xmm2,%xmm2
   14003241e:	pcmpeqb %xmm2,%xmm0
   140032422:	pcmpeqb %xmm2,%xmm1
   140032426:	pcmpeqb %xmm2,%xmm0
   14003242a:	pcmpeqb %xmm2,%xmm1
   14003242e:	pand   %xmm1,%xmm0
   140032432:	movq   0x59da6(%rip),%xmm1        # 14008c1e0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x70>
   14003243a:	pand   %xmm1,%xmm0
   14003243e:	por    %xmm0,%xmm4
   140032442:	movq   %xmm4,%r9
   140032447:	mov    %r9,%rax
   14003244a:	mov    %r9,%r10
   14003244d:	shr    $0x10,%r10
   140032451:	or     %ah,%al
   140032453:	or     %r10d,%eax
   140032456:	mov    %r9,%r10
   140032459:	shr    $0x18,%r10
   14003245d:	or     %r10d,%eax
   140032460:	mov    %r9,%r10
   140032463:	shr    $0x20,%r10
   140032467:	or     %r10d,%eax
   14003246a:	mov    %r9,%r10
   14003246d:	shr    $0x28,%r10
   140032471:	or     %r10d,%eax
   140032474:	mov    %r9,%r10
   140032477:	shr    $0x38,%r9
   14003247b:	shr    $0x30,%r10
   14003247f:	or     %r10d,%eax
   140032482:	or     %r9d,%eax
   140032485:	mov    %rcx,%r9
   140032488:	and    $0xfffffffffffffff8,%r9
   14003248c:	add    %r9,%rdx
   14003248f:	and    $0x7,%ecx
   140032492:	je     1400325d6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   140032498:	movzbl (%rdx),%ecx
   14003249b:	mov    %ecx,%r9d
   14003249e:	or     $0x20,%r9d
   1400324a2:	sub    $0x61,%r9d
   1400324a6:	cmp    $0x5,%r9b
   1400324aa:	seta   %r9b
   1400324ae:	sub    $0x30,%ecx
   1400324b1:	cmp    $0x9,%cl
   1400324b4:	seta   %cl
   1400324b7:	and    %r9d,%ecx
   1400324ba:	or     %ecx,%eax
   1400324bc:	lea    0x1(%rdx),%rcx
   1400324c0:	cmp    %rcx,%r8
   1400324c3:	je     1400325d6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   1400324c9:	movzbl 0x1(%rdx),%ecx
   1400324cd:	mov    %ecx,%r9d
   1400324d0:	or     $0x20,%r9d
   1400324d4:	sub    $0x61,%r9d
   1400324d8:	cmp    $0x5,%r9b
   1400324dc:	seta   %r9b
   1400324e0:	sub    $0x30,%ecx
   1400324e3:	cmp    $0x9,%cl
   1400324e6:	seta   %cl
   1400324e9:	and    %r9d,%ecx
   1400324ec:	or     %ecx,%eax
   1400324ee:	lea    0x2(%rdx),%rcx
   1400324f2:	cmp    %rcx,%r8
   1400324f5:	je     1400325d6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   1400324fb:	movzbl 0x2(%rdx),%ecx
   1400324ff:	mov    %ecx,%r9d
   140032502:	or     $0x20,%r9d
   140032506:	sub    $0x61,%r9d
   14003250a:	cmp    $0x5,%r9b
   14003250e:	seta   %r9b
   140032512:	sub    $0x30,%ecx
   140032515:	cmp    $0x9,%cl
   140032518:	seta   %cl
   14003251b:	and    %r9d,%ecx
   14003251e:	or     %ecx,%eax
   140032520:	lea    0x3(%rdx),%rcx
   140032524:	cmp    %rcx,%r8
   140032527:	je     1400325d6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   14003252d:	movzbl 0x3(%rdx),%ecx
   140032531:	mov    %ecx,%r9d
   140032534:	or     $0x20,%r9d
   140032538:	sub    $0x61,%r9d
   14003253c:	cmp    $0x5,%r9b
   140032540:	seta   %r9b
   140032544:	sub    $0x30,%ecx
   140032547:	cmp    $0x9,%cl
   14003254a:	seta   %cl
   14003254d:	and    %r9d,%ecx
   140032550:	or     %ecx,%eax
   140032552:	lea    0x4(%rdx),%rcx
   140032556:	cmp    %rcx,%r8
   140032559:	je     1400325d6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   14003255b:	movzbl 0x4(%rdx),%ecx
   14003255f:	mov    %ecx,%r9d
   140032562:	or     $0x20,%r9d
   140032566:	sub    $0x61,%r9d
   14003256a:	cmp    $0x5,%r9b
   14003256e:	seta   %r9b
   140032572:	sub    $0x30,%ecx
   140032575:	cmp    $0x9,%cl
   140032578:	seta   %cl
   14003257b:	and    %r9d,%ecx
   14003257e:	or     %ecx,%eax
   140032580:	lea    0x5(%rdx),%rcx
   140032584:	cmp    %rcx,%r8
   140032587:	je     1400325d6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   140032589:	movzbl 0x5(%rdx),%ecx
   14003258d:	mov    %ecx,%r9d
   140032590:	or     $0x20,%r9d
   140032594:	sub    $0x61,%r9d
   140032598:	cmp    $0x5,%r9b
   14003259c:	seta   %r9b
   1400325a0:	sub    $0x30,%ecx
   1400325a3:	cmp    $0x9,%cl
   1400325a6:	seta   %cl
   1400325a9:	and    %r9d,%ecx
   1400325ac:	or     %ecx,%eax
   1400325ae:	lea    0x6(%rdx),%rcx
   1400325b2:	cmp    %rcx,%r8
   1400325b5:	je     1400325d6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   1400325b7:	movzbl 0x6(%rdx),%edx
   1400325bb:	mov    %edx,%ecx
   1400325bd:	or     $0x20,%ecx
   1400325c0:	sub    $0x61,%ecx
   1400325c3:	cmp    $0x5,%cl
   1400325c6:	seta   %cl
   1400325c9:	sub    $0x30,%edx
   1400325cc:	cmp    $0x9,%dl
   1400325cf:	seta   %dl
   1400325d2:	and    %ecx,%edx
   1400325d4:	or     %edx,%eax
   1400325d6:	test   %al,%al
   1400325d8:	jne    140032dc5 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xb45>
   1400325de:	movq   $0x0,0x210(%rsp)
   1400325ea:	pxor   %xmm0,%xmm0
   1400325ee:	movups %xmm0,0x200(%rsp)
   1400325f6:	test   %r13,%r13
   1400325f9:	je     140032898 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x618>
   1400325ff:	mov    %r13,%rcx
   140032602:	call   14006cfd0 <_Znwy>
   140032607:	mov    $0x1,%edx
   14003260c:	movb   $0x0,(%rax)
   14003260f:	mov    %rax,%rbp
   140032612:	lea    (%rax,%r13,1),%r14
   140032616:	lea    0x1(%rax),%r12
   14003261a:	lea    0x596ff(%rip),%rax        # 14008bd20 <_ZN12tx_generated12_GLOBAL__N_1L10hex_valuesE>
   140032621:	sub    $0x1,%r13
   140032625:	jne    140032950 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x6d0>
   14003262b:	movzbl (%rsi,%r13,2),%r8d
   140032630:	movzbl 0x1(%rsi,%r13,2),%r9d
   140032636:	lea    0x0(%r13,%r13,1),%rcx
   14003263b:	mov    (%rax,%r8,4),%r8d
   14003263f:	shl    $0x4,%r8d
   140032643:	or     (%rax,%r9,4),%r8b
   140032647:	mov    %r8b,0x0(%rbp,%r13,1)
   14003264c:	lea    0x1(%r13),%r8
   140032650:	cmp    %rdx,%r8
   140032653:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   140032659:	movzbl 0x2(%rcx,%rsi,1),%r8d
   14003265f:	movzbl 0x3(%rcx,%rsi,1),%r9d
   140032665:	mov    (%rax,%r8,4),%r8d
   140032669:	shl    $0x4,%r8d
   14003266d:	or     (%rax,%r9,4),%r8b
   140032671:	mov    %r8b,0x1(%rbp,%r13,1)
   140032676:	lea    0x2(%r13),%r8
   14003267a:	cmp    %rdx,%r8
   14003267d:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   140032683:	movzbl 0x4(%rcx,%rsi,1),%r8d
   140032689:	movzbl 0x5(%rcx,%rsi,1),%r9d
   14003268f:	mov    (%rax,%r8,4),%r8d
   140032693:	shl    $0x4,%r8d
   140032697:	or     (%rax,%r9,4),%r8b
   14003269b:	mov    %r8b,0x2(%rbp,%r13,1)
   1400326a0:	lea    0x3(%r13),%r8
   1400326a4:	cmp    %rdx,%r8
   1400326a7:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   1400326ad:	movzbl 0x6(%rcx,%rsi,1),%r8d
   1400326b3:	movzbl 0x7(%rcx,%rsi,1),%r9d
   1400326b9:	mov    (%rax,%r8,4),%r8d
   1400326bd:	shl    $0x4,%r8d
   1400326c1:	or     (%rax,%r9,4),%r8b
   1400326c5:	mov    %r8b,0x3(%rbp,%r13,1)
   1400326ca:	lea    0x4(%r13),%r8
   1400326ce:	cmp    %rdx,%r8
   1400326d1:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   1400326d7:	movzbl 0x8(%rcx,%rsi,1),%r8d
   1400326dd:	movzbl 0x9(%rcx,%rsi,1),%r9d
   1400326e3:	mov    (%rax,%r8,4),%r8d
   1400326e7:	shl    $0x4,%r8d
   1400326eb:	or     (%rax,%r9,4),%r8b
   1400326ef:	mov    %r8b,0x4(%rbp,%r13,1)
   1400326f4:	lea    0x5(%r13),%r8
   1400326f8:	cmp    %rdx,%r8
   1400326fb:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   140032701:	movzbl 0xa(%rcx,%rsi,1),%r8d
   140032707:	movzbl 0xb(%rcx,%rsi,1),%r9d
   14003270d:	mov    (%rax,%r8,4),%r8d
   140032711:	shl    $0x4,%r8d
   140032715:	or     (%rax,%r9,4),%r8b
   140032719:	mov    %r8b,0x5(%rbp,%r13,1)
   14003271e:	lea    0x6(%r13),%r8
   140032722:	cmp    %rdx,%r8
   140032725:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   14003272b:	movzbl 0xc(%rcx,%rsi,1),%r8d
   140032731:	movzbl 0xd(%rcx,%rsi,1),%r9d
   140032737:	mov    (%rax,%r8,4),%r8d
   14003273b:	shl    $0x4,%r8d
   14003273f:	or     (%rax,%r9,4),%r8b
   140032743:	mov    %r8b,0x6(%rbp,%r13,1)
   140032748:	lea    0x7(%r13),%r8
   14003274c:	cmp    %rdx,%r8
   14003274f:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   140032755:	movzbl 0xe(%rcx,%rsi,1),%r8d
   14003275b:	movzbl 0xf(%rcx,%rsi,1),%r9d
   140032761:	mov    (%rax,%r8,4),%r8d
   140032765:	shl    $0x4,%r8d
   140032769:	or     (%rax,%r9,4),%r8b
   14003276d:	mov    %r8b,0x7(%rbp,%r13,1)
   140032772:	lea    0x8(%r13),%r8
   140032776:	cmp    %rdx,%r8
   140032779:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   14003277f:	movzbl 0x10(%rcx,%rsi,1),%r8d
   140032785:	movzbl 0x11(%rcx,%rsi,1),%r9d
   14003278b:	mov    (%rax,%r8,4),%r8d
   14003278f:	shl    $0x4,%r8d
   140032793:	or     (%rax,%r9,4),%r8b
   140032797:	mov    %r8b,0x8(%rbp,%r13,1)
   14003279c:	lea    0x9(%r13),%r8
   1400327a0:	cmp    %rdx,%r8
   1400327a3:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   1400327a9:	movzbl 0x12(%rcx,%rsi,1),%r8d
   1400327af:	movzbl 0x13(%rcx,%rsi,1),%r9d
   1400327b5:	mov    (%rax,%r8,4),%r8d
   1400327b9:	shl    $0x4,%r8d
   1400327bd:	or     (%rax,%r9,4),%r8b
   1400327c1:	mov    %r8b,0x9(%rbp,%r13,1)
   1400327c6:	lea    0xa(%r13),%r8
   1400327ca:	cmp    %rdx,%r8
   1400327cd:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   1400327d3:	movzbl 0x14(%rcx,%rsi,1),%r8d
   1400327d9:	movzbl 0x15(%rcx,%rsi,1),%r9d
   1400327df:	mov    (%rax,%r8,4),%r8d
   1400327e3:	shl    $0x4,%r8d
   1400327e7:	or     (%rax,%r9,4),%r8b
   1400327eb:	mov    %r8b,0xa(%rbp,%r13,1)
   1400327f0:	lea    0xb(%r13),%r8
   1400327f4:	cmp    %rdx,%r8
   1400327f7:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   1400327fd:	movzbl 0x16(%rcx,%rsi,1),%r8d
   140032803:	movzbl 0x17(%rcx,%rsi,1),%r9d
   140032809:	mov    (%rax,%r8,4),%r8d
   14003280d:	shl    $0x4,%r8d
   140032811:	or     (%rax,%r9,4),%r8b
   140032815:	mov    %r8b,0xb(%rbp,%r13,1)
   14003281a:	lea    0xc(%r13),%r8
   14003281e:	cmp    %rdx,%r8
   140032821:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   140032823:	movzbl 0x18(%rcx,%rsi,1),%r8d
   140032829:	movzbl 0x19(%rcx,%rsi,1),%r9d
   14003282f:	mov    (%rax,%r8,4),%r8d
   140032833:	shl    $0x4,%r8d
   140032837:	or     (%rax,%r9,4),%r8b
   14003283b:	mov    %r8b,0xc(%rbp,%r13,1)
   140032840:	lea    0xd(%r13),%r8
   140032844:	cmp    %rdx,%r8
   140032847:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   140032849:	movzbl 0x1a(%rcx,%rsi,1),%r8d
   14003284f:	movzbl 0x1b(%rcx,%rsi,1),%r9d
   140032855:	mov    (%rax,%r8,4),%r8d
   140032859:	shl    $0x4,%r8d
   14003285d:	or     (%rax,%r9,4),%r8b
   140032861:	mov    %r8b,0xd(%rbp,%r13,1)
   140032866:	lea    0xe(%r13),%r8
   14003286a:	cmp    %rdx,%r8
   14003286d:	jae    140032890 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x610>
   14003286f:	movzbl 0x1c(%rcx,%rsi,1),%r8d
   140032875:	movzbl 0x1d(%rcx,%rsi,1),%ecx
   14003287a:	mov    (%rax,%r8,4),%r8d
   14003287e:	shl    $0x4,%r8d
   140032882:	or     (%rax,%rcx,4),%r8b
   140032886:	mov    %r8b,0xe(%rbp,%r13,1)
   14003288b:	nopl   0x0(%rax,%rax,1)
   140032890:	mov    %rdx,%rdi
   140032893:	jmp    1400328a0 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x620>
   140032895:	nopl   (%rax)
   140032898:	xor    %r14d,%r14d
   14003289b:	xor    %ebp,%ebp
   14003289d:	xor    %r12d,%r12d
   1400328a0:	pxor   %xmm0,%xmm0
   1400328a4:	mov    %rdi,%rcx
   1400328a7:	mov    %rbp,0x220(%rsp)
   1400328af:	mov    %r12,0x228(%rsp)
   1400328b7:	mov    %r14,0x230(%rsp)
   1400328bf:	movq   $0x0,0x210(%rsp)
   1400328cb:	movups %xmm0,0x200(%rsp)
   1400328d3:	call   14002f6b0 <_ZN12tx_generated12_GLOBAL__N_112require_sizeEy>
   1400328d8:	movq   $0x0,(%r15)
   1400328df:	mov    $0x28,%ecx
   1400328e4:	call   14006cfd0 <_Znwy>
   1400328e9:	mov    0x59898(%rip),%rdx        # 14008c188 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x18>
   1400328f0:	lea    0x623b9(%rip),%rbx        # 140094cb0 <_ZTVSt23_Sp_counted_ptr_inplaceIKSt6vectorIhSaIhEESaIvELN9__gnu_cxx12_Lock_policyE2EE+0x10>
   1400328f7:	mov    %rbp,0x10(%rax)
   1400328fb:	mov    %rbx,(%rax)
   1400328fe:	mov    %rdx,0x8(%rax)
   140032902:	mov    %r12,0x18(%rax)
   140032906:	mov    %r14,0x20(%rax)
   14003290a:	mov    %rax,0x8(%r15)
   14003290e:	add    $0x10,%rax
   140032912:	mov    %rax,(%r15)
   140032915:	movups 0x270(%rsp),%xmm6
   14003291d:	mov    %r15,%rax
   140032920:	movups 0x280(%rsp),%xmm7
   140032928:	movups 0x290(%rsp),%xmm8
   140032931:	movups 0x2a0(%rsp),%xmm9
   14003293a:	add    $0x2b8,%rsp
   140032941:	pop    %rbx
   140032942:	pop    %rsi
   140032943:	pop    %rdi
   140032944:	pop    %rbp
   140032945:	pop    %r12
   140032947:	pop    %r13
   140032949:	pop    %r14
   14003294b:	pop    %r15
   14003294d:	ret
   14003294e:	xchg   %ax,%ax
   140032950:	mov    %r12,%rcx
   140032953:	mov    %r14,%r12
   140032956:	xor    %edx,%edx
   140032958:	mov    %r13,%r8
   14003295b:	call   140074c78 <memset>
   140032960:	mov    %r12,%rdx
   140032963:	sub    %rbp,%rdx
   140032966:	je     140032da9 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xb29>
   14003296c:	lea    -0x1(%rdx),%rax
   140032970:	cmp    $0xe,%rax
   140032974:	jbe    140032db3 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xb33>
   14003297a:	mov    %rdx,%r13
   14003297d:	movdqu 0x5986b(%rip),%xmm4        # 14008c1f0 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x80>
   140032985:	lea    0x59394(%rip),%rax        # 14008bd20 <_ZN12tx_generated12_GLOBAL__N_1L10hex_valuesE>
   14003298c:	and    $0xfffffffffffffff0,%r13
   140032990:	movdqu (%rsi,%rdi,2),%xmm2
   140032995:	movdqu 0x10(%rsi,%rdi,2),%xmm1
   14003299b:	movups %xmm2,0x1e0(%rsp)
   1400329a3:	movd   %xmm2,%ecx
   1400329a7:	movzbl 0x1e4(%rsp),%r9d
   1400329b0:	movups %xmm2,0x1d0(%rsp)
   1400329b8:	movzbl %cl,%ecx
   1400329bb:	movzbl 0x1d6(%rsp),%r10d
   1400329c4:	movups %xmm2,0x1f0(%rsp)
   1400329cc:	movd   (%rax,%r9,4),%xmm3
   1400329d2:	movzbl 0x1f2(%rsp),%r8d
   1400329db:	movd   (%rax,%r10,4),%xmm0
   1400329e1:	movd   (%rax,%r8,4),%xmm5
   1400329e7:	punpckldq %xmm0,%xmm3
   1400329eb:	movd   (%rax,%rcx,4),%xmm0
   1400329f0:	movups %xmm2,0x1a0(%rsp)
   1400329f8:	movzbl 0x1ac(%rsp),%r9d
   140032a01:	movups %xmm2,0x190(%rsp)
   140032a09:	movzbl 0x19e(%rsp),%r10d
   140032a12:	punpckldq %xmm5,%xmm0
   140032a16:	movups %xmm2,0x1c0(%rsp)
   140032a1e:	movzbl 0x1c8(%rsp),%ecx
   140032a26:	movups %xmm2,0x1b0(%rsp)
   140032a2e:	punpcklqdq %xmm3,%xmm0
   140032a32:	movd   (%rax,%r9,4),%xmm5
   140032a38:	movd   (%rax,%r10,4),%xmm3
   140032a3e:	movzbl 0x1ba(%rsp),%r8d
   140032a47:	punpckldq %xmm3,%xmm5
   140032a4b:	movd   (%rax,%rcx,4),%xmm3
   140032a50:	movd   %xmm1,%ecx
   140032a54:	movd   (%rax,%r8,4),%xmm6
   140032a5a:	movups %xmm1,0x170(%rsp)
   140032a62:	movzbl %cl,%ecx
   140032a65:	movzbl 0x174(%rsp),%r9d
   140032a6e:	movups %xmm1,0x160(%rsp)
   140032a76:	movzbl 0x166(%rsp),%r10d
   140032a7f:	punpckldq %xmm6,%xmm3
   140032a83:	movups %xmm1,0x180(%rsp)
   140032a8b:	movzbl 0x182(%rsp),%r8d
   140032a94:	punpcklqdq %xmm5,%xmm3
   140032a98:	movdqa %xmm0,%xmm5
   140032a9c:	punpcklwd %xmm3,%xmm0
   140032aa0:	punpckhwd %xmm3,%xmm5
   140032aa4:	movdqa %xmm0,%xmm3
   140032aa8:	punpcklwd %xmm5,%xmm0
   140032aac:	punpckhwd %xmm5,%xmm3
   140032ab0:	movd   (%rax,%r9,4),%xmm5
   140032ab6:	punpcklwd %xmm3,%xmm0
   140032aba:	movd   (%rax,%r10,4),%xmm3
   140032ac0:	movd   (%rax,%r8,4),%xmm6
   140032ac6:	pand   %xmm4,%xmm0
   140032aca:	punpckldq %xmm3,%xmm5
   140032ace:	movd   (%rax,%rcx,4),%xmm3
   140032ad3:	movups %xmm1,0x130(%rsp)
   140032adb:	movzbl 0x13c(%rsp),%r9d
   140032ae4:	movups %xmm1,0x120(%rsp)
   140032aec:	movzbl 0x12e(%rsp),%r10d
   140032af5:	punpckldq %xmm6,%xmm3
   140032af9:	movups %xmm1,0x150(%rsp)
   140032b01:	movzbl 0x158(%rsp),%ecx
   140032b09:	movups %xmm1,0x140(%rsp)
   140032b11:	punpcklqdq %xmm5,%xmm3
   140032b15:	movd   (%rax,%r9,4),%xmm6
   140032b1b:	movd   (%rax,%r10,4),%xmm5
   140032b21:	movzbl 0x14a(%rsp),%r8d
   140032b2a:	punpckldq %xmm5,%xmm6
   140032b2e:	movd   (%rax,%rcx,4),%xmm5
   140032b33:	movd   (%rax,%r8,4),%xmm7
   140032b39:	movups %xmm2,0xf0(%rsp)
   140032b41:	movzbl 0xf5(%rsp),%r9d
   140032b4a:	movups %xmm2,0xe0(%rsp)
   140032b52:	movzbl 0xe7(%rsp),%r10d
   140032b5b:	punpckldq %xmm7,%xmm5
   140032b5f:	movups %xmm2,0x110(%rsp)
   140032b67:	movzbl 0x111(%rsp),%ecx
   140032b6f:	punpcklqdq %xmm6,%xmm5
   140032b73:	movdqa %xmm3,%xmm6
   140032b77:	movups %xmm2,0x100(%rsp)
   140032b7f:	movzbl 0x103(%rsp),%r8d
   140032b88:	punpcklwd %xmm5,%xmm3
   140032b8c:	punpckhwd %xmm5,%xmm6
   140032b90:	movdqa %xmm3,%xmm5
   140032b94:	punpcklwd %xmm6,%xmm3
   140032b98:	punpckhwd %xmm6,%xmm5
   140032b9c:	movd   (%rax,%r8,4),%xmm6
   140032ba2:	punpcklwd %xmm5,%xmm3
   140032ba6:	movd   (%rax,%r10,4),%xmm5
   140032bac:	pand   %xmm4,%xmm3
   140032bb0:	packuswb %xmm3,%xmm0
   140032bb4:	movd   (%rax,%r9,4),%xmm3
   140032bba:	paddb  %xmm0,%xmm0
   140032bbe:	punpckldq %xmm5,%xmm3
   140032bc2:	movd   (%rax,%rcx,4),%xmm5
   140032bc7:	paddb  %xmm0,%xmm0
   140032bcb:	movups %xmm2,0xd0(%rsp)
   140032bd3:	movups %xmm2,0xc0(%rsp)
   140032bdb:	movzbl 0xd9(%rsp),%ecx
   140032be3:	paddb  %xmm0,%xmm0
   140032be7:	movzbl 0xcb(%rsp),%r8d
   140032bf0:	movups %xmm2,0xb0(%rsp)
   140032bf8:	punpckldq %xmm6,%xmm5
   140032bfc:	paddb  %xmm0,%xmm0
   140032c00:	movzbl 0xbd(%rsp),%r9d
   140032c09:	movups %xmm2,0xa0(%rsp)
   140032c11:	punpcklqdq %xmm3,%xmm5
   140032c15:	movzbl 0xaf(%rsp),%r10d
   140032c1e:	movd   (%rax,%r8,4),%xmm6
   140032c24:	movd   (%rax,%r9,4),%xmm2
   140032c2a:	movd   (%rax,%r10,4),%xmm3
   140032c30:	punpckldq %xmm3,%xmm2
   140032c34:	movd   (%rax,%rcx,4),%xmm3
   140032c39:	movups %xmm1,0x70(%rsp)
   140032c3e:	movzbl 0x75(%rsp),%r9d
   140032c44:	movups %xmm1,0x60(%rsp)
   140032c49:	movzbl 0x67(%rsp),%r10d
   140032c4f:	punpckldq %xmm6,%xmm3
   140032c53:	movups %xmm1,0x90(%rsp)
   140032c5b:	movzbl 0x91(%rsp),%ecx
   140032c63:	punpcklqdq %xmm2,%xmm3
   140032c67:	movdqa %xmm5,%xmm2
   140032c6b:	movups %xmm1,0x80(%rsp)
   140032c73:	movzbl 0x83(%rsp),%r8d
   140032c7c:	punpcklwd %xmm3,%xmm5
   140032c80:	punpckhwd %xmm3,%xmm2
   140032c84:	movdqa %xmm5,%xmm3
   140032c88:	punpcklwd %xmm2,%xmm5
   140032c8c:	movd   (%rax,%r8,4),%xmm6
   140032c92:	punpckhwd %xmm2,%xmm3
   140032c96:	movd   (%rax,%r10,4),%xmm2
   140032c9c:	punpcklwd %xmm3,%xmm5
   140032ca0:	movd   (%rax,%r9,4),%xmm3
   140032ca6:	pand   %xmm4,%xmm5
   140032caa:	punpckldq %xmm2,%xmm3
   140032cae:	movd   (%rax,%rcx,4),%xmm2
   140032cb3:	movups %xmm1,0x30(%rsp)
   140032cb8:	movzbl 0x3d(%rsp),%r9d
   140032cbe:	movups %xmm1,0x20(%rsp)
   140032cc3:	movzbl 0x2f(%rsp),%r10d
   140032cc9:	punpckldq %xmm6,%xmm2
   140032ccd:	movups %xmm1,0x50(%rsp)
   140032cd2:	movzbl 0x59(%rsp),%ecx
   140032cd7:	movups %xmm1,0x40(%rsp)
   140032cdc:	punpcklqdq %xmm3,%xmm2
   140032ce0:	movzbl 0x4b(%rsp),%r8d
   140032ce6:	movd   (%rax,%r10,4),%xmm3
   140032cec:	movd   (%rax,%r9,4),%xmm1
   140032cf2:	movd   (%rax,%r8,4),%xmm6
   140032cf8:	punpckldq %xmm3,%xmm1
   140032cfc:	movd   (%rax,%rcx,4),%xmm3
   140032d01:	punpckldq %xmm6,%xmm3
   140032d05:	movdqa %xmm2,%xmm6
   140032d09:	punpcklqdq %xmm1,%xmm3
   140032d0d:	movdqa %xmm2,%xmm1
   140032d11:	punpcklwd %xmm3,%xmm1
   140032d15:	punpckhwd %xmm3,%xmm6
   140032d19:	movdqa %xmm1,%xmm2
   140032d1d:	punpcklwd %xmm6,%xmm1
   140032d21:	punpckhwd %xmm6,%xmm2
   140032d25:	punpcklwd %xmm2,%xmm1
   140032d29:	movdqa %xmm5,%xmm2
   140032d2d:	pand   %xmm4,%xmm1
   140032d31:	packuswb %xmm1,%xmm2
   140032d35:	por    %xmm2,%xmm0
   140032d39:	movups %xmm0,0x0(%rbp,%rdi,1)
   140032d3e:	add    $0x10,%rdi
   140032d42:	cmp    %r13,%rdi
   140032d45:	jne    140032990 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x710>
   140032d4b:	mov    %r12,%r14
   140032d4e:	cmp    %r13,%rdx
   140032d51:	jne    14003262b <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x3ab>
   140032d57:	jmp    1400328a0 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x620>
   140032d5c:	xor    %r11d,%r11d
   140032d5f:	mov    %rsi,%rdx
   140032d62:	pxor   %xmm4,%xmm4
   140032d66:	xor    %eax,%eax
   140032d68:	jmp    1400323c7 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x147>
   140032d6d:	test   %r10b,%r10b
   140032d70:	jne    140032dc5 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xb45>
   140032d72:	mov    %r13,%rcx
   140032d75:	pxor   %xmm0,%xmm0
   140032d79:	movq   $0x0,0x210(%rsp)
   140032d85:	movups %xmm0,0x200(%rsp)
   140032d8d:	call   14006cfd0 <_Znwy>
   140032d92:	lea    (%rax,%r13,1),%r12
   140032d96:	movb   $0x0,(%rax)
   140032d99:	mov    %rax,%rbp
   140032d9c:	lea    0x1(%rax),%rcx
   140032da0:	sub    $0x1,%r13
   140032da4:	jmp    140032956 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x6d6>
   140032da9:	mov    %r12,%r14
   140032dac:	xor    %edi,%edi
   140032dae:	jmp    1400328a0 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x620>
   140032db3:	mov    %r12,%r14
   140032db6:	xor    %r13d,%r13d
   140032db9:	lea    0x58f60(%rip),%rax        # 14008bd20 <_ZN12tx_generated12_GLOBAL__N_1L10hex_valuesE>
   140032dc0:	jmp    14003262b <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x3ab>
   140032dc5:	mov    $0x58,%ecx
   140032dca:	lea    0x224(%rsp),%rdi
   140032dd2:	call   14006cfc8 <__cxa_allocate_exception>
   140032dd7:	mov    $0x11,%ecx
   140032ddc:	lea    0x58e9a(%rip),%rdx        # 14008bc7d <.rdata+0x23d>
   140032de3:	mov    %rax,%rsi
   140032de6:	xor    %eax,%eax
   140032de8:	rep stos %eax,%es:(%rdi)
   140032dea:	lea    0x228(%rsp),%rdi
   140032df2:	movl   $0x2,0x220(%rsp)
   140032dfd:	mov    %rdi,%rcx
   140032e00:	call   140085670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   140032e05:	lea    0x248(%rsp),%rbp
   140032e0d:	lea    0x58ea4(%rip),%rdx        # 14008bcb8 <.rdata+0x278>
   140032e14:	mov    %rbp,%rcx
   140032e17:	call   140085670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   140032e1c:	lea    0x220(%rsp),%rdx
   140032e24:	mov    %rsi,%rcx
   140032e27:	call   14007b030 <_ZN12tx_generated15runtime_failureC1ENS_10error_infoE>
   140032e2c:	mov    %rbp,%rcx
   140032e2f:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   140032e34:	mov    %rdi,%rcx
   140032e37:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   140032e3c:	lea    0x4847d(%rip),%r8        # 14007b2c0 <_ZN12tx_generated15runtime_failureD1Ev>
   140032e43:	lea    0x5bb26(%rip),%rdx        # 14008e970 <_ZTIN12tx_generated15runtime_failureE>
   140032e4a:	mov    %rsi,%rcx
   140032e4d:	call   14006cf88 <__cxa_throw>
   140032e52:	lea    0x220(%rsp),%rcx
   140032e5a:	mov    %rax,%rbx
   140032e5d:	call   140081640 <_ZNSt6vectorIhSaIhEED1Ev>
   140032e62:	lea    0x200(%rsp),%rcx
   140032e6a:	call   140081640 <_ZNSt6vectorIhSaIhEED1Ev>
   140032e6f:	mov    %rbx,%rcx
   140032e72:	call   14006e8e8 <_Unwind_Resume>
   140032e77:	mov    $0x58,%ecx
   140032e7c:	lea    0x224(%rsp),%rdi
   140032e84:	call   14006cfc8 <__cxa_allocate_exception>
   140032e89:	mov    $0x11,%ecx
   140032e8e:	lea    0x58de8(%rip),%rdx        # 14008bc7d <.rdata+0x23d>
   140032e95:	mov    %rax,%rsi
   140032e98:	xor    %eax,%eax
   140032e9a:	rep stos %eax,%es:(%rdi)
   140032e9c:	lea    0x228(%rsp),%rdi
   140032ea4:	movl   $0x2,0x220(%rsp)
   140032eaf:	mov    %rdi,%rcx
   140032eb2:	call   140085670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   140032eb7:	lea    0x248(%rsp),%rbp
   140032ebf:	lea    0x58dca(%rip),%rdx        # 14008bc90 <.rdata+0x250>
   140032ec6:	mov    %rbp,%rcx
   140032ec9:	call   140085670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   140032ece:	lea    0x220(%rsp),%rdx
   140032ed6:	mov    %rsi,%rcx
   140032ed9:	call   14007b030 <_ZN12tx_generated15runtime_failureC1ENS_10error_infoE>
   140032ede:	jmp    140032e2c <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xbac>
   140032ee3:	mov    %rbp,%rcx
   140032ee6:	mov    %rax,%rbx
   140032ee9:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   140032eee:	mov    %rdi,%rcx
   140032ef1:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   140032ef6:	mov    %rsi,%rcx
   140032ef9:	call   14006cfb0 <__cxa_free_exception>
   140032efe:	mov    %rbx,%rcx
   140032f01:	call   14006e8e8 <_Unwind_Resume>
   140032f06:	jmp    140032ee3 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xc63>
   140032f08:	mov    %rdi,%rcx
   140032f0b:	mov    %rax,%rbx
   140032f0e:	call   140081670 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   140032f13:	jmp    140032ef6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xc76>
   140032f15:	mov    %rax,%rbx
   140032f18:	jmp    140032ef6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xc76>
   140032f1a:	jmp    140032f08 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xc88>
   140032f1c:	jmp    140032f15 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xc95>
   140032f1e:	xchg   %ax,%ax


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001830 <tx_fn_m0_hex_paths_0>:
   140001830:	push   %r15
   140001832:	push   %r14
   140001834:	push   %r13
   140001836:	push   %r12
   140001838:	push   %rsi
   140001839:	push   %rdi
   14000183a:	push   %rbp
   14000183b:	push   %rbx
   14000183c:	sub    $0xf8,%rsp
   140001843:	mov    %r9,0x40(%rsp)
   140001848:	mov    %r8,%r12
   14000184b:	mov    %rdx,%r15
   14000184e:	mov    %rcx,%rsi
   140001851:	mov    (%rcx),%r14
   140001854:	lea    0x865c0(%rip),%rax        # 140087e1b <.rdata+0xe1b>
   14000185b:	mov    %rax,0x60(%rsp)
   140001860:	lea    0x865c9(%rip),%rax        # 140087e30 <.rdata+0xe30>
   140001867:	mov    %rax,0x68(%rsp)
   14000186c:	movq   $0xd,0x70(%rsp)
   140001875:	movq   $0x1,0x78(%rsp)
   14000187e:	mov    %r14,0x80(%rsp)
   140001886:	lea    0x60(%rsp),%rax
   14000188b:	mov    %rax,(%rcx)
   14000188e:	lea    0xe0(%rsp),%r8
   140001896:	xor    %ecx,%ecx
   140001898:	xor    %edx,%edx
   14000189a:	call   14001be60 <txrt_vector_new_i64>
   14000189f:	test   %eax,%eax
   1400018a1:	jne    140001d6c <tx_fn_m0_hex_paths_0+0x53c>
   1400018a7:	mov    0xe0(%rsp),%r13
   1400018af:	mov    %r13,%rcx
   1400018b2:	call   14001b8d0 <txrt_vector_ref_i64>
   1400018b7:	mov    %rsi,%rcx
   1400018ba:	call   140016360 <txrt_gc_safepoint_context>
   1400018bf:	test   %eax,%eax
   1400018c1:	jne    140001d75 <tx_fn_m0_hex_paths_0+0x545>
   1400018c7:	test   %r15,%r15
   1400018ca:	jle    140001920 <tx_fn_m0_hex_paths_0+0xf0>
   1400018cc:	mov    %r13,%rcx
   1400018cf:	xor    %edx,%edx
   1400018d1:	call   14001c2f0 <txrt_vector_push_back_i64>
   1400018d6:	test   %eax,%eax
   1400018d8:	jne    14000190c <tx_fn_m0_hex_paths_0+0xdc>
   1400018da:	mov    $0x1,%edi
   1400018df:	nop
   1400018e0:	mov    %rsi,%rcx
   1400018e3:	call   140016360 <txrt_gc_safepoint_context>
   1400018e8:	test   %eax,%eax
   1400018ea:	jne    140001cbd <tx_fn_m0_hex_paths_0+0x48d>
   1400018f0:	cmp    %rdi,%r15
   1400018f3:	je     140001920 <tx_fn_m0_hex_paths_0+0xf0>
   1400018f5:	lea    0x1(%rdi),%rbx
   1400018f9:	movzbl %dil,%edx
   1400018fd:	mov    %r13,%rcx
   140001900:	call   14001c2f0 <txrt_vector_push_back_i64>
   140001905:	mov    %rbx,%rdi
   140001908:	test   %eax,%eax
   14000190a:	je     1400018e0 <tx_fn_m0_hex_paths_0+0xb0>
   14000190c:	mov    %eax,%edi
   14000190e:	lea    0x85b8b(%rip),%rdx        # 1400874a0 <.rdata+0x4a0>
   140001915:	mov    $0x12,%r8d
   14000191b:	jmp    140001d3f <tx_fn_m0_hex_paths_0+0x50f>
   140001920:	lea    0xd8(%rsp),%rdx
   140001928:	mov    %r13,%rcx
   14000192b:	call   14000b430 <txrt_value_clone>
   140001930:	test   %eax,%eax
   140001932:	jne    140001d87 <tx_fn_m0_hex_paths_0+0x557>
   140001938:	mov    0xd8(%rsp),%rdi
   140001940:	lea    0xd0(%rsp),%rdx
   140001948:	mov    %rdi,%rcx
   14000194b:	call   140020810 <txrt_bytes_from_vector>
   140001950:	test   %eax,%eax
   140001952:	jne    140001d99 <tx_fn_m0_hex_paths_0+0x569>
   140001958:	mov    %rdi,%rcx
   14000195b:	call   14000b5c0 <txrt_value_release>
   140001960:	mov    0xd0(%rsp),%rdi
   140001968:	mov    %rsi,%rcx
   14000196b:	call   140016360 <txrt_gc_safepoint_context>
   140001970:	test   %eax,%eax
   140001972:	jne    140001dab <tx_fn_m0_hex_paths_0+0x57b>
   140001978:	lea    0xc8(%rsp),%rdx
   140001980:	mov    %rdi,%rcx
   140001983:	call   140020ae0 <txrt_bytes_to_hex>
   140001988:	test   %eax,%eax
   14000198a:	jne    140001dbd <tx_fn_m0_hex_paths_0+0x58d>
   140001990:	mov    0xc8(%rsp),%rax
   140001998:	mov    %rax,0x30(%rsp)
   14000199d:	mov    %rsi,%rcx
   1400019a0:	call   140016360 <txrt_gc_safepoint_context>
   1400019a5:	test   %eax,%eax
   1400019a7:	jne    140001dcc <tx_fn_m0_hex_paths_0+0x59c>
   1400019ad:	lea    0xc0(%rsp),%rcx
   1400019b5:	call   140010990 <txrt_time_monotonic_micros>
   1400019ba:	test   %eax,%eax
   1400019bc:	jne    140001ddb <tx_fn_m0_hex_paths_0+0x5ab>
   1400019c2:	mov    0xc0(%rsp),%rbx
   1400019ca:	mov    %rsi,%rcx
   1400019cd:	call   140016360 <txrt_gc_safepoint_context>
   1400019d2:	test   %eax,%eax
   1400019d4:	jne    140001dea <tx_fn_m0_hex_paths_0+0x5ba>
   1400019da:	test   %r12,%r12
   1400019dd:	mov    %r14,0x38(%rsp)
   1400019e2:	mov    %r13,0x90(%rsp)
   1400019ea:	mov    %rdi,0x48(%rsp)
   1400019ef:	jle    140001ab2 <tx_fn_m0_hex_paths_0+0x282>
   1400019f5:	mov    %rbx,0x88(%rsp)
   1400019fd:	lea    0x58(%rsp),%rdx
   140001a02:	mov    %rdi,%rcx
   140001a05:	call   140020ae0 <txrt_bytes_to_hex>
   140001a0a:	test   %eax,%eax
   140001a0c:	jne    140001a7d <tx_fn_m0_hex_paths_0+0x24d>
   140001a0e:	xor    %edi,%edi
   140001a10:	lea    0xb8(%rsp),%rbx
   140001a18:	lea    0x58(%rsp),%r13
   140001a1d:	mov    %r12,%r14
   140001a20:	mov    0x58(%rsp),%r15
   140001a25:	mov    %r15,%rcx
   140001a28:	mov    %rbx,%rdx
   140001a2b:	call   140026dc0 <txrt_str_len>
   140001a30:	test   %eax,%eax
   140001a32:	jne    140001cd5 <tx_fn_m0_hex_paths_0+0x4a5>
   140001a38:	mov    %r15,%rcx
   140001a3b:	call   140026ca0 <txrt_str_release>
   140001a40:	mov    0xb8(%rsp),%rdx
   140001a48:	mov    %rdi,%rbp
   140001a4b:	add    %rdx,%rbp
   140001a4e:	jo     140001cde <tx_fn_m0_hex_paths_0+0x4ae>
   140001a54:	mov    %rsi,%rcx
   140001a57:	call   140016360 <txrt_gc_safepoint_context>
   140001a5c:	test   %eax,%eax
   140001a5e:	jne    140001cff <tx_fn_m0_hex_paths_0+0x4cf>
   140001a64:	dec    %r14
   140001a67:	je     140001aa3 <tx_fn_m0_hex_paths_0+0x273>
   140001a69:	mov    0x48(%rsp),%rcx
   140001a6e:	mov    %r13,%rdx
   140001a71:	call   140020ae0 <txrt_bytes_to_hex>
   140001a76:	mov    %rbp,%rdi
   140001a79:	test   %eax,%eax
   140001a7b:	je     140001a20 <tx_fn_m0_hex_paths_0+0x1f0>
   140001a7d:	mov    %eax,%r15d
   140001a80:	lea    0x85d79(%rip),%rdx        # 140087800 <.rdata+0x800>
   140001a87:	mov    $0x1a,%r8d
   140001a8d:	mov    $0x9,%r9d
   140001a93:	mov    %rsi,%rcx
   140001a96:	call   140021b60 <txrt_stack_error_location>
   140001a9b:	mov    %r15d,%ecx
   140001a9e:	call   1400262e0 <txrt_require_success>
   140001aa3:	mov    0x38(%rsp),%r14
   140001aa8:	mov    0x88(%rsp),%rbx
   140001ab0:	jmp    140001ab4 <tx_fn_m0_hex_paths_0+0x284>
   140001ab2:	xor    %ebp,%ebp
   140001ab4:	lea    0xb0(%rsp),%rax
   140001abc:	mov    %rax,0x20(%rsp)
   140001ac1:	lea    0x85eb3(%rip),%rdx        # 14008797b <.rdata+0x97b>
   140001ac8:	mov    $0xb,%r8d
   140001ace:	mov    0x40(%rsp),%rcx
   140001ad3:	mov    $0x1,%r9b
   140001ad6:	call   14001d120 <txrt_str_concat_literal>
   140001adb:	test   %eax,%eax
   140001add:	jne    140001df9 <tx_fn_m0_hex_paths_0+0x5c9>
   140001ae3:	mov    0xb0(%rsp),%rdi
   140001aeb:	lea    0x85efe(%rip),%rax        # 1400879f0 <.rdata+0x9f0>
   140001af2:	mov    %rax,0x68(%rsp)
   140001af7:	movq   $0x1c,0x70(%rsp)
   140001b00:	movq   $0x5,0x78(%rsp)
   140001b09:	mov    %rsi,%rcx
   140001b0c:	mov    %rdi,%rdx
   140001b0f:	mov    %rbx,%r8
   140001b12:	mov    %rbp,%r9
   140001b15:	call   1400015e0 <tx_fn_m0_report_0>
   140001b1a:	mov    %rdi,%rcx
   140001b1d:	call   140026ca0 <txrt_str_release>
   140001b22:	mov    %rsi,%rcx
   140001b25:	call   140016360 <txrt_gc_safepoint_context>
   140001b2a:	test   %eax,%eax
   140001b2c:	jne    140001e08 <tx_fn_m0_hex_paths_0+0x5d8>
   140001b32:	lea    0xa8(%rsp),%rcx
   140001b3a:	call   140010990 <txrt_time_monotonic_micros>
   140001b3f:	test   %eax,%eax
   140001b41:	jne    140001e17 <tx_fn_m0_hex_paths_0+0x5e7>
   140001b47:	mov    0xa8(%rsp),%rbx
   140001b4f:	mov    %rsi,%rcx
   140001b52:	call   140016360 <txrt_gc_safepoint_context>
   140001b57:	test   %eax,%eax
   140001b59:	jne    140001e26 <tx_fn_m0_hex_paths_0+0x5f6>
   140001b5f:	test   %r12,%r12
   140001b62:	jle    140001c05 <tx_fn_m0_hex_paths_0+0x3d5>
   140001b68:	mov    %rbx,%r14
   140001b6b:	lea    0x50(%rsp),%rdx
   140001b70:	mov    0x30(%rsp),%rcx
   140001b75:	call   14001fa10 <txrt_bytes_from_hex>
   140001b7a:	test   %eax,%eax
   140001b7c:	jne    140001bed <tx_fn_m0_hex_paths_0+0x3bd>
   140001b7e:	xor    %edi,%edi
   140001b80:	lea    0xa0(%rsp),%rbx
   140001b88:	lea    0x50(%rsp),%r13
   140001b8d:	nopl   (%rax)
   140001b90:	mov    0x50(%rsp),%r15
   140001b95:	mov    %r15,%rcx
   140001b98:	mov    %rbx,%rdx
   140001b9b:	call   14000c390 <txrt_value_len>
   140001ba0:	test   %eax,%eax
   140001ba2:	jne    140001d17 <tx_fn_m0_hex_paths_0+0x4e7>
   140001ba8:	mov    %r15,%rcx
   140001bab:	call   14000b5c0 <txrt_value_release>
   140001bb0:	mov    0xa0(%rsp),%rdx
   140001bb8:	mov    %rdi,%rbp
   140001bbb:	add    %rdx,%rbp
   140001bbe:	jo     140001d20 <tx_fn_m0_hex_paths_0+0x4f0>
   140001bc4:	mov    %rsi,%rcx
   140001bc7:	call   140016360 <txrt_gc_safepoint_context>
   140001bcc:	test   %eax,%eax
   140001bce:	jne    140001d54 <tx_fn_m0_hex_paths_0+0x524>
   140001bd4:	dec    %r12
   140001bd7:	je     140001bfb <tx_fn_m0_hex_paths_0+0x3cb>
   140001bd9:	mov    0x30(%rsp),%rcx
   140001bde:	mov    %r13,%rdx
   140001be1:	call   14001fa10 <txrt_bytes_from_hex>
   140001be6:	mov    %rbp,%rdi
   140001be9:	test   %eax,%eax
   140001beb:	je     140001b90 <tx_fn_m0_hex_paths_0+0x360>
   140001bed:	mov    %eax,%edi
   140001bef:	lea    0x85f7a(%rip),%rdx        # 140087b70 <.rdata+0xb70>
   140001bf6:	jmp    140001d39 <tx_fn_m0_hex_paths_0+0x509>
   140001bfb:	mov    %r14,%rbx
   140001bfe:	mov    0x38(%rsp),%r14
   140001c03:	jmp    140001c07 <tx_fn_m0_hex_paths_0+0x3d7>
   140001c05:	xor    %ebp,%ebp
   140001c07:	lea    0x98(%rsp),%rax
   140001c0f:	mov    %rax,0x20(%rsp)
   140001c14:	lea    0x860d0(%rip),%rdx        # 140087ceb <.rdata+0xceb>
   140001c1b:	mov    $0xb,%r8d
   140001c21:	mov    0x40(%rsp),%rcx
   140001c26:	mov    $0x1,%r9b
   140001c29:	call   14001d120 <txrt_str_concat_literal>
   140001c2e:	test   %eax,%eax
   140001c30:	jne    140001e35 <tx_fn_m0_hex_paths_0+0x605>
   140001c36:	mov    0x98(%rsp),%rdi
   140001c3e:	lea    0x8611b(%rip),%rax        # 140087d60 <.rdata+0xd60>
   140001c45:	mov    %rax,0x68(%rsp)
   140001c4a:	movq   $0x23,0x70(%rsp)
   140001c53:	movq   $0x5,0x78(%rsp)
   140001c5c:	mov    %rsi,%rcx
   140001c5f:	mov    %rdi,%rdx
   140001c62:	mov    %rbx,%r8
   140001c65:	mov    %rbp,%r9
   140001c68:	call   1400015e0 <tx_fn_m0_report_0>
   140001c6d:	mov    %rdi,%rcx
   140001c70:	call   140026ca0 <txrt_str_release>
   140001c75:	mov    %rsi,%rcx
   140001c78:	call   140016360 <txrt_gc_safepoint_context>
   140001c7d:	test   %eax,%eax
   140001c7f:	jne    140001e3e <tx_fn_m0_hex_paths_0+0x60e>
   140001c85:	mov    0x30(%rsp),%rcx
   140001c8a:	call   140026ca0 <txrt_str_release>
   140001c8f:	mov    0x90(%rsp),%rcx
   140001c97:	call   14000b5c0 <txrt_value_release>
   140001c9c:	mov    0x48(%rsp),%rcx
   140001ca1:	call   14000b5c0 <txrt_value_release>
   140001ca6:	mov    %r14,(%rsi)
   140001ca9:	add    $0xf8,%rsp
   140001cb0:	pop    %rbx
   140001cb1:	pop    %rbp
   140001cb2:	pop    %rdi
   140001cb3:	pop    %rsi
   140001cb4:	pop    %r12
   140001cb6:	pop    %r13
   140001cb8:	pop    %r14
   140001cba:	pop    %r15
   140001cbc:	ret
   140001cbd:	lea    0x8583c(%rip),%rdx        # 140087500 <.rdata+0x500>
   140001cc4:	mov    $0x12,%r8d
   140001cca:	mov    $0x9,%r9d
   140001cd0:	jmp    140001e51 <tx_fn_m0_hex_paths_0+0x621>
   140001cd5:	lea    0x85b84(%rip),%rdx        # 140087860 <.rdata+0x860>
   140001cdc:	jmp    140001d06 <tx_fn_m0_hex_paths_0+0x4d6>
   140001cde:	lea    0xf0(%rsp),%r8
   140001ce6:	mov    %rdi,%rcx
   140001ce9:	call   1400273e0 <txrt_add_i64>
   140001cee:	mov    %eax,%edi
   140001cf0:	lea    0x85bc9(%rip),%rdx        # 1400878c0 <.rdata+0x8c0>
   140001cf7:	mov    $0x1a,%r8d
   140001cfd:	jmp    140001d3f <tx_fn_m0_hex_paths_0+0x50f>
   140001cff:	lea    0x85c1a(%rip),%rdx        # 140087920 <.rdata+0x920>
   140001d06:	mov    $0x1a,%r8d
   140001d0c:	mov    $0x9,%r9d
   140001d12:	jmp    140001e51 <tx_fn_m0_hex_paths_0+0x621>
   140001d17:	lea    0x85eb2(%rip),%rdx        # 140087bd0 <.rdata+0xbd0>
   140001d1e:	jmp    140001d5b <tx_fn_m0_hex_paths_0+0x52b>
   140001d20:	lea    0xe8(%rsp),%r8
   140001d28:	mov    %rdi,%rcx
   140001d2b:	call   1400273e0 <txrt_add_i64>
   140001d30:	mov    %eax,%edi
   140001d32:	lea    0x85ef7(%rip),%rdx        # 140087c30 <.rdata+0xc30>
   140001d39:	mov    $0x21,%r8d
   140001d3f:	mov    $0x9,%r9d
   140001d45:	mov    %rsi,%rcx
   140001d48:	call   140021b60 <txrt_stack_error_location>
   140001d4d:	mov    %edi,%ecx
   140001d4f:	call   1400262e0 <txrt_require_success>
   140001d54:	lea    0x85f35(%rip),%rdx        # 140087c90 <.rdata+0xc90>
   140001d5b:	mov    $0x21,%r8d
   140001d61:	mov    $0x9,%r9d
   140001d67:	jmp    140001e51 <tx_fn_m0_hex_paths_0+0x621>
   140001d6c:	lea    0x8566d(%rip),%rdx        # 1400873e0 <.rdata+0x3e0>
   140001d73:	jmp    140001d7c <tx_fn_m0_hex_paths_0+0x54c>
   140001d75:	lea    0x856c4(%rip),%rdx        # 140087440 <.rdata+0x440>
   140001d7c:	mov    $0xf,%r8d
   140001d82:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001d87:	lea    0x857d2(%rip),%rdx        # 140087560 <.rdata+0x560>
   140001d8e:	mov    $0x14,%r8d
   140001d94:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001d99:	lea    0x85820(%rip),%rdx        # 1400875c0 <.rdata+0x5c0>
   140001da0:	mov    $0x14,%r8d
   140001da6:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001dab:	lea    0x8586e(%rip),%rdx        # 140087620 <.rdata+0x620>
   140001db2:	mov    $0x14,%r8d
   140001db8:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001dbd:	lea    0x858bc(%rip),%rdx        # 140087680 <.rdata+0x680>
   140001dc4:	mov    $0x15,%r8d
   140001dca:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001dcc:	lea    0x8590d(%rip),%rdx        # 1400876e0 <.rdata+0x6e0>
   140001dd3:	mov    $0x15,%r8d
   140001dd9:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001ddb:	lea    0x8595e(%rip),%rdx        # 140087740 <.rdata+0x740>
   140001de2:	mov    $0x17,%r8d
   140001de8:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001dea:	lea    0x859af(%rip),%rdx        # 1400877a0 <.rdata+0x7a0>
   140001df1:	mov    $0x17,%r8d
   140001df7:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001df9:	lea    0x85b90(%rip),%rdx        # 140087990 <.rdata+0x990>
   140001e00:	mov    $0x1c,%r8d
   140001e06:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001e08:	lea    0x85c41(%rip),%rdx        # 140087a50 <.rdata+0xa50>
   140001e0f:	mov    $0x1c,%r8d
   140001e15:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001e17:	lea    0x85c92(%rip),%rdx        # 140087ab0 <.rdata+0xab0>
   140001e1e:	mov    $0x1e,%r8d
   140001e24:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001e26:	lea    0x85ce3(%rip),%rdx        # 140087b10 <.rdata+0xb10>
   140001e2d:	mov    $0x1e,%r8d
   140001e33:	jmp    140001e4b <tx_fn_m0_hex_paths_0+0x61b>
   140001e35:	lea    0x85ec4(%rip),%rdx        # 140087d00 <.rdata+0xd00>
   140001e3c:	jmp    140001e45 <tx_fn_m0_hex_paths_0+0x615>
   140001e3e:	lea    0x85f7b(%rip),%rdx        # 140087dc0 <.rdata+0xdc0>
   140001e45:	mov    $0x23,%r8d
   140001e4b:	mov    $0x5,%r9d
   140001e51:	mov    %rsi,%rcx
   140001e54:	mov    %eax,%esi
   140001e56:	call   140021b60 <txrt_stack_error_location>
   140001e5b:	mov    %esi,%ecx
   140001e5d:	call   1400262e0 <txrt_require_success>
   140001e62:	int3
   140001e63:	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001f00 <tx_fn_m0_encoding_paths_0>:
   140001f00:	push   %r15
   140001f02:	push   %r14
   140001f04:	push   %r13
   140001f06:	push   %r12
   140001f08:	push   %rsi
   140001f09:	push   %rdi
   140001f0a:	push   %rbp
   140001f0b:	push   %rbx
   140001f0c:	sub    $0xf8,%rsp
   140001f13:	mov    %r9,0x48(%rsp)
   140001f18:	mov    %r8,%r13
   140001f1b:	mov    %rdx,%r15
   140001f1e:	mov    %rcx,%r14
   140001f21:	mov    (%rcx),%rcx
   140001f24:	lea    0x86950(%rip),%rax        # 14008887b <.rdata+0x187b>
   140001f2b:	mov    %rax,0x68(%rsp)
   140001f30:	lea    0x86959(%rip),%rax        # 140088890 <.rdata+0x1890>
   140001f37:	mov    %rax,0x70(%rsp)
   140001f3c:	movq   $0x26,0x78(%rsp)
   140001f45:	movq   $0x1,0x80(%rsp)
   140001f51:	mov    %rcx,0x90(%rsp)
   140001f59:	mov    %rcx,0x88(%rsp)
   140001f61:	lea    0x68(%rsp),%rax
   140001f66:	mov    %rax,(%r14)
   140001f69:	lea    0x85f1b(%rip),%rcx        # 140087e8b <.rdata+0xe8b>
   140001f70:	lea    0xe0(%rsp),%r8
   140001f78:	xor    %edx,%edx
   140001f7a:	call   1400268f0 <txrt_str_new>
   140001f7f:	test   %eax,%eax
   140001f81:	jne    1400024bd <tx_fn_m0_encoding_paths_0+0x5bd>
   140001f87:	mov    0xe0(%rsp),%rsi
   140001f8f:	mov    %r14,%rcx
   140001f92:	call   140016360 <txrt_gc_safepoint_context>
   140001f97:	test   %eax,%eax
   140001f99:	jne    1400024c6 <tx_fn_m0_encoding_paths_0+0x5c6>
   140001f9f:	test   %r15,%r15
   140001fa2:	mov    %r14,0x30(%rsp)
   140001fa7:	jle    140002094 <tx_fn_m0_encoding_paths_0+0x194>
   140001fad:	lea    0x85f97(%rip),%rcx        # 140087f4b <.rdata+0xf4b>
   140001fb4:	lea    0x60(%rsp),%r8
   140001fb9:	mov    $0xa,%edx
   140001fbe:	call   1400268f0 <txrt_str_new>
   140001fc3:	test   %eax,%eax
   140001fc5:	jne    140002076 <tx_fn_m0_encoding_paths_0+0x176>
   140001fcb:	lea    0xd0(%rsp),%rbx
   140001fd3:	lea    0x60(%rsp),%rdi
   140001fd8:	mov    %rsi,%rcx
   140001fdb:	nopl   0x0(%rax,%rax,1)
   140001fe0:	mov    0x60(%rsp),%rsi
   140001fe5:	mov    %rcx,%r12
   140001fe8:	lea    0xd8(%rsp),%rdx
   140001ff0:	call   140026b60 <txrt_str_clone>
   140001ff5:	test   %eax,%eax
   140001ff7:	jne    1400023d3 <tx_fn_m0_encoding_paths_0+0x4d3>
   140001ffd:	mov    0xd8(%rsp),%rbp
   140002005:	mov    %rbp,%rcx
   140002008:	mov    %rsi,%rdx
   14000200b:	mov    %rbx,%r8
   14000200e:	call   140027f50 <txrt_str_concat>
   140002013:	test   %eax,%eax
   140002015:	jne    1400023f7 <tx_fn_m0_encoding_paths_0+0x4f7>
   14000201b:	mov    0xd0(%rsp),%r14
   140002023:	mov    %rbp,%rcx
   140002026:	call   140026ca0 <txrt_str_release>
   14000202b:	mov    %rsi,%rcx
   14000202e:	call   140026ca0 <txrt_str_release>
   140002033:	mov    %r12,%rcx
   140002036:	call   140026ca0 <txrt_str_release>
   14000203b:	mov    0x30(%rsp),%rcx
   140002040:	call   140016360 <txrt_gc_safepoint_context>
   140002045:	test   %eax,%eax
   140002047:	jne    140002400 <tx_fn_m0_encoding_paths_0+0x500>
   14000204d:	dec    %r15
   140002050:	je     14000208a <tx_fn_m0_encoding_paths_0+0x18a>
   140002052:	mov    $0xa,%edx
   140002057:	lea    0x85eed(%rip),%rcx        # 140087f4b <.rdata+0xf4b>
   14000205e:	mov    %rdi,%r8
   140002061:	call   1400268f0 <txrt_str_new>
   140002066:	mov    %r14,%rcx
   140002069:	test   %eax,%eax
   14000206b:	mov    0x30(%rsp),%r14
   140002070:	je     140001fe0 <tx_fn_m0_encoding_paths_0+0xe0>
   140002076:	mov    %eax,%esi
   140002078:	lea    0x85ee1(%rip),%rdx        # 140087f60 <.rdata+0xf60>
   14000207f:	mov    $0x2b,%r8d
   140002085:	jmp    1400022fe <tx_fn_m0_encoding_paths_0+0x3fe>
   14000208a:	mov    %r14,%rcx
   14000208d:	mov    0x30(%rsp),%r14
   140002092:	jmp    140002097 <tx_fn_m0_encoding_paths_0+0x197>
   140002094:	mov    %rsi,%rcx
   140002097:	lea    0xc8(%rsp),%r8
   14000209f:	mov    %rcx,%rbp
   1400020a2:	xor    %edx,%edx
   1400020a4:	call   14001f290 <txrt_encoding_encode_known>
   1400020a9:	test   %eax,%eax
   1400020ab:	jne    1400024d8 <tx_fn_m0_encoding_paths_0+0x5d8>
   1400020b1:	mov    0xc8(%rsp),%rax
   1400020b9:	mov    %rax,0x40(%rsp)
   1400020be:	mov    %r14,%rcx
   1400020c1:	call   140016360 <txrt_gc_safepoint_context>
   1400020c6:	test   %eax,%eax
   1400020c8:	jne    1400024e7 <tx_fn_m0_encoding_paths_0+0x5e7>
   1400020ce:	lea    0xc0(%rsp),%rcx
   1400020d6:	call   140010990 <txrt_time_monotonic_micros>
   1400020db:	test   %eax,%eax
   1400020dd:	jne    1400024f6 <tx_fn_m0_encoding_paths_0+0x5f6>
   1400020e3:	mov    0xc0(%rsp),%r15
   1400020eb:	mov    %r14,%rcx
   1400020ee:	call   140016360 <txrt_gc_safepoint_context>
   1400020f3:	test   %eax,%eax
   1400020f5:	jne    140002505 <tx_fn_m0_encoding_paths_0+0x605>
   1400020fb:	test   %r13,%r13
   1400020fe:	jle    1400021aa <tx_fn_m0_encoding_paths_0+0x2aa>
   140002104:	mov    %rbp,%rcx
   140002107:	mov    %r15,0x38(%rsp)
   14000210c:	lea    0x58(%rsp),%r8
   140002111:	xor    %edx,%edx
   140002113:	call   14001f290 <txrt_encoding_encode_known>
   140002118:	test   %eax,%eax
   14000211a:	jne    14000218f <tx_fn_m0_encoding_paths_0+0x28f>
   14000211c:	xor    %r12d,%r12d
   14000211f:	lea    0xb8(%rsp),%r15
   140002127:	mov    %r13,%rbx
   14000212a:	nopw   0x0(%rax,%rax,1)
   140002130:	mov    0x58(%rsp),%rsi
   140002135:	mov    %rsi,%rcx
   140002138:	mov    %r15,%rdx
   14000213b:	call   14000c390 <txrt_value_len>
   140002140:	test   %eax,%eax
   140002142:	jne    140002426 <tx_fn_m0_encoding_paths_0+0x526>
   140002148:	mov    %rsi,%rcx
   14000214b:	call   14000b5c0 <txrt_value_release>
   140002150:	mov    0xb8(%rsp),%rdx
   140002158:	mov    %r12,%rdi
   14000215b:	add    %rdx,%rdi
   14000215e:	jo     140002447 <tx_fn_m0_encoding_paths_0+0x547>
   140002164:	mov    %r14,%rcx
   140002167:	call   140016360 <txrt_gc_safepoint_context>
   14000216c:	test   %eax,%eax
   14000216e:	jne    14000242f <tx_fn_m0_encoding_paths_0+0x52f>
   140002174:	dec    %rbx
   140002177:	je     1400021a3 <tx_fn_m0_encoding_paths_0+0x2a3>
   140002179:	mov    %rbp,%rcx
   14000217c:	xor    %edx,%edx
   14000217e:	lea    0x58(%rsp),%r8
   140002183:	call   14001f290 <txrt_encoding_encode_known>
   140002188:	mov    %rdi,%r12
   14000218b:	test   %eax,%eax
   14000218d:	je     140002130 <tx_fn_m0_encoding_paths_0+0x230>
   14000218f:	mov    %eax,%esi
   140002191:	lea    0x860c8(%rip),%rdx        # 140088260 <.rdata+0x1260>
   140002198:	mov    $0x32,%r8d
   14000219e:	jmp    1400022fe <tx_fn_m0_encoding_paths_0+0x3fe>
   1400021a3:	mov    0x38(%rsp),%r15
   1400021a8:	jmp    1400021ac <tx_fn_m0_encoding_paths_0+0x2ac>
   1400021aa:	xor    %edi,%edi
   1400021ac:	lea    0xb0(%rsp),%rax
   1400021b4:	mov    %rax,0x20(%rsp)
   1400021b9:	lea    0x8621b(%rip),%rdx        # 1400883db <.rdata+0x13db>
   1400021c0:	mov    $0xc,%r8d
   1400021c6:	mov    0x48(%rsp),%rcx
   1400021cb:	mov    $0x1,%r9b
   1400021ce:	call   14001d120 <txrt_str_concat_literal>
   1400021d3:	test   %eax,%eax
   1400021d5:	jne    140002514 <tx_fn_m0_encoding_paths_0+0x614>
   1400021db:	mov    0xb0(%rsp),%rsi
   1400021e3:	lea    0x86266(%rip),%rax        # 140088450 <.rdata+0x1450>
   1400021ea:	mov    %rax,0x70(%rsp)
   1400021ef:	movq   $0x34,0x78(%rsp)
   1400021f8:	movq   $0x5,0x80(%rsp)
   140002204:	mov    %r14,%rcx
   140002207:	mov    %rsi,%rdx
   14000220a:	mov    %r15,%r8
   14000220d:	mov    %rdi,%r9
   140002210:	call   1400015e0 <tx_fn_m0_report_0>
   140002215:	mov    %rsi,%rcx
   140002218:	call   140026ca0 <txrt_str_release>
   14000221d:	mov    %r14,%rcx
   140002220:	call   140016360 <txrt_gc_safepoint_context>
   140002225:	test   %eax,%eax
   140002227:	jne    140002523 <tx_fn_m0_encoding_paths_0+0x623>
   14000222d:	lea    0xa8(%rsp),%rcx
   140002235:	call   140010990 <txrt_time_monotonic_micros>
   14000223a:	test   %eax,%eax
   14000223c:	jne    140002532 <tx_fn_m0_encoding_paths_0+0x632>
   140002242:	mov    0xa8(%rsp),%r12
   14000224a:	mov    %r14,%rcx
   14000224d:	call   140016360 <txrt_gc_safepoint_context>
   140002252:	test   %eax,%eax
   140002254:	jne    140002541 <tx_fn_m0_encoding_paths_0+0x641>
   14000225a:	test   %r13,%r13
   14000225d:	jle    14000231f <tx_fn_m0_encoding_paths_0+0x41f>
   140002263:	mov    %rbp,%r15
   140002266:	mov    %r12,0x38(%rsp)
   14000226b:	lea    0x50(%rsp),%r8
   140002270:	mov    0x40(%rsp),%rcx
   140002275:	xor    %edx,%edx
   140002277:	call   14001e310 <txrt_encoding_decode_known>
   14000227c:	test   %eax,%eax
   14000227e:	jne    1400022ef <tx_fn_m0_encoding_paths_0+0x3ef>
   140002280:	xor    %r12d,%r12d
   140002283:	lea    0xa0(%rsp),%rbx
   14000228b:	lea    0x50(%rsp),%rbp
   140002290:	mov    0x50(%rsp),%rsi
   140002295:	mov    %rsi,%rcx
   140002298:	mov    %rbx,%rdx
   14000229b:	call   140026dc0 <txrt_str_len>
   1400022a0:	test   %eax,%eax
   1400022a2:	jne    140002468 <tx_fn_m0_encoding_paths_0+0x568>
   1400022a8:	mov    %rsi,%rcx
   1400022ab:	call   140026ca0 <txrt_str_release>
   1400022b0:	mov    0xa0(%rsp),%rdx
   1400022b8:	mov    %r12,%rdi
   1400022bb:	add    %rdx,%rdi
   1400022be:	jo     140002471 <tx_fn_m0_encoding_paths_0+0x571>
   1400022c4:	mov    %r14,%rcx
   1400022c7:	call   140016360 <txrt_gc_safepoint_context>
   1400022cc:	test   %eax,%eax
   1400022ce:	jne    1400024a5 <tx_fn_m0_encoding_paths_0+0x5a5>
   1400022d4:	dec    %r13
   1400022d7:	je     140002315 <tx_fn_m0_encoding_paths_0+0x415>
   1400022d9:	mov    0x40(%rsp),%rcx
   1400022de:	xor    %edx,%edx
   1400022e0:	mov    %rbp,%r8
   1400022e3:	call   14001e310 <txrt_encoding_decode_known>
   1400022e8:	mov    %rdi,%r12
   1400022eb:	test   %eax,%eax
   1400022ed:	je     140002290 <tx_fn_m0_encoding_paths_0+0x390>
   1400022ef:	mov    %eax,%esi
   1400022f1:	lea    0x862d8(%rip),%rdx        # 1400885d0 <.rdata+0x15d0>
   1400022f8:	mov    $0x39,%r8d
   1400022fe:	mov    $0x9,%r9d
   140002304:	mov    0x30(%rsp),%rcx
   140002309:	call   140021b60 <txrt_stack_error_location>
   14000230e:	mov    %esi,%ecx
   140002310:	call   1400262e0 <txrt_require_success>
   140002315:	mov    %r15,%rbp
   140002318:	mov    0x38(%rsp),%r12
   14000231d:	jmp    140002321 <tx_fn_m0_encoding_paths_0+0x421>
   14000231f:	xor    %edi,%edi
   140002321:	lea    0x98(%rsp),%rax
   140002329:	mov    %rax,0x20(%rsp)
   14000232e:	lea    0x86416(%rip),%rdx        # 14008874b <.rdata+0x174b>
   140002335:	mov    $0xc,%r8d
   14000233b:	mov    0x48(%rsp),%rcx
   140002340:	mov    $0x1,%r9b
   140002343:	call   14001d120 <txrt_str_concat_literal>
   140002348:	test   %eax,%eax
   14000234a:	jne    140002550 <tx_fn_m0_encoding_paths_0+0x650>
   140002350:	mov    0x98(%rsp),%rsi
   140002358:	lea    0x86461(%rip),%rax        # 1400887c0 <.rdata+0x17c0>
   14000235f:	mov    %rax,0x70(%rsp)
   140002364:	movq   $0x3b,0x78(%rsp)
   14000236d:	movq   $0x5,0x80(%rsp)
   140002379:	mov    %r14,%rcx
   14000237c:	mov    %rsi,%rdx
   14000237f:	mov    %r12,%r8
   140002382:	mov    %rdi,%r9
   140002385:	call   1400015e0 <tx_fn_m0_report_0>
   14000238a:	mov    %rsi,%rcx
   14000238d:	call   140026ca0 <txrt_str_release>
   140002392:	mov    %r14,%rcx
   140002395:	call   140016360 <txrt_gc_safepoint_context>
   14000239a:	test   %eax,%eax
   14000239c:	jne    140002559 <tx_fn_m0_encoding_paths_0+0x659>
   1400023a2:	mov    %rbp,%rcx
   1400023a5:	call   140026ca0 <txrt_str_release>
   1400023aa:	mov    0x40(%rsp),%rcx
   1400023af:	call   14000b5c0 <txrt_value_release>
   1400023b4:	mov    0x90(%rsp),%rax
   1400023bc:	mov    %rax,(%r14)
   1400023bf:	add    $0xf8,%rsp
   1400023c6:	pop    %rbx
   1400023c7:	pop    %rbp
   1400023c8:	pop    %rdi
   1400023c9:	pop    %rsi
   1400023ca:	pop    %r12
   1400023cc:	pop    %r13
   1400023ce:	pop    %r14
   1400023d0:	pop    %r15
   1400023d2:	ret
   1400023d3:	mov    %eax,%ebp
   1400023d5:	lea    0x85be4(%rip),%rdx        # 140087fc0 <.rdata+0xfc0>
   1400023dc:	mov    $0x2b,%r8d
   1400023e2:	mov    $0x9,%r9d
   1400023e8:	mov    %r14,%rcx
   1400023eb:	call   140021b60 <txrt_stack_error_location>
   1400023f0:	mov    %ebp,%ecx
   1400023f2:	call   1400262e0 <txrt_require_success>
   1400023f7:	lea    0x85c22(%rip),%rdx        # 140088020 <.rdata+0x1020>
   1400023fe:	jmp    140002407 <tx_fn_m0_encoding_paths_0+0x507>
   140002400:	lea    0x85c79(%rip),%rdx        # 140088080 <.rdata+0x1080>
   140002407:	mov    $0x2b,%r8d
   14000240d:	mov    $0x9,%r9d
   140002413:	mov    0x30(%rsp),%rcx
   140002418:	mov    %eax,%esi
   14000241a:	call   140021b60 <txrt_stack_error_location>
   14000241f:	mov    %esi,%ecx
   140002421:	call   1400262e0 <txrt_require_success>
   140002426:	lea    0x85e93(%rip),%rdx        # 1400882c0 <.rdata+0x12c0>
   14000242d:	jmp    140002436 <tx_fn_m0_encoding_paths_0+0x536>
   14000242f:	lea    0x85f4a(%rip),%rdx        # 140088380 <.rdata+0x1380>
   140002436:	mov    $0x32,%r8d
   14000243c:	mov    $0x9,%r9d
   140002442:	jmp    14000256c <tx_fn_m0_encoding_paths_0+0x66c>
   140002447:	lea    0xf0(%rsp),%r8
   14000244f:	mov    %r12,%rcx
   140002452:	call   1400273e0 <txrt_add_i64>
   140002457:	mov    %eax,%esi
   140002459:	lea    0x85ec0(%rip),%rdx        # 140088320 <.rdata+0x1320>
   140002460:	mov    $0x32,%r8d
   140002466:	jmp    140002490 <tx_fn_m0_encoding_paths_0+0x590>
   140002468:	lea    0x861c1(%rip),%rdx        # 140088630 <.rdata+0x1630>
   14000246f:	jmp    1400024ac <tx_fn_m0_encoding_paths_0+0x5ac>
   140002471:	lea    0xe8(%rsp),%r8
   140002479:	mov    %r12,%rcx
   14000247c:	call   1400273e0 <txrt_add_i64>
   140002481:	mov    %eax,%esi
   140002483:	lea    0x86206(%rip),%rdx        # 140088690 <.rdata+0x1690>
   14000248a:	mov    $0x39,%r8d
   140002490:	mov    $0x9,%r9d
   140002496:	mov    %r14,%rcx
   140002499:	call   140021b60 <txrt_stack_error_location>
   14000249e:	mov    %esi,%ecx
   1400024a0:	call   1400262e0 <txrt_require_success>
   1400024a5:	lea    0x86244(%rip),%rdx        # 1400886f0 <.rdata+0x16f0>
   1400024ac:	mov    $0x39,%r8d
   1400024b2:	mov    $0x9,%r9d
   1400024b8:	jmp    14000256c <tx_fn_m0_encoding_paths_0+0x66c>
   1400024bd:	lea    0x859cc(%rip),%rdx        # 140087e90 <.rdata+0xe90>
   1400024c4:	jmp    1400024cd <tx_fn_m0_encoding_paths_0+0x5cd>
   1400024c6:	lea    0x85a23(%rip),%rdx        # 140087ef0 <.rdata+0xef0>
   1400024cd:	mov    $0x28,%r8d
   1400024d3:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   1400024d8:	lea    0x85c01(%rip),%rdx        # 1400880e0 <.rdata+0x10e0>
   1400024df:	mov    $0x2d,%r8d
   1400024e5:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   1400024e7:	lea    0x85c52(%rip),%rdx        # 140088140 <.rdata+0x1140>
   1400024ee:	mov    $0x2d,%r8d
   1400024f4:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   1400024f6:	lea    0x85ca3(%rip),%rdx        # 1400881a0 <.rdata+0x11a0>
   1400024fd:	mov    $0x2f,%r8d
   140002503:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   140002505:	lea    0x85cf4(%rip),%rdx        # 140088200 <.rdata+0x1200>
   14000250c:	mov    $0x2f,%r8d
   140002512:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   140002514:	lea    0x85ed5(%rip),%rdx        # 1400883f0 <.rdata+0x13f0>
   14000251b:	mov    $0x34,%r8d
   140002521:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   140002523:	lea    0x85f86(%rip),%rdx        # 1400884b0 <.rdata+0x14b0>
   14000252a:	mov    $0x34,%r8d
   140002530:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   140002532:	lea    0x85fd7(%rip),%rdx        # 140088510 <.rdata+0x1510>
   140002539:	mov    $0x36,%r8d
   14000253f:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   140002541:	lea    0x86028(%rip),%rdx        # 140088570 <.rdata+0x1570>
   140002548:	mov    $0x36,%r8d
   14000254e:	jmp    140002566 <tx_fn_m0_encoding_paths_0+0x666>
   140002550:	lea    0x86209(%rip),%rdx        # 140088760 <.rdata+0x1760>
   140002557:	jmp    140002560 <tx_fn_m0_encoding_paths_0+0x660>
   140002559:	lea    0x862c0(%rip),%rdx        # 140088820 <.rdata+0x1820>
   140002560:	mov    $0x3b,%r8d
   140002566:	mov    $0x5,%r9d
   14000256c:	mov    %r14,%rcx
   14000256f:	mov    %eax,%esi
   140002571:	call   140021b60 <txrt_stack_error_location>
   140002576:	mov    %esi,%ecx
   140002578:	call   1400262e0 <txrt_require_success>
   14000257d:	int3
   14000257e:	xchg   %ax,%ax


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140003ea0 <txrt_statistics_mean_vector>:
   140003ea0:	push   %rbx
   140003ea1:	sub    $0x70,%rsp
   140003ea5:	fldz
   140003ea7:	lea    0x40(%rsp),%rax
   140003eac:	lea    0x38(%rsp),%r8
   140003eb1:	mov    %rdx,%rbx
   140003eb4:	fstpt  0x50(%rsp)
   140003eb8:	fldt   0x50(%rsp)
   140003ebc:	xor    %edx,%edx
   140003ebe:	movq   $0x0,0x40(%rsp)
   140003ec7:	fstpt  0x60(%rsp)
   140003ecb:	mov    %rax,0x38(%rsp)
   140003ed0:	call   140003b50 <_ZN12tx_generated10statistics5visitIZNS0_12_GLOBAL__N_19aggregateEPKvbNS2_14aggregate_kindEEUldE_EEvS4_bOT_>
   140003ed5:	mov    0x40(%rsp),%rax
   140003eda:	test   %rax,%rax
   140003edd:	je     140003f52 <txrt_statistics_mean_vector+0xb2>
   140003edf:	fldt   0x50(%rsp)
   140003ee3:	mov    %rax,0x28(%rsp)
   140003ee8:	fldt   0x60(%rsp)
   140003eec:	faddp  %st,%st(1)
   140003eee:	fildll 0x28(%rsp)
   140003ef2:	fdivrp %st,%st(1)
   140003ef4:	fld    %st(0)
   140003ef6:	fabs
   140003ef8:	fldt   0x85b02(%rip)        # 140089a00 <.rdata+0x370>
   140003efe:	fucomip %st(1),%st
   140003f00:	jb     140003f37 <txrt_statistics_mean_vector+0x97>
   140003f02:	fldl   0x85ae8(%rip)        # 1400899f0 <.rdata+0x360>
   140003f08:	fxch   %st(1)
   140003f0a:	fcomip %st(1),%st
   140003f0c:	fstp   %st(0)
   140003f0e:	ja     140003f3d <txrt_statistics_mean_vector+0x9d>
   140003f10:	mov    0x8a859(%rip),%rcx        # 14008e770 <.refptr.__emutls_v._ZN12tx_generated6detail14thread_contextE>
   140003f17:	fstpl  (%rbx)
   140003f19:	call   14006e8e0 <__emutls_get_address>
   140003f1e:	mov    (%rax),%rax
   140003f21:	test   %rax,%rax
   140003f24:	je     140003f30 <txrt_statistics_mean_vector+0x90>
   140003f26:	mov    0x8(%rax),%eax
   140003f29:	add    $0x70,%rsp
   140003f2d:	pop    %rbx
   140003f2e:	ret
   140003f2f:	nop
   140003f30:	call   140021200 <_ZN12tx_generated6detail26initialize_runtime_contextEv>
   140003f35:	jmp    140003f26 <txrt_statistics_mean_vector+0x86>
   140003f37:	fstp   %st(0)
   140003f39:	fstp   %st(0)
   140003f3b:	jmp    140003f3f <txrt_statistics_mean_vector+0x9f>
   140003f3d:	fstp   %st(0)
   140003f3f:	lea    0x85912(%rip),%rdx        # 140089858 <.rdata+0x1c8>
   140003f46:	lea    0x85897(%rip),%rcx        # 1400897e4 <.rdata+0x154>
   140003f4d:	call   14007a6f0 <_ZN12tx_generated10statistics4failEPKcS2_>
   140003f52:	lea    0x858db(%rip),%rdx        # 140089834 <.rdata+0x1a4>
   140003f59:	lea    0x858ea(%rip),%rcx        # 14008984a <.rdata+0x1ba>
   140003f60:	call   14007a6f0 <_ZN12tx_generated10statistics4failEPKcS2_>
   140003f65:	mov    %rax,%rcx
   140003f68:	cmp    $0x3,%rdx
   140003f6c:	je     140003fa0 <txrt_statistics_mean_vector+0x100>
   140003f6e:	jg     140003f7c <txrt_statistics_mean_vector+0xdc>
   140003f70:	cmp    $0x1,%rdx
   140003f74:	je     140003fd1 <txrt_statistics_mean_vector+0x131>
   140003f76:	cmp    $0x2,%rdx
   140003f7a:	je     140003ff8 <txrt_statistics_mean_vector+0x158>
   140003f7c:	call   14006cfc0 <__cxa_begin_catch>
   140003f81:	lea    0x857b9(%rip),%r8        # 140089741 <.rdata+0xb1>
   140003f88:	mov    $0x1,%ecx
   140003f8d:	lea    0x857c3(%rip),%rdx        # 140089757 <.rdata+0xc7>
   140003f94:	call   140021690 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   140003f99:	call   14006cfb8 <__cxa_end_catch>
   140003f9e:	jmp    140003fc7 <txrt_statistics_mean_vector+0x127>
   140003fa0:	call   14006cfc0 <__cxa_begin_catch>
   140003fa5:	mov    %rax,%rcx
   140003fa8:	mov    (%rax),%rax
   140003fab:	call   *0x10(%rax)
   140003fae:	lea    0x8577b(%rip),%rdx        # 140089730 <.rdata+0xa0>
   140003fb5:	mov    $0x1,%ecx
   140003fba:	mov    %rax,%r8
   140003fbd:	call   140021690 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   140003fc2:	call   14006cfb8 <__cxa_end_catch>
   140003fc7:	mov    $0x1,%eax
   140003fcc:	jmp    140003f29 <txrt_statistics_mean_vector+0x89>
   140003fd1:	call   14006cfc0 <__cxa_begin_catch>
   140003fd6:	mov    %rax,%rbx
   140003fd9:	mov    (%rax),%rax
   140003fdc:	mov    %rbx,%rcx
   140003fdf:	call   *0x10(%rax)
   140003fe2:	mov    0x18(%rbx),%rdx
   140003fe6:	mov    0x10(%rbx),%ecx
   140003fe9:	mov    %rax,%r8
   140003fec:	call   140021690 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   140003ff1:	call   14006cfb8 <__cxa_end_catch>
   140003ff6:	jmp    140003fc7 <txrt_statistics_mean_vector+0x127>
   140003ff8:	call   14006cfc0 <__cxa_begin_catch>
   140003ffd:	mov    %rax,%rcx
   140004000:	mov    (%rax),%rax
   140004003:	call   *0x10(%rax)
   140004006:	lea    0x85711(%rip),%rdx        # 14008971e <.rdata+0x8e>
   14000400d:	mov    $0x1,%ecx
   140004012:	mov    %rax,%r8
   140004015:	call   140021690 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14000401a:	call   14006cfb8 <__cxa_end_catch>
   14000401f:	jmp    140003fc7 <txrt_statistics_mean_vector+0x127>
   140004021:	data16 cs nopw 0x0(%rax,%rax,1)
   14000402c:	nopl   0x0(%rax)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\language.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140006180 <tx_fn_m0_bench_string_conversion_0>:
   140006180:	push   %r15
   140006182:	push   %r14
   140006184:	push   %r13
   140006186:	push   %r12
   140006188:	push   %rsi
   140006189:	push   %rdi
   14000618a:	push   %rbp
   14000618b:	push   %rbx
   14000618c:	sub    $0xa8,%rsp
   140006193:	mov    %rcx,%rsi
   140006196:	mov    (%rcx),%rdi
   140006199:	lea    0x9e480(%rip),%rax        # 1400a4620 <.rdata+0x6620>
   1400061a0:	mov    %rax,0x70(%rsp)
   1400061a5:	lea    0x9e494(%rip),%rax        # 1400a4640 <.rdata+0x6640>
   1400061ac:	mov    %rax,0x78(%rsp)
   1400061b1:	movq   $0x1a9,0x80(%rsp)
   1400061bd:	movq   $0x1,0x88(%rsp)
   1400061c9:	mov    %rdi,0x90(%rsp)
   1400061d1:	lea    0x70(%rsp),%rax
   1400061d6:	mov    %rax,(%rcx)
   1400061d9:	lea    0x68(%rsp),%rcx
   1400061de:	call   140021e20 <txrt_time_monotonic_micros>
   1400061e3:	test   %eax,%eax
   1400061e5:	jne    140006482 <tx_fn_m0_bench_string_conversion_0+0x302>
   1400061eb:	mov    0x68(%rsp),%rbx
   1400061f0:	mov    %rsi,%rcx
   1400061f3:	call   14002e180 <txrt_gc_safepoint_context>
   1400061f8:	test   %eax,%eax
   1400061fa:	jne    14000648b <tx_fn_m0_bench_string_conversion_0+0x30b>
   140006200:	mov    %rbx,0x30(%rsp)
   140006205:	mov    %rdi,0x38(%rsp)
   14000620a:	lea    0x28(%rsp),%rdx
   14000620f:	mov    $0x1,%ecx
   140006214:	call   14003ae40 <txrt_int_to_str>
   140006219:	test   %eax,%eax
   14000621b:	jne    14000632b <tx_fn_m0_bench_string_conversion_0+0x1ab>
   140006221:	mov    $0x2,%ebx
   140006226:	xor    %r15d,%r15d
   140006229:	lea    0x9e195(%rip),%r12        # 1400a43c5 <.rdata+0x63c5>
   140006230:	lea    0x48(%rsp),%rbp
   140006235:	lea    0x28(%rsp),%rdi
   14000623a:	nopw   0x0(%rax,%rax,1)
   140006240:	mov    0x28(%rsp),%r14
   140006245:	mov    %rsi,%rcx
   140006248:	call   14002e180 <txrt_gc_safepoint_context>
   14000624d:	test   %eax,%eax
   14000624f:	jne    1400063cd <tx_fn_m0_bench_string_conversion_0+0x24d>
   140006255:	mov    %r14,%rcx
   140006258:	lea    0x60(%rsp),%rdx
   14000625d:	call   140039e40 <txrt_str_clone>
   140006262:	test   %eax,%eax
   140006264:	jne    1400063df <tx_fn_m0_bench_string_conversion_0+0x25f>
   14000626a:	mov    0x60(%rsp),%r13
   14000626f:	mov    %r13,%rcx
   140006272:	lea    0x58(%rsp),%rdx
   140006277:	call   14003a4c0 <txrt_parse_int>
   14000627c:	test   %eax,%eax
   14000627e:	jne    1400063e8 <tx_fn_m0_bench_string_conversion_0+0x268>
   140006284:	mov    %r13,%rcx
   140006287:	call   140039f80 <txrt_str_release>
   14000628c:	mov    0x58(%rsp),%rdx
   140006291:	mov    %r15,%r13
   140006294:	add    %rdx,%r13
   140006297:	jo     1400063f7 <tx_fn_m0_bench_string_conversion_0+0x277>
   14000629d:	lea    0x50(%rsp),%rax
   1400062a2:	mov    %rax,0x20(%rsp)
   1400062a7:	mov    $0x3,%r8d
   1400062ad:	mov    %r14,%rcx
   1400062b0:	mov    %r12,%rdx
   1400062b3:	mov    $0x1,%r9b
   1400062b6:	call   140033840 <txrt_str_concat_literal>
   1400062bb:	test   %eax,%eax
   1400062bd:	jne    140006418 <tx_fn_m0_bench_string_conversion_0+0x298>
   1400062c3:	mov    0x50(%rsp),%r15
   1400062c8:	mov    %r15,%rcx
   1400062cb:	mov    %rbp,%rdx
   1400062ce:	call   14003a0a0 <txrt_str_len>
   1400062d3:	test   %eax,%eax
   1400062d5:	jne    140006421 <tx_fn_m0_bench_string_conversion_0+0x2a1>
   1400062db:	mov    %r15,%rcx
   1400062de:	call   140039f80 <txrt_str_release>
   1400062e3:	mov    0x48(%rsp),%rdx
   1400062e8:	mov    %r13,%r15
   1400062eb:	add    %rdx,%r15
   1400062ee:	jo     14000642a <tx_fn_m0_bench_string_conversion_0+0x2aa>
   1400062f4:	mov    %rsi,%rcx
   1400062f7:	call   14002e180 <txrt_gc_safepoint_context>
   1400062fc:	test   %eax,%eax
   1400062fe:	jne    14000645e <tx_fn_m0_bench_string_conversion_0+0x2de>
   140006304:	mov    %r14,%rcx
   140006307:	call   140039f80 <txrt_str_release>
   14000630c:	cmp    $0xc351,%rbx
   140006313:	je     14000633f <tx_fn_m0_bench_string_conversion_0+0x1bf>
   140006315:	mov    %rbx,%rcx
   140006318:	mov    %rdi,%rdx
   14000631b:	call   14003ae40 <txrt_int_to_str>
   140006320:	inc    %rbx
   140006323:	test   %eax,%eax
   140006325:	je     140006240 <tx_fn_m0_bench_string_conversion_0+0xc0>
   14000632b:	mov    %eax,%edi
   14000632d:	lea    0x9df0c(%rip),%rdx        # 1400a4240 <.rdata+0x6240>
   140006334:	mov    $0x1af,%r8d
   14000633a:	jmp    140006449 <tx_fn_m0_bench_string_conversion_0+0x2c9>
   14000633f:	lea    0x9e1ca(%rip),%rcx        # 1400a4510 <.rdata+0x6510>
   140006346:	lea    0x40(%rsp),%r8
   14000634b:	mov    $0x11,%edx
   140006350:	call   140039bd0 <txrt_str_new>
   140006355:	test   %eax,%eax
   140006357:	jne    14000649a <tx_fn_m0_bench_string_conversion_0+0x31a>
   14000635d:	mov    0x40(%rsp),%rdi
   140006362:	lea    0x9e217(%rip),%rax        # 1400a4580 <.rdata+0x6580>
   140006369:	mov    %rax,0x78(%rsp)
   14000636e:	movq   $0x1b3,0x80(%rsp)
   14000637a:	movq   $0x5,0x88(%rsp)
   140006386:	mov    %rsi,%rcx
   140006389:	mov    %rdi,%rdx
   14000638c:	mov    0x30(%rsp),%r8
   140006391:	mov    %r15,%r9
   140006394:	call   140001930 <tx_fn_m0_report_0>
   140006399:	mov    %rdi,%rcx
   14000639c:	call   140039f80 <txrt_str_release>
   1400063a1:	mov    %rsi,%rcx
   1400063a4:	call   14002e180 <txrt_gc_safepoint_context>
   1400063a9:	test   %eax,%eax
   1400063ab:	jne    1400064a3 <tx_fn_m0_bench_string_conversion_0+0x323>
   1400063b1:	mov    0x38(%rsp),%rax
   1400063b6:	mov    %rax,(%rsi)
   1400063b9:	add    $0xa8,%rsp
   1400063c0:	pop    %rbx
   1400063c1:	pop    %rbp
   1400063c2:	pop    %rdi
   1400063c3:	pop    %rsi
   1400063c4:	pop    %r12
   1400063c6:	pop    %r13
   1400063c8:	pop    %r14
   1400063ca:	pop    %r15
   1400063cc:	ret
   1400063cd:	lea    0x9debc(%rip),%rdx        # 1400a4290 <.rdata+0x6290>
   1400063d4:	mov    $0x1af,%r8d
   1400063da:	jmp    14000646b <tx_fn_m0_bench_string_conversion_0+0x2eb>
   1400063df:	lea    0x9defa(%rip),%rdx        # 1400a42e0 <.rdata+0x62e0>
   1400063e6:	jmp    1400063ef <tx_fn_m0_bench_string_conversion_0+0x26f>
   1400063e8:	lea    0x9df41(%rip),%rdx        # 1400a4330 <.rdata+0x6330>
   1400063ef:	mov    $0x1b0,%r8d
   1400063f5:	jmp    14000646b <tx_fn_m0_bench_string_conversion_0+0x2eb>
   1400063f7:	lea    0xa0(%rsp),%r8
   1400063ff:	mov    %r15,%rcx
   140006402:	call   14003a6c0 <txrt_add_i64>
   140006407:	mov    %eax,%edi
   140006409:	lea    0x9df70(%rip),%rdx        # 1400a4380 <.rdata+0x6380>
   140006410:	mov    $0x1b0,%r8d
   140006416:	jmp    140006449 <tx_fn_m0_bench_string_conversion_0+0x2c9>
   140006418:	lea    0x9dfb1(%rip),%rdx        # 1400a43d0 <.rdata+0x63d0>
   14000641f:	jmp    140006465 <tx_fn_m0_bench_string_conversion_0+0x2e5>
   140006421:	lea    0x9dff8(%rip),%rdx        # 1400a4420 <.rdata+0x6420>
   140006428:	jmp    140006465 <tx_fn_m0_bench_string_conversion_0+0x2e5>
   14000642a:	lea    0x98(%rsp),%r8
   140006432:	mov    %r13,%rcx
   140006435:	call   14003a6c0 <txrt_add_i64>
   14000643a:	mov    %eax,%edi
   14000643c:	lea    0x9e02d(%rip),%rdx        # 1400a4470 <.rdata+0x6470>
   140006443:	mov    $0x1b1,%r8d
   140006449:	mov    $0x9,%r9d
   14000644f:	mov    %rsi,%rcx
   140006452:	call   140034e40 <txrt_stack_error_location>
   140006457:	mov    %edi,%ecx
   140006459:	call   1400395c0 <txrt_require_success>
   14000645e:	lea    0x9e05b(%rip),%rdx        # 1400a44c0 <.rdata+0x64c0>
   140006465:	mov    $0x1b1,%r8d
   14000646b:	mov    $0x9,%r9d
   140006471:	mov    %rsi,%rcx
   140006474:	mov    %eax,%esi
   140006476:	call   140034e40 <txrt_stack_error_location>
   14000647b:	mov    %esi,%ecx
   14000647d:	call   1400395c0 <txrt_require_success>
   140006482:	lea    0x9dd17(%rip),%rdx        # 1400a41a0 <.rdata+0x61a0>
   140006489:	jmp    140006492 <tx_fn_m0_bench_string_conversion_0+0x312>
   14000648b:	lea    0x9dd5e(%rip),%rdx        # 1400a41f0 <.rdata+0x61f0>
   140006492:	mov    $0x1ac,%r8d
   140006498:	jmp    1400064b0 <tx_fn_m0_bench_string_conversion_0+0x330>
   14000649a:	lea    0x9e08f(%rip),%rdx        # 1400a4530 <.rdata+0x6530>
   1400064a1:	jmp    1400064aa <tx_fn_m0_bench_string_conversion_0+0x32a>
   1400064a3:	lea    0x9e126(%rip),%rdx        # 1400a45d0 <.rdata+0x65d0>
   1400064aa:	mov    $0x1b3,%r8d
   1400064b0:	mov    $0x5,%r9d
   1400064b6:	jmp    140006471 <tx_fn_m0_bench_string_conversion_0+0x2f1>
   1400064b8:	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\language.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140005de0 <tx_fn_m0_bench_module_call_0>:
   140005de0:	push   %r15
   140005de2:	push   %r14
   140005de4:	push   %r13
   140005de6:	push   %r12
   140005de8:	push   %rsi
   140005de9:	push   %rdi
   140005dea:	push   %rbp
   140005deb:	push   %rbx
   140005dec:	sub    $0xc8,%rsp
   140005df3:	movdqa %xmm6,0xb0(%rsp)
   140005dfc:	mov    %rcx,%rsi
   140005dff:	mov    (%rcx),%rdi
   140005e02:	lea    0x9e327(%rip),%rax        # 1400a4130 <.rdata+0x6130>
   140005e09:	mov    %rax,0x20(%rsp)
   140005e0e:	lea    0x9e33b(%rip),%rax        # 1400a4150 <.rdata+0x6150>
   140005e15:	mov    %rax,0x28(%rsp)
   140005e1a:	movq   $0x19d,0x30(%rsp)
   140005e23:	movq   $0x1,0x38(%rsp)
   140005e2c:	mov    %rdi,0x40(%rsp)
   140005e31:	lea    0x20(%rsp),%rax
   140005e36:	mov    %rax,(%rcx)
   140005e39:	lea    0x68(%rsp),%rcx
   140005e3e:	call   140021e20 <txrt_time_monotonic_micros>
   140005e43:	test   %eax,%eax
   140005e45:	jne    1400060f1 <tx_fn_m0_bench_module_call_0+0x311>
   140005e4b:	mov    %rdi,0x50(%rsp)
   140005e50:	mov    0x68(%rsp),%rax
   140005e55:	mov    %rax,0x48(%rsp)
   140005e5a:	mov    %rsi,%rcx
   140005e5d:	call   14002e180 <txrt_gc_safepoint_context>
   140005e62:	test   %eax,%eax
   140005e64:	jne    1400060fa <tx_fn_m0_bench_module_call_0+0x31a>
   140005e6a:	mov    $0x8,%ebx
   140005e6f:	xor    %r12d,%r12d
   140005e72:	lea    0x60(%rsp),%r15
   140005e77:	lea    0x98d02(%rip),%rax        # 14009eb80 <.rdata+0xb80>
   140005e7e:	movq   %rax,%xmm0
   140005e83:	lea    0x98cec(%rip),%rax        # 14009eb76 <.rdata+0xb76>
   140005e8a:	movq   %rax,%xmm6
   140005e8f:	punpcklqdq %xmm0,%xmm6
   140005e93:	lea    0x70(%rsp),%r14
   140005e98:	nopl   0x0(%rax,%rax,1)
   140005ea0:	lea    0x989d9(%rip),%rcx        # 14009e880 <.rdata+0x880>
   140005ea7:	mov    %r15,%rdx
   140005eaa:	call   140032940 <txrt_record_struct_new>
   140005eaf:	test   %eax,%eax
   140005eb1:	jne    14000602c <tx_fn_m0_bench_module_call_0+0x24c>
   140005eb7:	lea    -0x7(%rbx),%rbp
   140005ebb:	lea    -0x6(%rbx),%rdi
   140005ebf:	mov    0x60(%rsp),%r13
   140005ec4:	mov    %r13,%rcx
   140005ec7:	call   1400314d0 <txrt_record_struct_view>
   140005ecc:	mov    (%rax),%rax
   140005ecf:	mov    %rbp,(%rax)
   140005ed2:	mov    %rdi,0x8(%rax)
   140005ed6:	mov    %r13,%rcx
   140005ed9:	call   1400314d0 <txrt_record_struct_view>
   140005ede:	mov    %rsi,%rcx
   140005ee1:	call   14002e180 <txrt_gc_safepoint_context>
   140005ee6:	test   %eax,%eax
   140005ee8:	jne    140006035 <tx_fn_m0_bench_module_call_0+0x255>
   140005eee:	lea    0x9dffb(%rip),%rax        # 1400a3ef0 <.rdata+0x5ef0>
   140005ef5:	mov    %rax,0x28(%rsp)
   140005efa:	movq   $0x1a4,0x30(%rsp)
   140005f03:	movq   $0x9,0x38(%rsp)
   140005f0c:	mov    (%rsi),%rbp
   140005f0f:	movdqa %xmm6,0x70(%rsp)
   140005f15:	movq   $0x6,0x80(%rsp)
   140005f21:	movq   $0x1,0x88(%rsp)
   140005f2d:	mov    %rbp,0x90(%rsp)
   140005f35:	mov    %r14,(%rsi)
   140005f38:	mov    %r13,%rcx
   140005f3b:	call   1400314d0 <txrt_record_struct_view>
   140005f40:	mov    (%rax),%rax
   140005f43:	mov    (%rax),%rcx
   140005f46:	mov    0x8(%rax),%rax
   140005f4a:	mov    %rcx,%rdx
   140005f4d:	add    %rax,%rdx
   140005f50:	jo     140006047 <tx_fn_m0_bench_module_call_0+0x267>
   140005f56:	mov    %rbp,(%rsi)
   140005f59:	mov    %rbx,%rax
   140005f5c:	add    %rdx,%rax
   140005f5f:	jo     14000607b <tx_fn_m0_bench_module_call_0+0x29b>
   140005f65:	mov    %r12,%rbp
   140005f68:	add    %rax,%rbp
   140005f6b:	jo     140006096 <tx_fn_m0_bench_module_call_0+0x2b6>
   140005f71:	mov    %rsi,%rcx
   140005f74:	call   14002e180 <txrt_gc_safepoint_context>
   140005f79:	test   %eax,%eax
   140005f7b:	jne    1400060cd <tx_fn_m0_bench_module_call_0+0x2ed>
   140005f81:	mov    %r13,%rcx
   140005f84:	call   14001ca50 <txrt_value_release>
   140005f89:	inc    %rbx
   140005f8c:	mov    %rbp,%r12
   140005f8f:	cmp    $0x186a8,%rbx
   140005f96:	jne    140005ea0 <tx_fn_m0_bench_module_call_0+0xc0>
   140005f9c:	lea    0x9e082(%rip),%rcx        # 1400a4025 <.rdata+0x6025>
   140005fa3:	lea    0x58(%rsp),%r8
   140005fa8:	mov    $0xb,%edx
   140005fad:	call   140039bd0 <txrt_str_new>
   140005fb2:	test   %eax,%eax
   140005fb4:	jne    140006109 <tx_fn_m0_bench_module_call_0+0x329>
   140005fba:	mov    0x58(%rsp),%rbx
   140005fbf:	lea    0x9e0ca(%rip),%rax        # 1400a4090 <.rdata+0x6090>
   140005fc6:	mov    %rax,0x28(%rsp)
   140005fcb:	movq   $0x1a6,0x30(%rsp)
   140005fd4:	movq   $0x5,0x38(%rsp)
   140005fdd:	mov    %rsi,%rcx
   140005fe0:	mov    %rbx,%rdx
   140005fe3:	mov    0x48(%rsp),%r8
   140005fe8:	mov    %rbp,%r9
   140005feb:	call   140001930 <tx_fn_m0_report_0>
   140005ff0:	mov    %rbx,%rcx
   140005ff3:	call   140039f80 <txrt_str_release>
   140005ff8:	mov    %rsi,%rcx
   140005ffb:	call   14002e180 <txrt_gc_safepoint_context>
   140006000:	test   %eax,%eax
   140006002:	jne    140006112 <tx_fn_m0_bench_module_call_0+0x332>
   140006008:	mov    0x50(%rsp),%rax
   14000600d:	mov    %rax,(%rsi)
   140006010:	movaps 0xb0(%rsp),%xmm6
   140006018:	add    $0xc8,%rsp
   14000601f:	pop    %rbx
   140006020:	pop    %rbp
   140006021:	pop    %rdi
   140006022:	pop    %rsi
   140006023:	pop    %r12
   140006025:	pop    %r13
   140006027:	pop    %r14
   140006029:	pop    %r15
   14000602b:	ret
   14000602c:	lea    0x9de1d(%rip),%rdx        # 1400a3e50 <.rdata+0x5e50>
   140006033:	jmp    14000603c <tx_fn_m0_bench_module_call_0+0x25c>
   140006035:	lea    0x9de64(%rip),%rdx        # 1400a3ea0 <.rdata+0x5ea0>
   14000603c:	mov    $0x1a3,%r8d
   140006042:	jmp    1400060da <tx_fn_m0_bench_module_call_0+0x2fa>
   140006047:	lea    0xa8(%rsp),%r8
   14000604f:	mov    %rax,%rdx
   140006052:	call   14003a6c0 <txrt_add_i64>
   140006057:	mov    %eax,%edi
   140006059:	lea    0x98ad0(%rip),%rdx        # 14009eb30 <.rdata+0xb30>
   140006060:	mov    $0x8,%r8d
   140006066:	mov    $0x5,%r9d
   14000606c:	mov    %rsi,%rcx
   14000606f:	call   140034e40 <txrt_stack_error_location>
   140006074:	mov    %edi,%ecx
   140006076:	call   1400395c0 <txrt_require_success>
   14000607b:	lea    0xa0(%rsp),%r8
   140006083:	mov    %rbx,%rcx
   140006086:	call   14003a6c0 <txrt_add_i64>
   14000608b:	mov    %eax,%edi
   14000608d:	lea    0x9deac(%rip),%rdx        # 1400a3f40 <.rdata+0x5f40>
   140006094:	jmp    1400060b2 <tx_fn_m0_bench_module_call_0+0x2d2>
   140006096:	lea    0x98(%rsp),%r8
   14000609e:	mov    %r12,%rcx
   1400060a1:	mov    %rax,%rdx
   1400060a4:	call   14003a6c0 <txrt_add_i64>
   1400060a9:	mov    %eax,%edi
   1400060ab:	lea    0x9dede(%rip),%rdx        # 1400a3f90 <.rdata+0x5f90>
   1400060b2:	mov    $0x1a4,%r8d
   1400060b8:	mov    $0x9,%r9d
   1400060be:	mov    %rsi,%rcx
   1400060c1:	call   140034e40 <txrt_stack_error_location>
   1400060c6:	mov    %edi,%ecx
   1400060c8:	call   1400395c0 <txrt_require_success>
   1400060cd:	lea    0x9df0c(%rip),%rdx        # 1400a3fe0 <.rdata+0x5fe0>
   1400060d4:	mov    $0x1a4,%r8d
   1400060da:	mov    $0x9,%r9d
   1400060e0:	mov    %rsi,%rcx
   1400060e3:	mov    %eax,%esi
   1400060e5:	call   140034e40 <txrt_stack_error_location>
   1400060ea:	mov    %esi,%ecx
   1400060ec:	call   1400395c0 <txrt_require_success>
   1400060f1:	lea    0x9dcb8(%rip),%rdx        # 1400a3db0 <.rdata+0x5db0>
   1400060f8:	jmp    140006101 <tx_fn_m0_bench_module_call_0+0x321>
   1400060fa:	lea    0x9dcff(%rip),%rdx        # 1400a3e00 <.rdata+0x5e00>
   140006101:	mov    $0x1a0,%r8d
   140006107:	jmp    14000611f <tx_fn_m0_bench_module_call_0+0x33f>
   140006109:	lea    0x9df30(%rip),%rdx        # 1400a4040 <.rdata+0x6040>
   140006110:	jmp    140006119 <tx_fn_m0_bench_module_call_0+0x339>
   140006112:	lea    0x9dfc7(%rip),%rdx        # 1400a40e0 <.rdata+0x60e0>
   140006119:	mov    $0x1a6,%r8d
   14000611f:	mov    $0x5,%r9d
   140006125:	jmp    1400060e0 <tx_fn_m0_bench_module_call_0+0x300>
   140006127:	nopw   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\language.exe:     file format pei-x86-64


Disassembly of section .text:

000000014004cf30 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex>:
   14004cf30:	push   %r12
   14004cf32:	push   %rbp
   14004cf33:	push   %rdi
   14004cf34:	push   %rsi
   14004cf35:	push   %rbx
   14004cf36:	sub    $0xf0,%rsp
   14004cf3d:	mov    %rdx,%rbp
   14004cf40:	mov    %rcx,%rsi
   14004cf43:	mov    %rdx,%rbx
   14004cf46:	shr    $0x3f,%rbp
   14004cf4a:	test   %rdx,%rdx
   14004cf4d:	js     14004d258 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x328>
   14004cf53:	xor    %r10d,%r10d
   14004cf56:	cmp    $0x9,%rdx
   14004cf5a:	jbe    14004d502 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x5d2>
   14004cf60:	cmp    $0x63,%rbx
   14004cf64:	jbe    14004d4c3 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x593>
   14004cf6a:	cmp    $0x3e7,%rbx
   14004cf71:	jbe    14004d4ed <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x5bd>
   14004cf77:	cmp    $0x270f,%rbx
   14004cf7e:	jbe    14004d4d8 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x5a8>
   14004cf84:	mov    %rbx,%rdx
   14004cf87:	mov    $0x1,%r8d
   14004cf8d:	movabs $0x346dc5d63886594b,%r9
   14004cf97:	jmp    14004cfc7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x97>
   14004cf99:	nopl   0x0(%rax)
   14004cfa0:	cmp    $0xf423f,%rcx
   14004cfa7:	jbe    14004d478 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x548>
   14004cfad:	cmp    $0x98967f,%rcx
   14004cfb4:	jbe    14004d488 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x558>
   14004cfba:	cmp    $0x5f5e0ff,%rcx
   14004cfc1:	jbe    14004d498 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x568>
   14004cfc7:	mov    %rdx,%rax
   14004cfca:	mov    %rdx,%rcx
   14004cfcd:	mul    %r9
   14004cfd0:	mov    %r8d,%eax
   14004cfd3:	add    $0x4,%r8d
   14004cfd7:	shr    $0xb,%rdx
   14004cfdb:	cmp    $0x1869f,%rcx
   14004cfe2:	ja     14004cfa0 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x70>
   14004cfe4:	lea    0x3(%rax),%edi
   14004cfe7:	lea    0x10(%rsi),%rcx
   14004cfeb:	lea    (%r8,%r10,1),%r12d
   14004cfef:	mov    %rcx,(%rsi)
   14004cff2:	cmp    $0xf,%r12
   14004cff6:	ja     14004d280 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x350>
   14004cffc:	test   %r12,%r12
   14004cfff:	jne    14004d4a4 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x574>
   14004d005:	movabs $0x3330323031303030,%rax
   14004d00f:	movq   $0x0,0x8(%rsi)
   14004d017:	add    %rbp,%rcx
   14004d01a:	movabs $0x3730363035303430,%rdx
   14004d024:	mov    %rax,0x20(%rsp)
   14004d029:	movabs $0x3131303139303830,%rax
   14004d033:	mov    %rdx,0x28(%rsp)
   14004d038:	movabs $0x3531343133313231,%rdx
   14004d042:	mov    %rax,0x30(%rsp)
   14004d047:	movabs $0x3931383137313631,%rax
   14004d051:	mov    %rdx,0x38(%rsp)
   14004d056:	movabs $0x3332323231323032,%rdx
   14004d060:	mov    %rax,0x40(%rsp)
   14004d065:	movabs $0x3732363235323432,%rax
   14004d06f:	mov    %rdx,0x48(%rsp)
   14004d074:	movabs $0x3133303339323832,%rdx
   14004d07e:	mov    %rax,0x50(%rsp)
   14004d083:	movabs $0x3533343333333233,%rax
   14004d08d:	mov    %rdx,0x58(%rsp)
   14004d092:	movabs $0x3933383337333633,%rdx
   14004d09c:	mov    %rax,0x60(%rsp)
   14004d0a1:	movabs $0x3334323431343034,%rax
   14004d0ab:	mov    %rdx,0x68(%rsp)
   14004d0b0:	movabs $0x3734363435343434,%rdx
   14004d0ba:	mov    %rax,0x70(%rsp)
   14004d0bf:	movabs $0x3135303539343834,%rax
   14004d0c9:	mov    %rdx,0x78(%rsp)
   14004d0ce:	movabs $0x3535343533353235,%rdx
   14004d0d8:	mov    %rax,0x80(%rsp)
   14004d0e0:	movabs $0x3935383537353635,%rax
   14004d0ea:	mov    %rdx,0x88(%rsp)
   14004d0f2:	movabs $0x3336323631363036,%rdx
   14004d0fc:	mov    %rax,0x90(%rsp)
   14004d104:	movabs $0x3736363635363436,%rax
   14004d10e:	mov    %rdx,0x98(%rsp)
   14004d116:	movabs $0x3137303739363836,%rdx
   14004d120:	mov    %rax,0xa0(%rsp)
   14004d128:	movabs $0x3537343733373237,%rax
   14004d132:	mov    %rdx,0xa8(%rsp)
   14004d13a:	movabs $0x3937383737373637,%rdx
   14004d144:	mov    %rax,0xb0(%rsp)
   14004d14c:	movabs $0x3338323831383038,%rax
   14004d156:	mov    %rdx,0xb8(%rsp)
   14004d15e:	movabs $0x3738363835383438,%rdx
   14004d168:	mov    %rax,0xc0(%rsp)
   14004d170:	movabs $0x3139303939383838,%rax
   14004d17a:	mov    %rdx,0xc8(%rsp)
   14004d182:	movabs $0x3539343933393239,%rdx
   14004d18c:	mov    %rdx,0xd8(%rsp)
   14004d194:	movabs $0x39393839373936,%rdx
   14004d19e:	mov    %rax,0xd0(%rsp)
   14004d1a6:	movabs $0x3935393439333932,%rax
   14004d1b0:	movb   $0x0,0x10(%rsi)
   14004d1b4:	mov    %rax,0xd9(%rsp)
   14004d1bc:	mov    %rdx,0xe1(%rsp)
   14004d1c4:	movabs $0x28f5c28f5c28f5c3,%r8
   14004d1ce:	xchg   %ax,%ax
   14004d1d0:	mov    %rbx,%rdx
   14004d1d3:	shr    $0x2,%rdx
   14004d1d7:	mov    %rdx,%rax
   14004d1da:	mul    %r8
   14004d1dd:	mov    %rbx,%rax
   14004d1e0:	mov    %rdx,%r9
   14004d1e3:	and    $0xfffffffffffffffc,%rdx
   14004d1e7:	shr    $0x2,%r9
   14004d1eb:	add    %r9,%rdx
   14004d1ee:	lea    (%rdx,%rdx,4),%rdx
   14004d1f2:	shl    $0x2,%rdx
   14004d1f6:	sub    %rdx,%rax
   14004d1f9:	mov    %rbx,%rdx
   14004d1fc:	mov    %r9,%rbx
   14004d1ff:	mov    %edi,%r9d
   14004d202:	add    %rax,%rax
   14004d205:	movzbl 0x21(%rsp,%rax,1),%r10d
   14004d20b:	movzbl 0x20(%rsp,%rax,1),%eax
   14004d210:	mov    %r10b,(%rcx,%r9,1)
   14004d214:	lea    -0x1(%rdi),%r9d
   14004d218:	sub    $0x2,%edi
   14004d21b:	mov    %al,(%rcx,%r9,1)
   14004d21f:	cmp    $0x270f,%rdx
   14004d226:	ja     14004d1d0 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x2a0>
   14004d228:	lea    0x30(%rbx),%eax
   14004d22b:	cmp    $0x9,%rbx
   14004d22f:	jbe    14004d241 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x311>
   14004d231:	add    %rbx,%rbx
   14004d234:	movzbl 0x21(%rsp,%rbx,1),%eax
   14004d239:	mov    %al,0x1(%rcx)
   14004d23c:	movzbl 0x20(%rsp,%rbx,1),%eax
   14004d241:	mov    %al,(%rcx)
   14004d243:	mov    %rsi,%rax
   14004d246:	add    $0xf0,%rsp
   14004d24d:	pop    %rbx
   14004d24e:	pop    %rsi
   14004d24f:	pop    %rdi
   14004d250:	pop    %rbp
   14004d251:	pop    %r12
   14004d253:	ret
   14004d254:	nopl   0x0(%rax)
   14004d258:	neg    %rbx
   14004d25b:	mov    $0x1,%r10d
   14004d261:	cmp    $0x9,%rbx
   14004d265:	ja     14004cf60 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x30>
   14004d26b:	lea    0x10(%rcx),%rcx
   14004d26f:	xor    %edi,%edi
   14004d271:	mov    $0x2,%r12d
   14004d277:	mov    %rcx,(%rsi)
   14004d27a:	jmp    14004d294 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004d27c:	nopl   0x0(%rax)
   14004d280:	lea    0x1(%r12),%rcx
   14004d285:	call   140080620 <_Znwy>
   14004d28a:	mov    %r12,0x10(%rsi)
   14004d28e:	mov    %rax,(%rsi)
   14004d291:	mov    %rax,%rcx
   14004d294:	mov    %r12,%r8
   14004d297:	mov    $0x2d,%edx
   14004d29c:	call   140088128 <memset>
   14004d2a1:	mov    (%rsi),%rax
   14004d2a4:	add    %r12,%rax
   14004d2a7:	movabs $0x3730363035303430,%rdx
   14004d2b1:	mov    %r12,0x8(%rsi)
   14004d2b5:	movb   $0x0,(%rax)
   14004d2b8:	mov    (%rsi),%rcx
   14004d2bb:	movabs $0x3330323031303030,%rax
   14004d2c5:	mov    %rax,0x20(%rsp)
   14004d2ca:	movabs $0x3131303139303830,%rax
   14004d2d4:	mov    %rdx,0x28(%rsp)
   14004d2d9:	add    %rbp,%rcx
   14004d2dc:	movabs $0x3531343133313231,%rdx
   14004d2e6:	mov    %rax,0x30(%rsp)
   14004d2eb:	movabs $0x3931383137313631,%rax
   14004d2f5:	mov    %rdx,0x38(%rsp)
   14004d2fa:	movabs $0x3332323231323032,%rdx
   14004d304:	mov    %rax,0x40(%rsp)
   14004d309:	movabs $0x3732363235323432,%rax
   14004d313:	mov    %rdx,0x48(%rsp)
   14004d318:	movabs $0x3133303339323832,%rdx
   14004d322:	mov    %rax,0x50(%rsp)
   14004d327:	movabs $0x3533343333333233,%rax
   14004d331:	mov    %rdx,0x58(%rsp)
   14004d336:	movabs $0x3933383337333633,%rdx
   14004d340:	mov    %rax,0x60(%rsp)
   14004d345:	movabs $0x3334323431343034,%rax
   14004d34f:	mov    %rdx,0x68(%rsp)
   14004d354:	movabs $0x3734363435343434,%rdx
   14004d35e:	mov    %rax,0x70(%rsp)
   14004d363:	movabs $0x3135303539343834,%rax
   14004d36d:	mov    %rdx,0x78(%rsp)
   14004d372:	movabs $0x3535343533353235,%rdx
   14004d37c:	mov    %rax,0x80(%rsp)
   14004d384:	movabs $0x3935383537353635,%rax
   14004d38e:	mov    %rdx,0x88(%rsp)
   14004d396:	movabs $0x3336323631363036,%rdx
   14004d3a0:	mov    %rax,0x90(%rsp)
   14004d3a8:	movabs $0x3736363635363436,%rax
   14004d3b2:	mov    %rdx,0x98(%rsp)
   14004d3ba:	movabs $0x3137303739363836,%rdx
   14004d3c4:	mov    %rax,0xa0(%rsp)
   14004d3cc:	movabs $0x3537343733373237,%rax
   14004d3d6:	mov    %rdx,0xa8(%rsp)
   14004d3de:	movabs $0x3937383737373637,%rdx
   14004d3e8:	mov    %rax,0xb0(%rsp)
   14004d3f0:	movabs $0x3338323831383038,%rax
   14004d3fa:	mov    %rdx,0xb8(%rsp)
   14004d402:	movabs $0x3738363835383438,%rdx
   14004d40c:	mov    %rax,0xc0(%rsp)
   14004d414:	movabs $0x3139303939383838,%rax
   14004d41e:	mov    %rdx,0xc8(%rsp)
   14004d426:	movabs $0x3539343933393239,%rdx
   14004d430:	mov    %rdx,0xd8(%rsp)
   14004d438:	movabs $0x39393839373936,%rdx
   14004d442:	mov    %rax,0xd0(%rsp)
   14004d44a:	movabs $0x3935393439333932,%rax
   14004d454:	mov    %rax,0xd9(%rsp)
   14004d45c:	mov    %rdx,0xe1(%rsp)
   14004d464:	cmp    $0x63,%rbx
   14004d468:	ja     14004d1c4 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x294>
   14004d46e:	jmp    14004d228 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x2f8>
   14004d473:	nopl   0x0(%rax,%rax,1)
   14004d478:	mov    %r8d,%edi
   14004d47b:	lea    0x5(%rax),%r8d
   14004d47f:	jmp    14004cfe7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0xb7>
   14004d484:	nopl   0x0(%rax)
   14004d488:	lea    0x6(%rax),%r8d
   14004d48c:	lea    0x5(%rax),%edi
   14004d48f:	jmp    14004cfe7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0xb7>
   14004d494:	nopl   0x0(%rax)
   14004d498:	lea    0x7(%rax),%r8d
   14004d49c:	lea    0x6(%rax),%edi
   14004d49f:	jmp    14004cfe7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0xb7>
   14004d4a4:	cmp    $0x1,%r12
   14004d4a8:	jne    14004d294 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004d4ae:	movb   $0x2d,(%rcx)
   14004d4b1:	mov    (%rsi),%rax
   14004d4b4:	mov    $0x1,%r12d
   14004d4ba:	add    $0x1,%rax
   14004d4be:	jmp    14004d2a7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x377>
   14004d4c3:	lea    0x10(%rsi),%rcx
   14004d4c7:	lea    0x2(%r10),%r12d
   14004d4cb:	mov    $0x1,%edi
   14004d4d0:	mov    %rcx,(%rsi)
   14004d4d3:	jmp    14004d294 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004d4d8:	lea    0x10(%rsi),%rcx
   14004d4dc:	lea    0x4(%r10),%r12d
   14004d4e0:	mov    $0x3,%edi
   14004d4e5:	mov    %rcx,(%rsi)
   14004d4e8:	jmp    14004d294 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004d4ed:	lea    0x10(%rsi),%rcx
   14004d4f1:	lea    0x3(%r10),%r12d
   14004d4f5:	mov    $0x2,%edi
   14004d4fa:	mov    %rcx,(%rsi)
   14004d4fd:	jmp    14004d294 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004d502:	lea    0x10(%rcx),%rcx
   14004d506:	xor    %edi,%edi
   14004d508:	mov    %rcx,(%rsi)
   14004d50b:	jmp    14004d4ae <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x57e>
   14004d50d:	nopl   (%rax)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\language.exe:     file format pei-x86-64


Disassembly of section .text:

000000014004dee0 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed>:
   14004dee0:	push   %r14
   14004dee2:	push   %r13
   14004dee4:	push   %r12
   14004dee6:	push   %rbp
   14004dee7:	push   %rdi
   14004dee8:	push   %rsi
   14004dee9:	push   %rbx
   14004deea:	sub    $0x90,%rsp
   14004def1:	movups %xmm6,0x80(%rsp)
   14004def9:	lea    0x40(%rsp),%rbp
   14004defe:	movapd %xmm1,%xmm3
   14004df02:	movapd %xmm1,%xmm6
   14004df06:	mov    %rcx,%rsi
   14004df09:	movl   $0x3,0x20(%rsp)
   14004df11:	lea    0x30(%rsp),%rcx
   14004df16:	lea    0x80(%rsp),%r8
   14004df1e:	mov    %rbp,%rdx
   14004df21:	call   140080638 <_ZSt8to_charsPcS_dSt12chars_format>
   14004df26:	mov    0x38(%rsp),%ecx
   14004df2a:	mov    0x30(%rsp),%rbx
   14004df2f:	test   %ecx,%ecx
   14004df31:	jne    14004e1a9 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x2c9>
   14004df37:	lea    0x10(%rsi),%rdi
   14004df3b:	sub    %rbp,%rbx
   14004df3e:	mov    %rdi,(%rsi)
   14004df41:	cmp    $0xf,%rbx
   14004df45:	ja     14004dff0 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x110>
   14004df4b:	cmp    $0x1,%rbx
   14004df4f:	jne    14004dfd8 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xf8>
   14004df55:	movzbl 0x40(%rsp),%eax
   14004df5a:	mov    %al,0x10(%rsi)
   14004df5d:	mov    %rdi,%rax
   14004df60:	andpd  0x5d7e8(%rip),%xmm6        # 1400ab750 <.rdata+0x1d0>
   14004df68:	movsd  0x5d7f0(%rip),%xmm0        # 1400ab760 <.rdata+0x1e0>
   14004df70:	mov    %rbx,0x8(%rsi)
   14004df74:	movb   $0x0,(%rax,%rbx,1)
   14004df78:	ucomisd %xmm6,%xmm0
   14004df7c:	jb     14004dfb8 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xd8>
   14004df7e:	mov    (%rsi),%r8
   14004df81:	xor    %edx,%edx
   14004df83:	test   %rbx,%rbx
   14004df86:	jne    14004df9d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xbd>
   14004df88:	jmp    14004e0f0 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x210>
   14004df8d:	nopl   (%rax)
   14004df90:	add    $0x1,%rdx
   14004df94:	cmp    %rdx,%rbx
   14004df97:	je     14004e030 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x150>
   14004df9d:	movzbl (%r8,%rdx,1),%ecx
   14004dfa2:	mov    %ecx,%eax
   14004dfa4:	and    $0xffffffdf,%eax
   14004dfa7:	cmp    $0x45,%al
   14004dfa9:	sete   %al
   14004dfac:	cmp    $0x2e,%cl
   14004dfaf:	sete   %cl
   14004dfb2:	or     %ecx,%eax
   14004dfb4:	test   $0x1,%al
   14004dfb6:	je     14004df90 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xb0>
   14004dfb8:	movups 0x80(%rsp),%xmm6
   14004dfc0:	mov    %rsi,%rax
   14004dfc3:	add    $0x90,%rsp
   14004dfca:	pop    %rbx
   14004dfcb:	pop    %rsi
   14004dfcc:	pop    %rdi
   14004dfcd:	pop    %rbp
   14004dfce:	pop    %r12
   14004dfd0:	pop    %r13
   14004dfd2:	pop    %r14
   14004dfd4:	ret
   14004dfd5:	nopl   (%rax)
   14004dfd8:	test   %rbx,%rbx
   14004dfdb:	je     14004df5d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x7d>
   14004dfe1:	mov    %rdi,%rcx
   14004dfe4:	jmp    14004e015 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x135>
   14004dfe6:	cs nopw 0x0(%rax,%rax,1)
   14004dff0:	test   %rbx,%rbx
   14004dff3:	js     14004e17e <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x29e>
   14004dff9:	mov    %rbx,%rcx
   14004dffc:	add    $0x1,%rcx
   14004e000:	js     14004e0a0 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1c0>
   14004e006:	call   140080620 <_Znwy>
   14004e00b:	mov    %rbx,0x10(%rsi)
   14004e00f:	mov    %rax,(%rsi)
   14004e012:	mov    %rax,%rcx
   14004e015:	mov    %rbx,%r8
   14004e018:	mov    %rbp,%rdx
   14004e01b:	call   140088118 <memcpy>
   14004e020:	mov    (%rsi),%rax
   14004e023:	jmp    14004df60 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x80>
   14004e028:	nopl   0x0(%rax,%rax,1)
   14004e030:	movabs $0x7fffffffffffffff,%rax
   14004e03a:	sub    %rbx,%rax
   14004e03d:	cmp    $0x1,%rax
   14004e041:	jbe    14004e18a <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x2aa>
   14004e047:	lea    0x2(%rbx),%rbp
   14004e04b:	cmp    %r8,%rdi
   14004e04e:	je     14004e116 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x236>
   14004e054:	mov    0x10(%rsi),%rax
   14004e058:	cmp    %rbp,%rax
   14004e05b:	jb     14004e078 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x198>
   14004e05d:	mov    $0x302e,%edx
   14004e062:	mov    %dx,(%r8,%rbx,1)
   14004e067:	mov    (%rsi),%r12
   14004e06a:	mov    %rbp,0x8(%rsi)
   14004e06e:	movb   $0x0,(%r12,%rbp,1)
   14004e073:	jmp    14004dfb8 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xd8>
   14004e078:	test   %rbp,%rbp
   14004e07b:	js     14004e172 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x292>
   14004e081:	lea    (%rax,%rax,1),%r13
   14004e085:	cmp    %r13,%rbp
   14004e088:	jae    14004e0a5 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1c5>
   14004e08a:	test   %r13,%r13
   14004e08d:	jns    14004e148 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x268>
   14004e093:	call   140080670 <_ZSt17__throw_bad_allocv>
   14004e098:	nopl   0x0(%rax,%rax,1)
   14004e0a0:	call   140080670 <_ZSt17__throw_bad_allocv>
   14004e0a5:	mov    %rbx,%rcx
   14004e0a8:	mov    %rbp,%r14
   14004e0ab:	add    $0x3,%rcx
   14004e0af:	js     14004e093 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1b3>
   14004e0b1:	call   140080620 <_Znwy>
   14004e0b6:	mov    %rax,%r12
   14004e0b9:	test   %rbx,%rbx
   14004e0bc:	jne    14004e15a <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x27a>
   14004e0c2:	mov    (%rsi),%rcx
   14004e0c5:	mov    $0x302e,%eax
   14004e0ca:	mov    %ax,(%r12,%rbx,1)
   14004e0cf:	cmp    %rcx,%rdi
   14004e0d2:	je     14004e0e1 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x201>
   14004e0d4:	mov    0x10(%rsi),%rax
   14004e0d8:	lea    0x1(%rax),%rdx
   14004e0dc:	call   140080628 <_ZdlPvy>
   14004e0e1:	mov    %rbp,0x10(%rsi)
   14004e0e5:	mov    %r14,%rbp
   14004e0e8:	mov    %r12,(%rsi)
   14004e0eb:	jmp    14004e06a <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x18a>
   14004e0f0:	mov    $0x2,%ebp
   14004e0f5:	cmp    %r8,%rdi
   14004e0f8:	je     14004e05d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x17d>
   14004e0fe:	cmpq   $0x1,0x10(%rsi)
   14004e103:	mov    $0x2,%r14d
   14004e109:	mov    $0x3,%ecx
   14004e10e:	ja     14004e05d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x17d>
   14004e114:	jmp    14004e0b1 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1d1>
   14004e116:	cmp    $0xf,%rbp
   14004e11a:	jbe    14004e05d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x17d>
   14004e120:	mov    $0x1f,%ecx
   14004e125:	call   140080620 <_Znwy>
   14004e12a:	mov    (%rsi),%rdx
   14004e12d:	mov    %rbp,%r14
   14004e130:	mov    %rax,%r12
   14004e133:	mov    $0x1e,%ebp
   14004e138:	mov    %rbx,%r8
   14004e13b:	mov    %r12,%rcx
   14004e13e:	call   140088118 <memcpy>
   14004e143:	jmp    14004e0c2 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1e2>
   14004e148:	lea    0x1(%r13),%rcx
   14004e14c:	call   140080620 <_Znwy>
   14004e151:	mov    %rbp,%r14
   14004e154:	mov    %rax,%r12
   14004e157:	mov    %r13,%rbp
   14004e15a:	mov    (%rsi),%rcx
   14004e15d:	mov    %rcx,%rdx
   14004e160:	cmp    $0x1,%rbx
   14004e164:	jne    14004e138 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x258>
   14004e166:	movzbl (%rcx),%eax
   14004e169:	mov    %al,(%r12)
   14004e16d:	jmp    14004e0c5 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1e5>
   14004e172:	lea    0x5d489(%rip),%rcx        # 1400ab602 <.rdata+0x82>
   14004e179:	call   140080660 <_ZSt20__throw_length_errorPKc>
   14004e17e:	lea    0x5d47d(%rip),%rcx        # 1400ab602 <.rdata+0x82>
   14004e185:	call   140080660 <_ZSt20__throw_length_errorPKc>
   14004e18a:	lea    0x5d576(%rip),%rcx        # 1400ab707 <.rdata+0x187>
   14004e191:	call   140080660 <_ZSt20__throw_length_errorPKc>
   14004e196:	mov    %rax,%rbx
   14004e199:	mov    %rsi,%rcx
   14004e19c:	call   140099c20 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14004e1a1:	mov    %rbx,%rcx
   14004e1a4:	call   140081f38 <_Unwind_Resume>
   14004e1a9:	mov    $0x10,%ecx
   14004e1ae:	call   140080618 <__cxa_allocate_exception>
   14004e1b3:	lea    0x5d538(%rip),%rdx        # 1400ab6f2 <.rdata+0x172>
   14004e1ba:	mov    %rax,%rcx
   14004e1bd:	mov    %rax,%rbx
   14004e1c0:	call   140080718 <_ZNSt13runtime_errorC1EPKc>
   14004e1c5:	lea    0x32534(%rip),%r8        # 140080700 <_ZNSt13runtime_errorD1Ev>
   14004e1cc:	lea    0x607ad(%rip),%rdx        # 1400ae980 <_ZTISt13runtime_error>
   14004e1d3:	mov    %rbx,%rcx
   14004e1d6:	call   1400805d0 <__cxa_throw>
   14004e1db:	mov    %rax,%rsi
   14004e1de:	mov    %rbx,%rcx
   14004e1e1:	call   140080600 <__cxa_free_exception>
   14004e1e6:	mov    %rsi,%rcx
   14004e1e9:	call   140081f38 <_Unwind_Resume>
   14004e1ee:	nop
   14004e1ef:	nop


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\diverse.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140004410 <tx_fn_m0_bench_parse_paths_0>:
   140004410:	push   %r15
   140004412:	push   %r14
   140004414:	push   %r13
   140004416:	push   %r12
   140004418:	push   %rsi
   140004419:	push   %rdi
   14000441a:	push   %rbp
   14000441b:	push   %rbx
   14000441c:	sub    $0xf8,%rsp
   140004423:	mov    %rcx,%rsi
   140004426:	mov    (%rcx),%rdi
   140004429:	lea    0xd8a20(%rip),%rax        # 1400dce50 <.rdata+0x3e50>
   140004430:	mov    %rax,0x50(%rsp)
   140004435:	lea    0xd8a34(%rip),%rax        # 1400dce70 <.rdata+0x3e70>
   14000443c:	mov    %rax,0x58(%rsp)
   140004441:	movq   $0xdb,0x60(%rsp)
   14000444a:	movq   $0x1,0x68(%rsp)
   140004453:	mov    %rdi,0x70(%rsp)
   140004458:	lea    0x50(%rsp),%rax
   14000445d:	mov    %rax,(%rcx)
   140004460:	lea    0xc0(%rsp),%r8
   140004468:	xor    %ecx,%ecx
   14000446a:	xor    %edx,%edx
   14000446c:	call   140047e00 <txrt_vector_new_str>
   140004471:	test   %eax,%eax
   140004473:	jne    140004a30 <tx_fn_m0_bench_parse_paths_0+0x620>
   140004479:	mov    0xc0(%rsp),%rcx
   140004481:	mov    %rcx,0x40(%rsp)
   140004486:	call   140047a20 <txrt_vector_ref_str>
   14000448b:	mov    %rax,%r15
   14000448e:	mov    %rsi,%rcx
   140004491:	call   14003b010 <txrt_gc_safepoint_context>
   140004496:	test   %eax,%eax
   140004498:	jne    140004a39 <tx_fn_m0_bench_parse_paths_0+0x629>
   14000449e:	lea    0xb8(%rsp),%r8
   1400044a6:	xor    %ecx,%ecx
   1400044a8:	xor    %edx,%edx
   1400044aa:	call   140047e00 <txrt_vector_new_str>
   1400044af:	test   %eax,%eax
   1400044b1:	jne    140004a4b <tx_fn_m0_bench_parse_paths_0+0x63b>
   1400044b7:	mov    0xb8(%rsp),%rcx
   1400044bf:	mov    %rcx,0x38(%rsp)
   1400044c4:	call   140047a20 <txrt_vector_ref_str>
   1400044c9:	mov    %rax,%r14
   1400044cc:	mov    %rsi,%rcx
   1400044cf:	call   14003b010 <txrt_gc_safepoint_context>
   1400044d4:	test   %eax,%eax
   1400044d6:	jne    140004a5d <tx_fn_m0_bench_parse_paths_0+0x64d>
   1400044dc:	lea    0xd837c(%rip),%r8        # 1400dc85f <.rdata+0x385f>
   1400044e3:	lea    0x48(%rsp),%rcx
   1400044e8:	xor    %edx,%edx
   1400044ea:	xor    %r9d,%r9d
   1400044ed:	call   140031370 <txrt_format_begin>
   1400044f2:	test   %eax,%eax
   1400044f4:	jne    140004635 <tx_fn_m0_bench_parse_paths_0+0x225>
   1400044fa:	mov    %rdi,0x80(%rsp)
   140004502:	lea    0xd84d6(%rip),%rbp        # 1400dc9df <.rdata+0x39df>
   140004509:	lea    0x48(%rsp),%rbx
   14000450e:	xor    %r12d,%r12d
   140004511:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140004520:	mov    0x48(%rsp),%rdi
   140004525:	mov    %rdi,%rcx
   140004528:	mov    %r12,%rdx
   14000452b:	lea    0xd836d(%rip),%r8        # 1400dc89f <.rdata+0x389f>
   140004532:	xor    %r9d,%r9d
   140004535:	call   140032230 <txrt_format_plain_i64>
   14000453a:	test   %eax,%eax
   14000453c:	jne    14000490e <tx_fn_m0_bench_parse_paths_0+0x4fe>
   140004542:	mov    %rdi,%rcx
   140004545:	call   140032c20 <txrt_format_finish>
   14000454a:	mov    %rsi,%rcx
   14000454d:	call   14003b010 <txrt_gc_safepoint_context>
   140004552:	test   %eax,%eax
   140004554:	jne    140004934 <tx_fn_m0_bench_parse_paths_0+0x524>
   14000455a:	mov    %rdi,%rcx
   14000455d:	lea    0xb0(%rsp),%rdx
   140004565:	call   140054650 <txrt_str_clone>
   14000456a:	test   %eax,%eax
   14000456c:	jne    140004943 <tx_fn_m0_bench_parse_paths_0+0x533>
   140004572:	mov    0xb0(%rsp),%r13
   14000457a:	mov    0x40(%rsp),%rcx
   14000457f:	mov    %r13,%rdx
   140004582:	call   1400482e0 <txrt_vector_push_back_str>
   140004587:	test   %eax,%eax
   140004589:	jne    14000494c <tx_fn_m0_bench_parse_paths_0+0x53c>
   14000458f:	mov    %r13,%rcx
   140004592:	call   140054790 <txrt_str_release>
   140004597:	mov    %rsi,%rcx
   14000459a:	call   14003b010 <txrt_gc_safepoint_context>
   14000459f:	test   %eax,%eax
   1400045a1:	jne    140004955 <tx_fn_m0_bench_parse_paths_0+0x545>
   1400045a7:	lea    0xa8(%rsp),%rax
   1400045af:	mov    %rax,0x20(%rsp)
   1400045b4:	mov    $0x1,%r8d
   1400045ba:	mov    %rdi,%rcx
   1400045bd:	mov    %rbp,%rdx
   1400045c0:	xor    %r9d,%r9d
   1400045c3:	call   14004add0 <txrt_str_concat_literal>
   1400045c8:	test   %eax,%eax
   1400045ca:	jne    140004964 <tx_fn_m0_bench_parse_paths_0+0x554>
   1400045d0:	mov    0xa8(%rsp),%r13
   1400045d8:	mov    0x38(%rsp),%rcx
   1400045dd:	mov    %r13,%rdx
   1400045e0:	call   1400482e0 <txrt_vector_push_back_str>
   1400045e5:	test   %eax,%eax
   1400045e7:	jne    14000496d <tx_fn_m0_bench_parse_paths_0+0x55d>
   1400045ed:	mov    %r13,%rcx
   1400045f0:	call   140054790 <txrt_str_release>
   1400045f5:	mov    %rsi,%rcx
   1400045f8:	call   14003b010 <txrt_gc_safepoint_context>
   1400045fd:	test   %eax,%eax
   1400045ff:	jne    140004976 <tx_fn_m0_bench_parse_paths_0+0x566>
   140004605:	mov    %rdi,%rcx
   140004608:	call   140054790 <txrt_str_release>
   14000460d:	inc    %r12
   140004610:	cmp    $0x3e8,%r12
   140004617:	je     140004659 <tx_fn_m0_bench_parse_paths_0+0x249>
   140004619:	mov    %rbx,%rcx
   14000461c:	xor    %edx,%edx
   14000461e:	lea    0xd823a(%rip),%r8        # 1400dc85f <.rdata+0x385f>
   140004625:	xor    %r9d,%r9d
   140004628:	call   140031370 <txrt_format_begin>
   14000462d:	test   %eax,%eax
   14000462f:	je     140004520 <tx_fn_m0_bench_parse_paths_0+0x110>
   140004635:	mov    %eax,%edi
   140004637:	lea    0xd8222(%rip),%rdx        # 1400dc860 <.rdata+0x3860>
   14000463e:	mov    $0xe1,%r8d
   140004644:	mov    $0x9,%r9d
   14000464a:	mov    %rsi,%rcx
   14000464d:	call   14004f650 <txrt_stack_error_location>
   140004652:	mov    %edi,%ecx
   140004654:	call   140053dd0 <txrt_require_success>
   140004659:	lea    0xa0(%rsp),%rcx
   140004661:	call   140030c50 <txrt_time_monotonic_micros>
   140004666:	test   %eax,%eax
   140004668:	jne    140004a6c <tx_fn_m0_bench_parse_paths_0+0x65c>
   14000466e:	mov    0xa0(%rsp),%rax
   140004676:	mov    %rax,0x78(%rsp)
   14000467b:	mov    %rsi,0x30(%rsp)
   140004680:	mov    %rsi,%rcx
   140004683:	call   14003b010 <txrt_gc_safepoint_context>
   140004688:	test   %eax,%eax
   14000468a:	jne    140004a7b <tx_fn_m0_bench_parse_paths_0+0x66b>
   140004690:	cmpq   $0x0,0x8(%r15)
   140004695:	je     140004861 <tx_fn_m0_bench_parse_paths_0+0x451>
   14000469b:	mov    $0x1,%ebp
   1400046a0:	mov    $0x1869f,%edi
   1400046a5:	xor    %ebx,%ebx
   1400046a7:	lea    0xe8(%rsp),%rsi
   1400046af:	lea    0x2f(%rsp),%r12
   1400046b4:	lea    0xf0(%rsp),%r13
   1400046bc:	xor    %eax,%eax
   1400046be:	xchg   %ax,%ax
   1400046c0:	mov    (%r15),%rcx
   1400046c3:	mov    (%rcx,%rax,8),%rcx
   1400046c7:	mov    %rsi,0x20(%rsp)
   1400046cc:	mov    $0xa,%edx
   1400046d1:	mov    %r12,%r8
   1400046d4:	mov    %r13,%r9
   1400046d7:	call   14004d940 <txrt_parse_int_scalar>
   1400046dc:	test   %eax,%eax
   1400046de:	jne    14000499a <tx_fn_m0_bench_parse_paths_0+0x58a>
   1400046e4:	cmpb   $0x1,0x2f(%rsp)
   1400046e9:	jne    1400046fa <tx_fn_m0_bench_parse_paths_0+0x2ea>
   1400046eb:	mov    %rbx,%rax
   1400046ee:	inc    %rax
   1400046f1:	jo     1400049cf <tx_fn_m0_bench_parse_paths_0+0x5bf>
   1400046f7:	mov    %rax,%rbx
   1400046fa:	sub    $0x1,%rdi
   1400046fe:	jb     140004726 <tx_fn_m0_bench_parse_paths_0+0x316>
   140004700:	mov    %ebp,%eax
   140004702:	imul   $0x10624dd3,%rax,%rax
   140004709:	shr    $0x26,%rax
   14000470d:	imul   $0x3e8,%eax,%eax
   140004713:	mov    %ebp,%ecx
   140004715:	sub    %eax,%ecx
   140004717:	mov    %ecx,%eax
   140004719:	inc    %ebp
   14000471b:	cmp    %rax,0x8(%r15)
   14000471f:	ja     1400046c0 <tx_fn_m0_bench_parse_paths_0+0x2b0>
   140004721:	jmp    140004861 <tx_fn_m0_bench_parse_paths_0+0x451>
   140004726:	lea    0xd8482(%rip),%rcx        # 1400dcbaf <.rdata+0x3baf>
   14000472d:	lea    0x98(%rsp),%r8
   140004735:	mov    $0xb,%edx
   14000473a:	call   1400543e0 <txrt_str_new>
   14000473f:	test   %eax,%eax
   140004741:	jne    140004a8a <tx_fn_m0_bench_parse_paths_0+0x67a>
   140004747:	mov    0x98(%rsp),%r15
   14000474f:	lea    0xd84aa(%rip),%rax        # 1400dcc00 <.rdata+0x3c00>
   140004756:	mov    %rax,0x58(%rsp)
   14000475b:	movq   $0xf0,0x60(%rsp)
   140004764:	movq   $0x5,0x68(%rsp)
   14000476d:	mov    0x30(%rsp),%rsi
   140004772:	mov    %rsi,%rcx
   140004775:	mov    %r15,%rdx
   140004778:	mov    0x78(%rsp),%r8
   14000477d:	mov    %rbx,%r9
   140004780:	call   1400015e0 <tx_fn_m0_report_0>
   140004785:	mov    %r15,%rcx
   140004788:	call   140054790 <txrt_str_release>
   14000478d:	mov    %rsi,%rcx
   140004790:	call   14003b010 <txrt_gc_safepoint_context>
   140004795:	test   %eax,%eax
   140004797:	jne    140004a99 <tx_fn_m0_bench_parse_paths_0+0x689>
   14000479d:	lea    0x90(%rsp),%rcx
   1400047a5:	call   140030c50 <txrt_time_monotonic_micros>
   1400047aa:	test   %eax,%eax
   1400047ac:	jne    140004aa8 <tx_fn_m0_bench_parse_paths_0+0x698>
   1400047b2:	mov    0x90(%rsp),%rdi
   1400047ba:	mov    %rsi,%rcx
   1400047bd:	call   14003b010 <txrt_gc_safepoint_context>
   1400047c2:	test   %eax,%eax
   1400047c4:	jne    140004ab7 <tx_fn_m0_bench_parse_paths_0+0x6a7>
   1400047ca:	cmpq   $0x0,0x8(%r14)
   1400047cf:	je     140004861 <tx_fn_m0_bench_parse_paths_0+0x451>
   1400047d5:	mov    $0x1,%ebp
   1400047da:	mov    $0x1869f,%r13d
   1400047e0:	xor    %ebx,%ebx
   1400047e2:	lea    0xd0(%rsp),%rsi
   1400047ea:	lea    0x2e(%rsp),%r15
   1400047ef:	lea    0xd8(%rsp),%r12
   1400047f7:	xor    %eax,%eax
   1400047f9:	nopl   0x0(%rax)
   140004800:	mov    (%r14),%rcx
   140004803:	mov    (%rcx,%rax,8),%rcx
   140004807:	mov    %rsi,0x20(%rsp)
   14000480c:	mov    $0xa,%edx
   140004811:	mov    %r15,%r8
   140004814:	mov    %r12,%r9
   140004817:	call   14004d940 <txrt_parse_int_scalar>
   14000481c:	test   %eax,%eax
   14000481e:	jne    1400049a9 <tx_fn_m0_bench_parse_paths_0+0x599>
   140004824:	cmpb   $0x0,0x2e(%rsp)
   140004829:	jne    14000483a <tx_fn_m0_bench_parse_paths_0+0x42a>
   14000482b:	mov    %rbx,%rax
   14000482e:	inc    %rax
   140004831:	jo     1400049f5 <tx_fn_m0_bench_parse_paths_0+0x5e5>
   140004837:	mov    %rax,%rbx
   14000483a:	sub    $0x1,%r13
   14000483e:	jb     140004866 <tx_fn_m0_bench_parse_paths_0+0x456>
   140004840:	mov    %ebp,%eax
   140004842:	imul   $0x10624dd3,%rax,%rax
   140004849:	shr    $0x26,%rax
   14000484d:	imul   $0x3e8,%eax,%eax
   140004853:	mov    %ebp,%ecx
   140004855:	sub    %eax,%ecx
   140004857:	mov    %ecx,%eax
   140004859:	inc    %ebp
   14000485b:	cmp    %rax,0x8(%r14)
   14000485f:	ja     140004800 <tx_fn_m0_bench_parse_paths_0+0x3f0>
   140004861:	call   140041370 <txrt_vector_index_error>
   140004866:	lea    0xd8512(%rip),%rcx        # 1400dcd7f <.rdata+0x3d7f>
   14000486d:	lea    0x88(%rsp),%r8
   140004875:	mov    $0xd,%edx
   14000487a:	call   1400543e0 <txrt_str_new>
   14000487f:	test   %eax,%eax
   140004881:	jne    140004ac6 <tx_fn_m0_bench_parse_paths_0+0x6b6>
   140004887:	mov    0x88(%rsp),%r14
   14000488f:	lea    0xd853a(%rip),%rax        # 1400dcdd0 <.rdata+0x3dd0>
   140004896:	mov    %rax,0x58(%rsp)
   14000489b:	movq   $0xfc,0x60(%rsp)
   1400048a4:	movq   $0x5,0x68(%rsp)
   1400048ad:	mov    0x30(%rsp),%rsi
   1400048b2:	mov    %rsi,%rcx
   1400048b5:	mov    %r14,%rdx
   1400048b8:	mov    %rdi,%r8
   1400048bb:	mov    %rbx,%r9
   1400048be:	call   1400015e0 <tx_fn_m0_report_0>
   1400048c3:	mov    %r14,%rcx
   1400048c6:	call   140054790 <txrt_str_release>
   1400048cb:	mov    %rsi,%rcx
   1400048ce:	call   14003b010 <txrt_gc_safepoint_context>
   1400048d3:	test   %eax,%eax
   1400048d5:	jne    140004ade <tx_fn_m0_bench_parse_paths_0+0x6ce>
   1400048db:	mov    0x38(%rsp),%rcx
   1400048e0:	call   14002b880 <txrt_value_release>
   1400048e5:	mov    0x40(%rsp),%rcx
   1400048ea:	call   14002b880 <txrt_value_release>
   1400048ef:	mov    0x80(%rsp),%rax
   1400048f7:	mov    %rax,(%rsi)
   1400048fa:	add    $0xf8,%rsp
   140004901:	pop    %rbx
   140004902:	pop    %rbp
   140004903:	pop    %rdi
   140004904:	pop    %rsi
   140004905:	pop    %r12
   140004907:	pop    %r13
   140004909:	pop    %r14
   14000490b:	pop    %r15
   14000490d:	ret
   14000490e:	mov    %eax,%r13d
   140004911:	lea    0xd7f88(%rip),%rdx        # 1400dc8a0 <.rdata+0x38a0>
   140004918:	mov    $0xe1,%r8d
   14000491e:	mov    $0x9,%r9d
   140004924:	mov    %rsi,%rcx
   140004927:	call   14004f650 <txrt_stack_error_location>
   14000492c:	mov    %r13d,%ecx
   14000492f:	call   140053dd0 <txrt_require_success>
   140004934:	lea    0xd7fa5(%rip),%rdx        # 1400dc8e0 <.rdata+0x38e0>
   14000493b:	mov    $0xe1,%r8d
   140004941:	jmp    140004983 <tx_fn_m0_bench_parse_paths_0+0x573>
   140004943:	lea    0xd7fd6(%rip),%rdx        # 1400dc920 <.rdata+0x3920>
   14000494a:	jmp    14000495c <tx_fn_m0_bench_parse_paths_0+0x54c>
   14000494c:	lea    0xd800d(%rip),%rdx        # 1400dc960 <.rdata+0x3960>
   140004953:	jmp    14000495c <tx_fn_m0_bench_parse_paths_0+0x54c>
   140004955:	lea    0xd8044(%rip),%rdx        # 1400dc9a0 <.rdata+0x39a0>
   14000495c:	mov    $0xe2,%r8d
   140004962:	jmp    140004983 <tx_fn_m0_bench_parse_paths_0+0x573>
   140004964:	lea    0xd8085(%rip),%rdx        # 1400dc9f0 <.rdata+0x39f0>
   14000496b:	jmp    14000497d <tx_fn_m0_bench_parse_paths_0+0x56d>
   14000496d:	lea    0xd80bc(%rip),%rdx        # 1400dca30 <.rdata+0x3a30>
   140004974:	jmp    14000497d <tx_fn_m0_bench_parse_paths_0+0x56d>
   140004976:	lea    0xd80f3(%rip),%rdx        # 1400dca70 <.rdata+0x3a70>
   14000497d:	mov    $0xe3,%r8d
   140004983:	mov    $0x9,%r9d
   140004989:	mov    %rsi,%rcx
   14000498c:	mov    %eax,%esi
   14000498e:	call   14004f650 <txrt_stack_error_location>
   140004993:	mov    %esi,%ecx
   140004995:	call   140053dd0 <txrt_require_success>
   14000499a:	lea    0xd818f(%rip),%rdx        # 1400dcb30 <.rdata+0x3b30>
   1400049a1:	mov    $0xea,%r8d
   1400049a7:	jmp    1400049b6 <tx_fn_m0_bench_parse_paths_0+0x5a6>
   1400049a9:	lea    0xd8350(%rip),%rdx        # 1400dcd00 <.rdata+0x3d00>
   1400049b0:	mov    $0xf6,%r8d
   1400049b6:	mov    $0x9,%r9d
   1400049bc:	mov    0x30(%rsp),%rcx
   1400049c1:	mov    %eax,%esi
   1400049c3:	call   14004f650 <txrt_stack_error_location>
   1400049c8:	mov    %esi,%ecx
   1400049ca:	call   140053dd0 <txrt_require_success>
   1400049cf:	lea    0xe0(%rsp),%r8
   1400049d7:	mov    $0x1,%edx
   1400049dc:	mov    %rbx,%rcx
   1400049df:	call   140054ed0 <txrt_add_i64>
   1400049e4:	mov    %eax,%edi
   1400049e6:	lea    0xd8183(%rip),%rdx        # 1400dcb70 <.rdata+0x3b70>
   1400049ed:	mov    $0xed,%r8d
   1400049f3:	jmp    140004a19 <tx_fn_m0_bench_parse_paths_0+0x609>
   1400049f5:	lea    0xc8(%rsp),%r8
   1400049fd:	mov    $0x1,%edx
   140004a02:	mov    %rbx,%rcx
   140004a05:	call   140054ed0 <txrt_add_i64>
   140004a0a:	mov    %eax,%edi
   140004a0c:	lea    0xd832d(%rip),%rdx        # 1400dcd40 <.rdata+0x3d40>
   140004a13:	mov    $0xf9,%r8d
   140004a19:	mov    $0xd,%r9d
   140004a1f:	mov    0x30(%rsp),%rcx
   140004a24:	call   14004f650 <txrt_stack_error_location>
   140004a29:	mov    %edi,%ecx
   140004a2b:	call   140053dd0 <txrt_require_success>
   140004a30:	lea    0xd7d29(%rip),%rdx        # 1400dc760 <.rdata+0x3760>
   140004a37:	jmp    140004a40 <tx_fn_m0_bench_parse_paths_0+0x630>
   140004a39:	lea    0xd7d60(%rip),%rdx        # 1400dc7a0 <.rdata+0x37a0>
   140004a40:	mov    $0xdd,%r8d
   140004a46:	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004a4b:	lea    0xd7d8e(%rip),%rdx        # 1400dc7e0 <.rdata+0x37e0>
   140004a52:	mov    $0xde,%r8d
   140004a58:	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004a5d:	lea    0xd7dbc(%rip),%rdx        # 1400dc820 <.rdata+0x3820>
   140004a64:	mov    $0xde,%r8d
   140004a6a:	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004a6c:	lea    0xd803d(%rip),%rdx        # 1400dcab0 <.rdata+0x3ab0>
   140004a73:	mov    $0xe7,%r8d
   140004a79:	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004a7b:	lea    0xd806e(%rip),%rdx        # 1400dcaf0 <.rdata+0x3af0>
   140004a82:	mov    $0xe7,%r8d
   140004a88:	jmp    140004ad3 <tx_fn_m0_bench_parse_paths_0+0x6c3>
   140004a8a:	lea    0xd812f(%rip),%rdx        # 1400dcbc0 <.rdata+0x3bc0>
   140004a91:	mov    $0xf0,%r8d
   140004a97:	jmp    140004ad3 <tx_fn_m0_bench_parse_paths_0+0x6c3>
   140004a99:	lea    0xd81a0(%rip),%rdx        # 1400dcc40 <.rdata+0x3c40>
   140004aa0:	mov    $0xf0,%r8d
   140004aa6:	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004aa8:	lea    0xd81d1(%rip),%rdx        # 1400dcc80 <.rdata+0x3c80>
   140004aaf:	mov    $0xf3,%r8d
   140004ab5:	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004ab7:	lea    0xd8202(%rip),%rdx        # 1400dccc0 <.rdata+0x3cc0>
   140004abe:	mov    $0xf3,%r8d
   140004ac4:	jmp    140004ad3 <tx_fn_m0_bench_parse_paths_0+0x6c3>
   140004ac6:	lea    0xd82c3(%rip),%rdx        # 1400dcd90 <.rdata+0x3d90>
   140004acd:	mov    $0xfc,%r8d
   140004ad3:	mov    $0x5,%r9d
   140004ad9:	jmp    1400049bc <tx_fn_m0_bench_parse_paths_0+0x5ac>
   140004ade:	lea    0xd832b(%rip),%rdx        # 1400dce10 <.rdata+0x3e10>
   140004ae5:	mov    $0xfc,%r8d
   140004aeb:	mov    $0x5,%r9d
   140004af1:	jmp    140004989 <tx_fn_m0_bench_parse_paths_0+0x579>
   140004af6:	cs nopw 0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\diverse.exe:     file format pei-x86-64


Disassembly of section .text:

000000014004d940 <txrt_parse_int_scalar>:
   14004d940:	push   %rdi
   14004d941:	push   %rsi
   14004d942:	push   %rbx
   14004d943:	sub    $0x40,%rsp
   14004d947:	mov    %r9,%rbx
   14004d94a:	mov    %rdx,%rdi
   14004d94d:	mov    %r8,%rsi
   14004d950:	call   14004b670 <_ZN12tx_generated6detail10text_valueB5cxx11EPKv>
   14004d955:	lea    0x30(%rsp),%rcx
   14004d95a:	mov    %rdi,%r8
   14004d95d:	mov    0x8(%rax),%rdx
   14004d961:	mov    (%rax),%rax
   14004d964:	mov    %rdx,0x20(%rsp)
   14004d969:	lea    0x20(%rsp),%rdx
   14004d96e:	mov    %rax,0x28(%rsp)
   14004d973:	call   14007b520 <_ZN12tx_generated16parse_int_scalarESt17basic_string_viewIcSt11char_traitsIcEEx>
   14004d978:	mov    0x38(%rsp),%rax
   14004d97d:	mov    0x30(%rsp),%rdx
   14004d982:	mov    0x989a7(%rip),%rcx        # 1400e6330 <.refptr.__emutls_v._ZN12tx_generated6detail14thread_contextE>
   14004d989:	test   %rax,%rax
   14004d98c:	sete   (%rsi)
   14004d98f:	mov    %rdx,(%rbx)
   14004d992:	mov    0x80(%rsp),%rdx
   14004d99a:	mov    %rax,(%rdx)
   14004d99d:	call   1400b35c0 <__emutls_get_address>
   14004d9a2:	mov    (%rax),%rax
   14004d9a5:	test   %rax,%rax
   14004d9a8:	je     14004d9b8 <txrt_parse_int_scalar+0x78>
   14004d9aa:	mov    0x8(%rax),%eax
   14004d9ad:	add    $0x40,%rsp
   14004d9b1:	pop    %rbx
   14004d9b2:	pop    %rsi
   14004d9b3:	pop    %rdi
   14004d9b4:	ret
   14004d9b5:	nopl   (%rax)
   14004d9b8:	call   14004ecf0 <_ZN12tx_generated6detail26initialize_runtime_contextEv>
   14004d9bd:	mov    0x8(%rax),%eax
   14004d9c0:	add    $0x40,%rsp
   14004d9c4:	pop    %rbx
   14004d9c5:	pop    %rsi
   14004d9c6:	pop    %rdi
   14004d9c7:	ret
   14004d9c8:	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\borrowing.exe:     file format pei-x86-64


Disassembly of section .text:

00000001400019e0 <tx_fn_m0_bench_map_0>:
   1400019e0:	push   %r15
   1400019e2:	push   %r14
   1400019e4:	push   %r13
   1400019e6:	push   %r12
   1400019e8:	push   %rsi
   1400019e9:	push   %rdi
   1400019ea:	push   %rbx
   1400019eb:	sub    $0x80,%rsp
   1400019f2:	mov    %rcx,%rsi
   1400019f5:	mov    (%rcx),%r13
   1400019f8:	lea    0xa0f2b(%rip),%rax        # 1400a292a <.rdata+0x92a>
   1400019ff:	mov    %rax,0x48(%rsp)
   140001a04:	lea    0xa0f35(%rip),%rax        # 1400a2940 <.rdata+0x940>
   140001a0b:	mov    %rax,0x50(%rsp)
   140001a10:	movq   $0x14,0x58(%rsp)
   140001a19:	movq   $0x1,0x60(%rsp)
   140001a22:	mov    %r13,0x68(%rsp)
   140001a27:	lea    0x48(%rsp),%rax
   140001a2c:	mov    %rax,(%rcx)
   140001a2f:	lea    0x40(%rsp),%rcx
   140001a34:	call   14001b9b0 <txrt_map_new_i64_i64>
   140001a39:	test   %eax,%eax
   140001a3b:	jne    140001c06 <tx_fn_m0_bench_map_0+0x226>
   140001a41:	mov    0x40(%rsp),%rdi
   140001a46:	mov    %rsi,%rcx
   140001a49:	call   140013620 <txrt_gc_safepoint_context>
   140001a4e:	test   %eax,%eax
   140001a50:	jne    140001c0f <tx_fn_m0_bench_map_0+0x22f>
   140001a56:	mov    $0x7,%edx
   140001a5b:	mov    $0x3,%r8d
   140001a61:	mov    %rdi,%rcx
   140001a64:	call   140020f30 <txrt_map_set_i64_i64>
   140001a69:	test   %eax,%eax
   140001a6b:	jne    140001c21 <tx_fn_m0_bench_map_0+0x241>
   140001a71:	mov    %rsi,%rcx
   140001a74:	call   140013620 <txrt_gc_safepoint_context>
   140001a79:	test   %eax,%eax
   140001a7b:	jne    140001c33 <tx_fn_m0_bench_map_0+0x253>
   140001a81:	lea    0x38(%rsp),%rcx
   140001a86:	call   14000c000 <txrt_time_monotonic_micros>
   140001a8b:	test   %eax,%eax
   140001a8d:	jne    140001c45 <tx_fn_m0_bench_map_0+0x265>
   140001a93:	mov    0x38(%rsp),%r14
   140001a98:	mov    %rsi,%rcx
   140001a9b:	call   140013620 <txrt_gc_safepoint_context>
   140001aa0:	test   %eax,%eax
   140001aa2:	jne    140001c57 <tx_fn_m0_bench_map_0+0x277>
   140001aa8:	lea    0x20(%rsp),%r8
   140001aad:	mov    $0x7,%edx
   140001ab2:	mov    %rdi,%rcx
   140001ab5:	call   14001ccb0 <txrt_map_read_i64_i64>
   140001aba:	test   %eax,%eax
   140001abc:	jne    140001afa <tx_fn_m0_bench_map_0+0x11a>
   140001abe:	mov    $0xf4240,%r12d
   140001ac4:	xor    %ecx,%ecx
   140001ac6:	lea    0x20(%rsp),%r15
   140001acb:	xor    %ebx,%ebx
   140001acd:	nopl   (%rax)
   140001ad0:	mov    0x20(%rsp),%rdx
   140001ad5:	add    %rdx,%rbx
   140001ad8:	jo     140001bd8 <tx_fn_m0_bench_map_0+0x1f8>
   140001ade:	dec    %r12
   140001ae1:	je     140001b08 <tx_fn_m0_bench_map_0+0x128>
   140001ae3:	mov    $0x7,%edx
   140001ae8:	mov    %rdi,%rcx
   140001aeb:	mov    %r15,%r8
   140001aee:	call   14001ccb0 <txrt_map_read_i64_i64>
   140001af3:	mov    %rbx,%rcx
   140001af6:	test   %eax,%eax
   140001af8:	je     140001ad0 <tx_fn_m0_bench_map_0+0xf0>
   140001afa:	mov    %eax,%edi
   140001afc:	lea    0xa0b6d(%rip),%rdx        # 1400a2670 <.rdata+0x670>
   140001b03:	jmp    140001beb <tx_fn_m0_bench_map_0+0x20b>
   140001b08:	lea    0xa0bdb(%rip),%rcx        # 1400a26ea <.rdata+0x6ea>
   140001b0f:	lea    0x30(%rsp),%r8
   140001b14:	mov    $0x3,%edx
   140001b19:	call   14002bcd0 <txrt_str_new>
   140001b1e:	test   %eax,%eax
   140001b20:	jne    140001c66 <tx_fn_m0_bench_map_0+0x286>
   140001b26:	mov    0x30(%rsp),%r15
   140001b2b:	lea    0x28(%rsp),%rcx
   140001b30:	call   14000c000 <txrt_time_monotonic_micros>
   140001b35:	test   %eax,%eax
   140001b37:	jne    140001c6f <tx_fn_m0_bench_map_0+0x28f>
   140001b3d:	mov    0x28(%rsp),%rcx
   140001b42:	mov    %rcx,%r12
   140001b45:	sub    %r14,%r12
   140001b48:	jo     140001c78 <tx_fn_m0_bench_map_0+0x298>
   140001b4e:	mov    %r15,%rcx
   140001b51:	xor    %edx,%edx
   140001b53:	call   14002c2a0 <txrt_print_str>
   140001b58:	test   %eax,%eax
   140001b5a:	jne    140001ca9 <tx_fn_m0_bench_map_0+0x2c9>
   140001b60:	mov    $0x20,%cl
   140001b62:	call   14002d940 <txrt_print_char>
   140001b67:	test   %eax,%eax
   140001b69:	jne    140001cb2 <tx_fn_m0_bench_map_0+0x2d2>
   140001b6f:	mov    %r12,%rcx
   140001b72:	xor    %edx,%edx
   140001b74:	call   14002b7c0 <txrt_print_i64>
   140001b79:	test   %eax,%eax
   140001b7b:	jne    140001cbb <tx_fn_m0_bench_map_0+0x2db>
   140001b81:	mov    $0x20,%cl
   140001b83:	call   14002d940 <txrt_print_char>
   140001b88:	test   %eax,%eax
   140001b8a:	jne    140001cc4 <tx_fn_m0_bench_map_0+0x2e4>
   140001b90:	mov    %rbx,%rcx
   140001b93:	mov    $0x1,%dl
   140001b95:	call   14002b7c0 <txrt_print_i64>
   140001b9a:	test   %eax,%eax
   140001b9c:	jne    140001ccd <tx_fn_m0_bench_map_0+0x2ed>
   140001ba2:	mov    %r15,%rcx
   140001ba5:	call   14002c080 <txrt_str_release>
   140001baa:	mov    %rsi,%rcx
   140001bad:	call   140013620 <txrt_gc_safepoint_context>
   140001bb2:	test   %eax,%eax
   140001bb4:	jne    140001cd6 <tx_fn_m0_bench_map_0+0x2f6>
   140001bba:	mov    %rdi,%rcx
   140001bbd:	call   140006c30 <txrt_value_release>
   140001bc2:	mov    %r13,(%rsi)
   140001bc5:	add    $0x80,%rsp
   140001bcc:	pop    %rbx
   140001bcd:	pop    %rdi
   140001bce:	pop    %rsi
   140001bcf:	pop    %r12
   140001bd1:	pop    %r13
   140001bd3:	pop    %r14
   140001bd5:	pop    %r15
   140001bd7:	ret
   140001bd8:	lea    0x78(%rsp),%r8
   140001bdd:	call   14002c7c0 <txrt_add_i64>
   140001be2:	mov    %eax,%edi
   140001be4:	lea    0xa0ac5(%rip),%rdx        # 1400a26b0 <.rdata+0x6b0>
   140001beb:	mov    $0x1c,%r8d
   140001bf1:	mov    $0x9,%r9d
   140001bf7:	mov    %rsi,%rcx
   140001bfa:	call   140026f40 <txrt_stack_error_location>
   140001bff:	mov    %edi,%ecx
   140001c01:	call   14002b6c0 <txrt_require_success>
   140001c06:	lea    0xa08e3(%rip),%rdx        # 1400a24f0 <.rdata+0x4f0>
   140001c0d:	jmp    140001c16 <tx_fn_m0_bench_map_0+0x236>
   140001c0f:	lea    0xa091a(%rip),%rdx        # 1400a2530 <.rdata+0x530>
   140001c16:	mov    $0x16,%r8d
   140001c1c:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c21:	lea    0xa0948(%rip),%rdx        # 1400a2570 <.rdata+0x570>
   140001c28:	mov    $0x17,%r8d
   140001c2e:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c33:	lea    0xa0976(%rip),%rdx        # 1400a25b0 <.rdata+0x5b0>
   140001c3a:	mov    $0x17,%r8d
   140001c40:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c45:	lea    0xa09a4(%rip),%rdx        # 1400a25f0 <.rdata+0x5f0>
   140001c4c:	mov    $0x19,%r8d
   140001c52:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c57:	lea    0xa09d2(%rip),%rdx        # 1400a2630 <.rdata+0x630>
   140001c5e:	mov    $0x19,%r8d
   140001c64:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c66:	lea    0xa0a83(%rip),%rdx        # 1400a26f0 <.rdata+0x6f0>
   140001c6d:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001c6f:	lea    0xa0aba(%rip),%rdx        # 1400a2730 <.rdata+0x730>
   140001c76:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001c78:	lea    0x70(%rsp),%r8
   140001c7d:	mov    %r14,%rdx
   140001c80:	call   14002c8a0 <txrt_sub_i64>
   140001c85:	mov    %eax,%edi
   140001c87:	lea    0xa0ae2(%rip),%rdx        # 1400a2770 <.rdata+0x770>
   140001c8e:	mov    $0x1e,%r8d
   140001c94:	mov    $0x5,%r9d
   140001c9a:	mov    %rsi,%rcx
   140001c9d:	call   140026f40 <txrt_stack_error_location>
   140001ca2:	mov    %edi,%ecx
   140001ca4:	call   14002b6c0 <txrt_require_success>
   140001ca9:	lea    0xa0b00(%rip),%rdx        # 1400a27b0 <.rdata+0x7b0>
   140001cb0:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cb2:	lea    0xa0b37(%rip),%rdx        # 1400a27f0 <.rdata+0x7f0>
   140001cb9:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cbb:	lea    0xa0b6e(%rip),%rdx        # 1400a2830 <.rdata+0x830>
   140001cc2:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cc4:	lea    0xa0ba5(%rip),%rdx        # 1400a2870 <.rdata+0x870>
   140001ccb:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001ccd:	lea    0xa0bdc(%rip),%rdx        # 1400a28b0 <.rdata+0x8b0>
   140001cd4:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cd6:	lea    0xa0c13(%rip),%rdx        # 1400a28f0 <.rdata+0x8f0>
   140001cdd:	mov    $0x1e,%r8d
   140001ce3:	mov    $0x5,%r9d
   140001ce9:	mov    %rsi,%rcx
   140001cec:	mov    %eax,%esi
   140001cee:	call   140026f40 <txrt_stack_error_location>
   140001cf3:	mov    %esi,%ecx
   140001cf5:	call   14002b6c0 <txrt_require_success>
   140001cfa:	int3
   140001cfb:	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_15_16\candidate\borrowing.exe:     file format pei-x86-64


Disassembly of section .text:

000000014001ccb0 <txrt_map_read_i64_i64>:
   14001ccb0:	push   %rdi
   14001ccb1:	push   %rsi
   14001ccb2:	push   %rbx
   14001ccb3:	sub    $0x20,%rsp
   14001ccb7:	mov    %rdx,%rbx
   14001ccba:	mov    %r8,%rsi
   14001ccbd:	call   14009bcf0 <_ZSt12__any_casterISt10shared_ptrIN12tx_generated17container_storageEEEPvPKSt3any>
   14001ccc2:	test   %rax,%rax
   14001ccc5:	je     14001cd97 <txrt_map_read_i64_i64+0xe7>
   14001cccb:	mov    (%rax),%rcx
   14001ccce:	cmpq   $0x0,0x28(%rcx)
   14001ccd3:	jne    14001cd30 <txrt_map_read_i64_i64+0x80>
   14001ccd5:	mov    0x20(%rcx),%rdx
   14001ccd9:	test   %rdx,%rdx
   14001ccdc:	jne    14001cd18 <txrt_map_read_i64_i64+0x68>
   14001ccde:	mov    $0x10,%ecx
   14001cce3:	call   140074b58 <__cxa_allocate_exception>
   14001cce8:	lea    0x87c22(%rip),%rdx        # 1400a4911 <.rdata+0x151>
   14001ccef:	mov    %rax,%rcx
   14001ccf2:	mov    %rax,%rdi
   14001ccf5:	call   140074ca0 <_ZNSt12out_of_rangeC1EPKc>
   14001ccfa:	lea    0x57f97(%rip),%r8        # 140074c98 <_ZNSt12out_of_rangeD1Ev>
   14001cd01:	lea    0x8d208(%rip),%rdx        # 1400a9f10 <_ZTISt12out_of_range>
   14001cd08:	mov    %rdi,%rcx
   14001cd0b:	call   140074b18 <__cxa_throw>
   14001cd10:	mov    (%rdx),%rdx
   14001cd13:	test   %rdx,%rdx
   14001cd16:	je     14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd18:	cmp    0x8(%rdx),%rbx
   14001cd1c:	jne    14001cd10 <txrt_map_read_i64_i64+0x60>
   14001cd1e:	mov    0x10(%rdx),%rax
   14001cd22:	mov    %rax,(%rsi)
   14001cd25:	xor    %eax,%eax
   14001cd27:	add    $0x20,%rsp
   14001cd2b:	pop    %rbx
   14001cd2c:	pop    %rsi
   14001cd2d:	pop    %rdi
   14001cd2e:	ret
   14001cd2f:	nop
   14001cd30:	mov    0x18(%rcx),%r10
   14001cd34:	mov    %rbx,%rax
   14001cd37:	xor    %edx,%edx
   14001cd39:	div    %r10
   14001cd3c:	mov    0x10(%rcx),%rax
   14001cd40:	mov    (%rax,%rdx,8),%r11
   14001cd44:	mov    %rdx,%r8
   14001cd47:	test   %r11,%r11
   14001cd4a:	je     14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd4c:	mov    (%r11),%rax
   14001cd4f:	mov    0x18(%rax),%rcx
   14001cd53:	jmp    14001cd7f <txrt_map_read_i64_i64+0xcf>
   14001cd55:	nopl   (%rax)
   14001cd58:	mov    (%rax),%r9
   14001cd5b:	test   %r9,%r9
   14001cd5e:	je     14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd64:	mov    0x18(%r9),%rcx
   14001cd68:	mov    %rax,%r11
   14001cd6b:	xor    %edx,%edx
   14001cd6d:	mov    %rcx,%rax
   14001cd70:	div    %r10
   14001cd73:	cmp    %rdx,%r8
   14001cd76:	jne    14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd7c:	mov    %r9,%rax
   14001cd7f:	cmp    %rbx,%rcx
   14001cd82:	jne    14001cd58 <txrt_map_read_i64_i64+0xa8>
   14001cd84:	cmp    0x8(%rax),%rcx
   14001cd88:	jne    14001cd58 <txrt_map_read_i64_i64+0xa8>
   14001cd8a:	mov    (%r11),%rdx
   14001cd8d:	test   %rdx,%rdx
   14001cd90:	jne    14001cd1e <txrt_map_read_i64_i64+0x6e>
   14001cd92:	jmp    14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd97:	call   14009c1d0 <_ZSt20__throw_bad_any_castv>
   14001cd9c:	mov    %rax,%rcx
   14001cd9f:	mov    %rdx,%rax
   14001cda2:	cmp    $0x3,%rax
   14001cda6:	je     14001cdf4 <txrt_map_read_i64_i64+0x144>
   14001cda8:	jg     14001cdba <txrt_map_read_i64_i64+0x10a>
   14001cdaa:	cmp    $0x1,%rax
   14001cdae:	je     14001ce25 <txrt_map_read_i64_i64+0x175>
   14001cdb0:	cmp    $0x2,%rax
   14001cdb4:	je     14001ce4c <txrt_map_read_i64_i64+0x19c>
   14001cdba:	call   140074b50 <__cxa_begin_catch>
   14001cdbf:	lea    0x87af9(%rip),%r8        # 1400a48bf <.rdata+0xff>
   14001cdc6:	mov    $0x1,%ecx
   14001cdcb:	lea    0x87b03(%rip),%rdx        # 1400a48d5 <.rdata+0x115>
   14001cdd2:	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001cdd7:	call   140074b48 <__cxa_end_catch>
   14001cddc:	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001cdde:	mov    %rax,%rbx
   14001cde1:	mov    %rdx,%rsi
   14001cde4:	mov    %rdi,%rcx
   14001cde7:	call   140074b40 <__cxa_free_exception>
   14001cdec:	mov    %rbx,%rcx
   14001cdef:	mov    %rsi,%rax
   14001cdf2:	jmp    14001cda2 <txrt_map_read_i64_i64+0xf2>
   14001cdf4:	call   140074b50 <__cxa_begin_catch>
   14001cdf9:	mov    %rax,%rcx
   14001cdfc:	mov    (%rax),%rax
   14001cdff:	call   *0x10(%rax)
   14001ce02:	lea    0x87aa5(%rip),%rdx        # 1400a48ae <.rdata+0xee>
   14001ce09:	mov    $0x1,%ecx
   14001ce0e:	mov    %rax,%r8
   14001ce11:	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce16:	call   140074b48 <__cxa_end_catch>
   14001ce1b:	mov    $0x1,%eax
   14001ce20:	jmp    14001cd27 <txrt_map_read_i64_i64+0x77>
   14001ce25:	call   140074b50 <__cxa_begin_catch>
   14001ce2a:	mov    %rax,%rbx
   14001ce2d:	mov    (%rax),%rax
   14001ce30:	mov    %rbx,%rcx
   14001ce33:	call   *0x10(%rax)
   14001ce36:	mov    0x18(%rbx),%rdx
   14001ce3a:	mov    0x10(%rbx),%ecx
   14001ce3d:	mov    %rax,%r8
   14001ce40:	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce45:	call   140074b48 <__cxa_end_catch>
   14001ce4a:	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001ce4c:	call   140074b50 <__cxa_begin_catch>
   14001ce51:	mov    %rax,%rcx
   14001ce54:	mov    (%rax),%rax
   14001ce57:	call   *0x10(%rax)
   14001ce5a:	lea    0x87a3b(%rip),%rdx        # 1400a489c <.rdata+0xdc>
   14001ce61:	mov    $0x1,%ecx
   14001ce66:	mov    %rax,%r8
   14001ce69:	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce6e:	call   140074b48 <__cxa_end_catch>
   14001ce73:	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001ce75:	data16 cs nopw 0x0(%rax,%rax,1)
