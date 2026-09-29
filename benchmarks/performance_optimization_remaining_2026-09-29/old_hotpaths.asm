
E:\Project\other\Compilation\tx_build\performance_remaining\baseline\borrowing.exe:     file format pei-x86-64


Disassembly of section .text:

00000001400019e0 <tx_fn_m0_bench_map_0>:
   1400019e0:	41 57                	push   %r15
   1400019e2:	41 56                	push   %r14
   1400019e4:	41 55                	push   %r13
   1400019e6:	41 54                	push   %r12
   1400019e8:	56                   	push   %rsi
   1400019e9:	57                   	push   %rdi
   1400019ea:	53                   	push   %rbx
   1400019eb:	48 81 ec 80 00 00 00 	sub    $0x80,%rsp
   1400019f2:	48 89 ce             	mov    %rcx,%rsi
   1400019f5:	4c 8b 29             	mov    (%rcx),%r13
   1400019f8:	48 8d 05 2b 0f 0a 00 	lea    0xa0f2b(%rip),%rax        # 1400a292a <.rdata+0x92a>
   1400019ff:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   140001a04:	48 8d 05 35 0f 0a 00 	lea    0xa0f35(%rip),%rax        # 1400a2940 <.rdata+0x940>
   140001a0b:	48 89 44 24 50       	mov    %rax,0x50(%rsp)
   140001a10:	48 c7 44 24 58 14 00 	movq   $0x14,0x58(%rsp)
   140001a17:	00 00 
   140001a19:	48 c7 44 24 60 01 00 	movq   $0x1,0x60(%rsp)
   140001a20:	00 00 
   140001a22:	4c 89 6c 24 68       	mov    %r13,0x68(%rsp)
   140001a27:	48 8d 44 24 48       	lea    0x48(%rsp),%rax
   140001a2c:	48 89 01             	mov    %rax,(%rcx)
   140001a2f:	48 8d 4c 24 40       	lea    0x40(%rsp),%rcx
   140001a34:	e8 77 9f 01 00       	call   14001b9b0 <txrt_map_new_i64_i64>
   140001a39:	85 c0                	test   %eax,%eax
   140001a3b:	0f 85 c5 01 00 00    	jne    140001c06 <tx_fn_m0_bench_map_0+0x226>
   140001a41:	48 8b 7c 24 40       	mov    0x40(%rsp),%rdi
   140001a46:	48 89 f1             	mov    %rsi,%rcx
   140001a49:	e8 d2 1b 01 00       	call   140013620 <txrt_gc_safepoint_context>
   140001a4e:	85 c0                	test   %eax,%eax
   140001a50:	0f 85 b9 01 00 00    	jne    140001c0f <tx_fn_m0_bench_map_0+0x22f>
   140001a56:	ba 07 00 00 00       	mov    $0x7,%edx
   140001a5b:	41 b8 03 00 00 00    	mov    $0x3,%r8d
   140001a61:	48 89 f9             	mov    %rdi,%rcx
   140001a64:	e8 c7 f4 01 00       	call   140020f30 <txrt_map_set_i64_i64>
   140001a69:	85 c0                	test   %eax,%eax
   140001a6b:	0f 85 b0 01 00 00    	jne    140001c21 <tx_fn_m0_bench_map_0+0x241>
   140001a71:	48 89 f1             	mov    %rsi,%rcx
   140001a74:	e8 a7 1b 01 00       	call   140013620 <txrt_gc_safepoint_context>
   140001a79:	85 c0                	test   %eax,%eax
   140001a7b:	0f 85 b2 01 00 00    	jne    140001c33 <tx_fn_m0_bench_map_0+0x253>
   140001a81:	48 8d 4c 24 38       	lea    0x38(%rsp),%rcx
   140001a86:	e8 75 a5 00 00       	call   14000c000 <txrt_time_monotonic_micros>
   140001a8b:	85 c0                	test   %eax,%eax
   140001a8d:	0f 85 b2 01 00 00    	jne    140001c45 <tx_fn_m0_bench_map_0+0x265>
   140001a93:	4c 8b 74 24 38       	mov    0x38(%rsp),%r14
   140001a98:	48 89 f1             	mov    %rsi,%rcx
   140001a9b:	e8 80 1b 01 00       	call   140013620 <txrt_gc_safepoint_context>
   140001aa0:	85 c0                	test   %eax,%eax
   140001aa2:	0f 85 af 01 00 00    	jne    140001c57 <tx_fn_m0_bench_map_0+0x277>
   140001aa8:	4c 8d 44 24 20       	lea    0x20(%rsp),%r8
   140001aad:	ba 07 00 00 00       	mov    $0x7,%edx
   140001ab2:	48 89 f9             	mov    %rdi,%rcx
   140001ab5:	e8 f6 b1 01 00       	call   14001ccb0 <txrt_map_read_i64_i64>
   140001aba:	85 c0                	test   %eax,%eax
   140001abc:	75 3c                	jne    140001afa <tx_fn_m0_bench_map_0+0x11a>
   140001abe:	41 bc 40 42 0f 00    	mov    $0xf4240,%r12d
   140001ac4:	31 c9                	xor    %ecx,%ecx
   140001ac6:	4c 8d 7c 24 20       	lea    0x20(%rsp),%r15
   140001acb:	31 db                	xor    %ebx,%ebx
   140001acd:	0f 1f 00             	nopl   (%rax)
   140001ad0:	48 8b 54 24 20       	mov    0x20(%rsp),%rdx
   140001ad5:	48 01 d3             	add    %rdx,%rbx
   140001ad8:	0f 80 fa 00 00 00    	jo     140001bd8 <tx_fn_m0_bench_map_0+0x1f8>
   140001ade:	49 ff cc             	dec    %r12
   140001ae1:	74 25                	je     140001b08 <tx_fn_m0_bench_map_0+0x128>
   140001ae3:	ba 07 00 00 00       	mov    $0x7,%edx
   140001ae8:	48 89 f9             	mov    %rdi,%rcx
   140001aeb:	4d 89 f8             	mov    %r15,%r8
   140001aee:	e8 bd b1 01 00       	call   14001ccb0 <txrt_map_read_i64_i64>
   140001af3:	48 89 d9             	mov    %rbx,%rcx
   140001af6:	85 c0                	test   %eax,%eax
   140001af8:	74 d6                	je     140001ad0 <tx_fn_m0_bench_map_0+0xf0>
   140001afa:	89 c7                	mov    %eax,%edi
   140001afc:	48 8d 15 6d 0b 0a 00 	lea    0xa0b6d(%rip),%rdx        # 1400a2670 <.rdata+0x670>
   140001b03:	e9 e3 00 00 00       	jmp    140001beb <tx_fn_m0_bench_map_0+0x20b>
   140001b08:	48 8d 0d db 0b 0a 00 	lea    0xa0bdb(%rip),%rcx        # 1400a26ea <.rdata+0x6ea>
   140001b0f:	4c 8d 44 24 30       	lea    0x30(%rsp),%r8
   140001b14:	ba 03 00 00 00       	mov    $0x3,%edx
   140001b19:	e8 b2 a1 02 00       	call   14002bcd0 <txrt_str_new>
   140001b1e:	85 c0                	test   %eax,%eax
   140001b20:	0f 85 40 01 00 00    	jne    140001c66 <tx_fn_m0_bench_map_0+0x286>
   140001b26:	4c 8b 7c 24 30       	mov    0x30(%rsp),%r15
   140001b2b:	48 8d 4c 24 28       	lea    0x28(%rsp),%rcx
   140001b30:	e8 cb a4 00 00       	call   14000c000 <txrt_time_monotonic_micros>
   140001b35:	85 c0                	test   %eax,%eax
   140001b37:	0f 85 32 01 00 00    	jne    140001c6f <tx_fn_m0_bench_map_0+0x28f>
   140001b3d:	48 8b 4c 24 28       	mov    0x28(%rsp),%rcx
   140001b42:	49 89 cc             	mov    %rcx,%r12
   140001b45:	4d 29 f4             	sub    %r14,%r12
   140001b48:	0f 80 2a 01 00 00    	jo     140001c78 <tx_fn_m0_bench_map_0+0x298>
   140001b4e:	4c 89 f9             	mov    %r15,%rcx
   140001b51:	31 d2                	xor    %edx,%edx
   140001b53:	e8 48 a7 02 00       	call   14002c2a0 <txrt_print_str>
   140001b58:	85 c0                	test   %eax,%eax
   140001b5a:	0f 85 49 01 00 00    	jne    140001ca9 <tx_fn_m0_bench_map_0+0x2c9>
   140001b60:	b1 20                	mov    $0x20,%cl
   140001b62:	e8 d9 bd 02 00       	call   14002d940 <txrt_print_char>
   140001b67:	85 c0                	test   %eax,%eax
   140001b69:	0f 85 43 01 00 00    	jne    140001cb2 <tx_fn_m0_bench_map_0+0x2d2>
   140001b6f:	4c 89 e1             	mov    %r12,%rcx
   140001b72:	31 d2                	xor    %edx,%edx
   140001b74:	e8 47 9c 02 00       	call   14002b7c0 <txrt_print_i64>
   140001b79:	85 c0                	test   %eax,%eax
   140001b7b:	0f 85 3a 01 00 00    	jne    140001cbb <tx_fn_m0_bench_map_0+0x2db>
   140001b81:	b1 20                	mov    $0x20,%cl
   140001b83:	e8 b8 bd 02 00       	call   14002d940 <txrt_print_char>
   140001b88:	85 c0                	test   %eax,%eax
   140001b8a:	0f 85 34 01 00 00    	jne    140001cc4 <tx_fn_m0_bench_map_0+0x2e4>
   140001b90:	48 89 d9             	mov    %rbx,%rcx
   140001b93:	b2 01                	mov    $0x1,%dl
   140001b95:	e8 26 9c 02 00       	call   14002b7c0 <txrt_print_i64>
   140001b9a:	85 c0                	test   %eax,%eax
   140001b9c:	0f 85 2b 01 00 00    	jne    140001ccd <tx_fn_m0_bench_map_0+0x2ed>
   140001ba2:	4c 89 f9             	mov    %r15,%rcx
   140001ba5:	e8 d6 a4 02 00       	call   14002c080 <txrt_str_release>
   140001baa:	48 89 f1             	mov    %rsi,%rcx
   140001bad:	e8 6e 1a 01 00       	call   140013620 <txrt_gc_safepoint_context>
   140001bb2:	85 c0                	test   %eax,%eax
   140001bb4:	0f 85 1c 01 00 00    	jne    140001cd6 <tx_fn_m0_bench_map_0+0x2f6>
   140001bba:	48 89 f9             	mov    %rdi,%rcx
   140001bbd:	e8 6e 50 00 00       	call   140006c30 <txrt_value_release>
   140001bc2:	4c 89 2e             	mov    %r13,(%rsi)
   140001bc5:	48 81 c4 80 00 00 00 	add    $0x80,%rsp
   140001bcc:	5b                   	pop    %rbx
   140001bcd:	5f                   	pop    %rdi
   140001bce:	5e                   	pop    %rsi
   140001bcf:	41 5c                	pop    %r12
   140001bd1:	41 5d                	pop    %r13
   140001bd3:	41 5e                	pop    %r14
   140001bd5:	41 5f                	pop    %r15
   140001bd7:	c3                   	ret
   140001bd8:	4c 8d 44 24 78       	lea    0x78(%rsp),%r8
   140001bdd:	e8 de ab 02 00       	call   14002c7c0 <txrt_add_i64>
   140001be2:	89 c7                	mov    %eax,%edi
   140001be4:	48 8d 15 c5 0a 0a 00 	lea    0xa0ac5(%rip),%rdx        # 1400a26b0 <.rdata+0x6b0>
   140001beb:	41 b8 1c 00 00 00    	mov    $0x1c,%r8d
   140001bf1:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001bf7:	48 89 f1             	mov    %rsi,%rcx
   140001bfa:	e8 41 53 02 00       	call   140026f40 <txrt_stack_error_location>
   140001bff:	89 f9                	mov    %edi,%ecx
   140001c01:	e8 ba 9a 02 00       	call   14002b6c0 <txrt_require_success>
   140001c06:	48 8d 15 e3 08 0a 00 	lea    0xa08e3(%rip),%rdx        # 1400a24f0 <.rdata+0x4f0>
   140001c0d:	eb 07                	jmp    140001c16 <tx_fn_m0_bench_map_0+0x236>
   140001c0f:	48 8d 15 1a 09 0a 00 	lea    0xa091a(%rip),%rdx        # 1400a2530 <.rdata+0x530>
   140001c16:	41 b8 16 00 00 00    	mov    $0x16,%r8d
   140001c1c:	e9 c2 00 00 00       	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c21:	48 8d 15 48 09 0a 00 	lea    0xa0948(%rip),%rdx        # 1400a2570 <.rdata+0x570>
   140001c28:	41 b8 17 00 00 00    	mov    $0x17,%r8d
   140001c2e:	e9 b0 00 00 00       	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c33:	48 8d 15 76 09 0a 00 	lea    0xa0976(%rip),%rdx        # 1400a25b0 <.rdata+0x5b0>
   140001c3a:	41 b8 17 00 00 00    	mov    $0x17,%r8d
   140001c40:	e9 9e 00 00 00       	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c45:	48 8d 15 a4 09 0a 00 	lea    0xa09a4(%rip),%rdx        # 1400a25f0 <.rdata+0x5f0>
   140001c4c:	41 b8 19 00 00 00    	mov    $0x19,%r8d
   140001c52:	e9 8c 00 00 00       	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c57:	48 8d 15 d2 09 0a 00 	lea    0xa09d2(%rip),%rdx        # 1400a2630 <.rdata+0x630>
   140001c5e:	41 b8 19 00 00 00    	mov    $0x19,%r8d
   140001c64:	eb 7d                	jmp    140001ce3 <tx_fn_m0_bench_map_0+0x303>
   140001c66:	48 8d 15 83 0a 0a 00 	lea    0xa0a83(%rip),%rdx        # 1400a26f0 <.rdata+0x6f0>
   140001c6d:	eb 6e                	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001c6f:	48 8d 15 ba 0a 0a 00 	lea    0xa0aba(%rip),%rdx        # 1400a2730 <.rdata+0x730>
   140001c76:	eb 65                	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001c78:	4c 8d 44 24 70       	lea    0x70(%rsp),%r8
   140001c7d:	4c 89 f2             	mov    %r14,%rdx
   140001c80:	e8 1b ac 02 00       	call   14002c8a0 <txrt_sub_i64>
   140001c85:	89 c7                	mov    %eax,%edi
   140001c87:	48 8d 15 e2 0a 0a 00 	lea    0xa0ae2(%rip),%rdx        # 1400a2770 <.rdata+0x770>
   140001c8e:	41 b8 1e 00 00 00    	mov    $0x1e,%r8d
   140001c94:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001c9a:	48 89 f1             	mov    %rsi,%rcx
   140001c9d:	e8 9e 52 02 00       	call   140026f40 <txrt_stack_error_location>
   140001ca2:	89 f9                	mov    %edi,%ecx
   140001ca4:	e8 17 9a 02 00       	call   14002b6c0 <txrt_require_success>
   140001ca9:	48 8d 15 00 0b 0a 00 	lea    0xa0b00(%rip),%rdx        # 1400a27b0 <.rdata+0x7b0>
   140001cb0:	eb 2b                	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cb2:	48 8d 15 37 0b 0a 00 	lea    0xa0b37(%rip),%rdx        # 1400a27f0 <.rdata+0x7f0>
   140001cb9:	eb 22                	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cbb:	48 8d 15 6e 0b 0a 00 	lea    0xa0b6e(%rip),%rdx        # 1400a2830 <.rdata+0x830>
   140001cc2:	eb 19                	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cc4:	48 8d 15 a5 0b 0a 00 	lea    0xa0ba5(%rip),%rdx        # 1400a2870 <.rdata+0x870>
   140001ccb:	eb 10                	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001ccd:	48 8d 15 dc 0b 0a 00 	lea    0xa0bdc(%rip),%rdx        # 1400a28b0 <.rdata+0x8b0>
   140001cd4:	eb 07                	jmp    140001cdd <tx_fn_m0_bench_map_0+0x2fd>
   140001cd6:	48 8d 15 13 0c 0a 00 	lea    0xa0c13(%rip),%rdx        # 1400a28f0 <.rdata+0x8f0>
   140001cdd:	41 b8 1e 00 00 00    	mov    $0x1e,%r8d
   140001ce3:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001ce9:	48 89 f1             	mov    %rsi,%rcx
   140001cec:	89 c6                	mov    %eax,%esi
   140001cee:	e8 4d 52 02 00       	call   140026f40 <txrt_stack_error_location>
   140001cf3:	89 f1                	mov    %esi,%ecx
   140001cf5:	e8 c6 99 02 00       	call   14002b6c0 <txrt_require_success>
   140001cfa:	cc                   	int3
   140001cfb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_remaining\baseline\borrowing.exe:     file format pei-x86-64


Disassembly of section .text:

000000014001ccb0 <txrt_map_read_i64_i64>:
   14001ccb0:	57                   	push   %rdi
   14001ccb1:	56                   	push   %rsi
   14001ccb2:	53                   	push   %rbx
   14001ccb3:	48 83 ec 20          	sub    $0x20,%rsp
   14001ccb7:	48 89 d3             	mov    %rdx,%rbx
   14001ccba:	4c 89 c6             	mov    %r8,%rsi
   14001ccbd:	e8 2e f0 07 00       	call   14009bcf0 <_ZSt12__any_casterISt10shared_ptrIN12tx_generated17container_storageEEEPvPKSt3any>
   14001ccc2:	48 85 c0             	test   %rax,%rax
   14001ccc5:	0f 84 cc 00 00 00    	je     14001cd97 <txrt_map_read_i64_i64+0xe7>
   14001cccb:	48 8b 08             	mov    (%rax),%rcx
   14001ccce:	48 83 79 28 00       	cmpq   $0x0,0x28(%rcx)
   14001ccd3:	75 5b                	jne    14001cd30 <txrt_map_read_i64_i64+0x80>
   14001ccd5:	48 8b 51 20          	mov    0x20(%rcx),%rdx
   14001ccd9:	48 85 d2             	test   %rdx,%rdx
   14001ccdc:	75 3a                	jne    14001cd18 <txrt_map_read_i64_i64+0x68>
   14001ccde:	b9 10 00 00 00       	mov    $0x10,%ecx
   14001cce3:	e8 70 7e 05 00       	call   140074b58 <__cxa_allocate_exception>
   14001cce8:	48 8d 15 22 7c 08 00 	lea    0x87c22(%rip),%rdx        # 1400a4911 <.rdata+0x151>
   14001ccef:	48 89 c1             	mov    %rax,%rcx
   14001ccf2:	48 89 c7             	mov    %rax,%rdi
   14001ccf5:	e8 a6 7f 05 00       	call   140074ca0 <_ZNSt12out_of_rangeC1EPKc>
   14001ccfa:	4c 8d 05 97 7f 05 00 	lea    0x57f97(%rip),%r8        # 140074c98 <_ZNSt12out_of_rangeD1Ev>
   14001cd01:	48 8d 15 08 d2 08 00 	lea    0x8d208(%rip),%rdx        # 1400a9f10 <_ZTISt12out_of_range>
   14001cd08:	48 89 f9             	mov    %rdi,%rcx
   14001cd0b:	e8 08 7e 05 00       	call   140074b18 <__cxa_throw>
   14001cd10:	48 8b 12             	mov    (%rdx),%rdx
   14001cd13:	48 85 d2             	test   %rdx,%rdx
   14001cd16:	74 c6                	je     14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd18:	48 3b 5a 08          	cmp    0x8(%rdx),%rbx
   14001cd1c:	75 f2                	jne    14001cd10 <txrt_map_read_i64_i64+0x60>
   14001cd1e:	48 8b 42 10          	mov    0x10(%rdx),%rax
   14001cd22:	48 89 06             	mov    %rax,(%rsi)
   14001cd25:	31 c0                	xor    %eax,%eax
   14001cd27:	48 83 c4 20          	add    $0x20,%rsp
   14001cd2b:	5b                   	pop    %rbx
   14001cd2c:	5e                   	pop    %rsi
   14001cd2d:	5f                   	pop    %rdi
   14001cd2e:	c3                   	ret
   14001cd2f:	90                   	nop
   14001cd30:	4c 8b 51 18          	mov    0x18(%rcx),%r10
   14001cd34:	48 89 d8             	mov    %rbx,%rax
   14001cd37:	31 d2                	xor    %edx,%edx
   14001cd39:	49 f7 f2             	div    %r10
   14001cd3c:	48 8b 41 10          	mov    0x10(%rcx),%rax
   14001cd40:	4c 8b 1c d0          	mov    (%rax,%rdx,8),%r11
   14001cd44:	49 89 d0             	mov    %rdx,%r8
   14001cd47:	4d 85 db             	test   %r11,%r11
   14001cd4a:	74 92                	je     14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd4c:	49 8b 03             	mov    (%r11),%rax
   14001cd4f:	48 8b 48 18          	mov    0x18(%rax),%rcx
   14001cd53:	eb 2a                	jmp    14001cd7f <txrt_map_read_i64_i64+0xcf>
   14001cd55:	0f 1f 00             	nopl   (%rax)
   14001cd58:	4c 8b 08             	mov    (%rax),%r9
   14001cd5b:	4d 85 c9             	test   %r9,%r9
   14001cd5e:	0f 84 7a ff ff ff    	je     14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd64:	49 8b 49 18          	mov    0x18(%r9),%rcx
   14001cd68:	49 89 c3             	mov    %rax,%r11
   14001cd6b:	31 d2                	xor    %edx,%edx
   14001cd6d:	48 89 c8             	mov    %rcx,%rax
   14001cd70:	49 f7 f2             	div    %r10
   14001cd73:	49 39 d0             	cmp    %rdx,%r8
   14001cd76:	0f 85 62 ff ff ff    	jne    14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd7c:	4c 89 c8             	mov    %r9,%rax
   14001cd7f:	48 39 d9             	cmp    %rbx,%rcx
   14001cd82:	75 d4                	jne    14001cd58 <txrt_map_read_i64_i64+0xa8>
   14001cd84:	48 3b 48 08          	cmp    0x8(%rax),%rcx
   14001cd88:	75 ce                	jne    14001cd58 <txrt_map_read_i64_i64+0xa8>
   14001cd8a:	49 8b 13             	mov    (%r11),%rdx
   14001cd8d:	48 85 d2             	test   %rdx,%rdx
   14001cd90:	75 8c                	jne    14001cd1e <txrt_map_read_i64_i64+0x6e>
   14001cd92:	e9 47 ff ff ff       	jmp    14001ccde <txrt_map_read_i64_i64+0x2e>
   14001cd97:	e8 34 f4 07 00       	call   14009c1d0 <_ZSt20__throw_bad_any_castv>
   14001cd9c:	48 89 c1             	mov    %rax,%rcx
   14001cd9f:	48 89 d0             	mov    %rdx,%rax
   14001cda2:	48 83 f8 03          	cmp    $0x3,%rax
   14001cda6:	74 4c                	je     14001cdf4 <txrt_map_read_i64_i64+0x144>
   14001cda8:	7f 10                	jg     14001cdba <txrt_map_read_i64_i64+0x10a>
   14001cdaa:	48 83 f8 01          	cmp    $0x1,%rax
   14001cdae:	74 75                	je     14001ce25 <txrt_map_read_i64_i64+0x175>
   14001cdb0:	48 83 f8 02          	cmp    $0x2,%rax
   14001cdb4:	0f 84 92 00 00 00    	je     14001ce4c <txrt_map_read_i64_i64+0x19c>
   14001cdba:	e8 91 7d 05 00       	call   140074b50 <__cxa_begin_catch>
   14001cdbf:	4c 8d 05 f9 7a 08 00 	lea    0x87af9(%rip),%r8        # 1400a48bf <.rdata+0xff>
   14001cdc6:	b9 01 00 00 00       	mov    $0x1,%ecx
   14001cdcb:	48 8d 15 03 7b 08 00 	lea    0x87b03(%rip),%rdx        # 1400a48d5 <.rdata+0x115>
   14001cdd2:	e8 99 9c 00 00       	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001cdd7:	e8 6c 7d 05 00       	call   140074b48 <__cxa_end_catch>
   14001cddc:	eb 3d                	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001cdde:	48 89 c3             	mov    %rax,%rbx
   14001cde1:	48 89 d6             	mov    %rdx,%rsi
   14001cde4:	48 89 f9             	mov    %rdi,%rcx
   14001cde7:	e8 54 7d 05 00       	call   140074b40 <__cxa_free_exception>
   14001cdec:	48 89 d9             	mov    %rbx,%rcx
   14001cdef:	48 89 f0             	mov    %rsi,%rax
   14001cdf2:	eb ae                	jmp    14001cda2 <txrt_map_read_i64_i64+0xf2>
   14001cdf4:	e8 57 7d 05 00       	call   140074b50 <__cxa_begin_catch>
   14001cdf9:	48 89 c1             	mov    %rax,%rcx
   14001cdfc:	48 8b 00             	mov    (%rax),%rax
   14001cdff:	ff 50 10             	call   *0x10(%rax)
   14001ce02:	48 8d 15 a5 7a 08 00 	lea    0x87aa5(%rip),%rdx        # 1400a48ae <.rdata+0xee>
   14001ce09:	b9 01 00 00 00       	mov    $0x1,%ecx
   14001ce0e:	49 89 c0             	mov    %rax,%r8
   14001ce11:	e8 5a 9c 00 00       	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce16:	e8 2d 7d 05 00       	call   140074b48 <__cxa_end_catch>
   14001ce1b:	b8 01 00 00 00       	mov    $0x1,%eax
   14001ce20:	e9 02 ff ff ff       	jmp    14001cd27 <txrt_map_read_i64_i64+0x77>
   14001ce25:	e8 26 7d 05 00       	call   140074b50 <__cxa_begin_catch>
   14001ce2a:	48 89 c3             	mov    %rax,%rbx
   14001ce2d:	48 8b 00             	mov    (%rax),%rax
   14001ce30:	48 89 d9             	mov    %rbx,%rcx
   14001ce33:	ff 50 10             	call   *0x10(%rax)
   14001ce36:	48 8b 53 18          	mov    0x18(%rbx),%rdx
   14001ce3a:	8b 4b 10             	mov    0x10(%rbx),%ecx
   14001ce3d:	49 89 c0             	mov    %rax,%r8
   14001ce40:	e8 2b 9c 00 00       	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce45:	e8 fe 7c 05 00       	call   140074b48 <__cxa_end_catch>
   14001ce4a:	eb cf                	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001ce4c:	e8 ff 7c 05 00       	call   140074b50 <__cxa_begin_catch>
   14001ce51:	48 89 c1             	mov    %rax,%rcx
   14001ce54:	48 8b 00             	mov    (%rax),%rax
   14001ce57:	ff 50 10             	call   *0x10(%rax)
   14001ce5a:	48 8d 15 3b 7a 08 00 	lea    0x87a3b(%rip),%rdx        # 1400a489c <.rdata+0xdc>
   14001ce61:	b9 01 00 00 00       	mov    $0x1,%ecx
   14001ce66:	49 89 c0             	mov    %rax,%r8
   14001ce69:	e8 02 9c 00 00       	call   140026a70 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001ce6e:	e8 d5 7c 05 00       	call   140074b48 <__cxa_end_catch>
   14001ce73:	eb a6                	jmp    14001ce1b <txrt_map_read_i64_i64+0x16b>
   14001ce75:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
   14001ce7c:	00 00 00 00 


E:\Project\other\Compilation\tx_build\performance_remaining\baseline\diverse.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140004410 <tx_fn_m0_bench_parse_paths_0>:
   140004410:	41 57                	push   %r15
   140004412:	41 56                	push   %r14
   140004414:	41 55                	push   %r13
   140004416:	41 54                	push   %r12
   140004418:	56                   	push   %rsi
   140004419:	57                   	push   %rdi
   14000441a:	55                   	push   %rbp
   14000441b:	53                   	push   %rbx
   14000441c:	48 81 ec f8 00 00 00 	sub    $0xf8,%rsp
   140004423:	48 89 ce             	mov    %rcx,%rsi
   140004426:	48 8b 39             	mov    (%rcx),%rdi
   140004429:	48 8d 05 20 8a 0d 00 	lea    0xd8a20(%rip),%rax        # 1400dce50 <.rdata+0x3e50>
   140004430:	48 89 44 24 50       	mov    %rax,0x50(%rsp)
   140004435:	48 8d 05 34 8a 0d 00 	lea    0xd8a34(%rip),%rax        # 1400dce70 <.rdata+0x3e70>
   14000443c:	48 89 44 24 58       	mov    %rax,0x58(%rsp)
   140004441:	48 c7 44 24 60 db 00 	movq   $0xdb,0x60(%rsp)
   140004448:	00 00 
   14000444a:	48 c7 44 24 68 01 00 	movq   $0x1,0x68(%rsp)
   140004451:	00 00 
   140004453:	48 89 7c 24 70       	mov    %rdi,0x70(%rsp)
   140004458:	48 8d 44 24 50       	lea    0x50(%rsp),%rax
   14000445d:	48 89 01             	mov    %rax,(%rcx)
   140004460:	4c 8d 84 24 c0 00 00 	lea    0xc0(%rsp),%r8
   140004467:	00 
   140004468:	31 c9                	xor    %ecx,%ecx
   14000446a:	31 d2                	xor    %edx,%edx
   14000446c:	e8 8f 39 04 00       	call   140047e00 <txrt_vector_new_str>
   140004471:	85 c0                	test   %eax,%eax
   140004473:	0f 85 b7 05 00 00    	jne    140004a30 <tx_fn_m0_bench_parse_paths_0+0x620>
   140004479:	48 8b 8c 24 c0 00 00 	mov    0xc0(%rsp),%rcx
   140004480:	00 
   140004481:	48 89 4c 24 40       	mov    %rcx,0x40(%rsp)
   140004486:	e8 95 35 04 00       	call   140047a20 <txrt_vector_ref_str>
   14000448b:	49 89 c7             	mov    %rax,%r15
   14000448e:	48 89 f1             	mov    %rsi,%rcx
   140004491:	e8 7a 6b 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   140004496:	85 c0                	test   %eax,%eax
   140004498:	0f 85 9b 05 00 00    	jne    140004a39 <tx_fn_m0_bench_parse_paths_0+0x629>
   14000449e:	4c 8d 84 24 b8 00 00 	lea    0xb8(%rsp),%r8
   1400044a5:	00 
   1400044a6:	31 c9                	xor    %ecx,%ecx
   1400044a8:	31 d2                	xor    %edx,%edx
   1400044aa:	e8 51 39 04 00       	call   140047e00 <txrt_vector_new_str>
   1400044af:	85 c0                	test   %eax,%eax
   1400044b1:	0f 85 94 05 00 00    	jne    140004a4b <tx_fn_m0_bench_parse_paths_0+0x63b>
   1400044b7:	48 8b 8c 24 b8 00 00 	mov    0xb8(%rsp),%rcx
   1400044be:	00 
   1400044bf:	48 89 4c 24 38       	mov    %rcx,0x38(%rsp)
   1400044c4:	e8 57 35 04 00       	call   140047a20 <txrt_vector_ref_str>
   1400044c9:	49 89 c6             	mov    %rax,%r14
   1400044cc:	48 89 f1             	mov    %rsi,%rcx
   1400044cf:	e8 3c 6b 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   1400044d4:	85 c0                	test   %eax,%eax
   1400044d6:	0f 85 81 05 00 00    	jne    140004a5d <tx_fn_m0_bench_parse_paths_0+0x64d>
   1400044dc:	4c 8d 05 7c 83 0d 00 	lea    0xd837c(%rip),%r8        # 1400dc85f <.rdata+0x385f>
   1400044e3:	48 8d 4c 24 48       	lea    0x48(%rsp),%rcx
   1400044e8:	31 d2                	xor    %edx,%edx
   1400044ea:	45 31 c9             	xor    %r9d,%r9d
   1400044ed:	e8 7e ce 02 00       	call   140031370 <txrt_format_begin>
   1400044f2:	85 c0                	test   %eax,%eax
   1400044f4:	0f 85 3b 01 00 00    	jne    140004635 <tx_fn_m0_bench_parse_paths_0+0x225>
   1400044fa:	48 89 bc 24 80 00 00 	mov    %rdi,0x80(%rsp)
   140004501:	00 
   140004502:	48 8d 2d d6 84 0d 00 	lea    0xd84d6(%rip),%rbp        # 1400dc9df <.rdata+0x39df>
   140004509:	48 8d 5c 24 48       	lea    0x48(%rsp),%rbx
   14000450e:	45 31 e4             	xor    %r12d,%r12d
   140004511:	66 66 66 66 66 66 2e 	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140004518:	0f 1f 84 00 00 00 00 
   14000451f:	00 
   140004520:	48 8b 7c 24 48       	mov    0x48(%rsp),%rdi
   140004525:	48 89 f9             	mov    %rdi,%rcx
   140004528:	4c 89 e2             	mov    %r12,%rdx
   14000452b:	4c 8d 05 6d 83 0d 00 	lea    0xd836d(%rip),%r8        # 1400dc89f <.rdata+0x389f>
   140004532:	45 31 c9             	xor    %r9d,%r9d
   140004535:	e8 f6 dc 02 00       	call   140032230 <txrt_format_plain_i64>
   14000453a:	85 c0                	test   %eax,%eax
   14000453c:	0f 85 cc 03 00 00    	jne    14000490e <tx_fn_m0_bench_parse_paths_0+0x4fe>
   140004542:	48 89 f9             	mov    %rdi,%rcx
   140004545:	e8 d6 e6 02 00       	call   140032c20 <txrt_format_finish>
   14000454a:	48 89 f1             	mov    %rsi,%rcx
   14000454d:	e8 be 6a 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   140004552:	85 c0                	test   %eax,%eax
   140004554:	0f 85 da 03 00 00    	jne    140004934 <tx_fn_m0_bench_parse_paths_0+0x524>
   14000455a:	48 89 f9             	mov    %rdi,%rcx
   14000455d:	48 8d 94 24 b0 00 00 	lea    0xb0(%rsp),%rdx
   140004564:	00 
   140004565:	e8 e6 00 05 00       	call   140054650 <txrt_str_clone>
   14000456a:	85 c0                	test   %eax,%eax
   14000456c:	0f 85 d1 03 00 00    	jne    140004943 <tx_fn_m0_bench_parse_paths_0+0x533>
   140004572:	4c 8b ac 24 b0 00 00 	mov    0xb0(%rsp),%r13
   140004579:	00 
   14000457a:	48 8b 4c 24 40       	mov    0x40(%rsp),%rcx
   14000457f:	4c 89 ea             	mov    %r13,%rdx
   140004582:	e8 59 3d 04 00       	call   1400482e0 <txrt_vector_push_back_str>
   140004587:	85 c0                	test   %eax,%eax
   140004589:	0f 85 bd 03 00 00    	jne    14000494c <tx_fn_m0_bench_parse_paths_0+0x53c>
   14000458f:	4c 89 e9             	mov    %r13,%rcx
   140004592:	e8 f9 01 05 00       	call   140054790 <txrt_str_release>
   140004597:	48 89 f1             	mov    %rsi,%rcx
   14000459a:	e8 71 6a 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   14000459f:	85 c0                	test   %eax,%eax
   1400045a1:	0f 85 ae 03 00 00    	jne    140004955 <tx_fn_m0_bench_parse_paths_0+0x545>
   1400045a7:	48 8d 84 24 a8 00 00 	lea    0xa8(%rsp),%rax
   1400045ae:	00 
   1400045af:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   1400045b4:	41 b8 01 00 00 00    	mov    $0x1,%r8d
   1400045ba:	48 89 f9             	mov    %rdi,%rcx
   1400045bd:	48 89 ea             	mov    %rbp,%rdx
   1400045c0:	45 31 c9             	xor    %r9d,%r9d
   1400045c3:	e8 08 68 04 00       	call   14004add0 <txrt_str_concat_literal>
   1400045c8:	85 c0                	test   %eax,%eax
   1400045ca:	0f 85 94 03 00 00    	jne    140004964 <tx_fn_m0_bench_parse_paths_0+0x554>
   1400045d0:	4c 8b ac 24 a8 00 00 	mov    0xa8(%rsp),%r13
   1400045d7:	00 
   1400045d8:	48 8b 4c 24 38       	mov    0x38(%rsp),%rcx
   1400045dd:	4c 89 ea             	mov    %r13,%rdx
   1400045e0:	e8 fb 3c 04 00       	call   1400482e0 <txrt_vector_push_back_str>
   1400045e5:	85 c0                	test   %eax,%eax
   1400045e7:	0f 85 80 03 00 00    	jne    14000496d <tx_fn_m0_bench_parse_paths_0+0x55d>
   1400045ed:	4c 89 e9             	mov    %r13,%rcx
   1400045f0:	e8 9b 01 05 00       	call   140054790 <txrt_str_release>
   1400045f5:	48 89 f1             	mov    %rsi,%rcx
   1400045f8:	e8 13 6a 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   1400045fd:	85 c0                	test   %eax,%eax
   1400045ff:	0f 85 71 03 00 00    	jne    140004976 <tx_fn_m0_bench_parse_paths_0+0x566>
   140004605:	48 89 f9             	mov    %rdi,%rcx
   140004608:	e8 83 01 05 00       	call   140054790 <txrt_str_release>
   14000460d:	49 ff c4             	inc    %r12
   140004610:	49 81 fc e8 03 00 00 	cmp    $0x3e8,%r12
   140004617:	74 40                	je     140004659 <tx_fn_m0_bench_parse_paths_0+0x249>
   140004619:	48 89 d9             	mov    %rbx,%rcx
   14000461c:	31 d2                	xor    %edx,%edx
   14000461e:	4c 8d 05 3a 82 0d 00 	lea    0xd823a(%rip),%r8        # 1400dc85f <.rdata+0x385f>
   140004625:	45 31 c9             	xor    %r9d,%r9d
   140004628:	e8 43 cd 02 00       	call   140031370 <txrt_format_begin>
   14000462d:	85 c0                	test   %eax,%eax
   14000462f:	0f 84 eb fe ff ff    	je     140004520 <tx_fn_m0_bench_parse_paths_0+0x110>
   140004635:	89 c7                	mov    %eax,%edi
   140004637:	48 8d 15 22 82 0d 00 	lea    0xd8222(%rip),%rdx        # 1400dc860 <.rdata+0x3860>
   14000463e:	41 b8 e1 00 00 00    	mov    $0xe1,%r8d
   140004644:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000464a:	48 89 f1             	mov    %rsi,%rcx
   14000464d:	e8 fe af 04 00       	call   14004f650 <txrt_stack_error_location>
   140004652:	89 f9                	mov    %edi,%ecx
   140004654:	e8 77 f7 04 00       	call   140053dd0 <txrt_require_success>
   140004659:	48 8d 8c 24 a0 00 00 	lea    0xa0(%rsp),%rcx
   140004660:	00 
   140004661:	e8 ea c5 02 00       	call   140030c50 <txrt_time_monotonic_micros>
   140004666:	85 c0                	test   %eax,%eax
   140004668:	0f 85 fe 03 00 00    	jne    140004a6c <tx_fn_m0_bench_parse_paths_0+0x65c>
   14000466e:	48 8b 84 24 a0 00 00 	mov    0xa0(%rsp),%rax
   140004675:	00 
   140004676:	48 89 44 24 78       	mov    %rax,0x78(%rsp)
   14000467b:	48 89 74 24 30       	mov    %rsi,0x30(%rsp)
   140004680:	48 89 f1             	mov    %rsi,%rcx
   140004683:	e8 88 69 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   140004688:	85 c0                	test   %eax,%eax
   14000468a:	0f 85 eb 03 00 00    	jne    140004a7b <tx_fn_m0_bench_parse_paths_0+0x66b>
   140004690:	49 83 7f 08 00       	cmpq   $0x0,0x8(%r15)
   140004695:	0f 84 c6 01 00 00    	je     140004861 <tx_fn_m0_bench_parse_paths_0+0x451>
   14000469b:	bd 01 00 00 00       	mov    $0x1,%ebp
   1400046a0:	bf 9f 86 01 00       	mov    $0x1869f,%edi
   1400046a5:	31 db                	xor    %ebx,%ebx
   1400046a7:	48 8d b4 24 e8 00 00 	lea    0xe8(%rsp),%rsi
   1400046ae:	00 
   1400046af:	4c 8d 64 24 2f       	lea    0x2f(%rsp),%r12
   1400046b4:	4c 8d ac 24 f0 00 00 	lea    0xf0(%rsp),%r13
   1400046bb:	00 
   1400046bc:	31 c0                	xor    %eax,%eax
   1400046be:	66 90                	xchg   %ax,%ax
   1400046c0:	49 8b 0f             	mov    (%r15),%rcx
   1400046c3:	48 8b 0c c1          	mov    (%rcx,%rax,8),%rcx
   1400046c7:	48 89 74 24 20       	mov    %rsi,0x20(%rsp)
   1400046cc:	ba 0a 00 00 00       	mov    $0xa,%edx
   1400046d1:	4d 89 e0             	mov    %r12,%r8
   1400046d4:	4d 89 e9             	mov    %r13,%r9
   1400046d7:	e8 64 92 04 00       	call   14004d940 <txrt_parse_int_scalar>
   1400046dc:	85 c0                	test   %eax,%eax
   1400046de:	0f 85 b6 02 00 00    	jne    14000499a <tx_fn_m0_bench_parse_paths_0+0x58a>
   1400046e4:	80 7c 24 2f 01       	cmpb   $0x1,0x2f(%rsp)
   1400046e9:	75 0f                	jne    1400046fa <tx_fn_m0_bench_parse_paths_0+0x2ea>
   1400046eb:	48 89 d8             	mov    %rbx,%rax
   1400046ee:	48 ff c0             	inc    %rax
   1400046f1:	0f 80 d8 02 00 00    	jo     1400049cf <tx_fn_m0_bench_parse_paths_0+0x5bf>
   1400046f7:	48 89 c3             	mov    %rax,%rbx
   1400046fa:	48 83 ef 01          	sub    $0x1,%rdi
   1400046fe:	72 26                	jb     140004726 <tx_fn_m0_bench_parse_paths_0+0x316>
   140004700:	89 e8                	mov    %ebp,%eax
   140004702:	48 69 c0 d3 4d 62 10 	imul   $0x10624dd3,%rax,%rax
   140004709:	48 c1 e8 26          	shr    $0x26,%rax
   14000470d:	69 c0 e8 03 00 00    	imul   $0x3e8,%eax,%eax
   140004713:	89 e9                	mov    %ebp,%ecx
   140004715:	29 c1                	sub    %eax,%ecx
   140004717:	89 c8                	mov    %ecx,%eax
   140004719:	ff c5                	inc    %ebp
   14000471b:	49 39 47 08          	cmp    %rax,0x8(%r15)
   14000471f:	77 9f                	ja     1400046c0 <tx_fn_m0_bench_parse_paths_0+0x2b0>
   140004721:	e9 3b 01 00 00       	jmp    140004861 <tx_fn_m0_bench_parse_paths_0+0x451>
   140004726:	48 8d 0d 82 84 0d 00 	lea    0xd8482(%rip),%rcx        # 1400dcbaf <.rdata+0x3baf>
   14000472d:	4c 8d 84 24 98 00 00 	lea    0x98(%rsp),%r8
   140004734:	00 
   140004735:	ba 0b 00 00 00       	mov    $0xb,%edx
   14000473a:	e8 a1 fc 04 00       	call   1400543e0 <txrt_str_new>
   14000473f:	85 c0                	test   %eax,%eax
   140004741:	0f 85 43 03 00 00    	jne    140004a8a <tx_fn_m0_bench_parse_paths_0+0x67a>
   140004747:	4c 8b bc 24 98 00 00 	mov    0x98(%rsp),%r15
   14000474e:	00 
   14000474f:	48 8d 05 aa 84 0d 00 	lea    0xd84aa(%rip),%rax        # 1400dcc00 <.rdata+0x3c00>
   140004756:	48 89 44 24 58       	mov    %rax,0x58(%rsp)
   14000475b:	48 c7 44 24 60 f0 00 	movq   $0xf0,0x60(%rsp)
   140004762:	00 00 
   140004764:	48 c7 44 24 68 05 00 	movq   $0x5,0x68(%rsp)
   14000476b:	00 00 
   14000476d:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
   140004772:	48 89 f1             	mov    %rsi,%rcx
   140004775:	4c 89 fa             	mov    %r15,%rdx
   140004778:	4c 8b 44 24 78       	mov    0x78(%rsp),%r8
   14000477d:	49 89 d9             	mov    %rbx,%r9
   140004780:	e8 5b ce ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140004785:	4c 89 f9             	mov    %r15,%rcx
   140004788:	e8 03 00 05 00       	call   140054790 <txrt_str_release>
   14000478d:	48 89 f1             	mov    %rsi,%rcx
   140004790:	e8 7b 68 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   140004795:	85 c0                	test   %eax,%eax
   140004797:	0f 85 fc 02 00 00    	jne    140004a99 <tx_fn_m0_bench_parse_paths_0+0x689>
   14000479d:	48 8d 8c 24 90 00 00 	lea    0x90(%rsp),%rcx
   1400047a4:	00 
   1400047a5:	e8 a6 c4 02 00       	call   140030c50 <txrt_time_monotonic_micros>
   1400047aa:	85 c0                	test   %eax,%eax
   1400047ac:	0f 85 f6 02 00 00    	jne    140004aa8 <tx_fn_m0_bench_parse_paths_0+0x698>
   1400047b2:	48 8b bc 24 90 00 00 	mov    0x90(%rsp),%rdi
   1400047b9:	00 
   1400047ba:	48 89 f1             	mov    %rsi,%rcx
   1400047bd:	e8 4e 68 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   1400047c2:	85 c0                	test   %eax,%eax
   1400047c4:	0f 85 ed 02 00 00    	jne    140004ab7 <tx_fn_m0_bench_parse_paths_0+0x6a7>
   1400047ca:	49 83 7e 08 00       	cmpq   $0x0,0x8(%r14)
   1400047cf:	0f 84 8c 00 00 00    	je     140004861 <tx_fn_m0_bench_parse_paths_0+0x451>
   1400047d5:	bd 01 00 00 00       	mov    $0x1,%ebp
   1400047da:	41 bd 9f 86 01 00    	mov    $0x1869f,%r13d
   1400047e0:	31 db                	xor    %ebx,%ebx
   1400047e2:	48 8d b4 24 d0 00 00 	lea    0xd0(%rsp),%rsi
   1400047e9:	00 
   1400047ea:	4c 8d 7c 24 2e       	lea    0x2e(%rsp),%r15
   1400047ef:	4c 8d a4 24 d8 00 00 	lea    0xd8(%rsp),%r12
   1400047f6:	00 
   1400047f7:	31 c0                	xor    %eax,%eax
   1400047f9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
   140004800:	49 8b 0e             	mov    (%r14),%rcx
   140004803:	48 8b 0c c1          	mov    (%rcx,%rax,8),%rcx
   140004807:	48 89 74 24 20       	mov    %rsi,0x20(%rsp)
   14000480c:	ba 0a 00 00 00       	mov    $0xa,%edx
   140004811:	4d 89 f8             	mov    %r15,%r8
   140004814:	4d 89 e1             	mov    %r12,%r9
   140004817:	e8 24 91 04 00       	call   14004d940 <txrt_parse_int_scalar>
   14000481c:	85 c0                	test   %eax,%eax
   14000481e:	0f 85 85 01 00 00    	jne    1400049a9 <tx_fn_m0_bench_parse_paths_0+0x599>
   140004824:	80 7c 24 2e 00       	cmpb   $0x0,0x2e(%rsp)
   140004829:	75 0f                	jne    14000483a <tx_fn_m0_bench_parse_paths_0+0x42a>
   14000482b:	48 89 d8             	mov    %rbx,%rax
   14000482e:	48 ff c0             	inc    %rax
   140004831:	0f 80 be 01 00 00    	jo     1400049f5 <tx_fn_m0_bench_parse_paths_0+0x5e5>
   140004837:	48 89 c3             	mov    %rax,%rbx
   14000483a:	49 83 ed 01          	sub    $0x1,%r13
   14000483e:	72 26                	jb     140004866 <tx_fn_m0_bench_parse_paths_0+0x456>
   140004840:	89 e8                	mov    %ebp,%eax
   140004842:	48 69 c0 d3 4d 62 10 	imul   $0x10624dd3,%rax,%rax
   140004849:	48 c1 e8 26          	shr    $0x26,%rax
   14000484d:	69 c0 e8 03 00 00    	imul   $0x3e8,%eax,%eax
   140004853:	89 e9                	mov    %ebp,%ecx
   140004855:	29 c1                	sub    %eax,%ecx
   140004857:	89 c8                	mov    %ecx,%eax
   140004859:	ff c5                	inc    %ebp
   14000485b:	49 39 46 08          	cmp    %rax,0x8(%r14)
   14000485f:	77 9f                	ja     140004800 <tx_fn_m0_bench_parse_paths_0+0x3f0>
   140004861:	e8 0a cb 03 00       	call   140041370 <txrt_vector_index_error>
   140004866:	48 8d 0d 12 85 0d 00 	lea    0xd8512(%rip),%rcx        # 1400dcd7f <.rdata+0x3d7f>
   14000486d:	4c 8d 84 24 88 00 00 	lea    0x88(%rsp),%r8
   140004874:	00 
   140004875:	ba 0d 00 00 00       	mov    $0xd,%edx
   14000487a:	e8 61 fb 04 00       	call   1400543e0 <txrt_str_new>
   14000487f:	85 c0                	test   %eax,%eax
   140004881:	0f 85 3f 02 00 00    	jne    140004ac6 <tx_fn_m0_bench_parse_paths_0+0x6b6>
   140004887:	4c 8b b4 24 88 00 00 	mov    0x88(%rsp),%r14
   14000488e:	00 
   14000488f:	48 8d 05 3a 85 0d 00 	lea    0xd853a(%rip),%rax        # 1400dcdd0 <.rdata+0x3dd0>
   140004896:	48 89 44 24 58       	mov    %rax,0x58(%rsp)
   14000489b:	48 c7 44 24 60 fc 00 	movq   $0xfc,0x60(%rsp)
   1400048a2:	00 00 
   1400048a4:	48 c7 44 24 68 05 00 	movq   $0x5,0x68(%rsp)
   1400048ab:	00 00 
   1400048ad:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
   1400048b2:	48 89 f1             	mov    %rsi,%rcx
   1400048b5:	4c 89 f2             	mov    %r14,%rdx
   1400048b8:	49 89 f8             	mov    %rdi,%r8
   1400048bb:	49 89 d9             	mov    %rbx,%r9
   1400048be:	e8 1d cd ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   1400048c3:	4c 89 f1             	mov    %r14,%rcx
   1400048c6:	e8 c5 fe 04 00       	call   140054790 <txrt_str_release>
   1400048cb:	48 89 f1             	mov    %rsi,%rcx
   1400048ce:	e8 3d 67 03 00       	call   14003b010 <txrt_gc_safepoint_context>
   1400048d3:	85 c0                	test   %eax,%eax
   1400048d5:	0f 85 03 02 00 00    	jne    140004ade <tx_fn_m0_bench_parse_paths_0+0x6ce>
   1400048db:	48 8b 4c 24 38       	mov    0x38(%rsp),%rcx
   1400048e0:	e8 9b 6f 02 00       	call   14002b880 <txrt_value_release>
   1400048e5:	48 8b 4c 24 40       	mov    0x40(%rsp),%rcx
   1400048ea:	e8 91 6f 02 00       	call   14002b880 <txrt_value_release>
   1400048ef:	48 8b 84 24 80 00 00 	mov    0x80(%rsp),%rax
   1400048f6:	00 
   1400048f7:	48 89 06             	mov    %rax,(%rsi)
   1400048fa:	48 81 c4 f8 00 00 00 	add    $0xf8,%rsp
   140004901:	5b                   	pop    %rbx
   140004902:	5d                   	pop    %rbp
   140004903:	5f                   	pop    %rdi
   140004904:	5e                   	pop    %rsi
   140004905:	41 5c                	pop    %r12
   140004907:	41 5d                	pop    %r13
   140004909:	41 5e                	pop    %r14
   14000490b:	41 5f                	pop    %r15
   14000490d:	c3                   	ret
   14000490e:	41 89 c5             	mov    %eax,%r13d
   140004911:	48 8d 15 88 7f 0d 00 	lea    0xd7f88(%rip),%rdx        # 1400dc8a0 <.rdata+0x38a0>
   140004918:	41 b8 e1 00 00 00    	mov    $0xe1,%r8d
   14000491e:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140004924:	48 89 f1             	mov    %rsi,%rcx
   140004927:	e8 24 ad 04 00       	call   14004f650 <txrt_stack_error_location>
   14000492c:	44 89 e9             	mov    %r13d,%ecx
   14000492f:	e8 9c f4 04 00       	call   140053dd0 <txrt_require_success>
   140004934:	48 8d 15 a5 7f 0d 00 	lea    0xd7fa5(%rip),%rdx        # 1400dc8e0 <.rdata+0x38e0>
   14000493b:	41 b8 e1 00 00 00    	mov    $0xe1,%r8d
   140004941:	eb 40                	jmp    140004983 <tx_fn_m0_bench_parse_paths_0+0x573>
   140004943:	48 8d 15 d6 7f 0d 00 	lea    0xd7fd6(%rip),%rdx        # 1400dc920 <.rdata+0x3920>
   14000494a:	eb 10                	jmp    14000495c <tx_fn_m0_bench_parse_paths_0+0x54c>
   14000494c:	48 8d 15 0d 80 0d 00 	lea    0xd800d(%rip),%rdx        # 1400dc960 <.rdata+0x3960>
   140004953:	eb 07                	jmp    14000495c <tx_fn_m0_bench_parse_paths_0+0x54c>
   140004955:	48 8d 15 44 80 0d 00 	lea    0xd8044(%rip),%rdx        # 1400dc9a0 <.rdata+0x39a0>
   14000495c:	41 b8 e2 00 00 00    	mov    $0xe2,%r8d
   140004962:	eb 1f                	jmp    140004983 <tx_fn_m0_bench_parse_paths_0+0x573>
   140004964:	48 8d 15 85 80 0d 00 	lea    0xd8085(%rip),%rdx        # 1400dc9f0 <.rdata+0x39f0>
   14000496b:	eb 10                	jmp    14000497d <tx_fn_m0_bench_parse_paths_0+0x56d>
   14000496d:	48 8d 15 bc 80 0d 00 	lea    0xd80bc(%rip),%rdx        # 1400dca30 <.rdata+0x3a30>
   140004974:	eb 07                	jmp    14000497d <tx_fn_m0_bench_parse_paths_0+0x56d>
   140004976:	48 8d 15 f3 80 0d 00 	lea    0xd80f3(%rip),%rdx        # 1400dca70 <.rdata+0x3a70>
   14000497d:	41 b8 e3 00 00 00    	mov    $0xe3,%r8d
   140004983:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140004989:	48 89 f1             	mov    %rsi,%rcx
   14000498c:	89 c6                	mov    %eax,%esi
   14000498e:	e8 bd ac 04 00       	call   14004f650 <txrt_stack_error_location>
   140004993:	89 f1                	mov    %esi,%ecx
   140004995:	e8 36 f4 04 00       	call   140053dd0 <txrt_require_success>
   14000499a:	48 8d 15 8f 81 0d 00 	lea    0xd818f(%rip),%rdx        # 1400dcb30 <.rdata+0x3b30>
   1400049a1:	41 b8 ea 00 00 00    	mov    $0xea,%r8d
   1400049a7:	eb 0d                	jmp    1400049b6 <tx_fn_m0_bench_parse_paths_0+0x5a6>
   1400049a9:	48 8d 15 50 83 0d 00 	lea    0xd8350(%rip),%rdx        # 1400dcd00 <.rdata+0x3d00>
   1400049b0:	41 b8 f6 00 00 00    	mov    $0xf6,%r8d
   1400049b6:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400049bc:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
   1400049c1:	89 c6                	mov    %eax,%esi
   1400049c3:	e8 88 ac 04 00       	call   14004f650 <txrt_stack_error_location>
   1400049c8:	89 f1                	mov    %esi,%ecx
   1400049ca:	e8 01 f4 04 00       	call   140053dd0 <txrt_require_success>
   1400049cf:	4c 8d 84 24 e0 00 00 	lea    0xe0(%rsp),%r8
   1400049d6:	00 
   1400049d7:	ba 01 00 00 00       	mov    $0x1,%edx
   1400049dc:	48 89 d9             	mov    %rbx,%rcx
   1400049df:	e8 ec 04 05 00       	call   140054ed0 <txrt_add_i64>
   1400049e4:	89 c7                	mov    %eax,%edi
   1400049e6:	48 8d 15 83 81 0d 00 	lea    0xd8183(%rip),%rdx        # 1400dcb70 <.rdata+0x3b70>
   1400049ed:	41 b8 ed 00 00 00    	mov    $0xed,%r8d
   1400049f3:	eb 24                	jmp    140004a19 <tx_fn_m0_bench_parse_paths_0+0x609>
   1400049f5:	4c 8d 84 24 c8 00 00 	lea    0xc8(%rsp),%r8
   1400049fc:	00 
   1400049fd:	ba 01 00 00 00       	mov    $0x1,%edx
   140004a02:	48 89 d9             	mov    %rbx,%rcx
   140004a05:	e8 c6 04 05 00       	call   140054ed0 <txrt_add_i64>
   140004a0a:	89 c7                	mov    %eax,%edi
   140004a0c:	48 8d 15 2d 83 0d 00 	lea    0xd832d(%rip),%rdx        # 1400dcd40 <.rdata+0x3d40>
   140004a13:	41 b8 f9 00 00 00    	mov    $0xf9,%r8d
   140004a19:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140004a1f:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
   140004a24:	e8 27 ac 04 00       	call   14004f650 <txrt_stack_error_location>
   140004a29:	89 f9                	mov    %edi,%ecx
   140004a2b:	e8 a0 f3 04 00       	call   140053dd0 <txrt_require_success>
   140004a30:	48 8d 15 29 7d 0d 00 	lea    0xd7d29(%rip),%rdx        # 1400dc760 <.rdata+0x3760>
   140004a37:	eb 07                	jmp    140004a40 <tx_fn_m0_bench_parse_paths_0+0x630>
   140004a39:	48 8d 15 60 7d 0d 00 	lea    0xd7d60(%rip),%rdx        # 1400dc7a0 <.rdata+0x37a0>
   140004a40:	41 b8 dd 00 00 00    	mov    $0xdd,%r8d
   140004a46:	e9 a0 00 00 00       	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004a4b:	48 8d 15 8e 7d 0d 00 	lea    0xd7d8e(%rip),%rdx        # 1400dc7e0 <.rdata+0x37e0>
   140004a52:	41 b8 de 00 00 00    	mov    $0xde,%r8d
   140004a58:	e9 8e 00 00 00       	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004a5d:	48 8d 15 bc 7d 0d 00 	lea    0xd7dbc(%rip),%rdx        # 1400dc820 <.rdata+0x3820>
   140004a64:	41 b8 de 00 00 00    	mov    $0xde,%r8d
   140004a6a:	eb 7f                	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004a6c:	48 8d 15 3d 80 0d 00 	lea    0xd803d(%rip),%rdx        # 1400dcab0 <.rdata+0x3ab0>
   140004a73:	41 b8 e7 00 00 00    	mov    $0xe7,%r8d
   140004a79:	eb 70                	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004a7b:	48 8d 15 6e 80 0d 00 	lea    0xd806e(%rip),%rdx        # 1400dcaf0 <.rdata+0x3af0>
   140004a82:	41 b8 e7 00 00 00    	mov    $0xe7,%r8d
   140004a88:	eb 49                	jmp    140004ad3 <tx_fn_m0_bench_parse_paths_0+0x6c3>
   140004a8a:	48 8d 15 2f 81 0d 00 	lea    0xd812f(%rip),%rdx        # 1400dcbc0 <.rdata+0x3bc0>
   140004a91:	41 b8 f0 00 00 00    	mov    $0xf0,%r8d
   140004a97:	eb 3a                	jmp    140004ad3 <tx_fn_m0_bench_parse_paths_0+0x6c3>
   140004a99:	48 8d 15 a0 81 0d 00 	lea    0xd81a0(%rip),%rdx        # 1400dcc40 <.rdata+0x3c40>
   140004aa0:	41 b8 f0 00 00 00    	mov    $0xf0,%r8d
   140004aa6:	eb 43                	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004aa8:	48 8d 15 d1 81 0d 00 	lea    0xd81d1(%rip),%rdx        # 1400dcc80 <.rdata+0x3c80>
   140004aaf:	41 b8 f3 00 00 00    	mov    $0xf3,%r8d
   140004ab5:	eb 34                	jmp    140004aeb <tx_fn_m0_bench_parse_paths_0+0x6db>
   140004ab7:	48 8d 15 02 82 0d 00 	lea    0xd8202(%rip),%rdx        # 1400dccc0 <.rdata+0x3cc0>
   140004abe:	41 b8 f3 00 00 00    	mov    $0xf3,%r8d
   140004ac4:	eb 0d                	jmp    140004ad3 <tx_fn_m0_bench_parse_paths_0+0x6c3>
   140004ac6:	48 8d 15 c3 82 0d 00 	lea    0xd82c3(%rip),%rdx        # 1400dcd90 <.rdata+0x3d90>
   140004acd:	41 b8 fc 00 00 00    	mov    $0xfc,%r8d
   140004ad3:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140004ad9:	e9 de fe ff ff       	jmp    1400049bc <tx_fn_m0_bench_parse_paths_0+0x5ac>
   140004ade:	48 8d 15 2b 83 0d 00 	lea    0xd832b(%rip),%rdx        # 1400dce10 <.rdata+0x3e10>
   140004ae5:	41 b8 fc 00 00 00    	mov    $0xfc,%r8d
   140004aeb:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140004af1:	e9 93 fe ff ff       	jmp    140004989 <tx_fn_m0_bench_parse_paths_0+0x579>
   140004af6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
   140004afd:	00 00 00 


E:\Project\other\Compilation\tx_build\performance_remaining\baseline\diverse.exe:     file format pei-x86-64


Disassembly of section .text:

000000014004d940 <txrt_parse_int_scalar>:
   14004d940:	57                   	push   %rdi
   14004d941:	56                   	push   %rsi
   14004d942:	53                   	push   %rbx
   14004d943:	48 83 ec 40          	sub    $0x40,%rsp
   14004d947:	4c 89 cb             	mov    %r9,%rbx
   14004d94a:	48 89 d7             	mov    %rdx,%rdi
   14004d94d:	4c 89 c6             	mov    %r8,%rsi
   14004d950:	e8 1b dd ff ff       	call   14004b670 <_ZN12tx_generated6detail10text_valueB5cxx11EPKv>
   14004d955:	48 8d 4c 24 30       	lea    0x30(%rsp),%rcx
   14004d95a:	49 89 f8             	mov    %rdi,%r8
   14004d95d:	48 8b 50 08          	mov    0x8(%rax),%rdx
   14004d961:	48 8b 00             	mov    (%rax),%rax
   14004d964:	48 89 54 24 20       	mov    %rdx,0x20(%rsp)
   14004d969:	48 8d 54 24 20       	lea    0x20(%rsp),%rdx
   14004d96e:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   14004d973:	e8 a8 db 02 00       	call   14007b520 <_ZN12tx_generated16parse_int_scalarESt17basic_string_viewIcSt11char_traitsIcEEx>
   14004d978:	48 8b 44 24 38       	mov    0x38(%rsp),%rax
   14004d97d:	48 8b 54 24 30       	mov    0x30(%rsp),%rdx
   14004d982:	48 8b 0d a7 89 09 00 	mov    0x989a7(%rip),%rcx        # 1400e6330 <.refptr.__emutls_v._ZN12tx_generated6detail14thread_contextE>
   14004d989:	48 85 c0             	test   %rax,%rax
   14004d98c:	0f 94 06             	sete   (%rsi)
   14004d98f:	48 89 13             	mov    %rdx,(%rbx)
   14004d992:	48 8b 94 24 80 00 00 	mov    0x80(%rsp),%rdx
   14004d999:	00 
   14004d99a:	48 89 02             	mov    %rax,(%rdx)
   14004d99d:	e8 1e 5c 06 00       	call   1400b35c0 <__emutls_get_address>
   14004d9a2:	48 8b 00             	mov    (%rax),%rax
   14004d9a5:	48 85 c0             	test   %rax,%rax
   14004d9a8:	74 0e                	je     14004d9b8 <txrt_parse_int_scalar+0x78>
   14004d9aa:	8b 40 08             	mov    0x8(%rax),%eax
   14004d9ad:	48 83 c4 40          	add    $0x40,%rsp
   14004d9b1:	5b                   	pop    %rbx
   14004d9b2:	5e                   	pop    %rsi
   14004d9b3:	5f                   	pop    %rdi
   14004d9b4:	c3                   	ret
   14004d9b5:	0f 1f 00             	nopl   (%rax)
   14004d9b8:	e8 33 13 00 00       	call   14004ecf0 <_ZN12tx_generated6detail26initialize_runtime_contextEv>
   14004d9bd:	8b 40 08             	mov    0x8(%rax),%eax
   14004d9c0:	48 83 c4 40          	add    $0x40,%rsp
   14004d9c4:	5b                   	pop    %rbx
   14004d9c5:	5e                   	pop    %rsi
   14004d9c6:	5f                   	pop    %rdi
   14004d9c7:	c3                   	ret
   14004d9c8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
   14004d9cf:	00 
