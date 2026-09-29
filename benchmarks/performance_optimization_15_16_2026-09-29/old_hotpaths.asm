
E:\Project\other\Compilation\tx_build\performance_15_16\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

000000014002f860 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE>:
   14002f860:	push   %r15
   14002f862:	push   %r14
   14002f864:	push   %r13
   14002f866:	push   %r12
   14002f868:	push   %rbp
   14002f869:	push   %rdi
   14002f86a:	push   %rsi
   14002f86b:	push   %rbx
   14002f86c:	sub    $0x98,%rsp
   14002f873:	mov    %rdx,%rsi
   14002f876:	mov    (%rdx),%rdx
   14002f879:	mov    %rcx,%rbx
   14002f87c:	mov    0x8(%rdx),%rax
   14002f880:	sub    (%rdx),%rax
   14002f883:	shr    $0x3e,%rax
   14002f887:	jne    14002fc25 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x3c5>
   14002f88d:	movb   $0x0,0x10(%rcx)
   14002f891:	mov    (%rsi),%rax
   14002f894:	lea    0x10(%rcx),%r13
   14002f898:	mov    %r13,(%rcx)
   14002f89b:	mov    %r13,%r9
   14002f89e:	mov    0x8(%rax),%rdx
   14002f8a2:	mov    (%rax),%rbp
   14002f8a5:	movq   $0x0,0x8(%rcx)
   14002f8ad:	mov    %rdx,0x28(%rsp)
   14002f8b2:	sub    %rbp,%rdx
   14002f8b5:	mov    %rdx,%rdi
   14002f8b8:	add    %rdi,%rdi
   14002f8bb:	cmp    $0xf,%rdi
   14002f8bf:	ja     14002fb20 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x2c0>
   14002f8c5:	cmp    %rbp,0x28(%rsp)
   14002f8ca:	je     14002fae0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x280>
   14002f8d0:	lea    0x5c499(%rip),%r14        # 14008bd70 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE>
   14002f8d7:	jmp    14002f93b <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0xdb>
   14002f8d9:	nopl   0x0(%rax)
   14002f8e0:	mov    %r15b,(%r9,%rsi,1)
   14002f8e4:	mov    (%rbx),%rax
   14002f8e7:	and    $0xf,%edi
   14002f8ea:	mov    %r12,0x8(%rbx)
   14002f8ee:	movzbl (%r14,%rdi,1),%r12d
   14002f8f3:	movb   $0x0,0x1(%rax,%rsi,1)
   14002f8f8:	mov    0x8(%rbx),%rsi
   14002f8fc:	mov    (%rbx),%r9
   14002f8ff:	lea    0x1(%rsi),%rdi
   14002f903:	cmp    %r9,%r13
   14002f906:	je     14002fa98 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x238>
   14002f90c:	mov    0x10(%rbx),%rax
   14002f910:	cmp    %rdi,%rax
   14002f913:	jb     14002f9e0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x180>
   14002f919:	mov    %r12b,(%r9,%rsi,1)
   14002f91d:	mov    (%rbx),%rax
   14002f920:	add    $0x1,%rbp
   14002f924:	mov    %rdi,0x8(%rbx)
   14002f928:	movb   $0x0,0x1(%rax,%rsi,1)
   14002f92d:	cmp    %rbp,0x28(%rsp)
   14002f932:	je     14002fae0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x280>
   14002f938:	mov    (%rbx),%r9
   14002f93b:	movzbl 0x0(%rbp),%edi
   14002f93f:	mov    0x8(%rbx),%rsi
   14002f943:	mov    %edi,%eax
   14002f945:	lea    0x1(%rsi),%r12
   14002f949:	shr    $0x4,%al
   14002f94c:	and    $0xf,%eax
   14002f94f:	movzbl (%r14,%rax,1),%r15d
   14002f954:	cmp    %r9,%r13
   14002f957:	je     14002fa48 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x1e8>
   14002f95d:	mov    0x10(%rbx),%rax
   14002f961:	cmp    %r12,%rax
   14002f964:	jae    14002f8e0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x80>
   14002f96a:	test   %r12,%r12
   14002f96d:	js     14002fbe8 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x388>
   14002f973:	add    %rax,%rax
   14002f976:	mov    %rax,0x30(%rsp)
   14002f97b:	cmp    %rax,%r12
   14002f97e:	jb     14002fbb0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x350>
   14002f984:	mov    %rsi,%rcx
   14002f987:	add    $0x2,%rcx
   14002f98b:	js     14002fbb5 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x355>
   14002f991:	mov    %r12,0x30(%rsp)
   14002f996:	call   14006c9b0 <_Znwy>
   14002f99b:	mov    (%rbx),%r10
   14002f99e:	mov    %rax,%r9
   14002f9a1:	test   %rsi,%rsi
   14002f9a4:	jne    14002fa6b <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x20b>
   14002f9aa:	cmp    %r13,%r10
   14002f9ad:	je     14002f9c9 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x169>
   14002f9af:	mov    0x10(%rbx),%rax
   14002f9b3:	mov    %r10,%rcx
   14002f9b6:	mov    %r9,0x38(%rsp)
   14002f9bb:	lea    0x1(%rax),%rdx
   14002f9bf:	call   14006c9b8 <_ZdlPvy>
   14002f9c4:	mov    0x38(%rsp),%r9
   14002f9c9:	mov    0x30(%rsp),%rax
   14002f9ce:	mov    %r9,(%rbx)
   14002f9d1:	mov    %rax,0x10(%rbx)
   14002f9d5:	jmp    14002f8e0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x80>
   14002f9da:	nopw   0x0(%rax,%rax,1)
   14002f9e0:	test   %rdi,%rdi
   14002f9e3:	js     14002fbdc <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x37c>
   14002f9e9:	lea    (%rax,%rax,1),%r15
   14002f9ed:	cmp    %r15,%rdi
   14002f9f0:	jb     14002fbc0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x360>
   14002f9f6:	mov    %rsi,%rcx
   14002f9f9:	add    $0x2,%rcx
   14002f9fd:	js     14002fbc5 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x365>
   14002fa03:	mov    %rdi,%r15
   14002fa06:	call   14006c9b0 <_Znwy>
   14002fa0b:	mov    (%rbx),%r10
   14002fa0e:	mov    %rax,%r9
   14002fa11:	test   %rsi,%rsi
   14002fa14:	jne    14002fab8 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x258>
   14002fa1a:	cmp    %r10,%r13
   14002fa1d:	je     14002fa39 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x1d9>
   14002fa1f:	mov    0x10(%rbx),%rax
   14002fa23:	mov    %r10,%rcx
   14002fa26:	mov    %r9,0x30(%rsp)
   14002fa2b:	lea    0x1(%rax),%rdx
   14002fa2f:	call   14006c9b8 <_ZdlPvy>
   14002fa34:	mov    0x30(%rsp),%r9
   14002fa39:	mov    %r9,(%rbx)
   14002fa3c:	mov    %r15,0x10(%rbx)
   14002fa40:	jmp    14002f919 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0xb9>
   14002fa45:	nopl   (%rax)
   14002fa48:	cmp    $0x10,%r12
   14002fa4c:	jne    14002f8e0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x80>
   14002fa52:	mov    $0x1f,%ecx
   14002fa57:	call   14006c9b0 <_Znwy>
   14002fa5c:	movq   $0x1e,0x30(%rsp)
   14002fa65:	mov    (%rbx),%r10
   14002fa68:	mov    %rax,%r9
   14002fa6b:	cmp    $0x1,%rsi
   14002fa6f:	je     14002fb00 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x2a0>
   14002fa75:	mov    %r10,%rdx
   14002fa78:	mov    %r9,%rcx
   14002fa7b:	mov    %rsi,%r8
   14002fa7e:	mov    %r10,0x38(%rsp)
   14002fa83:	call   140074648 <memcpy>
   14002fa88:	mov    0x38(%rsp),%r10
   14002fa8d:	mov    %rax,%r9
   14002fa90:	jmp    14002f9aa <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x14a>
   14002fa95:	nopl   (%rax)
   14002fa98:	cmp    $0x10,%rdi
   14002fa9c:	jne    14002f919 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0xb9>
   14002faa2:	mov    $0x1f,%ecx
   14002faa7:	call   14006c9b0 <_Znwy>
   14002faac:	mov    (%rbx),%r10
   14002faaf:	mov    %rax,%r9
   14002fab2:	mov    $0x1e,%r15d
   14002fab8:	cmp    $0x1,%rsi
   14002fabc:	je     14002fb10 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x2b0>
   14002fabe:	mov    %r10,%rdx
   14002fac1:	mov    %r9,%rcx
   14002fac4:	mov    %rsi,%r8
   14002fac7:	mov    %r10,0x30(%rsp)
   14002facc:	call   140074648 <memcpy>
   14002fad1:	mov    0x30(%rsp),%r10
   14002fad6:	mov    %rax,%r9
   14002fad9:	jmp    14002fa1a <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x1ba>
   14002fade:	xchg   %ax,%ax
   14002fae0:	mov    %rbx,%rax
   14002fae3:	add    $0x98,%rsp
   14002faea:	pop    %rbx
   14002faeb:	pop    %rsi
   14002faec:	pop    %rdi
   14002faed:	pop    %rbp
   14002faee:	pop    %r12
   14002faf0:	pop    %r13
   14002faf2:	pop    %r14
   14002faf4:	pop    %r15
   14002faf6:	ret
   14002faf7:	nopw   0x0(%rax,%rax,1)
   14002fb00:	movzbl (%r10),%eax
   14002fb04:	mov    %al,(%r9)
   14002fb07:	jmp    14002f9aa <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x14a>
   14002fb0c:	nopl   0x0(%rax)
   14002fb10:	movzbl (%r10),%eax
   14002fb14:	mov    %al,(%r9)
   14002fb17:	jmp    14002fa1a <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x1ba>
   14002fb1c:	nopl   0x0(%rax)
   14002fb20:	test   %rdi,%rdi
   14002fb23:	js     14002fbf4 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x394>
   14002fb29:	lea    0x1(%rdi),%rcx
   14002fb2d:	cmp    $0x1d,%rdi
   14002fb31:	jbe    14002fba0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x340>
   14002fb33:	call   14006c9b0 <_Znwy>
   14002fb38:	mov    %rax,%r9
   14002fb3b:	mov    0x8(%rbx),%rax
   14002fb3f:	mov    (%rbx),%rbp
   14002fb42:	lea    0x1(%rax),%r8
   14002fb46:	test   %rax,%rax
   14002fb49:	je     14002fbd0 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x370>
   14002fb4f:	test   %r8,%r8
   14002fb52:	jne    14002fb90 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x330>
   14002fb54:	cmp    %rbp,%r13
   14002fb57:	je     14002fb73 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x313>
   14002fb59:	mov    0x10(%rbx),%rax
   14002fb5d:	mov    %rbp,%rcx
   14002fb60:	mov    %r9,0x28(%rsp)
   14002fb65:	lea    0x1(%rax),%rdx
   14002fb69:	call   14006c9b8 <_ZdlPvy>
   14002fb6e:	mov    0x28(%rsp),%r9
   14002fb73:	mov    %rdi,0x10(%rbx)
   14002fb77:	mov    (%rsi),%rax
   14002fb7a:	mov    %r9,(%rbx)
   14002fb7d:	mov    0x8(%rax),%rcx
   14002fb81:	mov    (%rax),%rbp
   14002fb84:	mov    %rcx,0x28(%rsp)
   14002fb89:	jmp    14002f8c5 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x65>
   14002fb8e:	xchg   %ax,%ax
   14002fb90:	mov    %r9,%rcx
   14002fb93:	mov    %rbp,%rdx
   14002fb96:	call   140074648 <memcpy>
   14002fb9b:	mov    %rax,%r9
   14002fb9e:	jmp    14002fb54 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x2f4>
   14002fba0:	mov    $0x1e,%edi
   14002fba5:	mov    $0x1f,%ecx
   14002fbaa:	jmp    14002fb33 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x2d3>
   14002fbac:	nopl   0x0(%rax)
   14002fbb0:	test   %rax,%rax
   14002fbb3:	jns    14002fc09 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x3a9>
   14002fbb5:	call   14006ca00 <_ZSt17__throw_bad_allocv>
   14002fbba:	nopw   0x0(%rax,%rax,1)
   14002fbc0:	test   %r15,%r15
   14002fbc3:	jns    14002fc00 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x3a0>
   14002fbc5:	call   14006ca00 <_ZSt17__throw_bad_allocv>
   14002fbca:	nopw   0x0(%rax,%rax,1)
   14002fbd0:	movzbl 0x0(%rbp),%eax
   14002fbd4:	mov    %al,(%r9)
   14002fbd7:	jmp    14002fb54 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x2f4>
   14002fbdc:	lea    0x5bf4c(%rip),%rcx        # 14008bb2f <.rdata+0xef>
   14002fbe3:	call   14006c9f0 <_ZSt20__throw_length_errorPKc>
   14002fbe8:	lea    0x5bf40(%rip),%rcx        # 14008bb2f <.rdata+0xef>
   14002fbef:	call   14006c9f0 <_ZSt20__throw_length_errorPKc>
   14002fbf4:	lea    0x5bf34(%rip),%rcx        # 14008bb2f <.rdata+0xef>
   14002fbfb:	call   14006c9f0 <_ZSt20__throw_length_errorPKc>
   14002fc00:	lea    0x1(%r15),%rcx
   14002fc04:	jmp    14002fa06 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x1a6>
   14002fc09:	lea    0x1(%rax),%rcx
   14002fc0d:	jmp    14002f996 <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x136>
   14002fc12:	mov    %rax,%rsi
   14002fc15:	mov    %rbx,%rcx
   14002fc18:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fc1d:	mov    %rsi,%rcx
   14002fc20:	call   14006e2c8 <_Unwind_Resume>
   14002fc25:	mov    $0x58,%ecx
   14002fc2a:	lea    0x44(%rsp),%rdi
   14002fc2f:	call   14006c9a8 <__cxa_allocate_exception>
   14002fc34:	mov    $0x11,%ecx
   14002fc39:	lea    0x5be58(%rip),%rdx        # 14008ba98 <.rdata+0x58>
   14002fc40:	movl   $0x1,0x40(%rsp)
   14002fc48:	mov    %rax,%rsi
   14002fc4b:	xor    %eax,%eax
   14002fc4d:	rep stos %eax,%es:(%rdi)
   14002fc4f:	lea    0x48(%rsp),%rdi
   14002fc54:	mov    %rdi,%rcx
   14002fc57:	call   140085050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   14002fc5c:	lea    0x68(%rsp),%rbp
   14002fc61:	lea    0x5beae(%rip),%rdx        # 14008bb16 <.rdata+0xd6>
   14002fc68:	mov    %rbp,%rcx
   14002fc6b:	call   140085050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   14002fc70:	lea    0x40(%rsp),%rdx
   14002fc75:	mov    %rsi,%rcx
   14002fc78:	call   14007aa10 <_ZN12tx_generated15runtime_failureC1ENS_10error_infoE>
   14002fc7d:	mov    %rbp,%rcx
   14002fc80:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fc85:	mov    %rdi,%rcx
   14002fc88:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fc8d:	lea    0x4b00c(%rip),%r8        # 14007aca0 <_ZN12tx_generated15runtime_failureD1Ev>
   14002fc94:	lea    0x5e875(%rip),%rdx        # 14008e510 <_ZTIN12tx_generated15runtime_failureE>
   14002fc9b:	mov    %rsi,%rcx
   14002fc9e:	call   14006c968 <__cxa_throw>
   14002fca3:	mov    %rax,%rbx
   14002fca6:	jmp    14002fcbb <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x45b>
   14002fca8:	mov    %rbp,%rcx
   14002fcab:	mov    %rax,%rbx
   14002fcae:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fcb3:	mov    %rdi,%rcx
   14002fcb6:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fcbb:	mov    %rsi,%rcx
   14002fcbe:	call   14006c990 <__cxa_free_exception>
   14002fcc3:	mov    %rbx,%rcx
   14002fcc6:	call   14006e2c8 <_Unwind_Resume>
   14002fccb:	mov    %rdi,%rcx
   14002fcce:	mov    %rax,%rbx
   14002fcd1:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14002fcd6:	jmp    14002fcbb <_ZN12tx_generated12bytes_to_hexB5cxx11ERKSt10shared_ptrIKSt6vectorIhSaIhEEE+0x45b>
   14002fcd8:	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140030fa0 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE>:
   140030fa0:	push   %r15
   140030fa2:	push   %r14
   140030fa4:	push   %r13
   140030fa6:	push   %r12
   140030fa8:	push   %rbp
   140030fa9:	push   %rdi
   140030faa:	push   %rsi
   140030fab:	push   %rbx
   140030fac:	sub    $0xa8,%rsp
   140030fb3:	mov    (%rdx),%r13
   140030fb6:	mov    0x8(%rdx),%rbp
   140030fba:	mov    %r13,%rdi
   140030fbd:	mov    %rcx,0xf0(%rsp)
   140030fc5:	and    $0x1,%edi
   140030fc8:	jne    140031246 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x2a6>
   140030fce:	movq   $0x0,0x40(%rsp)
   140030fd7:	mov    %r13,%r14
   140030fda:	pxor   %xmm0,%xmm0
   140030fde:	shr    %r14
   140030fe1:	movups %xmm0,0x30(%rsp)
   140030fe6:	jne    140031080 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0xe0>
   140030fec:	xor    %r12d,%r12d
   140030fef:	xor    %esi,%esi
   140030ff1:	xor    %ebx,%ebx
   140030ff3:	pxor   %xmm0,%xmm0
   140030ff7:	mov    %r14,%rcx
   140030ffa:	mov    %rbx,0x50(%rsp)
   140030fff:	mov    %rsi,0x58(%rsp)
   140031004:	mov    %r12,0x60(%rsp)
   140031009:	movq   $0x0,0x40(%rsp)
   140031012:	movups %xmm0,0x30(%rsp)
   140031017:	call   14002f6b0 <_ZN12tx_generated12_GLOBAL__N_112require_sizeEy>
   14003101c:	mov    0xf0(%rsp),%rax
   140031024:	mov    $0x28,%ecx
   140031029:	movq   $0x0,(%rax)
   140031030:	call   14006c9b0 <_Znwy>
   140031035:	lea    0x63814(%rip),%rcx        # 140094850 <_ZTVSt23_Sp_counted_ptr_inplaceIKSt6vectorIhSaIhEESaIvELN9__gnu_cxx12_Lock_policyE2EE+0x10>
   14003103c:	mov    0x5ad45(%rip),%rdx        # 14008bd88 <_ZN12tx_generated12_GLOBAL__N_1L10hex_digitsE+0x18>
   140031043:	mov    %rbx,0x10(%rax)
   140031047:	mov    %rcx,(%rax)
   14003104a:	mov    0xf0(%rsp),%rcx
   140031052:	mov    %rdx,0x8(%rax)
   140031056:	mov    %rsi,0x18(%rax)
   14003105a:	mov    %r12,0x20(%rax)
   14003105e:	mov    %rax,0x8(%rcx)
   140031062:	add    $0x10,%rax
   140031066:	mov    %rax,(%rcx)
   140031069:	mov    %rcx,%rax
   14003106c:	add    $0xa8,%rsp
   140031073:	pop    %rbx
   140031074:	pop    %rsi
   140031075:	pop    %rdi
   140031076:	pop    %rbp
   140031077:	pop    %r12
   140031079:	pop    %r13
   14003107b:	pop    %r14
   14003107d:	pop    %r15
   14003107f:	ret
   140031080:	mov    %r14,%rcx
   140031083:	call   14006c9b0 <_Znwy>
   140031088:	movq   %rax,%xmm1
   14003108d:	lea    (%rax,%r14,1),%r12
   140031091:	mov    %rax,%rsi
   140031094:	movddup %xmm1,%xmm0
   140031098:	mov    %r12,0x40(%rsp)
   14003109d:	movups %xmm0,0x30(%rsp)
   1400310a2:	jmp    140031111 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x171>
   1400310a4:	nopl   0x0(%rax)
   1400310a8:	lea    -0x61(%rbx),%eax
   1400310ab:	cmp    $0x5,%al
   1400310ad:	jbe    140031138 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x198>
   1400310b3:	lea    -0x41(%rbx),%eax
   1400310b6:	sub    $0x37,%ebx
   1400310b9:	cmp    $0x6,%al
   1400310bb:	mov    $0xffffffff,%eax
   1400310c0:	cmovae %eax,%ebx
   1400310c3:	movsbl 0x1(%rbp,%rdi,1),%eax
   1400310c8:	lea    -0x30(%rax),%edx
   1400310cb:	cmp    $0x9,%dl
   1400310ce:	jbe    14003112d <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x18d>
   1400310d0:	lea    -0x61(%rax),%edx
   1400310d3:	cmp    $0x5,%dl
   1400310d6:	jbe    140031140 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x1a0>
   1400310d8:	lea    -0x41(%rax),%edx
   1400310db:	cmp    $0x5,%dl
   1400310de:	ja     14003131d <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x37d>
   1400310e4:	sub    $0x37,%eax
   1400310e7:	test   %ebx,%ebx
   1400310e9:	js     14003131d <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x37d>
   1400310ef:	shl    $0x4,%ebx
   1400310f2:	or     %eax,%ebx
   1400310f4:	cmp    %r12,%rsi
   1400310f7:	je     140031148 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x1a8>
   1400310f9:	mov    %bl,(%rsi)
   1400310fb:	add    $0x2,%rdi
   1400310ff:	add    $0x1,%rsi
   140031103:	mov    %rsi,0x38(%rsp)
   140031108:	cmp    %r13,%rdi
   14003110b:	jae    140031200 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x260>
   140031111:	movsbl 0x0(%rbp,%rdi,1),%ebx
   140031116:	lea    -0x30(%rbx),%eax
   140031119:	cmp    $0x9,%al
   14003111b:	ja     1400310a8 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x108>
   14003111d:	movsbl 0x1(%rbp,%rdi,1),%eax
   140031122:	sub    $0x30,%ebx
   140031125:	lea    -0x30(%rax),%edx
   140031128:	cmp    $0x9,%dl
   14003112b:	ja     1400310d0 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x130>
   14003112d:	sub    $0x30,%eax
   140031130:	jmp    1400310e7 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x147>
   140031132:	nopw   0x0(%rax,%rax,1)
   140031138:	sub    $0x57,%ebx
   14003113b:	jmp    1400310c3 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x123>
   14003113d:	nopl   (%rax)
   140031140:	sub    $0x57,%eax
   140031143:	jmp    1400310e7 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x147>
   140031145:	nopl   (%rax)
   140031148:	movabs $0x7fffffffffffffff,%rax
   140031152:	mov    0x30(%rsp),%r15
   140031157:	sub    %r15,%rsi
   14003115a:	mov    %rsi,%r14
   14003115d:	cmp    %rax,%rsi
   140031160:	je     14003123a <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x29a>
   140031166:	test   %rsi,%rsi
   140031169:	je     140031210 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x270>
   14003116f:	movabs $0x7fffffffffffffff,%rax
   140031179:	lea    (%rsi,%rsi,1),%rdx
   14003117d:	movabs $0x7fffffffffffffff,%rcx
   140031187:	cmp    %rax,%rdx
   14003118a:	cmovbe %rdx,%rax
   14003118e:	cmp    %rsi,%rdx
   140031191:	cmovb  %rcx,%rax
   140031195:	mov    %rax,%rcx
   140031198:	mov    %rax,0x20(%rsp)
   14003119d:	call   14006c9b0 <_Znwy>
   1400311a2:	mov    %bl,(%rax,%rsi,1)
   1400311a5:	mov    %rax,%r9
   1400311a8:	test   %rsi,%rsi
   1400311ab:	jle    140031228 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x288>
   1400311ad:	mov    %r15,%rdx
   1400311b0:	mov    %r14,%r8
   1400311b3:	lea    0x1(%rax,%rsi,1),%rsi
   1400311b8:	mov    %rax,%rcx
   1400311bb:	call   140074650 <memmove>
   1400311c0:	mov    %r12,%rdx
   1400311c3:	mov    %rax,%r9
   1400311c6:	sub    %r15,%rdx
   1400311c9:	mov    %r15,%rcx
   1400311cc:	mov    %r9,0x28(%rsp)
   1400311d1:	call   14006c9b8 <_ZdlPvy>
   1400311d6:	mov    0x28(%rsp),%r9
   1400311db:	mov    0x20(%rsp),%r12
   1400311e0:	add    $0x2,%rdi
   1400311e4:	mov    %r9,0x30(%rsp)
   1400311e9:	mov    %rsi,0x38(%rsp)
   1400311ee:	add    %r9,%r12
   1400311f1:	mov    %r12,0x40(%rsp)
   1400311f6:	cmp    %r13,%rdi
   1400311f9:	jb     140031111 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x171>
   1400311ff:	nop
   140031200:	mov    0x30(%rsp),%rbx
   140031205:	mov    %rsi,%r14
   140031208:	sub    %rbx,%r14
   14003120b:	jmp    140030ff3 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x53>
   140031210:	mov    $0x1,%ecx
   140031215:	call   14006c9b0 <_Znwy>
   14003121a:	mov    %bl,(%rax)
   14003121c:	mov    %rax,%r9
   14003121f:	movq   $0x1,0x20(%rsp)
   140031228:	lea    0x1(%r9,%r14,1),%rsi
   14003122d:	test   %r15,%r15
   140031230:	je     1400311db <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x23b>
   140031232:	mov    %r12,%rdx
   140031235:	sub    %r15,%rdx
   140031238:	jmp    1400311c9 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x229>
   14003123a:	lea    0x5a947(%rip),%rcx        # 14008bb88 <.rdata+0x148>
   140031241:	call   14006c9f0 <_ZSt20__throw_length_errorPKc>
   140031246:	mov    $0x58,%ecx
   14003124b:	lea    0x54(%rsp),%rdi
   140031250:	call   14006c9a8 <__cxa_allocate_exception>
   140031255:	mov    $0x11,%ecx
   14003125a:	lea    0x5a96e(%rip),%rdx        # 14008bbcf <.rdata+0x18f>
   140031261:	mov    %rax,%rsi
   140031264:	xor    %eax,%eax
   140031266:	rep stos %eax,%es:(%rdi)
   140031268:	lea    0x58(%rsp),%rdi
   14003126d:	movl   $0x2,0x50(%rsp)
   140031275:	mov    %rdi,%rcx
   140031278:	call   140085050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   14003127d:	lea    0x78(%rsp),%rbp
   140031282:	lea    0x5a957(%rip),%rdx        # 14008bbe0 <.rdata+0x1a0>
   140031289:	mov    %rbp,%rcx
   14003128c:	call   140085050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   140031291:	lea    0x50(%rsp),%rdx
   140031296:	mov    %rsi,%rcx
   140031299:	call   14007aa10 <_ZN12tx_generated15runtime_failureC1ENS_10error_infoE>
   14003129e:	mov    %rbp,%rcx
   1400312a1:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   1400312a6:	mov    %rdi,%rcx
   1400312a9:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   1400312ae:	lea    0x499eb(%rip),%r8        # 14007aca0 <_ZN12tx_generated15runtime_failureD1Ev>
   1400312b5:	lea    0x5d254(%rip),%rdx        # 14008e510 <_ZTIN12tx_generated15runtime_failureE>
   1400312bc:	mov    %rsi,%rcx
   1400312bf:	call   14006c968 <__cxa_throw>
   1400312c4:	lea    0x50(%rsp),%rcx
   1400312c9:	mov    %rax,%rbx
   1400312cc:	call   140081020 <_ZNSt6vectorIhSaIhEED1Ev>
   1400312d1:	lea    0x30(%rsp),%rcx
   1400312d6:	call   140081020 <_ZNSt6vectorIhSaIhEED1Ev>
   1400312db:	mov    %rbx,%rcx
   1400312de:	call   14006e2c8 <_Unwind_Resume>
   1400312e3:	mov    %rbp,%rcx
   1400312e6:	mov    %rax,%rbx
   1400312e9:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   1400312ee:	mov    %rdi,%rcx
   1400312f1:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   1400312f6:	mov    %rsi,%rcx
   1400312f9:	call   14006c990 <__cxa_free_exception>
   1400312fe:	mov    %rbx,%rcx
   140031301:	call   14006e2c8 <_Unwind_Resume>
   140031306:	mov    %rdi,%rcx
   140031309:	mov    %rax,%rbx
   14003130c:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   140031311:	jmp    1400312f6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   140031313:	mov    %rax,%rbx
   140031316:	jmp    1400312f6 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x356>
   140031318:	mov    %rax,%rbx
   14003131b:	jmp    1400312d1 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x331>
   14003131d:	mov    $0x58,%ecx
   140031322:	lea    0x54(%rsp),%rdi
   140031327:	call   14006c9a8 <__cxa_allocate_exception>
   14003132c:	mov    $0x11,%ecx
   140031331:	lea    0x5a897(%rip),%rdx        # 14008bbcf <.rdata+0x18f>
   140031338:	mov    %rax,%rsi
   14003133b:	xor    %eax,%eax
   14003133d:	rep stos %eax,%es:(%rdi)
   14003133f:	lea    0x58(%rsp),%rdi
   140031344:	movl   $0x2,0x50(%rsp)
   14003134c:	mov    %rdi,%rcx
   14003134f:	call   140085050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   140031354:	lea    0x78(%rsp),%rbp
   140031359:	lea    0x5a8a8(%rip),%rdx        # 14008bc08 <.rdata+0x1c8>
   140031360:	mov    %rbp,%rcx
   140031363:	call   140085050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   140031368:	lea    0x50(%rsp),%rdx
   14003136d:	mov    %rsi,%rcx
   140031370:	call   14007aa10 <_ZN12tx_generated15runtime_failureC1ENS_10error_infoE>
   140031375:	mov    %rbp,%rcx
   140031378:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14003137d:	mov    %rdi,%rcx
   140031380:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   140031385:	lea    0x49914(%rip),%r8        # 14007aca0 <_ZN12tx_generated15runtime_failureD1Ev>
   14003138c:	lea    0x5d17d(%rip),%rdx        # 14008e510 <_ZTIN12tx_generated15runtime_failureE>
   140031393:	mov    %rsi,%rcx
   140031396:	call   14006c968 <__cxa_throw>
   14003139b:	mov    %rax,%rbx
   14003139e:	jmp    1400313b3 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x413>
   1400313a0:	mov    %rbp,%rcx
   1400313a3:	mov    %rax,%rbx
   1400313a6:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   1400313ab:	mov    %rdi,%rcx
   1400313ae:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   1400313b3:	mov    %rsi,%rcx
   1400313b6:	call   14006c990 <__cxa_free_exception>
   1400313bb:	jmp    1400312d1 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x331>
   1400313c0:	mov    %rdi,%rcx
   1400313c3:	mov    %rax,%rbx
   1400313c6:	call   140081050 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   1400313cb:	jmp    1400313b3 <_ZN12tx_generated14bytes_from_hexESt17basic_string_viewIcSt11char_traitsIcEE+0x413>
   1400313cd:	nopl   (%rax)


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\paths.exe:     file format pei-x86-64


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


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\paths.exe:     file format pei-x86-64


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


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\paths.exe:     file format pei-x86-64


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
   140003f10:	mov    0x8a3f9(%rip),%rcx        # 14008e310 <.refptr.__emutls_v._ZN12tx_generated6detail14thread_contextE>
   140003f17:	fstpl  (%rbx)
   140003f19:	call   14006e2c0 <__emutls_get_address>
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
   140003f4d:	call   14007a0d0 <_ZN12tx_generated10statistics4failEPKcS2_>
   140003f52:	lea    0x858db(%rip),%rdx        # 140089834 <.rdata+0x1a4>
   140003f59:	lea    0x858ea(%rip),%rcx        # 14008984a <.rdata+0x1ba>
   140003f60:	call   14007a0d0 <_ZN12tx_generated10statistics4failEPKcS2_>
   140003f65:	mov    %rax,%rcx
   140003f68:	cmp    $0x3,%rdx
   140003f6c:	je     140003fa0 <txrt_statistics_mean_vector+0x100>
   140003f6e:	jg     140003f7c <txrt_statistics_mean_vector+0xdc>
   140003f70:	cmp    $0x1,%rdx
   140003f74:	je     140003fd1 <txrt_statistics_mean_vector+0x131>
   140003f76:	cmp    $0x2,%rdx
   140003f7a:	je     140003ff8 <txrt_statistics_mean_vector+0x158>
   140003f7c:	call   14006c9a0 <__cxa_begin_catch>
   140003f81:	lea    0x857b9(%rip),%r8        # 140089741 <.rdata+0xb1>
   140003f88:	mov    $0x1,%ecx
   140003f8d:	lea    0x857c3(%rip),%rdx        # 140089757 <.rdata+0xc7>
   140003f94:	call   140021690 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   140003f99:	call   14006c998 <__cxa_end_catch>
   140003f9e:	jmp    140003fc7 <txrt_statistics_mean_vector+0x127>
   140003fa0:	call   14006c9a0 <__cxa_begin_catch>
   140003fa5:	mov    %rax,%rcx
   140003fa8:	mov    (%rax),%rax
   140003fab:	call   *0x10(%rax)
   140003fae:	lea    0x8577b(%rip),%rdx        # 140089730 <.rdata+0xa0>
   140003fb5:	mov    $0x1,%ecx
   140003fba:	mov    %rax,%r8
   140003fbd:	call   140021690 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   140003fc2:	call   14006c998 <__cxa_end_catch>
   140003fc7:	mov    $0x1,%eax
   140003fcc:	jmp    140003f29 <txrt_statistics_mean_vector+0x89>
   140003fd1:	call   14006c9a0 <__cxa_begin_catch>
   140003fd6:	mov    %rax,%rbx
   140003fd9:	mov    (%rax),%rax
   140003fdc:	mov    %rbx,%rcx
   140003fdf:	call   *0x10(%rax)
   140003fe2:	mov    0x18(%rbx),%rdx
   140003fe6:	mov    0x10(%rbx),%ecx
   140003fe9:	mov    %rax,%r8
   140003fec:	call   140021690 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   140003ff1:	call   14006c998 <__cxa_end_catch>
   140003ff6:	jmp    140003fc7 <txrt_statistics_mean_vector+0x127>
   140003ff8:	call   14006c9a0 <__cxa_begin_catch>
   140003ffd:	mov    %rax,%rcx
   140004000:	mov    (%rax),%rax
   140004003:	call   *0x10(%rax)
   140004006:	lea    0x85711(%rip),%rdx        # 14008971e <.rdata+0x8e>
   14000400d:	mov    $0x1,%ecx
   140004012:	mov    %rax,%r8
   140004015:	call   140021690 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14000401a:	call   14006c998 <__cxa_end_catch>
   14000401f:	jmp    140003fc7 <txrt_statistics_mean_vector+0x127>
   140004021:	data16 cs nopw 0x0(%rax,%rax,1)
   14000402c:	nopl   0x0(%rax)


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\language.exe:     file format pei-x86-64


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


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\language.exe:     file format pei-x86-64


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


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\language.exe:     file format pei-x86-64


Disassembly of section .text:

000000014004c910 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex>:
   14004c910:	push   %r12
   14004c912:	push   %rbp
   14004c913:	push   %rdi
   14004c914:	push   %rsi
   14004c915:	push   %rbx
   14004c916:	sub    $0xf0,%rsp
   14004c91d:	mov    %rdx,%rbp
   14004c920:	mov    %rcx,%rsi
   14004c923:	mov    %rdx,%rbx
   14004c926:	shr    $0x3f,%rbp
   14004c92a:	test   %rdx,%rdx
   14004c92d:	js     14004cc38 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x328>
   14004c933:	xor    %r10d,%r10d
   14004c936:	cmp    $0x9,%rdx
   14004c93a:	jbe    14004cee2 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x5d2>
   14004c940:	cmp    $0x63,%rbx
   14004c944:	jbe    14004cea3 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x593>
   14004c94a:	cmp    $0x3e7,%rbx
   14004c951:	jbe    14004cecd <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x5bd>
   14004c957:	cmp    $0x270f,%rbx
   14004c95e:	jbe    14004ceb8 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x5a8>
   14004c964:	mov    %rbx,%rdx
   14004c967:	mov    $0x1,%r8d
   14004c96d:	movabs $0x346dc5d63886594b,%r9
   14004c977:	jmp    14004c9a7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x97>
   14004c979:	nopl   0x0(%rax)
   14004c980:	cmp    $0xf423f,%rcx
   14004c987:	jbe    14004ce58 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x548>
   14004c98d:	cmp    $0x98967f,%rcx
   14004c994:	jbe    14004ce68 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x558>
   14004c99a:	cmp    $0x5f5e0ff,%rcx
   14004c9a1:	jbe    14004ce78 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x568>
   14004c9a7:	mov    %rdx,%rax
   14004c9aa:	mov    %rdx,%rcx
   14004c9ad:	mul    %r9
   14004c9b0:	mov    %r8d,%eax
   14004c9b3:	add    $0x4,%r8d
   14004c9b7:	shr    $0xb,%rdx
   14004c9bb:	cmp    $0x1869f,%rcx
   14004c9c2:	ja     14004c980 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x70>
   14004c9c4:	lea    0x3(%rax),%edi
   14004c9c7:	lea    0x10(%rsi),%rcx
   14004c9cb:	lea    (%r8,%r10,1),%r12d
   14004c9cf:	mov    %rcx,(%rsi)
   14004c9d2:	cmp    $0xf,%r12
   14004c9d6:	ja     14004cc60 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x350>
   14004c9dc:	test   %r12,%r12
   14004c9df:	jne    14004ce84 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x574>
   14004c9e5:	movabs $0x3330323031303030,%rax
   14004c9ef:	movq   $0x0,0x8(%rsi)
   14004c9f7:	add    %rbp,%rcx
   14004c9fa:	movabs $0x3730363035303430,%rdx
   14004ca04:	mov    %rax,0x20(%rsp)
   14004ca09:	movabs $0x3131303139303830,%rax
   14004ca13:	mov    %rdx,0x28(%rsp)
   14004ca18:	movabs $0x3531343133313231,%rdx
   14004ca22:	mov    %rax,0x30(%rsp)
   14004ca27:	movabs $0x3931383137313631,%rax
   14004ca31:	mov    %rdx,0x38(%rsp)
   14004ca36:	movabs $0x3332323231323032,%rdx
   14004ca40:	mov    %rax,0x40(%rsp)
   14004ca45:	movabs $0x3732363235323432,%rax
   14004ca4f:	mov    %rdx,0x48(%rsp)
   14004ca54:	movabs $0x3133303339323832,%rdx
   14004ca5e:	mov    %rax,0x50(%rsp)
   14004ca63:	movabs $0x3533343333333233,%rax
   14004ca6d:	mov    %rdx,0x58(%rsp)
   14004ca72:	movabs $0x3933383337333633,%rdx
   14004ca7c:	mov    %rax,0x60(%rsp)
   14004ca81:	movabs $0x3334323431343034,%rax
   14004ca8b:	mov    %rdx,0x68(%rsp)
   14004ca90:	movabs $0x3734363435343434,%rdx
   14004ca9a:	mov    %rax,0x70(%rsp)
   14004ca9f:	movabs $0x3135303539343834,%rax
   14004caa9:	mov    %rdx,0x78(%rsp)
   14004caae:	movabs $0x3535343533353235,%rdx
   14004cab8:	mov    %rax,0x80(%rsp)
   14004cac0:	movabs $0x3935383537353635,%rax
   14004caca:	mov    %rdx,0x88(%rsp)
   14004cad2:	movabs $0x3336323631363036,%rdx
   14004cadc:	mov    %rax,0x90(%rsp)
   14004cae4:	movabs $0x3736363635363436,%rax
   14004caee:	mov    %rdx,0x98(%rsp)
   14004caf6:	movabs $0x3137303739363836,%rdx
   14004cb00:	mov    %rax,0xa0(%rsp)
   14004cb08:	movabs $0x3537343733373237,%rax
   14004cb12:	mov    %rdx,0xa8(%rsp)
   14004cb1a:	movabs $0x3937383737373637,%rdx
   14004cb24:	mov    %rax,0xb0(%rsp)
   14004cb2c:	movabs $0x3338323831383038,%rax
   14004cb36:	mov    %rdx,0xb8(%rsp)
   14004cb3e:	movabs $0x3738363835383438,%rdx
   14004cb48:	mov    %rax,0xc0(%rsp)
   14004cb50:	movabs $0x3139303939383838,%rax
   14004cb5a:	mov    %rdx,0xc8(%rsp)
   14004cb62:	movabs $0x3539343933393239,%rdx
   14004cb6c:	mov    %rdx,0xd8(%rsp)
   14004cb74:	movabs $0x39393839373936,%rdx
   14004cb7e:	mov    %rax,0xd0(%rsp)
   14004cb86:	movabs $0x3935393439333932,%rax
   14004cb90:	movb   $0x0,0x10(%rsi)
   14004cb94:	mov    %rax,0xd9(%rsp)
   14004cb9c:	mov    %rdx,0xe1(%rsp)
   14004cba4:	movabs $0x28f5c28f5c28f5c3,%r8
   14004cbae:	xchg   %ax,%ax
   14004cbb0:	mov    %rbx,%rdx
   14004cbb3:	shr    $0x2,%rdx
   14004cbb7:	mov    %rdx,%rax
   14004cbba:	mul    %r8
   14004cbbd:	mov    %rbx,%rax
   14004cbc0:	mov    %rdx,%r9
   14004cbc3:	and    $0xfffffffffffffffc,%rdx
   14004cbc7:	shr    $0x2,%r9
   14004cbcb:	add    %r9,%rdx
   14004cbce:	lea    (%rdx,%rdx,4),%rdx
   14004cbd2:	shl    $0x2,%rdx
   14004cbd6:	sub    %rdx,%rax
   14004cbd9:	mov    %rbx,%rdx
   14004cbdc:	mov    %r9,%rbx
   14004cbdf:	mov    %edi,%r9d
   14004cbe2:	add    %rax,%rax
   14004cbe5:	movzbl 0x21(%rsp,%rax,1),%r10d
   14004cbeb:	movzbl 0x20(%rsp,%rax,1),%eax
   14004cbf0:	mov    %r10b,(%rcx,%r9,1)
   14004cbf4:	lea    -0x1(%rdi),%r9d
   14004cbf8:	sub    $0x2,%edi
   14004cbfb:	mov    %al,(%rcx,%r9,1)
   14004cbff:	cmp    $0x270f,%rdx
   14004cc06:	ja     14004cbb0 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x2a0>
   14004cc08:	lea    0x30(%rbx),%eax
   14004cc0b:	cmp    $0x9,%rbx
   14004cc0f:	jbe    14004cc21 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x311>
   14004cc11:	add    %rbx,%rbx
   14004cc14:	movzbl 0x21(%rsp,%rbx,1),%eax
   14004cc19:	mov    %al,0x1(%rcx)
   14004cc1c:	movzbl 0x20(%rsp,%rbx,1),%eax
   14004cc21:	mov    %al,(%rcx)
   14004cc23:	mov    %rsi,%rax
   14004cc26:	add    $0xf0,%rsp
   14004cc2d:	pop    %rbx
   14004cc2e:	pop    %rsi
   14004cc2f:	pop    %rdi
   14004cc30:	pop    %rbp
   14004cc31:	pop    %r12
   14004cc33:	ret
   14004cc34:	nopl   0x0(%rax)
   14004cc38:	neg    %rbx
   14004cc3b:	mov    $0x1,%r10d
   14004cc41:	cmp    $0x9,%rbx
   14004cc45:	ja     14004c940 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x30>
   14004cc4b:	lea    0x10(%rcx),%rcx
   14004cc4f:	xor    %edi,%edi
   14004cc51:	mov    $0x2,%r12d
   14004cc57:	mov    %rcx,(%rsi)
   14004cc5a:	jmp    14004cc74 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004cc5c:	nopl   0x0(%rax)
   14004cc60:	lea    0x1(%r12),%rcx
   14004cc65:	call   140080000 <_Znwy>
   14004cc6a:	mov    %r12,0x10(%rsi)
   14004cc6e:	mov    %rax,(%rsi)
   14004cc71:	mov    %rax,%rcx
   14004cc74:	mov    %r12,%r8
   14004cc77:	mov    $0x2d,%edx
   14004cc7c:	call   140087b08 <memset>
   14004cc81:	mov    (%rsi),%rax
   14004cc84:	add    %r12,%rax
   14004cc87:	movabs $0x3730363035303430,%rdx
   14004cc91:	mov    %r12,0x8(%rsi)
   14004cc95:	movb   $0x0,(%rax)
   14004cc98:	mov    (%rsi),%rcx
   14004cc9b:	movabs $0x3330323031303030,%rax
   14004cca5:	mov    %rax,0x20(%rsp)
   14004ccaa:	movabs $0x3131303139303830,%rax
   14004ccb4:	mov    %rdx,0x28(%rsp)
   14004ccb9:	add    %rbp,%rcx
   14004ccbc:	movabs $0x3531343133313231,%rdx
   14004ccc6:	mov    %rax,0x30(%rsp)
   14004cccb:	movabs $0x3931383137313631,%rax
   14004ccd5:	mov    %rdx,0x38(%rsp)
   14004ccda:	movabs $0x3332323231323032,%rdx
   14004cce4:	mov    %rax,0x40(%rsp)
   14004cce9:	movabs $0x3732363235323432,%rax
   14004ccf3:	mov    %rdx,0x48(%rsp)
   14004ccf8:	movabs $0x3133303339323832,%rdx
   14004cd02:	mov    %rax,0x50(%rsp)
   14004cd07:	movabs $0x3533343333333233,%rax
   14004cd11:	mov    %rdx,0x58(%rsp)
   14004cd16:	movabs $0x3933383337333633,%rdx
   14004cd20:	mov    %rax,0x60(%rsp)
   14004cd25:	movabs $0x3334323431343034,%rax
   14004cd2f:	mov    %rdx,0x68(%rsp)
   14004cd34:	movabs $0x3734363435343434,%rdx
   14004cd3e:	mov    %rax,0x70(%rsp)
   14004cd43:	movabs $0x3135303539343834,%rax
   14004cd4d:	mov    %rdx,0x78(%rsp)
   14004cd52:	movabs $0x3535343533353235,%rdx
   14004cd5c:	mov    %rax,0x80(%rsp)
   14004cd64:	movabs $0x3935383537353635,%rax
   14004cd6e:	mov    %rdx,0x88(%rsp)
   14004cd76:	movabs $0x3336323631363036,%rdx
   14004cd80:	mov    %rax,0x90(%rsp)
   14004cd88:	movabs $0x3736363635363436,%rax
   14004cd92:	mov    %rdx,0x98(%rsp)
   14004cd9a:	movabs $0x3137303739363836,%rdx
   14004cda4:	mov    %rax,0xa0(%rsp)
   14004cdac:	movabs $0x3537343733373237,%rax
   14004cdb6:	mov    %rdx,0xa8(%rsp)
   14004cdbe:	movabs $0x3937383737373637,%rdx
   14004cdc8:	mov    %rax,0xb0(%rsp)
   14004cdd0:	movabs $0x3338323831383038,%rax
   14004cdda:	mov    %rdx,0xb8(%rsp)
   14004cde2:	movabs $0x3738363835383438,%rdx
   14004cdec:	mov    %rax,0xc0(%rsp)
   14004cdf4:	movabs $0x3139303939383838,%rax
   14004cdfe:	mov    %rdx,0xc8(%rsp)
   14004ce06:	movabs $0x3539343933393239,%rdx
   14004ce10:	mov    %rdx,0xd8(%rsp)
   14004ce18:	movabs $0x39393839373936,%rdx
   14004ce22:	mov    %rax,0xd0(%rsp)
   14004ce2a:	movabs $0x3935393439333932,%rax
   14004ce34:	mov    %rax,0xd9(%rsp)
   14004ce3c:	mov    %rdx,0xe1(%rsp)
   14004ce44:	cmp    $0x63,%rbx
   14004ce48:	ja     14004cba4 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x294>
   14004ce4e:	jmp    14004cc08 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x2f8>
   14004ce53:	nopl   0x0(%rax,%rax,1)
   14004ce58:	mov    %r8d,%edi
   14004ce5b:	lea    0x5(%rax),%r8d
   14004ce5f:	jmp    14004c9c7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0xb7>
   14004ce64:	nopl   0x0(%rax)
   14004ce68:	lea    0x6(%rax),%r8d
   14004ce6c:	lea    0x5(%rax),%edi
   14004ce6f:	jmp    14004c9c7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0xb7>
   14004ce74:	nopl   0x0(%rax)
   14004ce78:	lea    0x7(%rax),%r8d
   14004ce7c:	lea    0x6(%rax),%edi
   14004ce7f:	jmp    14004c9c7 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0xb7>
   14004ce84:	cmp    $0x1,%r12
   14004ce88:	jne    14004cc74 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004ce8e:	movb   $0x2d,(%rcx)
   14004ce91:	mov    (%rsi),%rax
   14004ce94:	mov    $0x1,%r12d
   14004ce9a:	add    $0x1,%rax
   14004ce9e:	jmp    14004cc87 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x377>
   14004cea3:	lea    0x10(%rsi),%rcx
   14004cea7:	lea    0x2(%r10),%r12d
   14004ceab:	mov    $0x1,%edi
   14004ceb0:	mov    %rcx,(%rsi)
   14004ceb3:	jmp    14004cc74 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004ceb8:	lea    0x10(%rsi),%rcx
   14004cebc:	lea    0x4(%r10),%r12d
   14004cec0:	mov    $0x3,%edi
   14004cec5:	mov    %rcx,(%rsi)
   14004cec8:	jmp    14004cc74 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004cecd:	lea    0x10(%rsi),%rcx
   14004ced1:	lea    0x3(%r10),%r12d
   14004ced5:	mov    $0x2,%edi
   14004ceda:	mov    %rcx,(%rsi)
   14004cedd:	jmp    14004cc74 <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x364>
   14004cee2:	lea    0x10(%rcx),%rcx
   14004cee6:	xor    %edi,%edi
   14004cee8:	mov    %rcx,(%rsi)
   14004ceeb:	jmp    14004ce8e <_ZN12tx_generated16tx_int_to_stringB5cxx11Ex+0x57e>
   14004ceed:	nopl   (%rax)


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\language.exe:     file format pei-x86-64


Disassembly of section .text:

000000014004d8c0 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed>:
   14004d8c0:	push   %r14
   14004d8c2:	push   %r13
   14004d8c4:	push   %r12
   14004d8c6:	push   %rbp
   14004d8c7:	push   %rdi
   14004d8c8:	push   %rsi
   14004d8c9:	push   %rbx
   14004d8ca:	sub    $0x90,%rsp
   14004d8d1:	movups %xmm6,0x80(%rsp)
   14004d8d9:	lea    0x40(%rsp),%rbp
   14004d8de:	movapd %xmm1,%xmm3
   14004d8e2:	movapd %xmm1,%xmm6
   14004d8e6:	mov    %rcx,%rsi
   14004d8e9:	movl   $0x3,0x20(%rsp)
   14004d8f1:	lea    0x30(%rsp),%rcx
   14004d8f6:	lea    0x80(%rsp),%r8
   14004d8fe:	mov    %rbp,%rdx
   14004d901:	call   140080018 <_ZSt8to_charsPcS_dSt12chars_format>
   14004d906:	mov    0x38(%rsp),%ecx
   14004d90a:	mov    0x30(%rsp),%rbx
   14004d90f:	test   %ecx,%ecx
   14004d911:	jne    14004db89 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x2c9>
   14004d917:	lea    0x10(%rsi),%rdi
   14004d91b:	sub    %rbp,%rbx
   14004d91e:	mov    %rdi,(%rsi)
   14004d921:	cmp    $0xf,%rbx
   14004d925:	ja     14004d9d0 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x110>
   14004d92b:	cmp    $0x1,%rbx
   14004d92f:	jne    14004d9b8 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xf8>
   14004d935:	movzbl 0x40(%rsp),%eax
   14004d93a:	mov    %al,0x10(%rsi)
   14004d93d:	mov    %rdi,%rax
   14004d940:	andpd  0x5d9a8(%rip),%xmm6        # 1400ab2f0 <.rdata+0x1d0>
   14004d948:	movsd  0x5d9b0(%rip),%xmm0        # 1400ab300 <.rdata+0x1e0>
   14004d950:	mov    %rbx,0x8(%rsi)
   14004d954:	movb   $0x0,(%rax,%rbx,1)
   14004d958:	ucomisd %xmm6,%xmm0
   14004d95c:	jb     14004d998 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xd8>
   14004d95e:	mov    (%rsi),%r8
   14004d961:	xor    %edx,%edx
   14004d963:	test   %rbx,%rbx
   14004d966:	jne    14004d97d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xbd>
   14004d968:	jmp    14004dad0 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x210>
   14004d96d:	nopl   (%rax)
   14004d970:	add    $0x1,%rdx
   14004d974:	cmp    %rdx,%rbx
   14004d977:	je     14004da10 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x150>
   14004d97d:	movzbl (%r8,%rdx,1),%ecx
   14004d982:	mov    %ecx,%eax
   14004d984:	and    $0xffffffdf,%eax
   14004d987:	cmp    $0x45,%al
   14004d989:	sete   %al
   14004d98c:	cmp    $0x2e,%cl
   14004d98f:	sete   %cl
   14004d992:	or     %ecx,%eax
   14004d994:	test   $0x1,%al
   14004d996:	je     14004d970 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xb0>
   14004d998:	movups 0x80(%rsp),%xmm6
   14004d9a0:	mov    %rsi,%rax
   14004d9a3:	add    $0x90,%rsp
   14004d9aa:	pop    %rbx
   14004d9ab:	pop    %rsi
   14004d9ac:	pop    %rdi
   14004d9ad:	pop    %rbp
   14004d9ae:	pop    %r12
   14004d9b0:	pop    %r13
   14004d9b2:	pop    %r14
   14004d9b4:	ret
   14004d9b5:	nopl   (%rax)
   14004d9b8:	test   %rbx,%rbx
   14004d9bb:	je     14004d93d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x7d>
   14004d9c1:	mov    %rdi,%rcx
   14004d9c4:	jmp    14004d9f5 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x135>
   14004d9c6:	cs nopw 0x0(%rax,%rax,1)
   14004d9d0:	test   %rbx,%rbx
   14004d9d3:	js     14004db5e <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x29e>
   14004d9d9:	mov    %rbx,%rcx
   14004d9dc:	add    $0x1,%rcx
   14004d9e0:	js     14004da80 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1c0>
   14004d9e6:	call   140080000 <_Znwy>
   14004d9eb:	mov    %rbx,0x10(%rsi)
   14004d9ef:	mov    %rax,(%rsi)
   14004d9f2:	mov    %rax,%rcx
   14004d9f5:	mov    %rbx,%r8
   14004d9f8:	mov    %rbp,%rdx
   14004d9fb:	call   140087af8 <memcpy>
   14004da00:	mov    (%rsi),%rax
   14004da03:	jmp    14004d940 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x80>
   14004da08:	nopl   0x0(%rax,%rax,1)
   14004da10:	movabs $0x7fffffffffffffff,%rax
   14004da1a:	sub    %rbx,%rax
   14004da1d:	cmp    $0x1,%rax
   14004da21:	jbe    14004db6a <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x2aa>
   14004da27:	lea    0x2(%rbx),%rbp
   14004da2b:	cmp    %r8,%rdi
   14004da2e:	je     14004daf6 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x236>
   14004da34:	mov    0x10(%rsi),%rax
   14004da38:	cmp    %rbp,%rax
   14004da3b:	jb     14004da58 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x198>
   14004da3d:	mov    $0x302e,%edx
   14004da42:	mov    %dx,(%r8,%rbx,1)
   14004da47:	mov    (%rsi),%r12
   14004da4a:	mov    %rbp,0x8(%rsi)
   14004da4e:	movb   $0x0,(%r12,%rbp,1)
   14004da53:	jmp    14004d998 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0xd8>
   14004da58:	test   %rbp,%rbp
   14004da5b:	js     14004db52 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x292>
   14004da61:	lea    (%rax,%rax,1),%r13
   14004da65:	cmp    %r13,%rbp
   14004da68:	jae    14004da85 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1c5>
   14004da6a:	test   %r13,%r13
   14004da6d:	jns    14004db28 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x268>
   14004da73:	call   140080050 <_ZSt17__throw_bad_allocv>
   14004da78:	nopl   0x0(%rax,%rax,1)
   14004da80:	call   140080050 <_ZSt17__throw_bad_allocv>
   14004da85:	mov    %rbx,%rcx
   14004da88:	mov    %rbp,%r14
   14004da8b:	add    $0x3,%rcx
   14004da8f:	js     14004da73 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1b3>
   14004da91:	call   140080000 <_Znwy>
   14004da96:	mov    %rax,%r12
   14004da99:	test   %rbx,%rbx
   14004da9c:	jne    14004db3a <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x27a>
   14004daa2:	mov    (%rsi),%rcx
   14004daa5:	mov    $0x302e,%eax
   14004daaa:	mov    %ax,(%r12,%rbx,1)
   14004daaf:	cmp    %rcx,%rdi
   14004dab2:	je     14004dac1 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x201>
   14004dab4:	mov    0x10(%rsi),%rax
   14004dab8:	lea    0x1(%rax),%rdx
   14004dabc:	call   140080008 <_ZdlPvy>
   14004dac1:	mov    %rbp,0x10(%rsi)
   14004dac5:	mov    %r14,%rbp
   14004dac8:	mov    %r12,(%rsi)
   14004dacb:	jmp    14004da4a <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x18a>
   14004dad0:	mov    $0x2,%ebp
   14004dad5:	cmp    %r8,%rdi
   14004dad8:	je     14004da3d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x17d>
   14004dade:	cmpq   $0x1,0x10(%rsi)
   14004dae3:	mov    $0x2,%r14d
   14004dae9:	mov    $0x3,%ecx
   14004daee:	ja     14004da3d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x17d>
   14004daf4:	jmp    14004da91 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1d1>
   14004daf6:	cmp    $0xf,%rbp
   14004dafa:	jbe    14004da3d <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x17d>
   14004db00:	mov    $0x1f,%ecx
   14004db05:	call   140080000 <_Znwy>
   14004db0a:	mov    (%rsi),%rdx
   14004db0d:	mov    %rbp,%r14
   14004db10:	mov    %rax,%r12
   14004db13:	mov    $0x1e,%ebp
   14004db18:	mov    %rbx,%r8
   14004db1b:	mov    %r12,%rcx
   14004db1e:	call   140087af8 <memcpy>
   14004db23:	jmp    14004daa2 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1e2>
   14004db28:	lea    0x1(%r13),%rcx
   14004db2c:	call   140080000 <_Znwy>
   14004db31:	mov    %rbp,%r14
   14004db34:	mov    %rax,%r12
   14004db37:	mov    %r13,%rbp
   14004db3a:	mov    (%rsi),%rcx
   14004db3d:	mov    %rcx,%rdx
   14004db40:	cmp    $0x1,%rbx
   14004db44:	jne    14004db18 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x258>
   14004db46:	movzbl (%rcx),%eax
   14004db49:	mov    %al,(%r12)
   14004db4d:	jmp    14004daa5 <_ZN12tx_generated18tx_float_to_stringB5cxx11Ed+0x1e5>
   14004db52:	lea    0x5d649(%rip),%rcx        # 1400ab1a2 <.rdata+0x82>
   14004db59:	call   140080040 <_ZSt20__throw_length_errorPKc>
   14004db5e:	lea    0x5d63d(%rip),%rcx        # 1400ab1a2 <.rdata+0x82>
   14004db65:	call   140080040 <_ZSt20__throw_length_errorPKc>
   14004db6a:	lea    0x5d736(%rip),%rcx        # 1400ab2a7 <.rdata+0x187>
   14004db71:	call   140080040 <_ZSt20__throw_length_errorPKc>
   14004db76:	mov    %rax,%rbx
   14004db79:	mov    %rsi,%rcx
   14004db7c:	call   140099600 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   14004db81:	mov    %rbx,%rcx
   14004db84:	call   140081918 <_Unwind_Resume>
   14004db89:	mov    $0x10,%ecx
   14004db8e:	call   14007fff8 <__cxa_allocate_exception>
   14004db93:	lea    0x5d6f8(%rip),%rdx        # 1400ab292 <.rdata+0x172>
   14004db9a:	mov    %rax,%rcx
   14004db9d:	mov    %rax,%rbx
   14004dba0:	call   1400800f8 <_ZNSt13runtime_errorC1EPKc>
   14004dba5:	lea    0x32534(%rip),%r8        # 1400800e0 <_ZNSt13runtime_errorD1Ev>
   14004dbac:	lea    0x6096d(%rip),%rdx        # 1400ae520 <_ZTISt13runtime_error>
   14004dbb3:	mov    %rbx,%rcx
   14004dbb6:	call   14007ffb0 <__cxa_throw>
   14004dbbb:	mov    %rax,%rsi
   14004dbbe:	mov    %rbx,%rcx
   14004dbc1:	call   14007ffe0 <__cxa_free_exception>
   14004dbc6:	mov    %rsi,%rcx
   14004dbc9:	call   140081918 <_Unwind_Resume>
   14004dbce:	nop
   14004dbcf:	nop


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\diverse.exe:     file format pei-x86-64


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


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\diverse.exe:     file format pei-x86-64


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
   14004d973:	call   14007af00 <_ZN12tx_generated16parse_int_scalarESt17basic_string_viewIcSt11char_traitsIcEEx>
   14004d978:	mov    0x38(%rsp),%rax
   14004d97d:	mov    0x30(%rsp),%rdx
   14004d982:	mov    0x98547(%rip),%rcx        # 1400e5ed0 <.refptr.__emutls_v._ZN12tx_generated6detail14thread_contextE>
   14004d989:	test   %rax,%rax
   14004d98c:	sete   (%rsi)
   14004d98f:	mov    %rdx,(%rbx)
   14004d992:	mov    0x80(%rsp),%rdx
   14004d99a:	mov    %rax,(%rdx)
   14004d99d:	call   1400b2fa0 <__emutls_get_address>
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


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\borrowing.exe:     file format pei-x86-64


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
   1400019f8:	lea    0x9ff2b(%rip),%rax        # 1400a192a <.rdata+0x92a>
   1400019ff:	mov    %rax,0x48(%rsp)
   140001a04:	lea    0x9ff35(%rip),%rax        # 1400a1940 <.rdata+0x940>
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
   140001afc:	lea    0x9fb6d(%rip),%rdx        # 1400a1670 <.rdata+0x670>
   140001b03:	jmp    140001beb <tx_fn_m0_bench_map_0+0x20b>
   140001b08:	lea    0x9fbdb(%rip),%rcx        # 1400a16ea <.rdata+0x6ea>
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
   140001be4:	lea    0x9fac5(%rip),%rdx        # 1400a16b0 <.rdata+0x6b0>
   140001beb:	mov    $0x1c,%r8d
   140001bf1:	mov    $0x9,%r9d
   140001bf7:	mov    %rsi,%rcx
   140001bfa:	call   140026f40 <txrt_stack_error_location>
   140001bff:	mov    %edi,%ecx
   140001c01:	call   14002b6c0 <txrt_require_success>
   140001c06:	lea    0x9f8e3(%rip),%rdx        # 1400a14f0 <.rdata+0x4f0>
   140001c0d:	jmp    140001c16 <tx_fn_m0_bench_map_0+0x236>
   140001c0f:	lea    0x9f91a(%rip),%rdx        # 1400a1530 <.rdata+0x530>
   140001c16:	mov    $0x16,%r8d
   140001c1c:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c21:	lea    0x9f948(%rip),%rdx        # 1400a1570 <.rdata+0x570>
   140001c28:	mov    $0x17,%r8d
   140001c2e:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c33:	lea    0x9f976(%rip),%rdx        # 1400a15b0 <.rdata+0x5b0>
   140001c3a:	mov    $0x17,%r8d
   140001c40:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c45:	lea    0x9f9a4(%rip),%rdx        # 1400a15f0 <.rdata+0x5f0>
   140001c4c:	mov    $0x19,%r8d
   140001c52:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c57:	lea    0x9f9d2(%rip),%rdx        # 1400a1630 <.rdata+0x630>
   140001c5e:	mov    $0x19,%r8d
   140001c64:	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c66:	lea    0x9fa83(%rip),%rdx        # 1400a16f0 <.rdata+0x6f0>
   140001c6d:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001c6f:	lea    0x9faba(%rip),%rdx        # 1400a1730 <.rdata+0x730>
   140001c76:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001c78:	lea    0x70(%rsp),%r8
   140001c7d:	mov    %r14,%rdx
   140001c80:	call   14002c8a0 <txrt_sub_i64>
   140001c85:	mov    %eax,%edi
   140001c87:	lea    0x9fae2(%rip),%rdx        # 1400a1770 <.rdata+0x770>
   140001c8e:	mov    $0x1e,%r8d
   140001c94:	mov    $0x5,%r9d
   140001c9a:	mov    %rsi,%rcx
   140001c9d:	call   140026f40 <txrt_stack_error_location>
   140001ca2:	mov    %edi,%ecx
   140001ca4:	call   14002b6c0 <txrt_require_success>
   140001ca9:	lea    0x9fb00(%rip),%rdx        # 1400a17b0 <.rdata+0x7b0>
   140001cb0:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cb2:	lea    0x9fb37(%rip),%rdx        # 1400a17f0 <.rdata+0x7f0>
   140001cb9:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cbb:	lea    0x9fb6e(%rip),%rdx        # 1400a1830 <.rdata+0x830>
   140001cc2:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cc4:	lea    0x9fba5(%rip),%rdx        # 1400a1870 <.rdata+0x870>
   140001ccb:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001ccd:	lea    0x9fbdc(%rip),%rdx        # 1400a18b0 <.rdata+0x8b0>
   140001cd4:	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cd6:	lea    0x9fc13(%rip),%rdx        # 1400a18f0 <.rdata+0x8f0>
   140001cdd:	mov    $0x1e,%r8d
   140001ce3:	mov    $0x5,%r9d
   140001ce9:	mov    %rsi,%rcx
   140001cec:	mov    %eax,%esi
   140001cee:	call   140026f40 <txrt_stack_error_location>
   140001cf3:	mov    %esi,%ecx
   140001cf5:	call   14002b6c0 <txrt_require_success>
   140001cfa:	int3
   140001cfb:	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_15_16\baseline\borrowing.exe:     file format pei-x86-64


Disassembly of section .text:

000000014001ccb0 <txrt_map_read_i64_i64>:
   14001ccb0:	push   %rdi
   14001ccb1:	push   %rsi
   14001ccb2:	push   %rbx
   14001ccb3:	sub    $0x20,%rsp
   14001ccb7:	mov    %rdx,%rbx
   14001ccba:	mov    %r8,%rsi
   14001ccbd:	call   14009b6d0 <_ZSt12__any_casterISt10shared_ptrIN12tx_generated17container_storageEEEPvPKSt3any>
   14001ccc2:	test   %rax,%rax
   14001ccc5:	je     14001cd97 <txrt_map_read_i64_i64+0xe7>
   14001cccb:	mov    (%rax),%rcx
   14001ccce:	cmpq   $0x0,0x28(%rcx)
   14001ccd3:	jne    14001cd30 <txrt_map_read_i64_i64+0x80>
   14001ccd5:	mov    0x20(%rcx),%rdx
   14001ccd9:	test   %rdx,%rdx
   14001ccdc:	jne    14001cd18 <txrt_map_read_i64_i64+0x68>
   14001ccde:	mov    $0x10,%ecx
   14001cce3:	call   140074538 <__cxa_allocate_exception>
   14001cce8:	lea    0x86c22(%rip),%rdx        # 1400a3911 <.rdata+0x151>
   14001ccef:	mov    %rax,%rcx
   14001ccf2:	mov    %rax,%rdi
   14001ccf5:	call   140074680 <_ZNSt12out_of_rangeC1EPKc>
   14001ccfa:	lea    0x57977(%rip),%r8        # 140074678 <_ZNSt12out_of_rangeD1Ev>
   14001cd01:	lea    0x8bda8(%rip),%rdx        # 1400a8ab0 <_ZTISt12out_of_range>
   14001cd08:	mov    %rdi,%rcx
   14001cd0b:	call   1400744f8 <__cxa_throw>
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
   14001cd97:	call   14009bbb0 <_ZSt20__throw_bad_any_castv>
   14001cd9c:	mov    %rax,%rcx
   14001cd9f:	mov    %rdx,%rax
   14001cda2:	cmp    $0x3,%rax
   14001cda6:	je     14001cdf4 <txrt_map_read_i64_i64+0x144>
   14001cda8:	jg     14001cdba <txrt_map_read_i64_i64+0x10a>
   14001cdaa:	cmp    $0x1,%rax
   14001cdae:	je     14001ce25 <txrt_map_read_i64_i64+0x175>
   14001cdb0:	cmp    $0x2,%rax
   14001cdb4:	je     14001ce4c <txrt_map_read_i64_i64+0x19c>
   14001cdba:	call   140074530 <__cxa_begin_catch>
   14001cdbf:	lea    0x86af9(%rip),%r8        # 1400a38bf <.rdata+0xff>
   14001cdc6:	mov    $0x1,%ecx
   14001cdcb:	lea    0x86b03(%rip),%rdx        # 1400a38d5 <.rdata+0x115>
   14001cdd2:	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001cdd7:	call   140074528 <__cxa_end_catch>
   14001cddc:	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001cdde:	mov    %rax,%rbx
   14001cde1:	mov    %rdx,%rsi
   14001cde4:	mov    %rdi,%rcx
   14001cde7:	call   140074520 <__cxa_free_exception>
   14001cdec:	mov    %rbx,%rcx
   14001cdef:	mov    %rsi,%rax
   14001cdf2:	jmp    14001cda2 <txrt_map_read_i64_i64+0xf2>
   14001cdf4:	call   140074530 <__cxa_begin_catch>
   14001cdf9:	mov    %rax,%rcx
   14001cdfc:	mov    (%rax),%rax
   14001cdff:	call   *0x10(%rax)
   14001ce02:	lea    0x86aa5(%rip),%rdx        # 1400a38ae <.rdata+0xee>
   14001ce09:	mov    $0x1,%ecx
   14001ce0e:	mov    %rax,%r8
   14001ce11:	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce16:	call   140074528 <__cxa_end_catch>
   14001ce1b:	mov    $0x1,%eax
   14001ce20:	jmp    14001cd27 <txrt_map_read_i64_i64+0x77>
   14001ce25:	call   140074530 <__cxa_begin_catch>
   14001ce2a:	mov    %rax,%rbx
   14001ce2d:	mov    (%rax),%rax
   14001ce30:	mov    %rbx,%rcx
   14001ce33:	call   *0x10(%rax)
   14001ce36:	mov    0x18(%rbx),%rdx
   14001ce3a:	mov    0x10(%rbx),%ecx
   14001ce3d:	mov    %rax,%r8
   14001ce40:	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce45:	call   140074528 <__cxa_end_catch>
   14001ce4a:	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001ce4c:	call   140074530 <__cxa_begin_catch>
   14001ce51:	mov    %rax,%rcx
   14001ce54:	mov    (%rax),%rax
   14001ce57:	call   *0x10(%rax)
   14001ce5a:	lea    0x86a3b(%rip),%rdx        # 1400a389c <.rdata+0xdc>
   14001ce61:	mov    $0x1,%ecx
   14001ce66:	mov    %rax,%r8
   14001ce69:	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce6e:	call   140074528 <__cxa_end_catch>
   14001ce73:	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001ce75:	data16 cs nopw 0x0(%rax,%rax,1)
