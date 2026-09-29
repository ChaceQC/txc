
E:\Project\other\Compilation\tx_build\performance_remaining\candidate\borrowing.exe:     file format pei-x86-64


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
   140001a34:	e8 57 a1 01 00       	call   14001bb90 <txrt_map_new_i64_i64>
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
   140001a64:	e8 d7 f6 01 00       	call   140021140 <txrt_map_set_i64_i64>
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
   140001ab5:	e8 c6 b3 01 00       	call   14001ce80 <txrt_map_read_i64_i64>
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
   140001aee:	e8 8d b3 01 00       	call   14001ce80 <txrt_map_read_i64_i64>
   140001af3:	48 89 d9             	mov    %rbx,%rcx
   140001af6:	85 c0                	test   %eax,%eax
   140001af8:	74 d6                	je     140001ad0 <tx_fn_m0_bench_map_0+0xf0>
   140001afa:	89 c7                	mov    %eax,%edi
   140001afc:	48 8d 15 6d 0b 0a 00 	lea    0xa0b6d(%rip),%rdx        # 1400a2670 <.rdata+0x670>
   140001b03:	e9 e3 00 00 00       	jmp    140001beb <tx_fn_m0_bench_map_0+0x20b>
   140001b08:	48 8d 0d db 0b 0a 00 	lea    0xa0bdb(%rip),%rcx        # 1400a26ea <.rdata+0x6ea>
   140001b0f:	4c 8d 44 24 30       	lea    0x30(%rsp),%r8
   140001b14:	ba 03 00 00 00       	mov    $0x3,%edx
   140001b19:	e8 82 a3 02 00       	call   14002bea0 <txrt_str_new>
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
   140001b53:	e8 18 a9 02 00       	call   14002c470 <txrt_print_str>
   140001b58:	85 c0                	test   %eax,%eax
   140001b5a:	0f 85 49 01 00 00    	jne    140001ca9 <tx_fn_m0_bench_map_0+0x2c9>
   140001b60:	b1 20                	mov    $0x20,%cl
   140001b62:	e8 a9 bf 02 00       	call   14002db10 <txrt_print_char>
   140001b67:	85 c0                	test   %eax,%eax
   140001b69:	0f 85 43 01 00 00    	jne    140001cb2 <tx_fn_m0_bench_map_0+0x2d2>
   140001b6f:	4c 89 e1             	mov    %r12,%rcx
   140001b72:	31 d2                	xor    %edx,%edx
   140001b74:	e8 17 9e 02 00       	call   14002b990 <txrt_print_i64>
   140001b79:	85 c0                	test   %eax,%eax
   140001b7b:	0f 85 3a 01 00 00    	jne    140001cbb <tx_fn_m0_bench_map_0+0x2db>
   140001b81:	b1 20                	mov    $0x20,%cl
   140001b83:	e8 88 bf 02 00       	call   14002db10 <txrt_print_char>
   140001b88:	85 c0                	test   %eax,%eax
   140001b8a:	0f 85 34 01 00 00    	jne    140001cc4 <tx_fn_m0_bench_map_0+0x2e4>
   140001b90:	48 89 d9             	mov    %rbx,%rcx
   140001b93:	b2 01                	mov    $0x1,%dl
   140001b95:	e8 f6 9d 02 00       	call   14002b990 <txrt_print_i64>
   140001b9a:	85 c0                	test   %eax,%eax
   140001b9c:	0f 85 2b 01 00 00    	jne    140001ccd <tx_fn_m0_bench_map_0+0x2ed>
   140001ba2:	4c 89 f9             	mov    %r15,%rcx
   140001ba5:	e8 a6 a6 02 00       	call   14002c250 <txrt_str_release>
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
   140001bdd:	e8 ae ad 02 00       	call   14002c990 <txrt_add_i64>
   140001be2:	89 c7                	mov    %eax,%edi
   140001be4:	48 8d 15 c5 0a 0a 00 	lea    0xa0ac5(%rip),%rdx        # 1400a26b0 <.rdata+0x6b0>
   140001beb:	41 b8 1c 00 00 00    	mov    $0x1c,%r8d
   140001bf1:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001bf7:	48 89 f1             	mov    %rsi,%rcx
   140001bfa:	e8 11 55 02 00       	call   140027110 <txrt_stack_error_location>
   140001bff:	89 f9                	mov    %edi,%ecx
   140001c01:	e8 8a 9c 02 00       	call   14002b890 <txrt_require_success>
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
   140001c80:	e8 eb ad 02 00       	call   14002ca70 <txrt_sub_i64>
   140001c85:	89 c7                	mov    %eax,%edi
   140001c87:	48 8d 15 e2 0a 0a 00 	lea    0xa0ae2(%rip),%rdx        # 1400a2770 <.rdata+0x770>
   140001c8e:	41 b8 1e 00 00 00    	mov    $0x1e,%r8d
   140001c94:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001c9a:	48 89 f1             	mov    %rsi,%rcx
   140001c9d:	e8 6e 54 02 00       	call   140027110 <txrt_stack_error_location>
   140001ca2:	89 f9                	mov    %edi,%ecx
   140001ca4:	e8 e7 9b 02 00       	call   14002b890 <txrt_require_success>
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
   140001cee:	e8 1d 54 02 00       	call   140027110 <txrt_stack_error_location>
   140001cf3:	89 f1                	mov    %esi,%ecx
   140001cf5:	e8 96 9b 02 00       	call   14002b890 <txrt_require_success>
   140001cfa:	cc                   	int3
   140001cfb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_remaining\candidate\borrowing.exe:     file format pei-x86-64


Disassembly of section .text:

000000014001ce80 <txrt_map_read_i64_i64>:
   14001ce80:	57                   	push   %rdi
   14001ce81:	56                   	push   %rsi
   14001ce82:	53                   	push   %rbx
   14001ce83:	48 83 ec 20          	sub    $0x20,%rsp
   14001ce87:	48 89 d3             	mov    %rdx,%rbx
   14001ce8a:	4c 89 c6             	mov    %r8,%rsi
   14001ce8d:	e8 ae eb 07 00       	call   14009ba40 <_ZSt12__any_casterISt10shared_ptrIN12tx_generated17container_storageEEEPvPKSt3any>
   14001ce92:	48 85 c0             	test   %rax,%rax
   14001ce95:	0f 84 39 01 00 00    	je     14001cfd4 <txrt_map_read_i64_i64+0x154>
   14001ce9b:	48 8b 08             	mov    (%rax),%rcx
   14001ce9e:	48 8b 41 28          	mov    0x28(%rcx),%rax
   14001cea2:	48 83 f8 01          	cmp    $0x1,%rax
   14001cea6:	0f 84 94 00 00 00    	je     14001cf40 <txrt_map_read_i64_i64+0xc0>
   14001ceac:	48 85 c0             	test   %rax,%rax
   14001ceaf:	74 6f                	je     14001cf20 <txrt_map_read_i64_i64+0xa0>
   14001ceb1:	4c 8b 51 18          	mov    0x18(%rcx),%r10
   14001ceb5:	48 89 d8             	mov    %rbx,%rax
   14001ceb8:	31 d2                	xor    %edx,%edx
   14001ceba:	49 f7 f2             	div    %r10
   14001cebd:	48 8b 41 10          	mov    0x10(%rcx),%rax
   14001cec1:	48 8b 0c d0          	mov    (%rax,%rdx,8),%rcx
   14001cec5:	49 89 d3             	mov    %rdx,%r11
   14001cec8:	48 85 c9             	test   %rcx,%rcx
   14001cecb:	74 7d                	je     14001cf4a <txrt_map_read_i64_i64+0xca>
   14001cecd:	48 8b 01             	mov    (%rcx),%rax
   14001ced0:	4c 8b 40 08          	mov    0x8(%rax),%r8
   14001ced4:	4c 39 c3             	cmp    %r8,%rbx
   14001ced7:	74 24                	je     14001cefd <txrt_map_read_i64_i64+0x7d>
   14001ced9:	4c 8b 08             	mov    (%rax),%r9
   14001cedc:	4d 85 c9             	test   %r9,%r9
   14001cedf:	74 69                	je     14001cf4a <txrt_map_read_i64_i64+0xca>
   14001cee1:	4d 8b 41 08          	mov    0x8(%r9),%r8
   14001cee5:	48 89 c1             	mov    %rax,%rcx
   14001cee8:	31 d2                	xor    %edx,%edx
   14001ceea:	4c 89 c0             	mov    %r8,%rax
   14001ceed:	49 f7 f2             	div    %r10
   14001cef0:	49 39 d3             	cmp    %rdx,%r11
   14001cef3:	75 55                	jne    14001cf4a <txrt_map_read_i64_i64+0xca>
   14001cef5:	4c 89 c8             	mov    %r9,%rax
   14001cef8:	4c 39 c3             	cmp    %r8,%rbx
   14001cefb:	75 dc                	jne    14001ced9 <txrt_map_read_i64_i64+0x59>
   14001cefd:	48 8b 09             	mov    (%rcx),%rcx
   14001cf00:	48 85 c9             	test   %rcx,%rcx
   14001cf03:	74 45                	je     14001cf4a <txrt_map_read_i64_i64+0xca>
   14001cf05:	48 8b 41 10          	mov    0x10(%rcx),%rax
   14001cf09:	48 89 06             	mov    %rax,(%rsi)
   14001cf0c:	31 c0                	xor    %eax,%eax
   14001cf0e:	48 83 c4 20          	add    $0x20,%rsp
   14001cf12:	5b                   	pop    %rbx
   14001cf13:	5e                   	pop    %rsi
   14001cf14:	5f                   	pop    %rdi
   14001cf15:	c3                   	ret
   14001cf16:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
   14001cf1d:	00 00 00 
   14001cf20:	48 8b 49 20          	mov    0x20(%rcx),%rcx
   14001cf24:	48 85 c9             	test   %rcx,%rcx
   14001cf27:	75 0f                	jne    14001cf38 <txrt_map_read_i64_i64+0xb8>
   14001cf29:	eb 1f                	jmp    14001cf4a <txrt_map_read_i64_i64+0xca>
   14001cf2b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
   14001cf30:	48 8b 09             	mov    (%rcx),%rcx
   14001cf33:	48 85 c9             	test   %rcx,%rcx
   14001cf36:	74 12                	je     14001cf4a <txrt_map_read_i64_i64+0xca>
   14001cf38:	48 3b 59 08          	cmp    0x8(%rcx),%rbx
   14001cf3c:	75 f2                	jne    14001cf30 <txrt_map_read_i64_i64+0xb0>
   14001cf3e:	eb c0                	jmp    14001cf00 <txrt_map_read_i64_i64+0x80>
   14001cf40:	48 8b 49 20          	mov    0x20(%rcx),%rcx
   14001cf44:	48 3b 59 08          	cmp    0x8(%rcx),%rbx
   14001cf48:	74 bb                	je     14001cf05 <txrt_map_read_i64_i64+0x85>
   14001cf4a:	b9 10 00 00 00       	mov    $0x10,%ecx
   14001cf4f:	e8 d4 82 05 00       	call   140075228 <__cxa_allocate_exception>
   14001cf54:	48 8d 15 b6 79 08 00 	lea    0x879b6(%rip),%rdx        # 1400a4911 <.rdata+0x151>
   14001cf5b:	48 89 c1             	mov    %rax,%rcx
   14001cf5e:	48 89 c7             	mov    %rax,%rdi
   14001cf61:	e8 0a 84 05 00       	call   140075370 <_ZNSt12out_of_rangeC1EPKc>
   14001cf66:	4c 8d 05 fb 83 05 00 	lea    0x583fb(%rip),%r8        # 140075368 <_ZNSt12out_of_rangeD1Ev>
   14001cf6d:	48 8d 15 bc d0 08 00 	lea    0x8d0bc(%rip),%rdx        # 1400aa030 <_ZTISt12out_of_range>
   14001cf74:	48 89 f9             	mov    %rdi,%rcx
   14001cf77:	e8 6c 82 05 00       	call   1400751e8 <__cxa_throw>
   14001cf7c:	48 89 c3             	mov    %rax,%rbx
   14001cf7f:	48 89 d6             	mov    %rdx,%rsi
   14001cf82:	48 89 f9             	mov    %rdi,%rcx
   14001cf85:	e8 86 82 05 00       	call   140075210 <__cxa_free_exception>
   14001cf8a:	48 89 d9             	mov    %rbx,%rcx
   14001cf8d:	48 89 f0             	mov    %rsi,%rax
   14001cf90:	48 83 f8 03          	cmp    $0x3,%rax
   14001cf94:	74 43                	je     14001cfd9 <txrt_map_read_i64_i64+0x159>
   14001cf96:	7f 10                	jg     14001cfa8 <txrt_map_read_i64_i64+0x128>
   14001cf98:	48 83 f8 01          	cmp    $0x1,%rax
   14001cf9c:	74 6c                	je     14001d00a <txrt_map_read_i64_i64+0x18a>
   14001cf9e:	48 83 f8 02          	cmp    $0x2,%rax
   14001cfa2:	0f 84 89 00 00 00    	je     14001d031 <txrt_map_read_i64_i64+0x1b1>
   14001cfa8:	e8 73 82 05 00       	call   140075220 <__cxa_begin_catch>
   14001cfad:	4c 8d 05 0b 79 08 00 	lea    0x8790b(%rip),%r8        # 1400a48bf <.rdata+0xff>
   14001cfb4:	b9 01 00 00 00       	mov    $0x1,%ecx
   14001cfb9:	48 8d 15 15 79 08 00 	lea    0x87915(%rip),%rdx        # 1400a48d5 <.rdata+0x115>
   14001cfc0:	e8 7b 9c 00 00       	call   140026c40 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001cfc5:	e8 4e 82 05 00       	call   140075218 <__cxa_end_catch>
   14001cfca:	eb 34                	jmp    14001d000 <txrt_map_read_i64_i64+0x180>
   14001cfcc:	48 89 c1             	mov    %rax,%rcx
   14001cfcf:	48 89 d0             	mov    %rdx,%rax
   14001cfd2:	eb bc                	jmp    14001cf90 <txrt_map_read_i64_i64+0x110>
   14001cfd4:	e8 47 ef 07 00       	call   14009bf20 <_ZSt20__throw_bad_any_castv>
   14001cfd9:	e8 42 82 05 00       	call   140075220 <__cxa_begin_catch>
   14001cfde:	48 89 c1             	mov    %rax,%rcx
   14001cfe1:	48 8b 00             	mov    (%rax),%rax
   14001cfe4:	ff 50 10             	call   *0x10(%rax)
   14001cfe7:	48 8d 15 c0 78 08 00 	lea    0x878c0(%rip),%rdx        # 1400a48ae <.rdata+0xee>
   14001cfee:	b9 01 00 00 00       	mov    $0x1,%ecx
   14001cff3:	49 89 c0             	mov    %rax,%r8
   14001cff6:	e8 45 9c 00 00       	call   140026c40 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001cffb:	e8 18 82 05 00       	call   140075218 <__cxa_end_catch>
   14001d000:	b8 01 00 00 00       	mov    $0x1,%eax
   14001d005:	e9 04 ff ff ff       	jmp    14001cf0e <txrt_map_read_i64_i64+0x8e>
   14001d00a:	e8 11 82 05 00       	call   140075220 <__cxa_begin_catch>
   14001d00f:	48 89 c3             	mov    %rax,%rbx
   14001d012:	48 8b 00             	mov    (%rax),%rax
   14001d015:	48 89 d9             	mov    %rbx,%rcx
   14001d018:	ff 50 10             	call   *0x10(%rax)
   14001d01b:	48 8b 53 18          	mov    0x18(%rbx),%rdx
   14001d01f:	8b 4b 10             	mov    0x10(%rbx),%ecx
   14001d022:	49 89 c0             	mov    %rax,%r8
   14001d025:	e8 16 9c 00 00       	call   140026c40 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001d02a:	e8 e9 81 05 00       	call   140075218 <__cxa_end_catch>
   14001d02f:	eb cf                	jmp    14001d000 <txrt_map_read_i64_i64+0x180>
   14001d031:	e8 ea 81 05 00       	call   140075220 <__cxa_begin_catch>
   14001d036:	48 89 c1             	mov    %rax,%rcx
   14001d039:	48 8b 00             	mov    (%rax),%rax
   14001d03c:	ff 50 10             	call   *0x10(%rax)
   14001d03f:	48 8d 15 56 78 08 00 	lea    0x87856(%rip),%rdx        # 1400a489c <.rdata+0xdc>
   14001d046:	b9 01 00 00 00       	mov    $0x1,%ecx
   14001d04b:	49 89 c0             	mov    %rax,%r8
   14001d04e:	e8 ed 9b 00 00       	call   140026c40 <_ZN12tx_generated6detail9set_errorEN2tx10error_kindEPKcS4_>
   14001d053:	e8 c0 81 05 00       	call   140075218 <__cxa_end_catch>
   14001d058:	eb a6                	jmp    14001d000 <txrt_map_read_i64_i64+0x180>
   14001d05a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_remaining\candidate\diverse.exe:     file format pei-x86-64


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
   14000441c:	48 81 ec 08 01 00 00 	sub    $0x108,%rsp
   140004423:	48 89 ce             	mov    %rcx,%rsi
   140004426:	4c 8b 31             	mov    (%rcx),%r14
   140004429:	48 8d 05 20 8a 0d 00 	lea    0xd8a20(%rip),%rax        # 1400dce50 <.rdata+0x3e50>
   140004430:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   140004435:	48 8d 05 34 8a 0d 00 	lea    0xd8a34(%rip),%rax        # 1400dce70 <.rdata+0x3e70>
   14000443c:	48 89 44 24 68       	mov    %rax,0x68(%rsp)
   140004441:	48 c7 44 24 70 db 00 	movq   $0xdb,0x70(%rsp)
   140004448:	00 00 
   14000444a:	48 c7 44 24 78 01 00 	movq   $0x1,0x78(%rsp)
   140004451:	00 00 
   140004453:	4c 89 b4 24 80 00 00 	mov    %r14,0x80(%rsp)
   14000445a:	00 
   14000445b:	48 8d 44 24 60       	lea    0x60(%rsp),%rax
   140004460:	48 89 01             	mov    %rax,(%rcx)
   140004463:	4c 8d 84 24 d0 00 00 	lea    0xd0(%rsp),%r8
   14000446a:	00 
   14000446b:	31 c9                	xor    %ecx,%ecx
   14000446d:	31 d2                	xor    %edx,%edx
   14000446f:	e8 8c 39 04 00       	call   140047e00 <txrt_vector_new_str>
   140004474:	85 c0                	test   %eax,%eax
   140004476:	0f 85 c9 05 00 00    	jne    140004a45 <tx_fn_m0_bench_parse_paths_0+0x635>
   14000447c:	48 8b 8c 24 d0 00 00 	mov    0xd0(%rsp),%rcx
   140004483:	00 
   140004484:	48 89 4c 24 50       	mov    %rcx,0x50(%rsp)
   140004489:	e8 92 35 04 00       	call   140047a20 <txrt_vector_ref_str>
   14000448e:	49 89 c7             	mov    %rax,%r15
   140004491:	48 89 f1             	mov    %rsi,%rcx
   140004494:	e8 87 6b 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   140004499:	85 c0                	test   %eax,%eax
   14000449b:	0f 85 ad 05 00 00    	jne    140004a4e <tx_fn_m0_bench_parse_paths_0+0x63e>
   1400044a1:	4c 8d 84 24 c8 00 00 	lea    0xc8(%rsp),%r8
   1400044a8:	00 
   1400044a9:	31 c9                	xor    %ecx,%ecx
   1400044ab:	31 d2                	xor    %edx,%edx
   1400044ad:	e8 4e 39 04 00       	call   140047e00 <txrt_vector_new_str>
   1400044b2:	85 c0                	test   %eax,%eax
   1400044b4:	0f 85 a6 05 00 00    	jne    140004a60 <tx_fn_m0_bench_parse_paths_0+0x650>
   1400044ba:	48 8b 8c 24 c8 00 00 	mov    0xc8(%rsp),%rcx
   1400044c1:	00 
   1400044c2:	48 89 4c 24 48       	mov    %rcx,0x48(%rsp)
   1400044c7:	e8 54 35 04 00       	call   140047a20 <txrt_vector_ref_str>
   1400044cc:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   1400044d1:	48 89 f1             	mov    %rsi,%rcx
   1400044d4:	e8 47 6b 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   1400044d9:	85 c0                	test   %eax,%eax
   1400044db:	0f 85 8e 05 00 00    	jne    140004a6f <tx_fn_m0_bench_parse_paths_0+0x65f>
   1400044e1:	4c 8d 05 77 83 0d 00 	lea    0xd8377(%rip),%r8        # 1400dc85f <.rdata+0x385f>
   1400044e8:	48 8d 4c 24 58       	lea    0x58(%rsp),%rcx
   1400044ed:	31 d2                	xor    %edx,%edx
   1400044ef:	45 31 c9             	xor    %r9d,%r9d
   1400044f2:	e8 89 ce 02 00       	call   140031380 <txrt_format_begin>
   1400044f7:	85 c0                	test   %eax,%eax
   1400044f9:	0f 85 26 01 00 00    	jne    140004625 <tx_fn_m0_bench_parse_paths_0+0x215>
   1400044ff:	48 8d 2d d9 84 0d 00 	lea    0xd84d9(%rip),%rbp        # 1400dc9df <.rdata+0x39df>
   140004506:	48 8d 5c 24 58       	lea    0x58(%rsp),%rbx
   14000450b:	45 31 e4             	xor    %r12d,%r12d
   14000450e:	66 90                	xchg   %ax,%ax
   140004510:	48 8b 7c 24 58       	mov    0x58(%rsp),%rdi
   140004515:	48 89 f9             	mov    %rdi,%rcx
   140004518:	4c 89 e2             	mov    %r12,%rdx
   14000451b:	4c 8d 05 7d 83 0d 00 	lea    0xd837d(%rip),%r8        # 1400dc89f <.rdata+0x389f>
   140004522:	45 31 c9             	xor    %r9d,%r9d
   140004525:	e8 16 dd 02 00       	call   140032240 <txrt_format_plain_i64>
   14000452a:	85 c0                	test   %eax,%eax
   14000452c:	0f 85 f2 03 00 00    	jne    140004924 <tx_fn_m0_bench_parse_paths_0+0x514>
   140004532:	48 89 f9             	mov    %rdi,%rcx
   140004535:	e8 f6 e6 02 00       	call   140032c30 <txrt_format_finish>
   14000453a:	48 89 f1             	mov    %rsi,%rcx
   14000453d:	e8 de 6a 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   140004542:	85 c0                	test   %eax,%eax
   140004544:	0f 85 00 04 00 00    	jne    14000494a <tx_fn_m0_bench_parse_paths_0+0x53a>
   14000454a:	48 89 f9             	mov    %rdi,%rcx
   14000454d:	48 8d 94 24 c0 00 00 	lea    0xc0(%rsp),%rdx
   140004554:	00 
   140004555:	e8 26 02 05 00       	call   140054780 <txrt_str_clone>
   14000455a:	85 c0                	test   %eax,%eax
   14000455c:	0f 85 00 04 00 00    	jne    140004962 <tx_fn_m0_bench_parse_paths_0+0x552>
   140004562:	4c 8b ac 24 c0 00 00 	mov    0xc0(%rsp),%r13
   140004569:	00 
   14000456a:	48 8b 4c 24 50       	mov    0x50(%rsp),%rcx
   14000456f:	4c 89 ea             	mov    %r13,%rdx
   140004572:	e8 69 3d 04 00       	call   1400482e0 <txrt_vector_push_back_str>
   140004577:	85 c0                	test   %eax,%eax
   140004579:	0f 85 ec 03 00 00    	jne    14000496b <tx_fn_m0_bench_parse_paths_0+0x55b>
   14000457f:	4c 89 e9             	mov    %r13,%rcx
   140004582:	e8 39 03 05 00       	call   1400548c0 <txrt_str_release>
   140004587:	48 89 f1             	mov    %rsi,%rcx
   14000458a:	e8 91 6a 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   14000458f:	85 c0                	test   %eax,%eax
   140004591:	0f 85 dd 03 00 00    	jne    140004974 <tx_fn_m0_bench_parse_paths_0+0x564>
   140004597:	48 8d 84 24 b8 00 00 	lea    0xb8(%rsp),%rax
   14000459e:	00 
   14000459f:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   1400045a4:	41 b8 01 00 00 00    	mov    $0x1,%r8d
   1400045aa:	48 89 f9             	mov    %rdi,%rcx
   1400045ad:	48 89 ea             	mov    %rbp,%rdx
   1400045b0:	45 31 c9             	xor    %r9d,%r9d
   1400045b3:	e8 18 68 04 00       	call   14004add0 <txrt_str_concat_literal>
   1400045b8:	85 c0                	test   %eax,%eax
   1400045ba:	0f 85 cc 03 00 00    	jne    14000498c <tx_fn_m0_bench_parse_paths_0+0x57c>
   1400045c0:	4c 8b ac 24 b8 00 00 	mov    0xb8(%rsp),%r13
   1400045c7:	00 
   1400045c8:	48 8b 4c 24 48       	mov    0x48(%rsp),%rcx
   1400045cd:	4c 89 ea             	mov    %r13,%rdx
   1400045d0:	e8 0b 3d 04 00       	call   1400482e0 <txrt_vector_push_back_str>
   1400045d5:	85 c0                	test   %eax,%eax
   1400045d7:	0f 85 b8 03 00 00    	jne    140004995 <tx_fn_m0_bench_parse_paths_0+0x585>
   1400045dd:	4c 89 e9             	mov    %r13,%rcx
   1400045e0:	e8 db 02 05 00       	call   1400548c0 <txrt_str_release>
   1400045e5:	48 89 f1             	mov    %rsi,%rcx
   1400045e8:	e8 33 6a 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   1400045ed:	85 c0                	test   %eax,%eax
   1400045ef:	0f 85 a9 03 00 00    	jne    14000499e <tx_fn_m0_bench_parse_paths_0+0x58e>
   1400045f5:	48 89 f9             	mov    %rdi,%rcx
   1400045f8:	e8 c3 02 05 00       	call   1400548c0 <txrt_str_release>
   1400045fd:	49 ff c4             	inc    %r12
   140004600:	49 81 fc e8 03 00 00 	cmp    $0x3e8,%r12
   140004607:	74 40                	je     140004649 <tx_fn_m0_bench_parse_paths_0+0x239>
   140004609:	48 89 d9             	mov    %rbx,%rcx
   14000460c:	31 d2                	xor    %edx,%edx
   14000460e:	4c 8d 05 4a 82 0d 00 	lea    0xd824a(%rip),%r8        # 1400dc85f <.rdata+0x385f>
   140004615:	45 31 c9             	xor    %r9d,%r9d
   140004618:	e8 63 cd 02 00       	call   140031380 <txrt_format_begin>
   14000461d:	85 c0                	test   %eax,%eax
   14000461f:	0f 84 eb fe ff ff    	je     140004510 <tx_fn_m0_bench_parse_paths_0+0x100>
   140004625:	89 c7                	mov    %eax,%edi
   140004627:	48 8d 15 32 82 0d 00 	lea    0xd8232(%rip),%rdx        # 1400dc860 <.rdata+0x3860>
   14000462e:	41 b8 e1 00 00 00    	mov    $0xe1,%r8d
   140004634:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000463a:	48 89 f1             	mov    %rsi,%rcx
   14000463d:	e8 3e b1 04 00       	call   14004f780 <txrt_stack_error_location>
   140004642:	89 f9                	mov    %edi,%ecx
   140004644:	e8 b7 f8 04 00       	call   140053f00 <txrt_require_success>
   140004649:	48 8d 8c 24 b0 00 00 	lea    0xb0(%rsp),%rcx
   140004650:	00 
   140004651:	e8 0a c6 02 00       	call   140030c60 <txrt_time_monotonic_micros>
   140004656:	85 c0                	test   %eax,%eax
   140004658:	0f 85 20 04 00 00    	jne    140004a7e <tx_fn_m0_bench_parse_paths_0+0x66e>
   14000465e:	4c 89 b4 24 88 00 00 	mov    %r14,0x88(%rsp)
   140004665:	00 
   140004666:	48 8b 84 24 b0 00 00 	mov    0xb0(%rsp),%rax
   14000466d:	00 
   14000466e:	48 89 84 24 90 00 00 	mov    %rax,0x90(%rsp)
   140004675:	00 
   140004676:	48 89 f1             	mov    %rsi,%rcx
   140004679:	e8 a2 69 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   14000467e:	85 c0                	test   %eax,%eax
   140004680:	0f 85 07 04 00 00    	jne    140004a8d <tx_fn_m0_bench_parse_paths_0+0x67d>
   140004686:	49 83 7f 08 00       	cmpq   $0x0,0x8(%r15)
   14000468b:	0f 84 eb 01 00 00    	je     14000487c <tx_fn_m0_bench_parse_paths_0+0x46c>
   140004691:	bd 01 00 00 00       	mov    $0x1,%ebp
   140004696:	41 bd 9f 86 01 00    	mov    $0x1869f,%r13d
   14000469c:	31 db                	xor    %ebx,%ebx
   14000469e:	48 8d bc 24 f8 00 00 	lea    0xf8(%rsp),%rdi
   1400046a5:	00 
   1400046a6:	4c 8d b4 24 00 01 00 	lea    0x100(%rsp),%r14
   1400046ad:	00 
   1400046ae:	4c 8d 64 24 3f       	lea    0x3f(%rsp),%r12
   1400046b3:	31 c0                	xor    %eax,%eax
   1400046b5:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
   1400046bc:	00 00 00 00 
   1400046c0:	49 8b 0f             	mov    (%r15),%rcx
   1400046c3:	48 8b 14 c1          	mov    (%rcx,%rax,8),%rdx
   1400046c7:	48 89 7c 24 28       	mov    %rdi,0x28(%rsp)
   1400046cc:	4c 89 74 24 20       	mov    %r14,0x20(%rsp)
   1400046d1:	41 b8 0a 00 00 00    	mov    $0xa,%r8d
   1400046d7:	48 89 f1             	mov    %rsi,%rcx
   1400046da:	4d 89 e1             	mov    %r12,%r9
   1400046dd:	e8 5e 92 04 00       	call   14004d940 <txrt_parse_int_scalar_context>
   1400046e2:	85 c0                	test   %eax,%eax
   1400046e4:	0f 85 cc 02 00 00    	jne    1400049b6 <tx_fn_m0_bench_parse_paths_0+0x5a6>
   1400046ea:	80 7c 24 3f 01       	cmpb   $0x1,0x3f(%rsp)
   1400046ef:	75 0f                	jne    140004700 <tx_fn_m0_bench_parse_paths_0+0x2f0>
   1400046f1:	48 89 d8             	mov    %rbx,%rax
   1400046f4:	48 ff c0             	inc    %rax
   1400046f7:	0f 80 e9 02 00 00    	jo     1400049e6 <tx_fn_m0_bench_parse_paths_0+0x5d6>
   1400046fd:	48 89 c3             	mov    %rax,%rbx
   140004700:	49 83 ed 01          	sub    $0x1,%r13
   140004704:	72 26                	jb     14000472c <tx_fn_m0_bench_parse_paths_0+0x31c>
   140004706:	89 e8                	mov    %ebp,%eax
   140004708:	48 69 c0 d3 4d 62 10 	imul   $0x10624dd3,%rax,%rax
   14000470f:	48 c1 e8 26          	shr    $0x26,%rax
   140004713:	69 c0 e8 03 00 00    	imul   $0x3e8,%eax,%eax
   140004719:	89 e9                	mov    %ebp,%ecx
   14000471b:	29 c1                	sub    %eax,%ecx
   14000471d:	89 c8                	mov    %ecx,%eax
   14000471f:	ff c5                	inc    %ebp
   140004721:	49 39 47 08          	cmp    %rax,0x8(%r15)
   140004725:	77 99                	ja     1400046c0 <tx_fn_m0_bench_parse_paths_0+0x2b0>
   140004727:	e9 50 01 00 00       	jmp    14000487c <tx_fn_m0_bench_parse_paths_0+0x46c>
   14000472c:	48 8d 0d 7c 84 0d 00 	lea    0xd847c(%rip),%rcx        # 1400dcbaf <.rdata+0x3baf>
   140004733:	4c 8d 84 24 a8 00 00 	lea    0xa8(%rsp),%r8
   14000473a:	00 
   14000473b:	ba 0b 00 00 00       	mov    $0xb,%edx
   140004740:	e8 cb fd 04 00       	call   140054510 <txrt_str_new>
   140004745:	85 c0                	test   %eax,%eax
   140004747:	0f 85 4f 03 00 00    	jne    140004a9c <tx_fn_m0_bench_parse_paths_0+0x68c>
   14000474d:	4c 8b bc 24 a8 00 00 	mov    0xa8(%rsp),%r15
   140004754:	00 
   140004755:	48 8d 05 a4 84 0d 00 	lea    0xd84a4(%rip),%rax        # 1400dcc00 <.rdata+0x3c00>
   14000475c:	48 89 44 24 68       	mov    %rax,0x68(%rsp)
   140004761:	48 c7 44 24 70 f0 00 	movq   $0xf0,0x70(%rsp)
   140004768:	00 00 
   14000476a:	48 c7 44 24 78 05 00 	movq   $0x5,0x78(%rsp)
   140004771:	00 00 
   140004773:	48 89 f1             	mov    %rsi,%rcx
   140004776:	4c 89 fa             	mov    %r15,%rdx
   140004779:	4c 8b 84 24 90 00 00 	mov    0x90(%rsp),%r8
   140004780:	00 
   140004781:	49 89 d9             	mov    %rbx,%r9
   140004784:	e8 57 ce ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140004789:	4c 89 f9             	mov    %r15,%rcx
   14000478c:	e8 2f 01 05 00       	call   1400548c0 <txrt_str_release>
   140004791:	48 89 f1             	mov    %rsi,%rcx
   140004794:	e8 87 68 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   140004799:	85 c0                	test   %eax,%eax
   14000479b:	0f 85 0a 03 00 00    	jne    140004aab <tx_fn_m0_bench_parse_paths_0+0x69b>
   1400047a1:	48 8d 8c 24 a0 00 00 	lea    0xa0(%rsp),%rcx
   1400047a8:	00 
   1400047a9:	e8 b2 c4 02 00       	call   140030c60 <txrt_time_monotonic_micros>
   1400047ae:	85 c0                	test   %eax,%eax
   1400047b0:	0f 85 04 03 00 00    	jne    140004aba <tx_fn_m0_bench_parse_paths_0+0x6aa>
   1400047b6:	48 8b bc 24 a0 00 00 	mov    0xa0(%rsp),%rdi
   1400047bd:	00 
   1400047be:	48 89 f1             	mov    %rsi,%rcx
   1400047c1:	e8 5a 68 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   1400047c6:	85 c0                	test   %eax,%eax
   1400047c8:	0f 85 fb 02 00 00    	jne    140004ac9 <tx_fn_m0_bench_parse_paths_0+0x6b9>
   1400047ce:	48 8b 4c 24 40       	mov    0x40(%rsp),%rcx
   1400047d3:	48 83 79 08 00       	cmpq   $0x0,0x8(%rcx)
   1400047d8:	0f 84 9e 00 00 00    	je     14000487c <tx_fn_m0_bench_parse_paths_0+0x46c>
   1400047de:	bd 01 00 00 00       	mov    $0x1,%ebp
   1400047e3:	41 bc 9f 86 01 00    	mov    $0x1869f,%r12d
   1400047e9:	31 db                	xor    %ebx,%ebx
   1400047eb:	4c 8d ac 24 e0 00 00 	lea    0xe0(%rsp),%r13
   1400047f2:	00 
   1400047f3:	4c 8d b4 24 e8 00 00 	lea    0xe8(%rsp),%r14
   1400047fa:	00 
   1400047fb:	4c 8d 7c 24 3e       	lea    0x3e(%rsp),%r15
   140004800:	31 c0                	xor    %eax,%eax
   140004802:	66 66 66 66 66 2e 0f 	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140004809:	1f 84 00 00 00 00 00 
   140004810:	48 8b 09             	mov    (%rcx),%rcx
   140004813:	48 8b 14 c1          	mov    (%rcx,%rax,8),%rdx
   140004817:	4c 89 6c 24 28       	mov    %r13,0x28(%rsp)
   14000481c:	4c 89 74 24 20       	mov    %r14,0x20(%rsp)
   140004821:	41 b8 0a 00 00 00    	mov    $0xa,%r8d
   140004827:	48 89 f1             	mov    %rsi,%rcx
   14000482a:	4d 89 f9             	mov    %r15,%r9
   14000482d:	e8 0e 91 04 00       	call   14004d940 <txrt_parse_int_scalar_context>
   140004832:	85 c0                	test   %eax,%eax
   140004834:	0f 85 94 01 00 00    	jne    1400049ce <tx_fn_m0_bench_parse_paths_0+0x5be>
   14000483a:	80 7c 24 3e 00       	cmpb   $0x0,0x3e(%rsp)
   14000483f:	75 0f                	jne    140004850 <tx_fn_m0_bench_parse_paths_0+0x440>
   140004841:	48 89 d8             	mov    %rbx,%rax
   140004844:	48 ff c0             	inc    %rax
   140004847:	0f 80 bf 01 00 00    	jo     140004a0c <tx_fn_m0_bench_parse_paths_0+0x5fc>
   14000484d:	48 89 c3             	mov    %rax,%rbx
   140004850:	49 83 ec 01          	sub    $0x1,%r12
   140004854:	72 2b                	jb     140004881 <tx_fn_m0_bench_parse_paths_0+0x471>
   140004856:	89 e8                	mov    %ebp,%eax
   140004858:	48 69 c0 d3 4d 62 10 	imul   $0x10624dd3,%rax,%rax
   14000485f:	48 c1 e8 26          	shr    $0x26,%rax
   140004863:	69 c0 e8 03 00 00    	imul   $0x3e8,%eax,%eax
   140004869:	89 e9                	mov    %ebp,%ecx
   14000486b:	29 c1                	sub    %eax,%ecx
   14000486d:	89 c8                	mov    %ecx,%eax
   14000486f:	ff c5                	inc    %ebp
   140004871:	48 8b 4c 24 40       	mov    0x40(%rsp),%rcx
   140004876:	48 39 41 08          	cmp    %rax,0x8(%rcx)
   14000487a:	77 94                	ja     140004810 <tx_fn_m0_bench_parse_paths_0+0x400>
   14000487c:	e8 ef ca 03 00       	call   140041370 <txrt_vector_index_error>
   140004881:	48 8d 0d f7 84 0d 00 	lea    0xd84f7(%rip),%rcx        # 1400dcd7f <.rdata+0x3d7f>
   140004888:	4c 8d 84 24 98 00 00 	lea    0x98(%rsp),%r8
   14000488f:	00 
   140004890:	ba 0d 00 00 00       	mov    $0xd,%edx
   140004895:	e8 76 fc 04 00       	call   140054510 <txrt_str_new>
   14000489a:	85 c0                	test   %eax,%eax
   14000489c:	0f 85 36 02 00 00    	jne    140004ad8 <tx_fn_m0_bench_parse_paths_0+0x6c8>
   1400048a2:	4c 8b b4 24 98 00 00 	mov    0x98(%rsp),%r14
   1400048a9:	00 
   1400048aa:	48 8d 05 1f 85 0d 00 	lea    0xd851f(%rip),%rax        # 1400dcdd0 <.rdata+0x3dd0>
   1400048b1:	48 89 44 24 68       	mov    %rax,0x68(%rsp)
   1400048b6:	48 c7 44 24 70 fc 00 	movq   $0xfc,0x70(%rsp)
   1400048bd:	00 00 
   1400048bf:	48 c7 44 24 78 05 00 	movq   $0x5,0x78(%rsp)
   1400048c6:	00 00 
   1400048c8:	48 89 f1             	mov    %rsi,%rcx
   1400048cb:	4c 89 f2             	mov    %r14,%rdx
   1400048ce:	49 89 f8             	mov    %rdi,%r8
   1400048d1:	49 89 d9             	mov    %rbx,%r9
   1400048d4:	e8 07 cd ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   1400048d9:	4c 89 f1             	mov    %r14,%rcx
   1400048dc:	e8 df ff 04 00       	call   1400548c0 <txrt_str_release>
   1400048e1:	48 89 f1             	mov    %rsi,%rcx
   1400048e4:	e8 37 67 03 00       	call   14003b020 <txrt_gc_safepoint_context>
   1400048e9:	85 c0                	test   %eax,%eax
   1400048eb:	0f 85 f0 01 00 00    	jne    140004ae1 <tx_fn_m0_bench_parse_paths_0+0x6d1>
   1400048f1:	48 8b 4c 24 48       	mov    0x48(%rsp),%rcx
   1400048f6:	e8 95 6f 02 00       	call   14002b890 <txrt_value_release>
   1400048fb:	48 8b 4c 24 50       	mov    0x50(%rsp),%rcx
   140004900:	e8 8b 6f 02 00       	call   14002b890 <txrt_value_release>
   140004905:	48 8b 84 24 88 00 00 	mov    0x88(%rsp),%rax
   14000490c:	00 
   14000490d:	48 89 06             	mov    %rax,(%rsi)
   140004910:	48 81 c4 08 01 00 00 	add    $0x108,%rsp
   140004917:	5b                   	pop    %rbx
   140004918:	5d                   	pop    %rbp
   140004919:	5f                   	pop    %rdi
   14000491a:	5e                   	pop    %rsi
   14000491b:	41 5c                	pop    %r12
   14000491d:	41 5d                	pop    %r13
   14000491f:	41 5e                	pop    %r14
   140004921:	41 5f                	pop    %r15
   140004923:	c3                   	ret
   140004924:	41 89 c5             	mov    %eax,%r13d
   140004927:	48 8d 15 72 7f 0d 00 	lea    0xd7f72(%rip),%rdx        # 1400dc8a0 <.rdata+0x38a0>
   14000492e:	41 b8 e1 00 00 00    	mov    $0xe1,%r8d
   140004934:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000493a:	48 89 f1             	mov    %rsi,%rcx
   14000493d:	e8 3e ae 04 00       	call   14004f780 <txrt_stack_error_location>
   140004942:	44 89 e9             	mov    %r13d,%ecx
   140004945:	e8 b6 f5 04 00       	call   140053f00 <txrt_require_success>
   14000494a:	48 8d 15 8f 7f 0d 00 	lea    0xd7f8f(%rip),%rdx        # 1400dc8e0 <.rdata+0x38e0>
   140004951:	41 b8 e1 00 00 00    	mov    $0xe1,%r8d
   140004957:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000495d:	e9 92 01 00 00       	jmp    140004af4 <tx_fn_m0_bench_parse_paths_0+0x6e4>
   140004962:	48 8d 15 b7 7f 0d 00 	lea    0xd7fb7(%rip),%rdx        # 1400dc920 <.rdata+0x3920>
   140004969:	eb 10                	jmp    14000497b <tx_fn_m0_bench_parse_paths_0+0x56b>
   14000496b:	48 8d 15 ee 7f 0d 00 	lea    0xd7fee(%rip),%rdx        # 1400dc960 <.rdata+0x3960>
   140004972:	eb 07                	jmp    14000497b <tx_fn_m0_bench_parse_paths_0+0x56b>
   140004974:	48 8d 15 25 80 0d 00 	lea    0xd8025(%rip),%rdx        # 1400dc9a0 <.rdata+0x39a0>
   14000497b:	41 b8 e2 00 00 00    	mov    $0xe2,%r8d
   140004981:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140004987:	e9 68 01 00 00       	jmp    140004af4 <tx_fn_m0_bench_parse_paths_0+0x6e4>
   14000498c:	48 8d 15 5d 80 0d 00 	lea    0xd805d(%rip),%rdx        # 1400dc9f0 <.rdata+0x39f0>
   140004993:	eb 10                	jmp    1400049a5 <tx_fn_m0_bench_parse_paths_0+0x595>
   140004995:	48 8d 15 94 80 0d 00 	lea    0xd8094(%rip),%rdx        # 1400dca30 <.rdata+0x3a30>
   14000499c:	eb 07                	jmp    1400049a5 <tx_fn_m0_bench_parse_paths_0+0x595>
   14000499e:	48 8d 15 cb 80 0d 00 	lea    0xd80cb(%rip),%rdx        # 1400dca70 <.rdata+0x3a70>
   1400049a5:	41 b8 e3 00 00 00    	mov    $0xe3,%r8d
   1400049ab:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400049b1:	e9 3e 01 00 00       	jmp    140004af4 <tx_fn_m0_bench_parse_paths_0+0x6e4>
   1400049b6:	48 8d 15 73 81 0d 00 	lea    0xd8173(%rip),%rdx        # 1400dcb30 <.rdata+0x3b30>
   1400049bd:	41 b8 ea 00 00 00    	mov    $0xea,%r8d
   1400049c3:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400049c9:	e9 26 01 00 00       	jmp    140004af4 <tx_fn_m0_bench_parse_paths_0+0x6e4>
   1400049ce:	48 8d 15 2b 83 0d 00 	lea    0xd832b(%rip),%rdx        # 1400dcd00 <.rdata+0x3d00>
   1400049d5:	41 b8 f6 00 00 00    	mov    $0xf6,%r8d
   1400049db:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400049e1:	e9 0e 01 00 00       	jmp    140004af4 <tx_fn_m0_bench_parse_paths_0+0x6e4>
   1400049e6:	4c 8d 84 24 f0 00 00 	lea    0xf0(%rsp),%r8
   1400049ed:	00 
   1400049ee:	ba 01 00 00 00       	mov    $0x1,%edx
   1400049f3:	48 89 d9             	mov    %rbx,%rcx
   1400049f6:	e8 05 06 05 00       	call   140055000 <txrt_add_i64>
   1400049fb:	89 c7                	mov    %eax,%edi
   1400049fd:	48 8d 15 6c 81 0d 00 	lea    0xd816c(%rip),%rdx        # 1400dcb70 <.rdata+0x3b70>
   140004a04:	41 b8 ed 00 00 00    	mov    $0xed,%r8d
   140004a0a:	eb 24                	jmp    140004a30 <tx_fn_m0_bench_parse_paths_0+0x620>
   140004a0c:	4c 8d 84 24 d8 00 00 	lea    0xd8(%rsp),%r8
   140004a13:	00 
   140004a14:	ba 01 00 00 00       	mov    $0x1,%edx
   140004a19:	48 89 d9             	mov    %rbx,%rcx
   140004a1c:	e8 df 05 05 00       	call   140055000 <txrt_add_i64>
   140004a21:	89 c7                	mov    %eax,%edi
   140004a23:	48 8d 15 16 83 0d 00 	lea    0xd8316(%rip),%rdx        # 1400dcd40 <.rdata+0x3d40>
   140004a2a:	41 b8 f9 00 00 00    	mov    $0xf9,%r8d
   140004a30:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140004a36:	48 89 f1             	mov    %rsi,%rcx
   140004a39:	e8 42 ad 04 00       	call   14004f780 <txrt_stack_error_location>
   140004a3e:	89 f9                	mov    %edi,%ecx
   140004a40:	e8 bb f4 04 00       	call   140053f00 <txrt_require_success>
   140004a45:	48 8d 15 14 7d 0d 00 	lea    0xd7d14(%rip),%rdx        # 1400dc760 <.rdata+0x3760>
   140004a4c:	eb 07                	jmp    140004a55 <tx_fn_m0_bench_parse_paths_0+0x645>
   140004a4e:	48 8d 15 4b 7d 0d 00 	lea    0xd7d4b(%rip),%rdx        # 1400dc7a0 <.rdata+0x37a0>
   140004a55:	41 b8 dd 00 00 00    	mov    $0xdd,%r8d
   140004a5b:	e9 8e 00 00 00       	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004a60:	48 8d 15 79 7d 0d 00 	lea    0xd7d79(%rip),%rdx        # 1400dc7e0 <.rdata+0x37e0>
   140004a67:	41 b8 de 00 00 00    	mov    $0xde,%r8d
   140004a6d:	eb 7f                	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004a6f:	48 8d 15 aa 7d 0d 00 	lea    0xd7daa(%rip),%rdx        # 1400dc820 <.rdata+0x3820>
   140004a76:	41 b8 de 00 00 00    	mov    $0xde,%r8d
   140004a7c:	eb 70                	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004a7e:	48 8d 15 2b 80 0d 00 	lea    0xd802b(%rip),%rdx        # 1400dcab0 <.rdata+0x3ab0>
   140004a85:	41 b8 e7 00 00 00    	mov    $0xe7,%r8d
   140004a8b:	eb 61                	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004a8d:	48 8d 15 5c 80 0d 00 	lea    0xd805c(%rip),%rdx        # 1400dcaf0 <.rdata+0x3af0>
   140004a94:	41 b8 e7 00 00 00    	mov    $0xe7,%r8d
   140004a9a:	eb 52                	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004a9c:	48 8d 15 1d 81 0d 00 	lea    0xd811d(%rip),%rdx        # 1400dcbc0 <.rdata+0x3bc0>
   140004aa3:	41 b8 f0 00 00 00    	mov    $0xf0,%r8d
   140004aa9:	eb 43                	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004aab:	48 8d 15 8e 81 0d 00 	lea    0xd818e(%rip),%rdx        # 1400dcc40 <.rdata+0x3c40>
   140004ab2:	41 b8 f0 00 00 00    	mov    $0xf0,%r8d
   140004ab8:	eb 34                	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004aba:	48 8d 15 bf 81 0d 00 	lea    0xd81bf(%rip),%rdx        # 1400dcc80 <.rdata+0x3c80>
   140004ac1:	41 b8 f3 00 00 00    	mov    $0xf3,%r8d
   140004ac7:	eb 25                	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004ac9:	48 8d 15 f0 81 0d 00 	lea    0xd81f0(%rip),%rdx        # 1400dccc0 <.rdata+0x3cc0>
   140004ad0:	41 b8 f3 00 00 00    	mov    $0xf3,%r8d
   140004ad6:	eb 16                	jmp    140004aee <tx_fn_m0_bench_parse_paths_0+0x6de>
   140004ad8:	48 8d 15 b1 82 0d 00 	lea    0xd82b1(%rip),%rdx        # 1400dcd90 <.rdata+0x3d90>
   140004adf:	eb 07                	jmp    140004ae8 <tx_fn_m0_bench_parse_paths_0+0x6d8>
   140004ae1:	48 8d 15 28 83 0d 00 	lea    0xd8328(%rip),%rdx        # 1400dce10 <.rdata+0x3e10>
   140004ae8:	41 b8 fc 00 00 00    	mov    $0xfc,%r8d
   140004aee:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140004af4:	48 89 f1             	mov    %rsi,%rcx
   140004af7:	89 c6                	mov    %eax,%esi
   140004af9:	e8 82 ac 04 00       	call   14004f780 <txrt_stack_error_location>
   140004afe:	89 f1                	mov    %esi,%ecx
   140004b00:	e8 fb f3 04 00       	call   140053f00 <txrt_require_success>
   140004b05:	cc                   	int3
   140004b06:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
   140004b0d:	00 00 00 


E:\Project\other\Compilation\tx_build\performance_remaining\candidate\diverse.exe:     file format pei-x86-64


Disassembly of section .text:

000000014004d940 <txrt_parse_int_scalar_context>:
   14004d940:	57                   	push   %rdi
   14004d941:	56                   	push   %rsi
   14004d942:	53                   	push   %rbx
   14004d943:	48 83 ec 40          	sub    $0x40,%rsp
   14004d947:	48 89 cb             	mov    %rcx,%rbx
   14004d94a:	48 89 d1             	mov    %rdx,%rcx
   14004d94d:	4c 89 ce             	mov    %r9,%rsi
   14004d950:	4c 89 c7             	mov    %r8,%rdi
   14004d953:	e8 18 dd ff ff       	call   14004b670 <_ZN12tx_generated6detail10text_valueB5cxx11EPKv>
   14004d958:	48 8d 4c 24 30       	lea    0x30(%rsp),%rcx
   14004d95d:	49 89 f8             	mov    %rdi,%r8
   14004d960:	48 8b 50 08          	mov    0x8(%rax),%rdx
   14004d964:	48 8b 00             	mov    (%rax),%rax
   14004d967:	48 89 54 24 20       	mov    %rdx,0x20(%rsp)
   14004d96c:	48 8d 54 24 20       	lea    0x20(%rsp),%rdx
   14004d971:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   14004d976:	e8 15 dd 02 00       	call   14007b690 <_ZN12tx_generated16parse_int_scalarESt17basic_string_viewIcSt11char_traitsIcEEx>
   14004d97b:	48 8b 44 24 38       	mov    0x38(%rsp),%rax
   14004d980:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
   14004d985:	48 8b 94 24 80 00 00 	mov    0x80(%rsp),%rdx
   14004d98c:	00 
   14004d98d:	48 85 c0             	test   %rax,%rax
   14004d990:	0f 94 06             	sete   (%rsi)
   14004d993:	48 89 0a             	mov    %rcx,(%rdx)
   14004d996:	48 8b 94 24 88 00 00 	mov    0x88(%rsp),%rdx
   14004d99d:	00 
   14004d99e:	48 89 02             	mov    %rax,(%rdx)
   14004d9a1:	48 85 db             	test   %rbx,%rbx
   14004d9a4:	74 12                	je     14004d9b8 <txrt_parse_int_scalar_context+0x78>
   14004d9a6:	8b 43 08             	mov    0x8(%rbx),%eax
   14004d9a9:	48 83 c4 40          	add    $0x40,%rsp
   14004d9ad:	5b                   	pop    %rbx
   14004d9ae:	5e                   	pop    %rsi
   14004d9af:	5f                   	pop    %rdi
   14004d9b0:	c3                   	ret
   14004d9b1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
   14004d9b8:	48 8b 0d 11 8b 09 00 	mov    0x98b11(%rip),%rcx        # 1400e64d0 <.refptr.__emutls_v._ZN12tx_generated6detail14thread_contextE>
   14004d9bf:	e8 ac 64 06 00       	call   1400b3e70 <__emutls_get_address>
   14004d9c4:	48 8b 18             	mov    (%rax),%rbx
   14004d9c7:	48 85 db             	test   %rbx,%rbx
   14004d9ca:	75 da                	jne    14004d9a6 <txrt_parse_int_scalar_context+0x66>
   14004d9cc:	e8 4f 14 00 00       	call   14004ee20 <_ZN12tx_generated6detail26initialize_runtime_contextEv>
   14004d9d1:	48 89 c3             	mov    %rax,%rbx
   14004d9d4:	eb d0                	jmp    14004d9a6 <txrt_parse_int_scalar_context+0x66>
   14004d9d6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
   14004d9dd:	00 00 00 


E:\Project\other\Compilation\tx_build\performance_remaining\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

000000014003ed40 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE>:
   14003ed40:	56                   	push   %rsi
   14003ed41:	53                   	push   %rbx
   14003ed42:	48 83 ec 48          	sub    $0x48,%rsp
   14003ed46:	48 8b 02             	mov    (%rdx),%rax
   14003ed49:	48 8b 52 08          	mov    0x8(%rdx),%rdx
   14003ed4d:	48 89 ce             	mov    %rcx,%rsi
   14003ed50:	48 83 f8 07          	cmp    $0x7,%rax
   14003ed54:	0f 86 c1 01 00 00    	jbe    14003ef1b <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x1db>
   14003ed5a:	49 b8 80 80 80 80 80 	movabs $0x8080808080808080,%r8
   14003ed61:	80 80 80 
   14003ed64:	48 8d 48 f8          	lea    -0x8(%rax),%rcx
   14003ed68:	31 db                	xor    %ebx,%ebx
   14003ed6a:	48 c1 e9 03          	shr    $0x3,%rcx
   14003ed6e:	4c 8d 0c cd 08 00 00 	lea    0x8(,%rcx,8),%r9
   14003ed75:	00 
   14003ed76:	eb 11                	jmp    14003ed89 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x49>
   14003ed78:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
   14003ed7f:	00 
   14003ed80:	48 83 c3 08          	add    $0x8,%rbx
   14003ed84:	4c 39 cb             	cmp    %r9,%rbx
   14003ed87:	74 09                	je     14003ed92 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x52>
   14003ed89:	4c 89 c1             	mov    %r8,%rcx
   14003ed8c:	48 23 0c 1a          	and    (%rdx,%rbx,1),%rcx
   14003ed90:	74 ee                	je     14003ed80 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x40>
   14003ed92:	4c 8d 0c 1a          	lea    (%rdx,%rbx,1),%r9
   14003ed96:	48 39 c3             	cmp    %rax,%rbx
   14003ed99:	72 0e                	jb     14003eda9 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x69>
   14003ed9b:	eb 16                	jmp    14003edb3 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x73>
   14003ed9d:	0f 1f 00             	nopl   (%rax)
   14003eda0:	48 83 c3 01          	add    $0x1,%rbx
   14003eda4:	48 39 c3             	cmp    %rax,%rbx
   14003eda7:	73 06                	jae    14003edaf <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x6f>
   14003eda9:	80 3c 1a 00          	cmpb   $0x0,(%rdx,%rbx,1)
   14003edad:	79 f1                	jns    14003eda0 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x60>
   14003edaf:	4c 8d 0c 1a          	lea    (%rdx,%rbx,1),%r9
   14003edb3:	48 29 d8             	sub    %rbx,%rax
   14003edb6:	48 89 c2             	mov    %rax,%rdx
   14003edb9:	48 83 f8 1f          	cmp    $0x1f,%rax
   14003edbd:	0f 87 25 01 00 00    	ja     14003eee8 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x1a8>
   14003edc3:	48 85 c0             	test   %rax,%rax
   14003edc6:	0f 84 da 00 00 00    	je     14003eea6 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x166>
   14003edcc:	31 c0                	xor    %eax,%eax
   14003edce:	66 90                	xchg   %ax,%ax
   14003edd0:	41 0f b6 0c 01       	movzbl (%r9,%rax,1),%ecx
   14003edd5:	84 c9                	test   %cl,%cl
   14003edd7:	0f 89 b3 00 00 00    	jns    14003ee90 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x150>
   14003eddd:	80 f9 ef             	cmp    $0xef,%cl
   14003ede0:	0f 87 da 00 00 00    	ja     14003eec0 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x180>
   14003ede6:	80 f9 df             	cmp    $0xdf,%cl
   14003ede9:	0f 87 e9 00 00 00    	ja     14003eed8 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x198>
   14003edef:	44 8d 41 3e          	lea    0x3e(%rcx),%r8d
   14003edf3:	41 80 f8 1d          	cmp    $0x1d,%r8b
   14003edf7:	77 58                	ja     14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003edf9:	41 b8 02 00 00 00    	mov    $0x2,%r8d
   14003edff:	49 89 d2             	mov    %rdx,%r10
   14003ee02:	49 29 c2             	sub    %rax,%r10
   14003ee05:	4d 39 c2             	cmp    %r8,%r10
   14003ee08:	72 47                	jb     14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003ee0a:	45 0f b6 5c 01 01    	movzbl 0x1(%r9,%rax,1),%r11d
   14003ee10:	45 8d 53 80          	lea    -0x80(%r11),%r10d
   14003ee14:	41 80 fa 3f          	cmp    $0x3f,%r10b
   14003ee18:	77 37                	ja     14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003ee1a:	49 83 f8 02          	cmp    $0x2,%r8
   14003ee1e:	74 26                	je     14003ee46 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x106>
   14003ee20:	45 0f b6 54 01 02    	movzbl 0x2(%r9,%rax,1),%r10d
   14003ee26:	41 83 c2 80          	add    $0xffffff80,%r10d
   14003ee2a:	41 80 fa 3f          	cmp    $0x3f,%r10b
   14003ee2e:	77 21                	ja     14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003ee30:	49 83 f8 04          	cmp    $0x4,%r8
   14003ee34:	75 10                	jne    14003ee46 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x106>
   14003ee36:	45 0f b6 54 01 03    	movzbl 0x3(%r9,%rax,1),%r10d
   14003ee3c:	41 83 c2 80          	add    $0xffffff80,%r10d
   14003ee40:	41 80 fa 3f          	cmp    $0x3f,%r10b
   14003ee44:	77 0b                	ja     14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003ee46:	80 f9 e0             	cmp    $0xe0,%cl
   14003ee49:	75 1d                	jne    14003ee68 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x128>
   14003ee4b:	41 80 fb 9f          	cmp    $0x9f,%r11b
   14003ee4f:	77 17                	ja     14003ee68 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x128>
   14003ee51:	31 db                	xor    %ebx,%ebx
   14003ee53:	48 89 f0             	mov    %rsi,%rax
   14003ee56:	c6 06 00             	movb   $0x0,(%rsi)
   14003ee59:	48 89 5e 08          	mov    %rbx,0x8(%rsi)
   14003ee5d:	48 83 c4 48          	add    $0x48,%rsp
   14003ee61:	5b                   	pop    %rbx
   14003ee62:	5e                   	pop    %rsi
   14003ee63:	c3                   	ret
   14003ee64:	0f 1f 40 00          	nopl   0x0(%rax)
   14003ee68:	80 f9 ed             	cmp    $0xed,%cl
   14003ee6b:	75 06                	jne    14003ee73 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x133>
   14003ee6d:	41 80 fb 9f          	cmp    $0x9f,%r11b
   14003ee71:	77 de                	ja     14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003ee73:	80 f9 f0             	cmp    $0xf0,%cl
   14003ee76:	75 06                	jne    14003ee7e <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x13e>
   14003ee78:	41 80 fb 8f          	cmp    $0x8f,%r11b
   14003ee7c:	76 d3                	jbe    14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003ee7e:	80 f9 f4             	cmp    $0xf4,%cl
   14003ee81:	75 13                	jne    14003ee96 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x156>
   14003ee83:	41 80 fb 8f          	cmp    $0x8f,%r11b
   14003ee87:	77 c8                	ja     14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003ee89:	eb 0b                	jmp    14003ee96 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x156>
   14003ee8b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
   14003ee90:	41 b8 01 00 00 00    	mov    $0x1,%r8d
   14003ee96:	4c 01 c0             	add    %r8,%rax
   14003ee99:	48 83 c3 01          	add    $0x1,%rbx
   14003ee9d:	48 39 d0             	cmp    %rdx,%rax
   14003eea0:	0f 82 2a ff ff ff    	jb     14003edd0 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x90>
   14003eea6:	48 89 f0             	mov    %rsi,%rax
   14003eea9:	c6 06 01             	movb   $0x1,(%rsi)
   14003eeac:	48 89 5e 08          	mov    %rbx,0x8(%rsi)
   14003eeb0:	48 83 c4 48          	add    $0x48,%rsp
   14003eeb4:	5b                   	pop    %rbx
   14003eeb5:	5e                   	pop    %rsi
   14003eeb6:	c3                   	ret
   14003eeb7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
   14003eebe:	00 00 
   14003eec0:	44 8d 41 10          	lea    0x10(%rcx),%r8d
   14003eec4:	41 80 f8 04          	cmp    $0x4,%r8b
   14003eec8:	77 87                	ja     14003ee51 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x111>
   14003eeca:	41 b8 04 00 00 00    	mov    $0x4,%r8d
   14003eed0:	e9 2a ff ff ff       	jmp    14003edff <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0xbf>
   14003eed5:	0f 1f 00             	nopl   (%rax)
   14003eed8:	41 b8 03 00 00 00    	mov    $0x3,%r8d
   14003eede:	e9 1c ff ff ff       	jmp    14003edff <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0xbf>
   14003eee3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
   14003eee8:	48 8d 4c 24 30       	lea    0x30(%rsp),%rcx
   14003eeed:	48 8d 54 24 20       	lea    0x20(%rsp),%rdx
   14003eef2:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   14003eef7:	4c 89 4c 24 28       	mov    %r9,0x28(%rsp)
   14003eefc:	e8 7f b9 02 00       	call   14006a880 <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE>
   14003ef01:	48 8b 44 24 30       	mov    0x30(%rsp),%rax
   14003ef06:	88 06                	mov    %al,(%rsi)
   14003ef08:	48 89 f0             	mov    %rsi,%rax
   14003ef0b:	48 03 5c 24 38       	add    0x38(%rsp),%rbx
   14003ef10:	48 89 5e 08          	mov    %rbx,0x8(%rsi)
   14003ef14:	48 83 c4 48          	add    $0x48,%rsp
   14003ef18:	5b                   	pop    %rbx
   14003ef19:	5e                   	pop    %rsi
   14003ef1a:	c3                   	ret
   14003ef1b:	31 db                	xor    %ebx,%ebx
   14003ef1d:	48 85 c0             	test   %rax,%rax
   14003ef20:	0f 85 83 fe ff ff    	jne    14003eda9 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x69>
   14003ef26:	e9 7b ff ff ff       	jmp    14003eea6 <_ZN2tx9scan_utf8ESt17basic_string_viewIcSt11char_traitsIcEE+0x166>
   14003ef2b:	90                   	nop
   14003ef2c:	90                   	nop
   14003ef2d:	90                   	nop
   14003ef2e:	90                   	nop
   14003ef2f:	90                   	nop


E:\Project\other\Compilation\tx_build\performance_remaining\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

000000014006a880 <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE>:
   14006a880:	41 57                	push   %r15
   14006a882:	41 56                	push   %r14
   14006a884:	41 55                	push   %r13
   14006a886:	41 54                	push   %r12
   14006a888:	55                   	push   %rbp
   14006a889:	57                   	push   %rdi
   14006a88a:	56                   	push   %rsi
   14006a88b:	53                   	push   %rbx
   14006a88c:	48 81 ec c8 00 00 00 	sub    $0xc8,%rsp
   14006a893:	0f 11 74 24 20       	movups %xmm6,0x20(%rsp)
   14006a898:	0f 11 7c 24 30       	movups %xmm7,0x30(%rsp)
   14006a89d:	44 0f 11 44 24 40    	movups %xmm8,0x40(%rsp)
   14006a8a3:	44 0f 11 4c 24 50    	movups %xmm9,0x50(%rsp)
   14006a8a9:	44 0f 11 54 24 60    	movups %xmm10,0x60(%rsp)
   14006a8af:	44 0f 11 5c 24 70    	movups %xmm11,0x70(%rsp)
   14006a8b5:	44 0f 11 a4 24 80 00 	movups %xmm12,0x80(%rsp)
   14006a8bc:	00 00 
   14006a8be:	44 0f 11 ac 24 90 00 	movups %xmm13,0x90(%rsp)
   14006a8c5:	00 00 
   14006a8c7:	44 0f 11 b4 24 a0 00 	movups %xmm14,0xa0(%rsp)
   14006a8ce:	00 00 
   14006a8d0:	44 0f 11 bc 24 b0 00 	movups %xmm15,0xb0(%rsp)
   14006a8d7:	00 00 
   14006a8d9:	4c 8b 4a 08          	mov    0x8(%rdx),%r9
   14006a8dd:	4c 8b 12             	mov    (%rdx),%r10
   14006a8e0:	41 0f b6 19          	movzbl (%r9),%ebx
   14006a8e4:	41 0f b6 51 01       	movzbl 0x1(%r9),%edx
   14006a8e9:	41 0f b6 71 02       	movzbl 0x2(%r9),%esi
   14006a8ee:	80 fb f4             	cmp    $0xf4,%bl
   14006a8f1:	44 8d 5b 80          	lea    -0x80(%rbx),%r11d
   14006a8f5:	44 8d 42 80          	lea    -0x80(%rdx),%r8d
   14006a8f9:	49 89 cc             	mov    %rcx,%r12
   14006a8fc:	0f 97 c0             	seta   %al
   14006a8ff:	80 fa f4             	cmp    $0xf4,%dl
   14006a902:	8d 4e 80             	lea    -0x80(%rsi),%ecx
   14006a905:	40 0f 97 c7          	seta   %dil
   14006a909:	09 f8                	or     %edi,%eax
   14006a90b:	40 80 fe f4          	cmp    $0xf4,%sil
   14006a90f:	40 0f 97 c7          	seta   %dil
   14006a913:	09 f8                	or     %edi,%eax
   14006a915:	41 80 fb 41          	cmp    $0x41,%r11b
   14006a919:	40 0f 96 c7          	setbe  %dil
   14006a91d:	09 f8                	or     %edi,%eax
   14006a91f:	8d 7a 40             	lea    0x40(%rdx),%edi
   14006a922:	40 80 ff 01          	cmp    $0x1,%dil
   14006a926:	40 0f 96 c7          	setbe  %dil
   14006a92a:	09 f8                	or     %edi,%eax
   14006a92c:	80 fb e0             	cmp    $0xe0,%bl
   14006a92f:	40 0f 94 c7          	sete   %dil
   14006a933:	80 fa 9f             	cmp    $0x9f,%dl
   14006a936:	40 0f 96 c5          	setbe  %bpl
   14006a93a:	21 ef                	and    %ebp,%edi
   14006a93c:	09 f8                	or     %edi,%eax
   14006a93e:	80 fb ed             	cmp    $0xed,%bl
   14006a941:	40 0f 94 c7          	sete   %dil
   14006a945:	80 fa 9f             	cmp    $0x9f,%dl
   14006a948:	40 0f 97 c5          	seta   %bpl
   14006a94c:	21 ef                	and    %ebp,%edi
   14006a94e:	09 f8                	or     %edi,%eax
   14006a950:	80 fb f0             	cmp    $0xf0,%bl
   14006a953:	40 0f 94 c7          	sete   %dil
   14006a957:	80 fa 8f             	cmp    $0x8f,%dl
   14006a95a:	40 0f 96 c5          	setbe  %bpl
   14006a95e:	21 ef                	and    %ebp,%edi
   14006a960:	09 f8                	or     %edi,%eax
   14006a962:	80 fb f4             	cmp    $0xf4,%bl
   14006a965:	40 0f 94 c7          	sete   %dil
   14006a969:	80 fa 8f             	cmp    $0x8f,%dl
   14006a96c:	40 0f 97 c5          	seta   %bpl
   14006a970:	21 ef                	and    %ebp,%edi
   14006a972:	09 f8                	or     %edi,%eax
   14006a974:	8d 7e 40             	lea    0x40(%rsi),%edi
   14006a977:	40 80 ff 01          	cmp    $0x1,%dil
   14006a97b:	40 0f 96 c7          	setbe  %dil
   14006a97f:	09 f8                	or     %edi,%eax
   14006a981:	80 fa e0             	cmp    $0xe0,%dl
   14006a984:	40 0f 94 c7          	sete   %dil
   14006a988:	40 80 fe 9f          	cmp    $0x9f,%sil
   14006a98c:	40 0f 96 c5          	setbe  %bpl
   14006a990:	21 ef                	and    %ebp,%edi
   14006a992:	09 f8                	or     %edi,%eax
   14006a994:	80 fa ed             	cmp    $0xed,%dl
   14006a997:	40 0f 94 c7          	sete   %dil
   14006a99b:	40 80 fe 9f          	cmp    $0x9f,%sil
   14006a99f:	40 0f 97 c5          	seta   %bpl
   14006a9a3:	21 ef                	and    %ebp,%edi
   14006a9a5:	09 f8                	or     %edi,%eax
   14006a9a7:	80 fa f0             	cmp    $0xf0,%dl
   14006a9aa:	40 0f 94 c7          	sete   %dil
   14006a9ae:	40 80 fe 8f          	cmp    $0x8f,%sil
   14006a9b2:	40 0f 96 c5          	setbe  %bpl
   14006a9b6:	21 ef                	and    %ebp,%edi
   14006a9b8:	09 f8                	or     %edi,%eax
   14006a9ba:	80 fa f4             	cmp    $0xf4,%dl
   14006a9bd:	40 0f 94 c7          	sete   %dil
   14006a9c1:	40 80 fe 8f          	cmp    $0x8f,%sil
   14006a9c5:	40 0f 97 c6          	seta   %sil
   14006a9c9:	21 fe                	and    %edi,%esi
   14006a9cb:	09 f0                	or     %esi,%eax
   14006a9cd:	8d 73 3e             	lea    0x3e(%rbx),%esi
   14006a9d0:	40 80 fe 32          	cmp    $0x32,%sil
   14006a9d4:	40 0f 96 c6          	setbe  %sil
   14006a9d8:	41 80 f8 3f          	cmp    $0x3f,%r8b
   14006a9dc:	40 0f 96 c7          	setbe  %dil
   14006a9e0:	83 c2 3e             	add    $0x3e,%edx
   14006a9e3:	31 fe                	xor    %edi,%esi
   14006a9e5:	09 f0                	or     %esi,%eax
   14006a9e7:	80 fa 32             	cmp    $0x32,%dl
   14006a9ea:	0f 96 c2             	setbe  %dl
   14006a9ed:	83 c3 20             	add    $0x20,%ebx
   14006a9f0:	80 fb 14             	cmp    $0x14,%bl
   14006a9f3:	0f 96 c3             	setbe  %bl
   14006a9f6:	09 da                	or     %ebx,%edx
   14006a9f8:	80 f9 3f             	cmp    $0x3f,%cl
   14006a9fb:	0f 96 c3             	setbe  %bl
   14006a9fe:	31 da                	xor    %ebx,%edx
   14006aa00:	09 d0                	or     %edx,%eax
   14006aa02:	31 d2                	xor    %edx,%edx
   14006aa04:	41 80 fb 3f          	cmp    $0x3f,%r11b
   14006aa08:	0f 97 c2             	seta   %dl
   14006aa0b:	41 80 f8 3f          	cmp    $0x3f,%r8b
   14006aa0f:	41 0f 97 c0          	seta   %r8b
   14006aa13:	45 0f b6 c0          	movzbl %r8b,%r8d
   14006aa17:	49 01 d0             	add    %rdx,%r8
   14006aa1a:	31 d2                	xor    %edx,%edx
   14006aa1c:	80 f9 3f             	cmp    $0x3f,%cl
   14006aa1f:	0f 97 c2             	seta   %dl
   14006aa22:	4c 01 c2             	add    %r8,%rdx
   14006aa25:	49 83 fa 03          	cmp    $0x3,%r10
   14006aa29:	0f 86 46 06 00 00    	jbe    14006b075 <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x7f5>
   14006aa2f:	49 8d 4a fc          	lea    -0x4(%r10),%rcx
   14006aa33:	49 8d 5a fd          	lea    -0x3(%r10),%rbx
   14006aa37:	48 83 f9 0e          	cmp    $0xe,%rcx
   14006aa3b:	0f 86 dd 06 00 00    	jbe    14006b11e <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x89e>
   14006aa41:	49 89 db             	mov    %rbx,%r11
   14006aa44:	66 45 0f ef c9       	pxor   %xmm9,%xmm9
   14006aa49:	66 0f ef c9          	pxor   %xmm1,%xmm1
   14006aa4d:	4c 89 c9             	mov    %r9,%rcx
   14006aa50:	49 83 e3 f0          	and    $0xfffffffffffffff0,%r11
   14006aa54:	66 41 0f 6f d1       	movdqa %xmm9,%xmm2
   14006aa59:	f3 44 0f 6f 3d ce 41 	movdqu 0x241ce(%rip),%xmm15        # 14008ec30 <.rdata+0x10>
   14006aa60:	02 00 
   14006aa62:	f3 44 0f 6f 25 25 42 	movdqu 0x24225(%rip),%xmm12        # 14008ec90 <.rdata+0x70>
   14006aa69:	02 00 
   14006aa6b:	4f 8d 04 19          	lea    (%r9,%r11,1),%r8
   14006aa6f:	66 45 0f ef d2       	pxor   %xmm10,%xmm10
   14006aa74:	66 0f ef ff          	pxor   %xmm7,%xmm7
   14006aa78:	f3 44 0f 6f 35 2f 42 	movdqu 0x2422f(%rip),%xmm14        # 14008ecb0 <.rdata+0x90>
   14006aa7f:	02 00 
   14006aa81:	f3 44 0f 6f 1d a6 42 	movdqu 0x242a6(%rip),%xmm11        # 14008ed30 <.rdata+0x110>
   14006aa88:	02 00 
   14006aa8a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
   14006aa90:	f3 0f 6f 61 01       	movdqu 0x1(%rcx),%xmm4
   14006aa95:	f3 0f 6f 71 02       	movdqu 0x2(%rcx),%xmm6
   14006aa9a:	48 83 c1 10          	add    $0x10,%rcx
   14006aa9e:	f3 0f 6f 1d 9a 41 02 	movdqu 0x2419a(%rip),%xmm3        # 14008ec40 <.rdata+0x20>
   14006aaa5:	00 
   14006aaa6:	f3 0f 6f 69 f3       	movdqu -0xd(%rcx),%xmm5
   14006aaab:	66 41 0f fc e7       	paddb  %xmm15,%xmm4
   14006aab0:	f3 0f 6f 05 68 41 02 	movdqu 0x24168(%rip),%xmm0        # 14008ec20 <.rdata>
   14006aab7:	00 
   14006aab8:	66 44 0f 6f ee       	movdqa %xmm6,%xmm13
   14006aabd:	f3 44 0f 6f 05 8a 41 	movdqu 0x2418a(%rip),%xmm8        # 14008ec50 <.rdata+0x30>
   14006aac4:	02 00 
   14006aac6:	66 44 0f 74 2d 21 42 	pcmpeqb 0x24221(%rip),%xmm13        # 14008ecf0 <.rdata+0xd0>
   14006aacd:	02 00 
   14006aacf:	66 0f d8 dc          	psubusb %xmm4,%xmm3
   14006aad3:	f3 0f 6f 25 85 41 02 	movdqu 0x24185(%rip),%xmm4        # 14008ec60 <.rdata+0x40>
   14006aada:	00 
   14006aadb:	66 44 0f fc c6       	paddb  %xmm6,%xmm8
   14006aae0:	66 0f 74 da          	pcmpeqb %xmm2,%xmm3
   14006aae4:	66 0f fc c5          	paddb  %xmm5,%xmm0
   14006aae8:	66 41 0f d8 e0       	psubusb %xmm8,%xmm4
   14006aaed:	f3 44 0f 6f 41 f0    	movdqu -0x10(%rcx),%xmm8
   14006aaf3:	66 44 0f fc 05 74 41 	paddb  0x24174(%rip),%xmm8        # 14008ec70 <.rdata+0x50>
   14006aafa:	02 00 
   14006aafc:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006ab00:	66 0f 74 da          	pcmpeqb %xmm2,%xmm3
   14006ab04:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006ab08:	66 0f eb dc          	por    %xmm4,%xmm3
   14006ab0c:	f3 0f 6f 25 6c 41 02 	movdqu 0x2416c(%rip),%xmm4        # 14008ec80 <.rdata+0x60>
   14006ab13:	00 
   14006ab14:	66 41 0f d8 e0       	psubusb %xmm8,%xmm4
   14006ab19:	66 44 0f 6f c6       	movdqa %xmm6,%xmm8
   14006ab1e:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006ab22:	66 45 0f 74 c6       	pcmpeqb %xmm14,%xmm8
   14006ab27:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006ab2b:	66 0f eb dc          	por    %xmm4,%xmm3
   14006ab2f:	66 41 0f 6f e4       	movdqa %xmm12,%xmm4
   14006ab34:	66 0f d8 e0          	psubusb %xmm0,%xmm4
   14006ab38:	66 0f d8 05 00 42 02 	psubusb 0x24200(%rip),%xmm0        # 14008ed40 <.rdata+0x120>
   14006ab3f:	00 
   14006ab40:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006ab44:	66 0f 74 c2          	pcmpeqb %xmm2,%xmm0
   14006ab48:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006ab4c:	66 41 0f df c3       	pandn  %xmm11,%xmm0
   14006ab51:	66 0f ef dc          	pxor   %xmm4,%xmm3
   14006ab55:	66 0f 6f e5          	movdqa %xmm5,%xmm4
   14006ab59:	66 0f d8 25 3f 41 02 	psubusb 0x2413f(%rip),%xmm4        # 14008eca0 <.rdata+0x80>
   14006ab60:	00 
   14006ab61:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006ab65:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006ab69:	66 41 0f db e0       	pand   %xmm8,%xmm4
   14006ab6e:	66 44 0f 6f c5       	movdqa %xmm5,%xmm8
   14006ab73:	66 45 0f d8 c6       	psubusb %xmm14,%xmm8
   14006ab78:	66 44 0f 74 c2       	pcmpeqb %xmm2,%xmm8
   14006ab7d:	66 44 0f 74 c2       	pcmpeqb %xmm2,%xmm8
   14006ab82:	66 41 0f eb e0       	por    %xmm8,%xmm4
   14006ab87:	f3 44 0f 6f 05 30 41 	movdqu 0x24130(%rip),%xmm8        # 14008ecc0 <.rdata+0xa0>
   14006ab8e:	02 00 
   14006ab90:	66 0f eb dc          	por    %xmm4,%xmm3
   14006ab94:	66 0f 6f e6          	movdqa %xmm6,%xmm4
   14006ab98:	66 44 0f d8 c5       	psubusb %xmm5,%xmm8
   14006ab9d:	66 0f 74 25 2b 41 02 	pcmpeqb 0x2412b(%rip),%xmm4        # 14008ecd0 <.rdata+0xb0>
   14006aba4:	00 
   14006aba5:	66 44 0f 74 c2       	pcmpeqb %xmm2,%xmm8
   14006abaa:	66 0f 74 35 5e 41 02 	pcmpeqb 0x2415e(%rip),%xmm6        # 14008ed10 <.rdata+0xf0>
   14006abb1:	00 
   14006abb2:	66 44 0f 74 c2       	pcmpeqb %xmm2,%xmm8
   14006abb7:	66 44 0f db c4       	pand   %xmm4,%xmm8
   14006abbc:	66 0f 6f e5          	movdqa %xmm5,%xmm4
   14006abc0:	66 0f d8 25 18 41 02 	psubusb 0x24118(%rip),%xmm4        # 14008ece0 <.rdata+0xc0>
   14006abc7:	00 
   14006abc8:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006abcc:	66 0f 74 e2          	pcmpeqb %xmm2,%xmm4
   14006abd0:	66 41 0f db e5       	pand   %xmm13,%xmm4
   14006abd5:	66 41 0f eb e0       	por    %xmm8,%xmm4
   14006abda:	f3 44 0f 6f 05 1d 41 	movdqu 0x2411d(%rip),%xmm8        # 14008ed00 <.rdata+0xe0>
   14006abe1:	02 00 
   14006abe3:	66 44 0f d8 c5       	psubusb %xmm5,%xmm8
   14006abe8:	66 41 0f fc ec       	paddb  %xmm12,%xmm5
   14006abed:	66 44 0f 74 c2       	pcmpeqb %xmm2,%xmm8
   14006abf2:	66 44 0f 74 c2       	pcmpeqb %xmm2,%xmm8
   14006abf7:	66 41 0f db f0       	pand   %xmm8,%xmm6
   14006abfc:	f3 44 0f 6f 05 1b 41 	movdqu 0x2411b(%rip),%xmm8        # 14008ed20 <.rdata+0x100>
   14006ac03:	02 00 
   14006ac05:	66 44 0f d8 c5       	psubusb %xmm5,%xmm8
   14006ac0a:	66 41 0f 6f e8       	movdqa %xmm8,%xmm5
   14006ac0f:	66 0f 74 ea          	pcmpeqb %xmm2,%xmm5
   14006ac13:	66 0f 74 ea          	pcmpeqb %xmm2,%xmm5
   14006ac17:	66 0f eb f5          	por    %xmm5,%xmm6
   14006ac1b:	66 0f eb e6          	por    %xmm6,%xmm4
   14006ac1f:	66 0f eb dc          	por    %xmm4,%xmm3
   14006ac23:	66 41 0f db db       	pand   %xmm11,%xmm3
   14006ac28:	66 44 0f eb cb       	por    %xmm3,%xmm9
   14006ac2d:	66 0f 6f d8          	movdqa %xmm0,%xmm3
   14006ac31:	66 0f 68 c2          	punpckhbw %xmm2,%xmm0
   14006ac35:	66 0f 60 da          	punpcklbw %xmm2,%xmm3
   14006ac39:	66 0f 6f e0          	movdqa %xmm0,%xmm4
   14006ac3d:	66 41 0f 69 c2       	punpckhwd %xmm10,%xmm0
   14006ac42:	66 0f 6f eb          	movdqa %xmm3,%xmm5
   14006ac46:	66 41 0f 69 da       	punpckhwd %xmm10,%xmm3
   14006ac4b:	66 41 0f 61 e2       	punpcklwd %xmm10,%xmm4
   14006ac50:	66 41 0f 61 ea       	punpcklwd %xmm10,%xmm5
   14006ac55:	66 0f 6f f5          	movdqa %xmm5,%xmm6
   14006ac59:	66 0f 6a ef          	punpckhdq %xmm7,%xmm5
   14006ac5d:	66 0f 62 f7          	punpckldq %xmm7,%xmm6
   14006ac61:	66 0f d4 ee          	paddq  %xmm6,%xmm5
   14006ac65:	66 0f 6f f3          	movdqa %xmm3,%xmm6
   14006ac69:	66 0f 6a df          	punpckhdq %xmm7,%xmm3
   14006ac6d:	66 0f 62 f7          	punpckldq %xmm7,%xmm6
   14006ac71:	66 0f d4 de          	paddq  %xmm6,%xmm3
   14006ac75:	66 0f d4 eb          	paddq  %xmm3,%xmm5
   14006ac79:	66 0f 6f dc          	movdqa %xmm4,%xmm3
   14006ac7d:	66 0f 6a e7          	punpckhdq %xmm7,%xmm4
   14006ac81:	66 0f 62 df          	punpckldq %xmm7,%xmm3
   14006ac85:	66 0f d4 e3          	paddq  %xmm3,%xmm4
   14006ac89:	66 0f 6f d8          	movdqa %xmm0,%xmm3
   14006ac8d:	66 0f 6a c7          	punpckhdq %xmm7,%xmm0
   14006ac91:	66 0f 62 df          	punpckldq %xmm7,%xmm3
   14006ac95:	66 0f d4 c1          	paddq  %xmm1,%xmm0
   14006ac99:	66 0f d4 e3          	paddq  %xmm3,%xmm4
   14006ac9d:	66 0f d4 ec          	paddq  %xmm4,%xmm5
   14006aca1:	66 0f 6f cd          	movdqa %xmm5,%xmm1
   14006aca5:	66 0f d4 c8          	paddq  %xmm0,%xmm1
   14006aca9:	49 39 c8             	cmp    %rcx,%r8
   14006acac:	0f 85 de fd ff ff    	jne    14006aa90 <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x210>
   14006acb2:	66 0f 6f c1          	movdqa %xmm1,%xmm0
   14006acb6:	4d 8d 43 03          	lea    0x3(%r11),%r8
   14006acba:	66 0f 73 d8 08       	psrldq $0x8,%xmm0
   14006acbf:	66 0f d4 c8          	paddq  %xmm0,%xmm1
   14006acc3:	66 41 0f 6f c1       	movdqa %xmm9,%xmm0
   14006acc8:	66 0f 73 d8 08       	psrldq $0x8,%xmm0
   14006accd:	66 48 0f 7e c9       	movq   %xmm1,%rcx
   14006acd2:	66 44 0f eb c8       	por    %xmm0,%xmm9
   14006acd7:	48 01 ca             	add    %rcx,%rdx
   14006acda:	66 41 0f 6f c1       	movdqa %xmm9,%xmm0
   14006acdf:	66 0f 73 d8 04       	psrldq $0x4,%xmm0
   14006ace4:	66 44 0f eb c8       	por    %xmm0,%xmm9
   14006ace9:	66 41 0f 6f c1       	movdqa %xmm9,%xmm0
   14006acee:	66 0f 73 d8 02       	psrldq $0x2,%xmm0
   14006acf3:	66 44 0f eb c8       	por    %xmm0,%xmm9
   14006acf8:	66 41 0f 6f c1       	movdqa %xmm9,%xmm0
   14006acfd:	66 0f 73 d8 01       	psrldq $0x1,%xmm0
   14006ad02:	66 44 0f eb c8       	por    %xmm0,%xmm9
   14006ad07:	66 44 0f 7e c9       	movd   %xmm9,%ecx
   14006ad0c:	09 c8                	or     %ecx,%eax
   14006ad0e:	4c 39 db             	cmp    %r11,%rbx
   14006ad11:	0f 84 5e 03 00 00    	je     14006b075 <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x7f5>
   14006ad17:	49 8d 7a fe          	lea    -0x2(%r10),%rdi
   14006ad1b:	49 8d 70 01          	lea    0x1(%r8),%rsi
   14006ad1f:	48 89 7c 24 08       	mov    %rdi,0x8(%rsp)
   14006ad24:	4b 8d 0c 01          	lea    (%r9,%r8,1),%rcx
   14006ad28:	48 39 fe             	cmp    %rdi,%rsi
   14006ad2b:	0f 83 7b 02 00 00    	jae    14006afac <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x72c>
   14006ad31:	49 39 f2             	cmp    %rsi,%r10
   14006ad34:	0f 82 72 02 00 00    	jb     14006afac <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x72c>
   14006ad3a:	44 0f b6 59 fd       	movzbl -0x3(%rcx),%r11d
   14006ad3f:	0f b6 69 fe          	movzbl -0x2(%rcx),%ebp
   14006ad43:	4c 89 54 24 18       	mov    %r10,0x18(%rsp)
   14006ad48:	4c 89 a4 24 10 01 00 	mov    %r12,0x110(%rsp)
   14006ad4f:	00 
   14006ad50:	44 0f b6 69 ff       	movzbl -0x1(%rcx),%r13d
   14006ad55:	44 89 db             	mov    %r11d,%ebx
   14006ad58:	47 0f b6 1c 01       	movzbl (%r9,%r8,1),%r11d
   14006ad5d:	41 80 fb 9f          	cmp    $0x9f,%r11b
   14006ad61:	45 8d 63 80          	lea    -0x80(%r11),%r12d
   14006ad65:	0f 96 c1             	setbe  %cl
   14006ad68:	41 80 fd e0          	cmp    $0xe0,%r13b
   14006ad6c:	40 0f 94 c6          	sete   %sil
   14006ad70:	21 f1                	and    %esi,%ecx
   14006ad72:	41 8d 73 40          	lea    0x40(%r11),%esi
   14006ad76:	40 80 fe 01          	cmp    $0x1,%sil
   14006ad7a:	40 0f 96 c6          	setbe  %sil
   14006ad7e:	09 f1                	or     %esi,%ecx
   14006ad80:	41 80 fb f4          	cmp    $0xf4,%r11b
   14006ad84:	40 0f 97 c6          	seta   %sil
   14006ad88:	09 f1                	or     %esi,%ecx
   14006ad8a:	41 80 fb 9f          	cmp    $0x9f,%r11b
   14006ad8e:	40 0f 97 c6          	seta   %sil
   14006ad92:	41 80 fd ed          	cmp    $0xed,%r13b
   14006ad96:	40 0f 94 c7          	sete   %dil
   14006ad9a:	21 fe                	and    %edi,%esi
   14006ad9c:	09 f1                	or     %esi,%ecx
   14006ad9e:	41 80 fb 8f          	cmp    $0x8f,%r11b
   14006ada2:	40 0f 96 c6          	setbe  %sil
   14006ada6:	41 80 fd f0          	cmp    $0xf0,%r13b
   14006adaa:	40 0f 94 c7          	sete   %dil
   14006adae:	21 fe                	and    %edi,%esi
   14006adb0:	09 f1                	or     %esi,%ecx
   14006adb2:	41 80 fb 8f          	cmp    $0x8f,%r11b
   14006adb6:	40 0f 97 c6          	seta   %sil
   14006adba:	41 80 fd f4          	cmp    $0xf4,%r13b
   14006adbe:	40 0f 94 c7          	sete   %dil
   14006adc2:	21 fe                	and    %edi,%esi
   14006adc4:	41 8d 7d 3e          	lea    0x3e(%r13),%edi
   14006adc8:	09 f1                	or     %esi,%ecx
   14006adca:	8d 75 20             	lea    0x20(%rbp),%esi
   14006adcd:	40 80 fe 14          	cmp    $0x14,%sil
   14006add1:	40 0f 96 c6          	setbe  %sil
   14006add5:	40 80 ff 32          	cmp    $0x32,%dil
   14006add9:	40 0f 96 c7          	setbe  %dil
   14006addd:	83 c3 10             	add    $0x10,%ebx
   14006ade0:	09 fe                	or     %edi,%esi
   14006ade2:	80 fb 04             	cmp    $0x4,%bl
   14006ade5:	0f 96 c3             	setbe  %bl
   14006ade8:	09 f3                	or     %esi,%ebx
   14006adea:	41 80 fc 3f          	cmp    $0x3f,%r12b
   14006adee:	40 0f 96 c6          	setbe  %sil
   14006adf2:	31 f3                	xor    %esi,%ebx
   14006adf4:	89 ee                	mov    %ebp,%esi
   14006adf6:	43 0f b6 6c 01 01    	movzbl 0x1(%r9,%r8,1),%ebp
   14006adfc:	09 d9                	or     %ebx,%ecx
   14006adfe:	41 80 fb e0          	cmp    $0xe0,%r11b
   14006ae02:	41 89 ca             	mov    %ecx,%r10d
   14006ae05:	0f 94 c1             	sete   %cl
   14006ae08:	8d 7d 80             	lea    -0x80(%rbp),%edi
   14006ae0b:	40 80 fd 9f          	cmp    $0x9f,%bpl
   14006ae0f:	0f 96 c3             	setbe  %bl
   14006ae12:	21 d9                	and    %ebx,%ecx
   14006ae14:	8d 5d 40             	lea    0x40(%rbp),%ebx
   14006ae17:	80 fb 01             	cmp    $0x1,%bl
   14006ae1a:	0f 96 c3             	setbe  %bl
   14006ae1d:	09 d9                	or     %ebx,%ecx
   14006ae1f:	40 80 fd f4          	cmp    $0xf4,%bpl
   14006ae23:	0f 97 c3             	seta   %bl
   14006ae26:	09 d9                	or     %ebx,%ecx
   14006ae28:	41 80 fb ed          	cmp    $0xed,%r11b
   14006ae2c:	0f 94 c3             	sete   %bl
   14006ae2f:	40 80 fd 9f          	cmp    $0x9f,%bpl
   14006ae33:	41 0f 97 c6          	seta   %r14b
   14006ae37:	44 21 f3             	and    %r14d,%ebx
   14006ae3a:	09 d9                	or     %ebx,%ecx
   14006ae3c:	41 80 fb f0          	cmp    $0xf0,%r11b
   14006ae40:	0f 94 c3             	sete   %bl
   14006ae43:	40 80 fd 8f          	cmp    $0x8f,%bpl
   14006ae47:	41 0f 96 c6          	setbe  %r14b
   14006ae4b:	44 21 f3             	and    %r14d,%ebx
   14006ae4e:	09 cb                	or     %ecx,%ebx
   14006ae50:	41 80 fb f4          	cmp    $0xf4,%r11b
   14006ae54:	0f 94 c1             	sete   %cl
   14006ae57:	40 80 fd 8f          	cmp    $0x8f,%bpl
   14006ae5b:	41 0f 97 c6          	seta   %r14b
   14006ae5f:	44 21 f1             	and    %r14d,%ecx
   14006ae62:	09 d9                	or     %ebx,%ecx
   14006ae64:	41 8d 5d 20          	lea    0x20(%r13),%ebx
   14006ae68:	80 fb 14             	cmp    $0x14,%bl
   14006ae6b:	41 0f 96 c6          	setbe  %r14b
   14006ae6f:	83 c6 10             	add    $0x10,%esi
   14006ae72:	40 80 fe 04          	cmp    $0x4,%sil
   14006ae76:	41 8d 73 3e          	lea    0x3e(%r11),%esi
   14006ae7a:	0f 96 c3             	setbe  %bl
   14006ae7d:	44 09 f3             	or     %r14d,%ebx
   14006ae80:	40 80 fe 32          	cmp    $0x32,%sil
   14006ae84:	40 0f 96 c6          	setbe  %sil
   14006ae88:	09 f3                	or     %esi,%ebx
   14006ae8a:	40 80 ff 3f          	cmp    $0x3f,%dil
   14006ae8e:	40 0f 96 c6          	setbe  %sil
   14006ae92:	31 f3                	xor    %esi,%ebx
   14006ae94:	09 d9                	or     %ebx,%ecx
   14006ae96:	44 89 eb             	mov    %r13d,%ebx
   14006ae99:	47 0f b6 6c 01 02    	movzbl 0x2(%r9,%r8,1),%r13d
   14006ae9f:	40 80 fd e0          	cmp    $0xe0,%bpl
   14006aea3:	41 0f 94 c6          	sete   %r14b
   14006aea7:	88 4c 24 17          	mov    %cl,0x17(%rsp)
   14006aeab:	41 80 fd 9f          	cmp    $0x9f,%r13b
   14006aeaf:	41 8d 75 80          	lea    -0x80(%r13),%esi
   14006aeb3:	0f 96 c1             	setbe  %cl
   14006aeb6:	41 21 ce             	and    %ecx,%r14d
   14006aeb9:	41 8d 4d 40          	lea    0x40(%r13),%ecx
   14006aebd:	80 f9 01             	cmp    $0x1,%cl
   14006aec0:	0f 96 c1             	setbe  %cl
   14006aec3:	44 09 f1             	or     %r14d,%ecx
   14006aec6:	41 80 fd f4          	cmp    $0xf4,%r13b
   14006aeca:	41 0f 97 c6          	seta   %r14b
   14006aece:	44 09 f1             	or     %r14d,%ecx
   14006aed1:	40 80 fd ed          	cmp    $0xed,%bpl
   14006aed5:	41 0f 94 c6          	sete   %r14b
   14006aed9:	41 80 fd 9f          	cmp    $0x9f,%r13b
   14006aedd:	41 0f 97 c7          	seta   %r15b
   14006aee1:	45 21 fe             	and    %r15d,%r14d
   14006aee4:	41 09 ce             	or     %ecx,%r14d
   14006aee7:	40 80 fd f0          	cmp    $0xf0,%bpl
   14006aeeb:	0f 94 c1             	sete   %cl
   14006aeee:	41 80 fd 8f          	cmp    $0x8f,%r13b
   14006aef2:	41 0f 96 c7          	setbe  %r15b
   14006aef6:	44 21 f9             	and    %r15d,%ecx
   14006aef9:	44 09 f1             	or     %r14d,%ecx
   14006aefc:	40 80 fd f4          	cmp    $0xf4,%bpl
   14006af00:	41 0f 94 c6          	sete   %r14b
   14006af04:	41 80 fd 8f          	cmp    $0x8f,%r13b
   14006af08:	41 0f 97 c7          	seta   %r15b
   14006af0c:	45 21 fe             	and    %r15d,%r14d
   14006af0f:	44 09 f1             	or     %r14d,%ecx
   14006af12:	44 8d 75 3e          	lea    0x3e(%rbp),%r14d
   14006af16:	41 80 fe 32          	cmp    $0x32,%r14b
   14006af1a:	45 8d 73 20          	lea    0x20(%r11),%r14d
   14006af1e:	41 0f 96 c7          	setbe  %r15b
   14006af22:	41 80 fe 14          	cmp    $0x14,%r14b
   14006af26:	41 0f 96 c6          	setbe  %r14b
   14006af2a:	83 c3 10             	add    $0x10,%ebx
   14006af2d:	45 09 fe             	or     %r15d,%r14d
   14006af30:	80 fb 04             	cmp    $0x4,%bl
   14006af33:	0f 96 c3             	setbe  %bl
   14006af36:	41 09 de             	or     %ebx,%r14d
   14006af39:	40 80 fe 3f          	cmp    $0x3f,%sil
   14006af3d:	0f 96 c3             	setbe  %bl
   14006af40:	41 31 de             	xor    %ebx,%r14d
   14006af43:	44 09 f1             	or     %r14d,%ecx
   14006af46:	0a 4c 24 17          	or     0x17(%rsp),%cl
   14006af4a:	09 c1                	or     %eax,%ecx
   14006af4c:	89 c8                	mov    %ecx,%eax
   14006af4e:	31 c9                	xor    %ecx,%ecx
   14006af50:	44 09 d0             	or     %r10d,%eax
   14006af53:	41 80 fc 3f          	cmp    $0x3f,%r12b
   14006af57:	0f 97 c1             	seta   %cl
   14006af5a:	31 db                	xor    %ebx,%ebx
   14006af5c:	40 80 ff 3f          	cmp    $0x3f,%dil
   14006af60:	0f 97 c3             	seta   %bl
   14006af63:	48 01 d9             	add    %rbx,%rcx
   14006af66:	31 db                	xor    %ebx,%ebx
   14006af68:	40 80 fe 3f          	cmp    $0x3f,%sil
   14006af6c:	48 8b 74 24 08       	mov    0x8(%rsp),%rsi
   14006af71:	0f 97 c3             	seta   %bl
   14006af74:	48 01 d9             	add    %rbx,%rcx
   14006af77:	48 01 ca             	add    %rcx,%rdx
   14006af7a:	4c 89 c1             	mov    %r8,%rcx
   14006af7d:	49 83 c0 03          	add    $0x3,%r8
   14006af81:	48 83 c1 04          	add    $0x4,%rcx
   14006af85:	48 39 f1             	cmp    %rsi,%rcx
   14006af88:	0f 82 c7 fd ff ff    	jb     14006ad55 <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x4d5>
   14006af8e:	4c 8b a4 24 10 01 00 	mov    0x110(%rsp),%r12
   14006af95:	00 
   14006af96:	4c 8b 54 24 18       	mov    0x18(%rsp),%r10
   14006af9b:	49 8d 70 01          	lea    0x1(%r8),%rsi
   14006af9f:	eb 0b                	jmp    14006afac <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x72c>
   14006afa1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
   14006afa8:	48 83 c6 01          	add    $0x1,%rsi
   14006afac:	43 0f b6 1c 01       	movzbl (%r9,%r8,1),%ebx
   14006afb1:	47 0f b6 5c 01 ff    	movzbl -0x1(%r9,%r8,1),%r11d
   14006afb7:	80 fb 9f             	cmp    $0x9f,%bl
   14006afba:	8d 7b 80             	lea    -0x80(%rbx),%edi
   14006afbd:	0f 96 c1             	setbe  %cl
   14006afc0:	41 80 fb e0          	cmp    $0xe0,%r11b
   14006afc4:	40 0f 94 c5          	sete   %bpl
   14006afc8:	21 e9                	and    %ebp,%ecx
   14006afca:	8d 6b 40             	lea    0x40(%rbx),%ebp
   14006afcd:	40 80 fd 01          	cmp    $0x1,%bpl
   14006afd1:	40 0f 96 c5          	setbe  %bpl
   14006afd5:	09 e9                	or     %ebp,%ecx
   14006afd7:	80 fb f4             	cmp    $0xf4,%bl
   14006afda:	40 0f 97 c5          	seta   %bpl
   14006afde:	09 e9                	or     %ebp,%ecx
   14006afe0:	80 fb 9f             	cmp    $0x9f,%bl
   14006afe3:	40 0f 97 c5          	seta   %bpl
   14006afe7:	41 80 fb ed          	cmp    $0xed,%r11b
   14006afeb:	41 0f 94 c5          	sete   %r13b
   14006afef:	44 21 ed             	and    %r13d,%ebp
   14006aff2:	09 e9                	or     %ebp,%ecx
   14006aff4:	41 80 fb f0          	cmp    $0xf0,%r11b
   14006aff8:	40 0f 94 c5          	sete   %bpl
   14006affc:	80 fb 8f             	cmp    $0x8f,%bl
   14006afff:	41 0f 96 c5          	setbe  %r13b
   14006b003:	44 21 ed             	and    %r13d,%ebp
   14006b006:	09 e9                	or     %ebp,%ecx
   14006b008:	41 80 fb f4          	cmp    $0xf4,%r11b
   14006b00c:	40 0f 94 c5          	sete   %bpl
   14006b010:	80 fb 8f             	cmp    $0x8f,%bl
   14006b013:	0f 97 c3             	seta   %bl
   14006b016:	21 eb                	and    %ebp,%ebx
   14006b018:	09 d9                	or     %ebx,%ecx
   14006b01a:	43 0f b6 5c 01 fe    	movzbl -0x2(%r9,%r8,1),%ebx
   14006b020:	47 0f b6 44 01 fd    	movzbl -0x3(%r9,%r8,1),%r8d
   14006b026:	83 c3 20             	add    $0x20,%ebx
   14006b029:	80 fb 14             	cmp    $0x14,%bl
   14006b02c:	0f 96 c3             	setbe  %bl
   14006b02f:	41 83 c3 3e          	add    $0x3e,%r11d
   14006b033:	41 80 fb 32          	cmp    $0x32,%r11b
   14006b037:	41 0f 96 c3          	setbe  %r11b
   14006b03b:	41 83 c0 10          	add    $0x10,%r8d
   14006b03f:	41 09 db             	or     %ebx,%r11d
   14006b042:	41 80 f8 04          	cmp    $0x4,%r8b
   14006b046:	41 0f 96 c0          	setbe  %r8b
   14006b04a:	45 09 d8             	or     %r11d,%r8d
   14006b04d:	40 80 ff 3f          	cmp    $0x3f,%dil
   14006b051:	41 0f 96 c3          	setbe  %r11b
   14006b055:	45 31 d8             	xor    %r11d,%r8d
   14006b058:	44 09 c1             	or     %r8d,%ecx
   14006b05b:	49 89 f0             	mov    %rsi,%r8
   14006b05e:	09 c8                	or     %ecx,%eax
   14006b060:	31 c9                	xor    %ecx,%ecx
   14006b062:	40 80 ff 3f          	cmp    $0x3f,%dil
   14006b066:	0f 97 c1             	seta   %cl
   14006b069:	48 01 ca             	add    %rcx,%rdx
   14006b06c:	4c 39 d6             	cmp    %r10,%rsi
   14006b06f:	0f 82 33 ff ff ff    	jb     14006afa8 <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x728>
   14006b075:	43 0f b6 74 0a ff    	movzbl -0x1(%r10,%r9,1),%esi
   14006b07b:	49 89 54 24 08       	mov    %rdx,0x8(%r12)
   14006b080:	8d 4e 3e             	lea    0x3e(%rsi),%ecx
   14006b083:	43 0f b6 74 0a fe    	movzbl -0x2(%r10,%r9,1),%esi
   14006b089:	80 f9 32             	cmp    $0x32,%cl
   14006b08c:	44 8d 46 20          	lea    0x20(%rsi),%r8d
   14006b090:	0f 96 c1             	setbe  %cl
   14006b093:	41 80 f8 14          	cmp    $0x14,%r8b
   14006b097:	41 0f 96 c0          	setbe  %r8b
   14006b09b:	44 09 c1             	or     %r8d,%ecx
   14006b09e:	47 0f b6 44 0a fd    	movzbl -0x3(%r10,%r9,1),%r8d
   14006b0a4:	41 83 c0 10          	add    $0x10,%r8d
   14006b0a8:	41 80 f8 04          	cmp    $0x4,%r8b
   14006b0ac:	41 0f 96 c0          	setbe  %r8b
   14006b0b0:	44 09 c1             	or     %r8d,%ecx
   14006b0b3:	09 c8                	or     %ecx,%eax
   14006b0b5:	83 f0 01             	xor    $0x1,%eax
   14006b0b8:	41 88 04 24          	mov    %al,(%r12)
   14006b0bc:	4c 89 e0             	mov    %r12,%rax
   14006b0bf:	41 80 24 24 01       	andb   $0x1,(%r12)
   14006b0c4:	0f 10 74 24 20       	movups 0x20(%rsp),%xmm6
   14006b0c9:	0f 10 7c 24 30       	movups 0x30(%rsp),%xmm7
   14006b0ce:	44 0f 10 44 24 40    	movups 0x40(%rsp),%xmm8
   14006b0d4:	44 0f 10 4c 24 50    	movups 0x50(%rsp),%xmm9
   14006b0da:	44 0f 10 54 24 60    	movups 0x60(%rsp),%xmm10
   14006b0e0:	44 0f 10 a4 24 80 00 	movups 0x80(%rsp),%xmm12
   14006b0e7:	00 00 
   14006b0e9:	44 0f 10 5c 24 70    	movups 0x70(%rsp),%xmm11
   14006b0ef:	44 0f 10 ac 24 90 00 	movups 0x90(%rsp),%xmm13
   14006b0f6:	00 00 
   14006b0f8:	44 0f 10 b4 24 a0 00 	movups 0xa0(%rsp),%xmm14
   14006b0ff:	00 00 
   14006b101:	44 0f 10 bc 24 b0 00 	movups 0xb0(%rsp),%xmm15
   14006b108:	00 00 
   14006b10a:	48 81 c4 c8 00 00 00 	add    $0xc8,%rsp
   14006b111:	5b                   	pop    %rbx
   14006b112:	5e                   	pop    %rsi
   14006b113:	5f                   	pop    %rdi
   14006b114:	5d                   	pop    %rbp
   14006b115:	41 5c                	pop    %r12
   14006b117:	41 5d                	pop    %r13
   14006b119:	41 5e                	pop    %r14
   14006b11b:	41 5f                	pop    %r15
   14006b11d:	c3                   	ret
   14006b11e:	41 b8 03 00 00 00    	mov    $0x3,%r8d
   14006b124:	e9 ee fb ff ff       	jmp    14006ad17 <_ZN2tx6detail14scan_utf8_bulkESt17basic_string_viewIcSt11char_traitsIcEE+0x497>
   14006b129:	90                   	nop
   14006b12a:	90                   	nop
   14006b12b:	90                   	nop
   14006b12c:	90                   	nop
   14006b12d:	90                   	nop
   14006b12e:	90                   	nop
   14006b12f:	90                   	nop
