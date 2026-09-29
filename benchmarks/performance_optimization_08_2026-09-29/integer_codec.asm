tx_build/performance_08/serde_paths_08.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140002820 <_ZN12tx_generated12_GLOBAL__N_113encode_scalarIxXadL_ZNS_12serde_writer7integerExEEEEvRS2_RKSt3anyRKNS_10serde_typeENS_11serde_depthE>:
   140002820:	56                   	push   %rsi
   140002821:	53                   	push   %rbx
   140002822:	48 83 ec 58          	sub    $0x58,%rsp
   140002826:	f3 41 0f 6f 01       	movdqu (%r9),%xmm0
   14000282b:	48 89 d3             	mov    %rdx,%rbx
   14000282e:	48 89 ce             	mov    %rcx,%rsi
   140002831:	ba 01 00 00 00       	mov    $0x1,%edx
   140002836:	0f 11 44 24 20       	movups %xmm0,0x20(%rsp)
   14000283b:	48 8d 4c 24 20       	lea    0x20(%rsp),%rcx
   140002840:	e8 db 1e 00 00       	call   140004720 <_ZNK12tx_generated11serde_depth5checkEb>
   140002845:	48 8b 03             	mov    (%rbx),%rax
   140002848:	48 8d 15 b1 03 0a 00 	lea    0xa03b1(%rip),%rdx        # 1400a2c00 <_ZNSt3any17_Manager_internalIxE9_S_manageENS_3_OpEPKS_PNS_4_ArgE>
   14000284f:	48 39 d0             	cmp    %rdx,%rax
   140002852:	74 33                	je     140002887 <_ZN12tx_generated12_GLOBAL__N_113encode_scalarIxXadL_ZNS_12serde_writer7integerExEEEEvRS2_RKSt3anyRKNS_10serde_typeENS_11serde_depthE+0x67>
   140002854:	48 85 c0             	test   %rax,%rax
   140002857:	74 6f                	je     1400028c8 <_ZN12tx_generated12_GLOBAL__N_113encode_scalarIxXadL_ZNS_12serde_writer7integerExEEEEvRS2_RKSt3anyRKNS_10serde_typeENS_11serde_depthE+0xa8>
   140002859:	b9 01 00 00 00       	mov    $0x1,%ecx
   14000285e:	4c 8d 44 24 30       	lea    0x30(%rsp),%r8
   140002863:	48 89 da             	mov    %rbx,%rdx
   140002866:	ff d0                	call   *%rax
   140002868:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
   14000286d:	48 8b 15 5c e6 0a 00 	mov    0xae65c(%rip),%rdx        # 1400b0ed0 <__fu11__ZTIx>
   140002874:	48 8b 42 08          	mov    0x8(%rdx),%rax
   140002878:	48 39 41 08          	cmp    %rax,0x8(%rcx)
   14000287c:	74 09                	je     140002887 <_ZN12tx_generated12_GLOBAL__N_113encode_scalarIxXadL_ZNS_12serde_writer7integerExEEEEvRS2_RKSt3anyRKNS_10serde_typeENS_11serde_depthE+0x67>
   14000287e:	e8 15 bd 08 00       	call   14008e598 <_ZNKSt9type_info7__equalERKS_>
   140002883:	84 c0                	test   %al,%al
   140002885:	74 19                	je     1400028a0 <_ZN12tx_generated12_GLOBAL__N_113encode_scalarIxXadL_ZNS_12serde_writer7integerExEEEEvRS2_RKSt3anyRKNS_10serde_typeENS_11serde_depthE+0x80>
   140002887:	48 8b 53 08          	mov    0x8(%rbx),%rdx
   14000288b:	48 89 f1             	mov    %rsi,%rcx
   14000288e:	e8 0d 2f 00 00       	call   1400057a0 <_ZN12tx_generated12serde_writer7integerEx>
   140002893:	90                   	nop
   140002894:	48 83 c4 58          	add    $0x58,%rsp
   140002898:	5b                   	pop    %rbx
   140002899:	5e                   	pop    %rsi
   14000289a:	c3                   	ret
   14000289b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
   1400028a0:	48 8d 5c 24 30       	lea    0x30(%rsp),%rbx
   1400028a5:	48 8d 15 f4 70 0a 00 	lea    0xa70f4(%rip),%rdx        # 1400a99a0 <.rdata+0x100>
   1400028ac:	48 89 d9             	mov    %rbx,%rcx
   1400028af:	e8 3c fc ff ff       	call   1400024f0 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0>
   1400028b4:	48 89 da             	mov    %rbx,%rdx
   1400028b7:	48 8d 0d d2 70 0a 00 	lea    0xa70d2(%rip),%rcx        # 1400a9990 <.rdata+0xf0>
   1400028be:	e8 9f 20 0a 00       	call   1400a4962 <_ZN12tx_generated18serde_encode_errorEPKcNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE>
   1400028c3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
   1400028c8:	48 8b 0d f1 e5 0a 00 	mov    0xae5f1(%rip),%rcx        # 1400b0ec0 <__fu9__ZTIv>
   1400028cf:	eb 9c                	jmp    14000286d <_ZN12tx_generated12_GLOBAL__N_113encode_scalarIxXadL_ZNS_12serde_writer7integerExEEEEvRS2_RKSt3anyRKNS_10serde_typeENS_11serde_depthE+0x4d>
   1400028d1:	48 89 c6             	mov    %rax,%rsi
   1400028d4:	48 89 d9             	mov    %rbx,%rcx
   1400028d7:	e8 44 04 0a 00       	call   1400a2d20 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv>
   1400028dc:	48 89 f1             	mov    %rsi,%rcx
   1400028df:	e8 b4 d3 08 00       	call   14008fc98 <_Unwind_Resume>
   1400028e4:	90                   	nop
   1400028e5:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
   1400028ec:	00 00 00 00
