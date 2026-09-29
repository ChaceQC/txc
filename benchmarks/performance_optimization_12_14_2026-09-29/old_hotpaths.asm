
E:\Project\other\Compilation\tx_build\performance_12_14\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001bf0 <tx_fn_m0_snapshot_0>:
   140001bf0:	41 57                	push   %r15
   140001bf2:	41 56                	push   %r14
   140001bf4:	41 55                	push   %r13
   140001bf6:	41 54                	push   %r12
   140001bf8:	56                   	push   %rsi
   140001bf9:	57                   	push   %rdi
   140001bfa:	55                   	push   %rbp
   140001bfb:	53                   	push   %rbx
   140001bfc:	48 81 ec a8 00 00 00 	sub    $0xa8,%rsp
   140001c03:	48 89 cf             	mov    %rcx,%rdi
   140001c06:	48 8b 31             	mov    (%rcx),%rsi
   140001c09:	48 8d 05 3b 53 0a 00 	lea    0xa533b(%rip),%rax        # 1400a6f4b <.rdata+0xf4b>
   140001c10:	48 89 44 24 78       	mov    %rax,0x78(%rsp)
   140001c15:	48 8d 05 44 53 0a 00 	lea    0xa5344(%rip),%rax        # 1400a6f60 <.rdata+0xf60>
   140001c1c:	48 89 84 24 80 00 00 	mov    %rax,0x80(%rsp)
   140001c23:	00
   140001c24:	48 c7 84 24 88 00 00 	movq   $0x26,0x88(%rsp)
   140001c2b:	00 26 00 00 00
   140001c30:	48 c7 84 24 90 00 00 	movq   $0x1,0x90(%rsp)
   140001c37:	00 01 00 00 00
   140001c3c:	48 89 b4 24 98 00 00 	mov    %rsi,0x98(%rsp)
   140001c43:	00
   140001c44:	48 8d 44 24 78       	lea    0x78(%rsp),%rax
   140001c49:	48 89 01             	mov    %rax,(%rcx)
   140001c4c:	4c 8d 44 24 70       	lea    0x70(%rsp),%r8
   140001c51:	31 c9                	xor    %ecx,%ecx
   140001c53:	31 d2                	xor    %edx,%edx
   140001c55:	e8 f6 fd 00 00       	call   140011a50 <txrt_vector_new_i64>
   140001c5a:	85 c0                	test   %eax,%eax
   140001c5c:	0f 85 35 03 00 00    	jne    140001f97 <tx_fn_m0_snapshot_0+0x3a7>
   140001c62:	4c 8b 6c 24 70       	mov    0x70(%rsp),%r13
   140001c67:	4c 89 e9             	mov    %r13,%rcx
   140001c6a:	e8 51 f8 00 00       	call   1400114c0 <txrt_vector_ref_i64>
   140001c6f:	48 89 f9             	mov    %rdi,%rcx
   140001c72:	e8 29 53 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140001c77:	85 c0                	test   %eax,%eax
   140001c79:	0f 85 21 03 00 00    	jne    140001fa0 <tx_fn_m0_snapshot_0+0x3b0>
   140001c7f:	ba 01 00 00 00       	mov    $0x1,%edx
   140001c84:	4c 89 e9             	mov    %r13,%rcx
   140001c87:	e8 54 02 01 00       	call   140011ee0 <txrt_vector_push_back_i64>
   140001c8c:	85 c0                	test   %eax,%eax
   140001c8e:	48 89 7c 24 30       	mov    %rdi,0x30(%rsp)
   140001c93:	75 36                	jne    140001ccb <tx_fn_m0_snapshot_0+0xdb>
   140001c95:	bb 02 00 00 00       	mov    $0x2,%ebx
   140001c9a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
   140001ca0:	48 89 f9             	mov    %rdi,%rcx
   140001ca3:	e8 f8 52 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140001ca8:	85 c0                	test   %eax,%eax
   140001caa:	0f 85 b4 02 00 00    	jne    140001f64 <tx_fn_m0_snapshot_0+0x374>
   140001cb0:	48 81 fb a1 86 01 00 	cmp    $0x186a1,%rbx
   140001cb7:	74 26                	je     140001cdf <tx_fn_m0_snapshot_0+0xef>
   140001cb9:	4c 89 e9             	mov    %r13,%rcx
   140001cbc:	48 89 da             	mov    %rbx,%rdx
   140001cbf:	e8 1c 02 01 00       	call   140011ee0 <txrt_vector_push_back_i64>
   140001cc4:	48 ff c3             	inc    %rbx
   140001cc7:	85 c0                	test   %eax,%eax
   140001cc9:	74 d5                	je     140001ca0 <tx_fn_m0_snapshot_0+0xb0>
   140001ccb:	89 c7                	mov    %eax,%edi
   140001ccd:	48 8d 15 ec 4d 0a 00 	lea    0xa4dec(%rip),%rdx        # 1400a6ac0 <.rdata+0xac0>
   140001cd4:	41 b8 2b 00 00 00    	mov    $0x2b,%r8d
   140001cda:	e9 47 01 00 00       	jmp    140001e26 <tx_fn_m0_snapshot_0+0x236>
   140001cdf:	48 8d 4c 24 68       	lea    0x68(%rsp),%rcx
   140001ce4:	e8 97 c8 02 00       	call   14002e580 <txrt_time_monotonic_micros>
   140001ce9:	85 c0                	test   %eax,%eax
   140001ceb:	0f 85 be 02 00 00    	jne    140001faf <tx_fn_m0_snapshot_0+0x3bf>
   140001cf1:	48 8b 5c 24 68       	mov    0x68(%rsp),%rbx
   140001cf6:	48 89 f9             	mov    %rdi,%rcx
   140001cf9:	e8 a2 52 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140001cfe:	85 c0                	test   %eax,%eax
   140001d00:	0f 85 b2 02 00 00    	jne    140001fb8 <tx_fn_m0_snapshot_0+0x3c8>
   140001d06:	48 89 74 24 50       	mov    %rsi,0x50(%rsp)
   140001d0b:	48 8d 15 29 4f 0a 00 	lea    0xa4f29(%rip),%rdx        # 1400a6c3b <.rdata+0xc3b>
   140001d12:	4c 8d 4c 24 40       	lea    0x40(%rsp),%r9
   140001d17:	4c 89 e9             	mov    %r13,%rcx
   140001d1a:	41 b0 01             	mov    $0x1,%r8b
   140001d1d:	e8 8e b7 01 00       	call   14001d4b0 <txrt_iterator_new_i64>
   140001d22:	85 c0                	test   %eax,%eax
   140001d24:	0f 85 ed 00 00 00    	jne    140001e17 <tx_fn_m0_snapshot_0+0x227>
   140001d2a:	48 89 5c 24 48       	mov    %rbx,0x48(%rsp)
   140001d2f:	be 01 00 00 00       	mov    $0x1,%esi
   140001d34:	45 31 f6             	xor    %r14d,%r14d
   140001d37:	4c 8d 7c 24 2f       	lea    0x2f(%rsp),%r15
   140001d3c:	4c 8d 64 24 60       	lea    0x60(%rsp),%r12
   140001d41:	4c 89 6c 24 38       	mov    %r13,0x38(%rsp)
   140001d46:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
   140001d4d:	00 00 00
   140001d50:	48 8b 5c 24 40       	mov    0x40(%rsp),%rbx
   140001d55:	48 89 f9             	mov    %rdi,%rcx
   140001d58:	e8 43 52 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140001d5d:	85 c0                	test   %eax,%eax
   140001d5f:	0f 85 0e 02 00 00    	jne    140001f73 <tx_fn_m0_snapshot_0+0x383>
   140001d65:	48 89 d9             	mov    %rbx,%rcx
   140001d68:	4c 89 fa             	mov    %r15,%rdx
   140001d6b:	4d 89 e0             	mov    %r12,%r8
   140001d6e:	e8 cd 9b 01 00       	call   14001b940 <txrt_iterator_next_scalar_i64>
   140001d73:	85 c0                	test   %eax,%eax
   140001d75:	0f 85 9b 01 00 00    	jne    140001f16 <tx_fn_m0_snapshot_0+0x326>
   140001d7b:	bf a0 86 01 00       	mov    $0x186a0,%edi
   140001d80:	4c 89 f5             	mov    %r14,%rbp
   140001d83:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001d8a:	84 00 00 00 00 00
   140001d90:	80 7c 24 2f 00       	cmpb   $0x0,0x2f(%rsp)
   140001d95:	75 0d                	jne    140001da4 <tx_fn_m0_snapshot_0+0x1b4>
   140001d97:	e8 ee 18 0a 00       	call   1400a368a <txrt_option_empty_error>
   140001d9c:	85 c0                	test   %eax,%eax
   140001d9e:	0f 85 9a 01 00 00    	jne    140001f3e <tx_fn_m0_snapshot_0+0x34e>
   140001da4:	48 8b 54 24 60       	mov    0x60(%rsp),%rdx
   140001da9:	49 89 ee             	mov    %rbp,%r14
   140001dac:	49 01 d6             	add    %rdx,%r14
   140001daf:	0f 80 2b 01 00 00    	jo     140001ee0 <tx_fn_m0_snapshot_0+0x2f0>
   140001db5:	48 ff cf             	dec    %rdi
   140001db8:	74 26                	je     140001de0 <tx_fn_m0_snapshot_0+0x1f0>
   140001dba:	48 89 d9             	mov    %rbx,%rcx
   140001dbd:	4c 89 fa             	mov    %r15,%rdx
   140001dc0:	4d 89 e0             	mov    %r12,%r8
   140001dc3:	e8 78 9b 01 00       	call   14001b940 <txrt_iterator_next_scalar_i64>
   140001dc8:	4c 89 f5             	mov    %r14,%rbp
   140001dcb:	85 c0                	test   %eax,%eax
   140001dcd:	74 c1                	je     140001d90 <tx_fn_m0_snapshot_0+0x1a0>
   140001dcf:	e9 42 01 00 00       	jmp    140001f16 <tx_fn_m0_snapshot_0+0x326>
   140001dd4:	66 66 66 2e 0f 1f 84 	data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001ddb:	00 00 00 00 00
   140001de0:	48 89 d9             	mov    %rbx,%rcx
   140001de3:	e8 b8 d7 02 00       	call   14002f5a0 <txrt_value_release>
   140001de8:	48 83 fe 0a          	cmp    $0xa,%rsi
   140001dec:	74 4f                	je     140001e3d <tx_fn_m0_snapshot_0+0x24d>
   140001dee:	48 ff c6             	inc    %rsi
   140001df1:	48 8b 4c 24 38       	mov    0x38(%rsp),%rcx
   140001df6:	48 8d 15 3e 4e 0a 00 	lea    0xa4e3e(%rip),%rdx        # 1400a6c3b <.rdata+0xc3b>
   140001dfd:	41 b0 01             	mov    $0x1,%r8b
   140001e00:	4c 8d 4c 24 40       	lea    0x40(%rsp),%r9
   140001e05:	e8 a6 b6 01 00       	call   14001d4b0 <txrt_iterator_new_i64>
   140001e0a:	85 c0                	test   %eax,%eax
   140001e0c:	48 8b 7c 24 30       	mov    0x30(%rsp),%rdi
   140001e11:	0f 84 39 ff ff ff    	je     140001d50 <tx_fn_m0_snapshot_0+0x160>
   140001e17:	89 c7                	mov    %eax,%edi
   140001e19:	48 8d 15 20 4e 0a 00 	lea    0xa4e20(%rip),%rdx        # 1400a6c40 <.rdata+0xc40>
   140001e20:	41 b8 31 00 00 00    	mov    $0x31,%r8d
   140001e26:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001e2c:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
   140001e31:	e8 fa 49 00 00       	call   140006830 <txrt_stack_error_location>
   140001e36:	89 f9                	mov    %edi,%ecx
   140001e38:	e8 43 20 00 00       	call   140003e80 <txrt_require_success>
   140001e3d:	48 8d 0d d7 4f 0a 00 	lea    0xa4fd7(%rip),%rcx        # 1400a6e1b <.rdata+0xe1b>
   140001e44:	4c 8d 44 24 58       	lea    0x58(%rsp),%r8
   140001e49:	ba 08 00 00 00       	mov    $0x8,%edx
   140001e4e:	e8 3d 26 00 00       	call   140004490 <txrt_str_new>
   140001e53:	85 c0                	test   %eax,%eax
   140001e55:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
   140001e5a:	48 8b 7c 24 38       	mov    0x38(%rsp),%rdi
   140001e5f:	0f 85 68 01 00 00    	jne    140001fcd <tx_fn_m0_snapshot_0+0x3dd>
   140001e65:	48 8b 5c 24 58       	mov    0x58(%rsp),%rbx
   140001e6a:	48 8d 05 1f 50 0a 00 	lea    0xa501f(%rip),%rax        # 1400a6e90 <.rdata+0xe90>
   140001e71:	48 89 84 24 80 00 00 	mov    %rax,0x80(%rsp)
   140001e78:	00
   140001e79:	48 c7 84 24 88 00 00 	movq   $0x38,0x88(%rsp)
   140001e80:	00 38 00 00 00
   140001e85:	48 c7 84 24 90 00 00 	movq   $0x5,0x90(%rsp)
   140001e8c:	00 05 00 00 00
   140001e91:	48 89 f1             	mov    %rsi,%rcx
   140001e94:	48 89 da             	mov    %rbx,%rdx
   140001e97:	4c 8b 44 24 48       	mov    0x48(%rsp),%r8
   140001e9c:	4d 89 f1             	mov    %r14,%r9
   140001e9f:	e8 3c f7 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140001ea4:	48 89 d9             	mov    %rbx,%rcx
   140001ea7:	e8 94 29 00 00       	call   140004840 <txrt_str_release>
   140001eac:	48 89 f1             	mov    %rsi,%rcx
   140001eaf:	e8 ec 50 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140001eb4:	85 c0                	test   %eax,%eax
   140001eb6:	0f 85 1a 01 00 00    	jne    140001fd6 <tx_fn_m0_snapshot_0+0x3e6>
   140001ebc:	48 89 f9             	mov    %rdi,%rcx
   140001ebf:	e8 dc d6 02 00       	call   14002f5a0 <txrt_value_release>
   140001ec4:	48 8b 44 24 50       	mov    0x50(%rsp),%rax
   140001ec9:	48 89 06             	mov    %rax,(%rsi)
   140001ecc:	48 81 c4 a8 00 00 00 	add    $0xa8,%rsp
   140001ed3:	5b                   	pop    %rbx
   140001ed4:	5d                   	pop    %rbp
   140001ed5:	5f                   	pop    %rdi
   140001ed6:	5e                   	pop    %rsi
   140001ed7:	41 5c                	pop    %r12
   140001ed9:	41 5d                	pop    %r13
   140001edb:	41 5e                	pop    %r14
   140001edd:	41 5f                	pop    %r15
   140001edf:	c3                   	ret
   140001ee0:	4c 8d 84 24 a0 00 00 	lea    0xa0(%rsp),%r8
   140001ee7:	00
   140001ee8:	48 89 e9             	mov    %rbp,%rcx
   140001eeb:	e8 90 30 00 00       	call   140004f80 <txrt_add_i64>
   140001ef0:	89 c7                	mov    %eax,%edi
   140001ef2:	48 8d 15 c7 4e 0a 00 	lea    0xa4ec7(%rip),%rdx        # 1400a6dc0 <.rdata+0xdc0>
   140001ef9:	41 b8 35 00 00 00    	mov    $0x35,%r8d
   140001eff:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140001f05:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
   140001f0a:	e8 21 49 00 00       	call   140006830 <txrt_stack_error_location>
   140001f0f:	89 f9                	mov    %edi,%ecx
   140001f11:	e8 6a 1f 00 00       	call   140003e80 <txrt_require_success>
   140001f16:	41 89 c5             	mov    %eax,%r13d
   140001f19:	48 8d 15 e0 4d 0a 00 	lea    0xa4de0(%rip),%rdx        # 1400a6d00 <.rdata+0xd00>
   140001f20:	41 b8 34 00 00 00    	mov    $0x34,%r8d
   140001f26:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140001f2c:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
   140001f31:	e8 fa 48 00 00       	call   140006830 <txrt_stack_error_location>
   140001f36:	44 89 e9             	mov    %r13d,%ecx
   140001f39:	e8 42 1f 00 00       	call   140003e80 <txrt_require_success>
   140001f3e:	48 8d 15 1b 4e 0a 00 	lea    0xa4e1b(%rip),%rdx        # 1400a6d60 <.rdata+0xd60>
   140001f45:	41 b8 35 00 00 00    	mov    $0x35,%r8d
   140001f4b:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140001f51:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
   140001f56:	89 c6                	mov    %eax,%esi
   140001f58:	e8 d3 48 00 00       	call   140006830 <txrt_stack_error_location>
   140001f5d:	89 f1                	mov    %esi,%ecx
   140001f5f:	e8 1c 1f 00 00       	call   140003e80 <txrt_require_success>
   140001f64:	48 8d 15 b5 4b 0a 00 	lea    0xa4bb5(%rip),%rdx        # 1400a6b20 <.rdata+0xb20>
   140001f6b:	41 b8 2b 00 00 00    	mov    $0x2b,%r8d
   140001f71:	eb 0d                	jmp    140001f80 <tx_fn_m0_snapshot_0+0x390>
   140001f73:	48 8d 15 26 4d 0a 00 	lea    0xa4d26(%rip),%rdx        # 1400a6ca0 <.rdata+0xca0>
   140001f7a:	41 b8 31 00 00 00    	mov    $0x31,%r8d
   140001f80:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001f86:	48 89 f9             	mov    %rdi,%rcx
   140001f89:	89 c6                	mov    %eax,%esi
   140001f8b:	e8 a0 48 00 00       	call   140006830 <txrt_stack_error_location>
   140001f90:	89 f1                	mov    %esi,%ecx
   140001f92:	e8 e9 1e 00 00       	call   140003e80 <txrt_require_success>
   140001f97:	48 8d 15 62 4a 0a 00 	lea    0xa4a62(%rip),%rdx        # 1400a6a00 <.rdata+0xa00>
   140001f9e:	eb 07                	jmp    140001fa7 <tx_fn_m0_snapshot_0+0x3b7>
   140001fa0:	48 8d 15 b9 4a 0a 00 	lea    0xa4ab9(%rip),%rdx        # 1400a6a60 <.rdata+0xa60>
   140001fa7:	41 b8 28 00 00 00    	mov    $0x28,%r8d
   140001fad:	eb 16                	jmp    140001fc5 <tx_fn_m0_snapshot_0+0x3d5>
   140001faf:	48 8d 15 ca 4b 0a 00 	lea    0xa4bca(%rip),%rdx        # 1400a6b80 <.rdata+0xb80>
   140001fb6:	eb 07                	jmp    140001fbf <tx_fn_m0_snapshot_0+0x3cf>
   140001fb8:	48 8d 15 21 4c 0a 00 	lea    0xa4c21(%rip),%rdx        # 1400a6be0 <.rdata+0xbe0>
   140001fbf:	41 b8 2e 00 00 00    	mov    $0x2e,%r8d
   140001fc5:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001fcb:	eb b9                	jmp    140001f86 <tx_fn_m0_snapshot_0+0x396>
   140001fcd:	48 8d 15 5c 4e 0a 00 	lea    0xa4e5c(%rip),%rdx        # 1400a6e30 <.rdata+0xe30>
   140001fd4:	eb 07                	jmp    140001fdd <tx_fn_m0_snapshot_0+0x3ed>
   140001fd6:	48 8d 15 13 4f 0a 00 	lea    0xa4f13(%rip),%rdx        # 1400a6ef0 <.rdata+0xef0>
   140001fdd:	41 b8 38 00 00 00    	mov    $0x38,%r8d
   140001fe3:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001fe9:	48 89 f1             	mov    %rsi,%rcx
   140001fec:	89 c6                	mov    %eax,%esi
   140001fee:	e8 3d 48 00 00       	call   140006830 <txrt_stack_error_location>
   140001ff3:	89 f1                	mov    %esi,%ecx
   140001ff5:	e8 86 1e 00 00       	call   140003e80 <txrt_require_success>
   140001ffa:	cc                   	int3
   140001ffb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_12_14\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001830 <tx_fn_m0_scan_0>:
   140001830:	41 57                	push   %r15
   140001832:	41 56                	push   %r14
   140001834:	41 55                	push   %r13
   140001836:	41 54                	push   %r12
   140001838:	56                   	push   %rsi
   140001839:	57                   	push   %rdi
   14000183a:	55                   	push   %rbp
   14000183b:	53                   	push   %rbx
   14000183c:	48 81 ec 98 00 00 00 	sub    $0x98,%rsp
   140001843:	4c 89 cd             	mov    %r9,%rbp
   140001846:	4d 89 c6             	mov    %r8,%r14
   140001849:	49 89 d4             	mov    %rdx,%r12
   14000184c:	48 89 ce             	mov    %rcx,%rsi
   14000184f:	48 8b 39             	mov    (%rcx),%rdi
   140001852:	48 8d 05 42 51 0a 00 	lea    0xa5142(%rip),%rax        # 1400a699b <.rdata+0x99b>
   140001859:	48 89 44 24 58       	mov    %rax,0x58(%rsp)
   14000185e:	48 8d 05 3b 51 0a 00 	lea    0xa513b(%rip),%rax        # 1400a69a0 <.rdata+0x9a0>
   140001865:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   14000186a:	48 c7 44 24 68 13 00 	movq   $0x13,0x68(%rsp)
   140001871:	00 00
   140001873:	48 c7 44 24 70 01 00 	movq   $0x1,0x70(%rsp)
   14000187a:	00 00
   14000187c:	48 89 7c 24 78       	mov    %rdi,0x78(%rsp)
   140001881:	48 8d 44 24 58       	lea    0x58(%rsp),%rax
   140001886:	48 89 01             	mov    %rax,(%rcx)
   140001889:	4c 8d 44 24 50       	lea    0x50(%rsp),%r8
   14000188e:	31 c9                	xor    %ecx,%ecx
   140001890:	31 d2                	xor    %edx,%edx
   140001892:	e8 b9 01 01 00       	call   140011a50 <txrt_vector_new_i64>
   140001897:	85 c0                	test   %eax,%eax
   140001899:	0f 85 63 02 00 00    	jne    140001b02 <tx_fn_m0_scan_0+0x2d2>
   14000189f:	48 8b 5c 24 50       	mov    0x50(%rsp),%rbx
   1400018a4:	48 89 d9             	mov    %rbx,%rcx
   1400018a7:	e8 14 fc 00 00       	call   1400114c0 <txrt_vector_ref_i64>
   1400018ac:	49 89 c7             	mov    %rax,%r15
   1400018af:	48 89 f1             	mov    %rsi,%rcx
   1400018b2:	e8 e9 56 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   1400018b7:	85 c0                	test   %eax,%eax
   1400018b9:	0f 85 4c 02 00 00    	jne    140001b0b <tx_fn_m0_scan_0+0x2db>
   1400018bf:	4d 85 e4             	test   %r12,%r12
   1400018c2:	7e 42                	jle    140001906 <tx_fn_m0_scan_0+0xd6>
   1400018c4:	41 bd 01 00 00 00    	mov    $0x1,%r13d
   1400018ca:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
   1400018d0:	48 89 d9             	mov    %rbx,%rcx
   1400018d3:	4c 89 ea             	mov    %r13,%rdx
   1400018d6:	e8 05 06 01 00       	call   140011ee0 <txrt_vector_push_back_i64>
   1400018db:	85 c0                	test   %eax,%eax
   1400018dd:	0f 85 6d 01 00 00    	jne    140001a50 <tx_fn_m0_scan_0+0x220>
   1400018e3:	48 89 f1             	mov    %rsi,%rcx
   1400018e6:	e8 b5 56 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   1400018eb:	85 c0                	test   %eax,%eax
   1400018ed:	0f 85 66 01 00 00    	jne    140001a59 <tx_fn_m0_scan_0+0x229>
   1400018f3:	4d 39 ec             	cmp    %r13,%r12
   1400018f6:	74 0e                	je     140001906 <tx_fn_m0_scan_0+0xd6>
   1400018f8:	49 ff c5             	inc    %r13
   1400018fb:	0f 80 94 01 00 00    	jo     140001a95 <tx_fn_m0_scan_0+0x265>
   140001901:	4d 39 e5             	cmp    %r12,%r13
   140001904:	7e ca                	jle    1400018d0 <tx_fn_m0_scan_0+0xa0>
   140001906:	48 8d 4c 24 48       	lea    0x48(%rsp),%rcx
   14000190b:	e8 70 cc 02 00       	call   14002e580 <txrt_time_monotonic_micros>
   140001910:	85 c0                	test   %eax,%eax
   140001912:	0f 85 02 02 00 00    	jne    140001b1a <tx_fn_m0_scan_0+0x2ea>
   140001918:	4c 8b 64 24 48       	mov    0x48(%rsp),%r12
   14000191d:	48 89 f1             	mov    %rsi,%rcx
   140001920:	e8 7b 56 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140001925:	85 c0                	test   %eax,%eax
   140001927:	0f 85 fc 01 00 00    	jne    140001b29 <tx_fn_m0_scan_0+0x2f9>
   14000192d:	4c 89 64 24 28       	mov    %r12,0x28(%rsp)
   140001932:	48 89 6c 24 30       	mov    %rbp,0x30(%rsp)
   140001937:	48 89 7c 24 38       	mov    %rdi,0x38(%rsp)
   14000193c:	4d 85 f6             	test   %r14,%r14
   14000193f:	7e 71                	jle    1400019b2 <tx_fn_m0_scan_0+0x182>
   140001941:	41 bc 01 00 00 00    	mov    $0x1,%r12d
   140001947:	45 31 ed             	xor    %r13d,%r13d
   14000194a:	48 8d 6c 24 40       	lea    0x40(%rsp),%rbp
   14000194f:	90                   	nop
   140001950:	48 89 d9             	mov    %rbx,%rcx
   140001953:	48 89 ea             	mov    %rbp,%rdx
   140001956:	e8 b5 da 02 00       	call   14002f410 <txrt_value_clone>
   14000195b:	85 c0                	test   %eax,%eax
   14000195d:	0f 85 0e 01 00 00    	jne    140001a71 <tx_fn_m0_scan_0+0x241>
   140001963:	48 8b 4c 24 40       	mov    0x40(%rsp),%rcx
   140001968:	4d 8b 47 08          	mov    0x8(%r15),%r8
   14000196c:	4d 85 c0             	test   %r8,%r8
   14000196f:	7e 27                	jle    140001998 <tx_fn_m0_scan_0+0x168>
   140001971:	4d 8b 0f             	mov    (%r15),%r9
   140001974:	45 31 d2             	xor    %r10d,%r10d
   140001977:	4c 89 e8             	mov    %r13,%rax
   14000197a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
   140001980:	4b 8b 14 d1          	mov    (%r9,%r10,8),%rdx
   140001984:	49 01 d5             	add    %rdx,%r13
   140001987:	0f 80 8f 00 00 00    	jo     140001a1c <tx_fn_m0_scan_0+0x1ec>
   14000198d:	49 ff c2             	inc    %r10
   140001990:	4c 89 e8             	mov    %r13,%rax
   140001993:	4d 39 d0             	cmp    %r10,%r8
   140001996:	75 e8                	jne    140001980 <tx_fn_m0_scan_0+0x150>
   140001998:	e8 03 dc 02 00       	call   14002f5a0 <txrt_value_release>
   14000199d:	4d 39 f4             	cmp    %r14,%r12
   1400019a0:	74 13                	je     1400019b5 <tx_fn_m0_scan_0+0x185>
   1400019a2:	49 ff c4             	inc    %r12
   1400019a5:	0f 80 17 01 00 00    	jo     140001ac2 <tx_fn_m0_scan_0+0x292>
   1400019ab:	4d 39 f4             	cmp    %r14,%r12
   1400019ae:	7e a0                	jle    140001950 <tx_fn_m0_scan_0+0x120>
   1400019b0:	eb 03                	jmp    1400019b5 <tx_fn_m0_scan_0+0x185>
   1400019b2:	45 31 ed             	xor    %r13d,%r13d
   1400019b5:	48 8d 05 24 4f 0a 00 	lea    0xa4f24(%rip),%rax        # 1400a68e0 <.rdata+0x8e0>
   1400019bc:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   1400019c1:	48 c7 44 24 68 23 00 	movq   $0x23,0x68(%rsp)
   1400019c8:	00 00
   1400019ca:	48 c7 44 24 70 05 00 	movq   $0x5,0x70(%rsp)
   1400019d1:	00 00
   1400019d3:	48 89 f1             	mov    %rsi,%rcx
   1400019d6:	48 8b 54 24 30       	mov    0x30(%rsp),%rdx
   1400019db:	4c 8b 44 24 28       	mov    0x28(%rsp),%r8
   1400019e0:	4d 89 e9             	mov    %r13,%r9
   1400019e3:	e8 f8 fb ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   1400019e8:	48 89 f1             	mov    %rsi,%rcx
   1400019eb:	e8 b0 55 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   1400019f0:	85 c0                	test   %eax,%eax
   1400019f2:	0f 85 40 01 00 00    	jne    140001b38 <tx_fn_m0_scan_0+0x308>
   1400019f8:	48 89 d9             	mov    %rbx,%rcx
   1400019fb:	e8 a0 db 02 00       	call   14002f5a0 <txrt_value_release>
   140001a00:	48 8b 44 24 38       	mov    0x38(%rsp),%rax
   140001a05:	48 89 06             	mov    %rax,(%rsi)
   140001a08:	48 81 c4 98 00 00 00 	add    $0x98,%rsp
   140001a0f:	5b                   	pop    %rbx
   140001a10:	5d                   	pop    %rbp
   140001a11:	5f                   	pop    %rdi
   140001a12:	5e                   	pop    %rsi
   140001a13:	41 5c                	pop    %r12
   140001a15:	41 5d                	pop    %r13
   140001a17:	41 5e                	pop    %r14
   140001a19:	41 5f                	pop    %r15
   140001a1b:	c3                   	ret
   140001a1c:	4c 8d 84 24 88 00 00 	lea    0x88(%rsp),%r8
   140001a23:	00
   140001a24:	48 89 c1             	mov    %rax,%rcx
   140001a27:	e8 54 35 00 00       	call   140004f80 <txrt_add_i64>
   140001a2c:	89 c7                	mov    %eax,%edi
   140001a2e:	48 8d 15 eb 4d 0a 00 	lea    0xa4deb(%rip),%rdx        # 1400a6820 <.rdata+0x820>
   140001a35:	41 b8 20 00 00 00    	mov    $0x20,%r8d
   140001a3b:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140001a41:	48 89 f1             	mov    %rsi,%rcx
   140001a44:	e8 e7 4d 00 00       	call   140006830 <txrt_stack_error_location>
   140001a49:	89 f9                	mov    %edi,%ecx
   140001a4b:	e8 30 24 00 00       	call   140003e80 <txrt_require_success>
   140001a50:	48 8d 15 89 4b 0a 00 	lea    0xa4b89(%rip),%rdx        # 1400a65e0 <.rdata+0x5e0>
   140001a57:	eb 07                	jmp    140001a60 <tx_fn_m0_scan_0+0x230>
   140001a59:	48 8d 15 e0 4b 0a 00 	lea    0xa4be0(%rip),%rdx        # 1400a6640 <.rdata+0x640>
   140001a60:	41 b8 18 00 00 00    	mov    $0x18,%r8d
   140001a66:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001a6c:	e9 da 00 00 00       	jmp    140001b4b <tx_fn_m0_scan_0+0x31b>
   140001a71:	89 c7                	mov    %eax,%edi
   140001a73:	48 8d 15 46 4d 0a 00 	lea    0xa4d46(%rip),%rdx        # 1400a67c0 <.rdata+0x7c0>
   140001a7a:	41 b8 1e 00 00 00    	mov    $0x1e,%r8d
   140001a80:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001a86:	48 89 f1             	mov    %rsi,%rcx
   140001a89:	e8 a2 4d 00 00       	call   140006830 <txrt_stack_error_location>
   140001a8e:	89 f9                	mov    %edi,%ecx
   140001a90:	e8 eb 23 00 00       	call   140003e80 <txrt_require_success>
   140001a95:	48 b9 ff ff ff ff ff 	movabs $0x7fffffffffffffff,%rcx
   140001a9c:	ff ff 7f
   140001a9f:	4c 8d 84 24 90 00 00 	lea    0x90(%rsp),%r8
   140001aa6:	00
   140001aa7:	ba 01 00 00 00       	mov    $0x1,%edx
   140001aac:	e8 cf 34 00 00       	call   140004f80 <txrt_add_i64>
   140001ab1:	89 c7                	mov    %eax,%edi
   140001ab3:	48 8d 15 e6 4b 0a 00 	lea    0xa4be6(%rip),%rdx        # 1400a66a0 <.rdata+0x6a0>
   140001aba:	41 b8 16 00 00 00    	mov    $0x16,%r8d
   140001ac0:	eb 2b                	jmp    140001aed <tx_fn_m0_scan_0+0x2bd>
   140001ac2:	48 b9 ff ff ff ff ff 	movabs $0x7fffffffffffffff,%rcx
   140001ac9:	ff ff 7f
   140001acc:	4c 8d 84 24 80 00 00 	lea    0x80(%rsp),%r8
   140001ad3:	00
   140001ad4:	ba 01 00 00 00       	mov    $0x1,%edx
   140001ad9:	e8 a2 34 00 00       	call   140004f80 <txrt_add_i64>
   140001ade:	89 c7                	mov    %eax,%edi
   140001ae0:	48 8d 15 99 4d 0a 00 	lea    0xa4d99(%rip),%rdx        # 1400a6880 <.rdata+0x880>
   140001ae7:	41 b8 1c 00 00 00    	mov    $0x1c,%r8d
   140001aed:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001af3:	48 89 f1             	mov    %rsi,%rcx
   140001af6:	e8 35 4d 00 00       	call   140006830 <txrt_stack_error_location>
   140001afb:	89 f9                	mov    %edi,%ecx
   140001afd:	e8 7e 23 00 00       	call   140003e80 <txrt_require_success>
   140001b02:	48 8d 15 17 4a 0a 00 	lea    0xa4a17(%rip),%rdx        # 1400a6520 <.rdata+0x520>
   140001b09:	eb 07                	jmp    140001b12 <tx_fn_m0_scan_0+0x2e2>
   140001b0b:	48 8d 15 6e 4a 0a 00 	lea    0xa4a6e(%rip),%rdx        # 1400a6580 <.rdata+0x580>
   140001b12:	41 b8 15 00 00 00    	mov    $0x15,%r8d
   140001b18:	eb 2b                	jmp    140001b45 <tx_fn_m0_scan_0+0x315>
   140001b1a:	48 8d 15 df 4b 0a 00 	lea    0xa4bdf(%rip),%rdx        # 1400a6700 <.rdata+0x700>
   140001b21:	41 b8 1b 00 00 00    	mov    $0x1b,%r8d
   140001b27:	eb 1c                	jmp    140001b45 <tx_fn_m0_scan_0+0x315>
   140001b29:	48 8d 15 30 4c 0a 00 	lea    0xa4c30(%rip),%rdx        # 1400a6760 <.rdata+0x760>
   140001b30:	41 b8 1b 00 00 00    	mov    $0x1b,%r8d
   140001b36:	eb 0d                	jmp    140001b45 <tx_fn_m0_scan_0+0x315>
   140001b38:	48 8d 15 01 4e 0a 00 	lea    0xa4e01(%rip),%rdx        # 1400a6940 <.rdata+0x940>
   140001b3f:	41 b8 23 00 00 00    	mov    $0x23,%r8d
   140001b45:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001b4b:	48 89 f1             	mov    %rsi,%rcx
   140001b4e:	89 c6                	mov    %eax,%esi
   140001b50:	e8 db 4c 00 00       	call   140006830 <txrt_stack_error_location>
   140001b55:	89 f1                	mov    %esi,%ecx
   140001b57:	e8 24 23 00 00       	call   140003e80 <txrt_require_success>
   140001b5c:	cc                   	int3
   140001b5d:	0f 1f 00             	nopl   (%rax)


E:\Project\other\Compilation\tx_build\performance_12_14\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

00000001400022a0 <tx_fn_m0_arithmetic_0>:
   1400022a0:	41 57                	push   %r15
   1400022a2:	41 56                	push   %r14
   1400022a4:	41 55                	push   %r13
   1400022a6:	41 54                	push   %r12
   1400022a8:	56                   	push   %rsi
   1400022a9:	57                   	push   %rdi
   1400022aa:	55                   	push   %rbp
   1400022ab:	53                   	push   %rbx
   1400022ac:	48 81 ec d8 01 00 00 	sub    $0x1d8,%rsp
   1400022b3:	66 0f 7f b4 24 c0 01 	movdqa %xmm6,0x1c0(%rsp)
   1400022ba:	00 00
   1400022bc:	49 89 cc             	mov    %rcx,%r12
   1400022bf:	48 8b 31             	mov    (%rcx),%rsi
   1400022c2:	48 8d 05 b2 69 0a 00 	lea    0xa69b2(%rip),%rax        # 1400a8c7b <.rdata+0x2c7b>
   1400022c9:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   1400022ce:	48 8d 05 bb 69 0a 00 	lea    0xa69bb(%rip),%rax        # 1400a8c90 <.rdata+0x2c90>
   1400022d5:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   1400022da:	48 c7 44 24 50 49 00 	movq   $0x49,0x50(%rsp)
   1400022e1:	00 00
   1400022e3:	48 c7 44 24 58 01 00 	movq   $0x1,0x58(%rsp)
   1400022ea:	00 00
   1400022ec:	48 89 74 24 60       	mov    %rsi,0x60(%rsp)
   1400022f1:	48 8d 44 24 40       	lea    0x40(%rsp),%rax
   1400022f6:	48 89 01             	mov    %rax,(%rcx)
   1400022f9:	48 8d 8c 24 78 01 00 	lea    0x178(%rsp),%rcx
   140002300:	00
   140002301:	e8 7a c2 02 00       	call   14002e580 <txrt_time_monotonic_micros>
   140002306:	85 c0                	test   %eax,%eax
   140002308:	0f 85 04 0d 00 00    	jne    140003012 <tx_fn_m0_arithmetic_0+0xd72>
   14000230e:	48 8b bc 24 78 01 00 	mov    0x178(%rsp),%rdi
   140002315:	00
   140002316:	4c 89 e1             	mov    %r12,%rcx
   140002319:	e8 82 4c 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   14000231e:	85 c0                	test   %eax,%eax
   140002320:	0f 85 f5 0c 00 00    	jne    14000301b <tx_fn_m0_arithmetic_0+0xd7b>
   140002326:	4c 8d 84 24 80 00 00 	lea    0x80(%rsp),%r8
   14000232d:	00
   14000232e:	b9 01 00 00 00       	mov    $0x1,%ecx
   140002333:	ba 07 00 00 00       	mov    $0x7,%edx
   140002338:	e8 e3 2e 00 00       	call   140005220 <txrt_div_i64>
   14000233d:	85 c0                	test   %eax,%eax
   14000233f:	4c 89 64 24 38       	mov    %r12,0x38(%rsp)
   140002344:	75 4e                	jne    140002394 <tx_fn_m0_arithmetic_0+0xf4>
   140002346:	41 be 02 00 00 00    	mov    $0x2,%r14d
   14000234c:	31 c9                	xor    %ecx,%ecx
   14000234e:	4c 8d bc 24 80 00 00 	lea    0x80(%rsp),%r15
   140002355:	00
   140002356:	31 db                	xor    %ebx,%ebx
   140002358:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
   14000235f:	00
   140002360:	48 8b 94 24 80 00 00 	mov    0x80(%rsp),%rdx
   140002367:	00
   140002368:	48 01 d3             	add    %rdx,%rbx
   14000236b:	0f 80 57 0a 00 00    	jo     140002dc8 <tx_fn_m0_arithmetic_0+0xb28>
   140002371:	49 81 fe 41 42 0f 00 	cmp    $0xf4241,%r14
   140002378:	74 2e                	je     1400023a8 <tx_fn_m0_arithmetic_0+0x108>
   14000237a:	ba 07 00 00 00       	mov    $0x7,%edx
   14000237f:	4c 89 f1             	mov    %r14,%rcx
   140002382:	4d 89 f8             	mov    %r15,%r8
   140002385:	e8 96 2e 00 00       	call   140005220 <txrt_div_i64>
   14000238a:	49 ff c6             	inc    %r14
   14000238d:	48 89 d9             	mov    %rbx,%rcx
   140002390:	85 c0                	test   %eax,%eax
   140002392:	74 cc                	je     140002360 <tx_fn_m0_arithmetic_0+0xc0>
   140002394:	89 c7                	mov    %eax,%edi
   140002396:	48 8d 15 43 4f 0a 00 	lea    0xa4f43(%rip),%rdx        # 1400a72e0 <.rdata+0x12e0>
   14000239d:	41 b8 4f 00 00 00    	mov    $0x4f,%r8d
   1400023a3:	e9 1a 06 00 00       	jmp    1400029c2 <tx_fn_m0_arithmetic_0+0x722>
   1400023a8:	48 89 b4 24 88 00 00 	mov    %rsi,0x88(%rsp)
   1400023af:	00
   1400023b0:	48 8d 0d e4 4f 0a 00 	lea    0xa4fe4(%rip),%rcx        # 1400a739b <.rdata+0x139b>
   1400023b7:	4c 8d 84 24 70 01 00 	lea    0x170(%rsp),%r8
   1400023be:	00
   1400023bf:	ba 08 00 00 00       	mov    $0x8,%edx
   1400023c4:	e8 c7 20 00 00       	call   140004490 <txrt_str_new>
   1400023c9:	85 c0                	test   %eax,%eax
   1400023cb:	0f 85 5c 0c 00 00    	jne    14000302d <tx_fn_m0_arithmetic_0+0xd8d>
   1400023d1:	48 8b b4 24 70 01 00 	mov    0x170(%rsp),%rsi
   1400023d8:	00
   1400023d9:	48 8d 05 30 50 0a 00 	lea    0xa5030(%rip),%rax        # 1400a7410 <.rdata+0x1410>
   1400023e0:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   1400023e5:	48 c7 44 24 50 51 00 	movq   $0x51,0x50(%rsp)
   1400023ec:	00 00
   1400023ee:	48 c7 44 24 58 05 00 	movq   $0x5,0x58(%rsp)
   1400023f5:	00 00
   1400023f7:	4c 89 e1             	mov    %r12,%rcx
   1400023fa:	48 89 f2             	mov    %rsi,%rdx
   1400023fd:	49 89 f8             	mov    %rdi,%r8
   140002400:	49 89 d9             	mov    %rbx,%r9
   140002403:	e8 d8 f1 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140002408:	48 89 f1             	mov    %rsi,%rcx
   14000240b:	e8 30 24 00 00       	call   140004840 <txrt_str_release>
   140002410:	4c 89 e1             	mov    %r12,%rcx
   140002413:	e8 88 4b 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002418:	85 c0                	test   %eax,%eax
   14000241a:	0f 85 1f 0c 00 00    	jne    14000303f <tx_fn_m0_arithmetic_0+0xd9f>
   140002420:	48 8d 8c 24 68 01 00 	lea    0x168(%rsp),%rcx
   140002427:	00
   140002428:	e8 53 c1 02 00       	call   14002e580 <txrt_time_monotonic_micros>
   14000242d:	85 c0                	test   %eax,%eax
   14000242f:	0f 85 1c 0c 00 00    	jne    140003051 <tx_fn_m0_arithmetic_0+0xdb1>
   140002435:	48 8b bc 24 68 01 00 	mov    0x168(%rsp),%rdi
   14000243c:	00
   14000243d:	4c 89 e1             	mov    %r12,%rcx
   140002440:	e8 5b 4b 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002445:	85 c0                	test   %eax,%eax
   140002447:	0f 85 16 0c 00 00    	jne    140003063 <tx_fn_m0_arithmetic_0+0xdc3>
   14000244d:	48 8d 35 3c 51 0a 00 	lea    0xa513c(%rip),%rsi        # 1400a7590 <.rdata+0x1590>
   140002454:	48 89 74 24 48       	mov    %rsi,0x48(%rsp)
   140002459:	48 c7 44 24 50 56 00 	movq   $0x56,0x50(%rsp)
   140002460:	00 00
   140002462:	48 c7 44 24 58 09 00 	movq   $0x9,0x58(%rsp)
   140002469:	00 00
   14000246b:	ba 64 00 00 00       	mov    $0x64,%edx
   140002470:	4c 89 e1             	mov    %r12,%rcx
   140002473:	e8 d8 fb ff ff       	call   140002050 <tx_fn_m0_recursive_0>
   140002478:	41 be 20 4e 00 00    	mov    $0x4e20,%r14d
   14000247e:	66 90                	xchg   %ax,%ax
   140002480:	48 89 c3             	mov    %rax,%rbx
   140002483:	4c 89 e1             	mov    %r12,%rcx
   140002486:	e8 15 4b 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   14000248b:	85 c0                	test   %eax,%eax
   14000248d:	0f 85 56 09 00 00    	jne    140002de9 <tx_fn_m0_arithmetic_0+0xb49>
   140002493:	49 ff ce             	dec    %r14
   140002496:	74 53                	je     1400024eb <tx_fn_m0_arithmetic_0+0x24b>
   140002498:	48 89 74 24 48       	mov    %rsi,0x48(%rsp)
   14000249d:	48 c7 44 24 50 56 00 	movq   $0x56,0x50(%rsp)
   1400024a4:	00 00
   1400024a6:	48 c7 44 24 58 09 00 	movq   $0x9,0x58(%rsp)
   1400024ad:	00 00
   1400024af:	ba 64 00 00 00       	mov    $0x64,%edx
   1400024b4:	4c 89 e1             	mov    %r12,%rcx
   1400024b7:	e8 94 fb ff ff       	call   140002050 <tx_fn_m0_recursive_0>
   1400024bc:	48 89 c2             	mov    %rax,%rdx
   1400024bf:	48 89 d8             	mov    %rbx,%rax
   1400024c2:	48 01 d0             	add    %rdx,%rax
   1400024c5:	71 b9                	jno    140002480 <tx_fn_m0_arithmetic_0+0x1e0>
   1400024c7:	4c 8d 84 24 b0 01 00 	lea    0x1b0(%rsp),%r8
   1400024ce:	00
   1400024cf:	48 89 d9             	mov    %rbx,%rcx
   1400024d2:	e8 a9 2a 00 00       	call   140004f80 <txrt_add_i64>
   1400024d7:	89 c6                	mov    %eax,%esi
   1400024d9:	48 8d 15 10 51 0a 00 	lea    0xa5110(%rip),%rdx        # 1400a75f0 <.rdata+0x15f0>
   1400024e0:	41 b8 56 00 00 00    	mov    $0x56,%r8d
   1400024e6:	e9 fa 0a 00 00       	jmp    140002fe5 <tx_fn_m0_arithmetic_0+0xd45>
   1400024eb:	48 8d 0d b9 51 0a 00 	lea    0xa51b9(%rip),%rcx        # 1400a76ab <.rdata+0x16ab>
   1400024f2:	4c 8d 84 24 60 01 00 	lea    0x160(%rsp),%r8
   1400024f9:	00
   1400024fa:	ba 09 00 00 00       	mov    $0x9,%edx
   1400024ff:	e8 8c 1f 00 00       	call   140004490 <txrt_str_new>
   140002504:	85 c0                	test   %eax,%eax
   140002506:	0f 85 69 0b 00 00    	jne    140003075 <tx_fn_m0_arithmetic_0+0xdd5>
   14000250c:	48 8b b4 24 60 01 00 	mov    0x160(%rsp),%rsi
   140002513:	00
   140002514:	48 8d 05 05 52 0a 00 	lea    0xa5205(%rip),%rax        # 1400a7720 <.rdata+0x1720>
   14000251b:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   140002520:	48 c7 44 24 50 58 00 	movq   $0x58,0x50(%rsp)
   140002527:	00 00
   140002529:	48 c7 44 24 58 05 00 	movq   $0x5,0x58(%rsp)
   140002530:	00 00
   140002532:	4c 89 e1             	mov    %r12,%rcx
   140002535:	48 89 f2             	mov    %rsi,%rdx
   140002538:	49 89 f8             	mov    %rdi,%r8
   14000253b:	49 89 d9             	mov    %rbx,%r9
   14000253e:	e8 9d f0 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140002543:	48 89 f1             	mov    %rsi,%rcx
   140002546:	e8 f5 22 00 00       	call   140004840 <txrt_str_release>
   14000254b:	4c 89 e1             	mov    %r12,%rcx
   14000254e:	e8 4d 4a 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002553:	85 c0                	test   %eax,%eax
   140002555:	0f 85 2c 0b 00 00    	jne    140003087 <tx_fn_m0_arithmetic_0+0xde7>
   14000255b:	48 8d 8c 24 58 01 00 	lea    0x158(%rsp),%rcx
   140002562:	00
   140002563:	e8 18 c0 02 00       	call   14002e580 <txrt_time_monotonic_micros>
   140002568:	85 c0                	test   %eax,%eax
   14000256a:	0f 85 29 0b 00 00    	jne    140003099 <tx_fn_m0_arithmetic_0+0xdf9>
   140002570:	48 8b b4 24 58 01 00 	mov    0x158(%rsp),%rsi
   140002577:	00
   140002578:	4c 89 e1             	mov    %r12,%rcx
   14000257b:	e8 20 4a 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002580:	85 c0                	test   %eax,%eax
   140002582:	0f 85 23 0b 00 00    	jne    1400030ab <tx_fn_m0_arithmetic_0+0xe0b>
   140002588:	48 89 b4 24 90 00 00 	mov    %rsi,0x90(%rsp)
   14000258f:	00
   140002590:	48 8d 54 24 78       	lea    0x78(%rsp),%rdx
   140002595:	31 c9                	xor    %ecx,%ecx
   140002597:	e8 b4 73 02 00       	call   140029950 <txrt_array_new>
   14000259c:	85 c0                	test   %eax,%eax
   14000259e:	0f 85 0f 04 00 00    	jne    1400029b3 <tx_fn_m0_arithmetic_0+0x713>
   1400025a4:	bb 01 00 00 00       	mov    $0x1,%ebx
   1400025a9:	31 f6                	xor    %esi,%esi
   1400025ab:	48 8d 05 db 57 0a 00 	lea    0xa57db(%rip),%rax        # 1400a7d8d <.rdata+0x1d8d>
   1400025b2:	66 48 0f 6e c0       	movq   %rax,%xmm0
   1400025b7:	48 8d 05 cd 57 0a 00 	lea    0xa57cd(%rip),%rax        # 1400a7d8b <.rdata+0x1d8b>
   1400025be:	66 48 0f 6e f0       	movq   %rax,%xmm6
   1400025c3:	66 0f 6c f0          	punpcklqdq %xmm0,%xmm6
   1400025c7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
   1400025ce:	00 00
   1400025d0:	48 8b 7c 24 78       	mov    0x78(%rsp),%rdi
   1400025d5:	48 8d 8c 24 50 01 00 	lea    0x150(%rsp),%rcx
   1400025dc:	00
   1400025dd:	e8 6e 80 04 00       	call   14004a650 <txrt_dict_new>
   1400025e2:	85 c0                	test   %eax,%eax
   1400025e4:	0f 85 17 08 00 00    	jne    140002e01 <tx_fn_m0_arithmetic_0+0xb61>
   1400025ea:	4c 8b b4 24 50 01 00 	mov    0x150(%rsp),%r14
   1400025f1:	00
   1400025f2:	b9 03 00 00 00       	mov    $0x3,%ecx
   1400025f7:	48 8d 94 24 48 01 00 	lea    0x148(%rsp),%rdx
   1400025fe:	00
   1400025ff:	e8 4c 73 02 00       	call   140029950 <txrt_array_new>
   140002604:	85 c0                	test   %eax,%eax
   140002606:	0f 85 01 08 00 00    	jne    140002e0d <tx_fn_m0_arithmetic_0+0xb6d>
   14000260c:	4c 8b a4 24 48 01 00 	mov    0x148(%rsp),%r12
   140002613:	00
   140002614:	4c 89 e1             	mov    %r12,%rcx
   140002617:	e8 d4 55 02 00       	call   140027bf0 <txrt_array_ref>
   14000261c:	49 89 c5             	mov    %rax,%r13
   14000261f:	48 89 c1             	mov    %rax,%rcx
   140002622:	31 d2                	xor    %edx,%edx
   140002624:	49 89 d8             	mov    %rbx,%r8
   140002627:	e8 04 63 02 00       	call   140028930 <txrt_array_ref_set_i64>
   14000262c:	85 c0                	test   %eax,%eax
   14000262e:	0f 85 e5 07 00 00    	jne    140002e19 <tx_fn_m0_arithmetic_0+0xb79>
   140002634:	ba 01 00 00 00       	mov    $0x1,%edx
   140002639:	41 b8 02 00 00 00    	mov    $0x2,%r8d
   14000263f:	4c 89 e9             	mov    %r13,%rcx
   140002642:	e8 e9 62 02 00       	call   140028930 <txrt_array_ref_set_i64>
   140002647:	85 c0                	test   %eax,%eax
   140002649:	0f 85 d6 07 00 00    	jne    140002e25 <tx_fn_m0_arithmetic_0+0xb85>
   14000264f:	ba 02 00 00 00       	mov    $0x2,%edx
   140002654:	41 b8 03 00 00 00    	mov    $0x3,%r8d
   14000265a:	4c 89 e9             	mov    %r13,%rcx
   14000265d:	e8 ce 62 02 00       	call   140028930 <txrt_array_ref_set_i64>
   140002662:	85 c0                	test   %eax,%eax
   140002664:	0f 85 c7 07 00 00    	jne    140002e31 <tx_fn_m0_arithmetic_0+0xb91>
   14000266a:	48 89 f9             	mov    %rdi,%rcx
   14000266d:	4c 89 e2             	mov    %r12,%rdx
   140002670:	e8 5b 87 02 00       	call   14002add0 <txrt_array_extend>
   140002675:	85 c0                	test   %eax,%eax
   140002677:	0f 85 c0 07 00 00    	jne    140002e3d <tx_fn_m0_arithmetic_0+0xb9d>
   14000267d:	4c 89 e1             	mov    %r12,%rcx
   140002680:	e8 1b cf 02 00       	call   14002f5a0 <txrt_value_release>
   140002685:	48 8d 8c 24 40 01 00 	lea    0x140(%rsp),%rcx
   14000268c:	00
   14000268d:	e8 be 7f 04 00       	call   14004a650 <txrt_dict_new>
   140002692:	85 c0                	test   %eax,%eax
   140002694:	0f 85 af 07 00 00    	jne    140002e49 <tx_fn_m0_arithmetic_0+0xba9>
   14000269a:	4c 8b a4 24 40 01 00 	mov    0x140(%rsp),%r12
   1400026a1:	00
   1400026a2:	ba 05 00 00 00       	mov    $0x5,%edx
   1400026a7:	48 8d 0d ed 54 0a 00 	lea    0xa54ed(%rip),%rcx        # 1400a7b9b <.rdata+0x1b9b>
   1400026ae:	4c 8d 84 24 38 01 00 	lea    0x138(%rsp),%r8
   1400026b5:	00
   1400026b6:	e8 d5 1d 00 00       	call   140004490 <txrt_str_new>
   1400026bb:	85 c0                	test   %eax,%eax
   1400026bd:	0f 85 92 07 00 00    	jne    140002e55 <tx_fn_m0_arithmetic_0+0xbb5>
   1400026c3:	48 89 74 24 68       	mov    %rsi,0x68(%rsp)
   1400026c8:	4c 8b ac 24 38 01 00 	mov    0x138(%rsp),%r13
   1400026cf:	00
   1400026d0:	4c 89 e9             	mov    %r13,%rcx
   1400026d3:	48 8d 94 24 30 01 00 	lea    0x130(%rsp),%rdx
   1400026da:	00
   1400026db:	e8 30 cc 02 00       	call   14002f310 <txrt_value_box_str>
   1400026e0:	85 c0                	test   %eax,%eax
   1400026e2:	0f 85 79 07 00 00    	jne    140002e61 <tx_fn_m0_arithmetic_0+0xbc1>
   1400026e8:	48 8b ac 24 30 01 00 	mov    0x130(%rsp),%rbp
   1400026ef:	00
   1400026f0:	b9 04 00 00 00       	mov    $0x4,%ecx
   1400026f5:	48 8d 94 24 28 01 00 	lea    0x128(%rsp),%rdx
   1400026fc:	00
   1400026fd:	e8 6e c8 02 00       	call   14002ef70 <txrt_value_box_i64>
   140002702:	85 c0                	test   %eax,%eax
   140002704:	0f 85 63 07 00 00    	jne    140002e6d <tx_fn_m0_arithmetic_0+0xbcd>
   14000270a:	48 8b b4 24 28 01 00 	mov    0x128(%rsp),%rsi
   140002711:	00
   140002712:	4c 89 e1             	mov    %r12,%rcx
   140002715:	48 89 ea             	mov    %rbp,%rdx
   140002718:	49 89 f0             	mov    %rsi,%r8
   14000271b:	e8 50 61 04 00       	call   140048870 <txrt_dict_set>
   140002720:	85 c0                	test   %eax,%eax
   140002722:	0f 85 6b 07 00 00    	jne    140002e93 <tx_fn_m0_arithmetic_0+0xbf3>
   140002728:	48 89 e9             	mov    %rbp,%rcx
   14000272b:	e8 70 ce 02 00       	call   14002f5a0 <txrt_value_release>
   140002730:	48 89 f1             	mov    %rsi,%rcx
   140002733:	e8 68 ce 02 00       	call   14002f5a0 <txrt_value_release>
   140002738:	4c 89 e9             	mov    %r13,%rcx
   14000273b:	e8 00 21 00 00       	call   140004840 <txrt_str_release>
   140002740:	4c 89 f1             	mov    %r14,%rcx
   140002743:	4c 89 e2             	mov    %r12,%rdx
   140002746:	e8 95 a1 04 00       	call   14004c8e0 <txrt_keyword_merge>
   14000274b:	85 c0                	test   %eax,%eax
   14000274d:	0f 85 68 07 00 00    	jne    140002ebb <tx_fn_m0_arithmetic_0+0xc1b>
   140002753:	4c 89 e1             	mov    %r12,%rcx
   140002756:	e8 45 ce 02 00       	call   14002f5a0 <txrt_value_release>
   14000275b:	66 0f 7f b4 24 90 01 	movdqa %xmm6,0x190(%rsp)
   140002762:	00 00
   140002764:	48 8d 84 24 20 01 00 	lea    0x120(%rsp),%rax
   14000276b:	00
   14000276c:	48 89 44 24 30       	mov    %rax,0x30(%rsp)
   140002771:	c7 44 24 28 01 00 00 	movl   $0x1,0x28(%rsp)
   140002778:	00
   140002779:	c7 44 24 20 01 00 00 	movl   $0x1,0x20(%rsp)
   140002780:	00
   140002781:	41 b9 02 00 00 00    	mov    $0x2,%r9d
   140002787:	48 89 f9             	mov    %rdi,%rcx
   14000278a:	4c 89 f2             	mov    %r14,%rdx
   14000278d:	4c 8d 84 24 90 01 00 	lea    0x190(%rsp),%r8
   140002794:	00
   140002795:	e8 96 bd 04 00       	call   14004e530 <txrt_call_bind>
   14000279a:	85 c0                	test   %eax,%eax
   14000279c:	0f 85 22 07 00 00    	jne    140002ec4 <tx_fn_m0_arithmetic_0+0xc24>
   1400027a2:	48 89 f9             	mov    %rdi,%rcx
   1400027a5:	e8 f6 cd 02 00       	call   14002f5a0 <txrt_value_release>
   1400027aa:	4c 89 f1             	mov    %r14,%rcx
   1400027ad:	e8 ee cd 02 00       	call   14002f5a0 <txrt_value_release>
   1400027b2:	48 8b bc 24 20 01 00 	mov    0x120(%rsp),%rdi
   1400027b9:	00
   1400027ba:	48 89 f9             	mov    %rdi,%rcx
   1400027bd:	31 d2                	xor    %edx,%edx
   1400027bf:	4c 8d 84 24 18 01 00 	lea    0x118(%rsp),%r8
   1400027c6:	00
   1400027c7:	e8 a4 5d 02 00       	call   140028570 <txrt_array_element_address>
   1400027cc:	85 c0                	test   %eax,%eax
   1400027ce:	0f 85 f9 06 00 00    	jne    140002ecd <tx_fn_m0_arithmetic_0+0xc2d>
   1400027d4:	48 8b b4 24 18 01 00 	mov    0x118(%rsp),%rsi
   1400027db:	00
   1400027dc:	48 89 f1             	mov    %rsi,%rcx
   1400027df:	48 8d 15 65 56 0a 00 	lea    0xa5665(%rip),%rdx        # 1400a7e4b <.rdata+0x1e4b>
   1400027e6:	e8 b5 1d 03 00       	call   1400345a0 <txrt_value_require_type>
   1400027eb:	85 c0                	test   %eax,%eax
   1400027ed:	0f 85 e3 06 00 00    	jne    140002ed6 <tx_fn_m0_arithmetic_0+0xc36>
   1400027f3:	48 89 f1             	mov    %rsi,%rcx
   1400027f6:	e8 25 d5 02 00       	call   14002fd20 <txrt_value_to_i64_fast>
   1400027fb:	49 89 c6             	mov    %rax,%r14
   1400027fe:	ba 01 00 00 00       	mov    $0x1,%edx
   140002803:	48 89 f9             	mov    %rdi,%rcx
   140002806:	4c 8d 84 24 10 01 00 	lea    0x110(%rsp),%r8
   14000280d:	00
   14000280e:	e8 5d 5d 02 00       	call   140028570 <txrt_array_element_address>
   140002813:	85 c0                	test   %eax,%eax
   140002815:	0f 85 c4 06 00 00    	jne    140002edf <tx_fn_m0_arithmetic_0+0xc3f>
   14000281b:	48 8b b4 24 10 01 00 	mov    0x110(%rsp),%rsi
   140002822:	00
   140002823:	48 89 f1             	mov    %rsi,%rcx
   140002826:	48 8d 15 de 56 0a 00 	lea    0xa56de(%rip),%rdx        # 1400a7f0b <.rdata+0x1f0b>
   14000282d:	e8 6e 1d 03 00       	call   1400345a0 <txrt_value_require_type>
   140002832:	85 c0                	test   %eax,%eax
   140002834:	0f 85 ae 06 00 00    	jne    140002ee8 <tx_fn_m0_arithmetic_0+0xc48>
   14000283a:	48 89 f1             	mov    %rsi,%rcx
   14000283d:	e8 de d4 02 00       	call   14002fd20 <txrt_value_to_i64_fast>
   140002842:	49 89 c4             	mov    %rax,%r12
   140002845:	ba 02 00 00 00       	mov    $0x2,%edx
   14000284a:	48 89 f9             	mov    %rdi,%rcx
   14000284d:	4c 8d 84 24 08 01 00 	lea    0x108(%rsp),%r8
   140002854:	00
   140002855:	e8 16 5d 02 00       	call   140028570 <txrt_array_element_address>
   14000285a:	85 c0                	test   %eax,%eax
   14000285c:	0f 85 8f 06 00 00    	jne    140002ef1 <tx_fn_m0_arithmetic_0+0xc51>
   140002862:	4c 8b ac 24 08 01 00 	mov    0x108(%rsp),%r13
   140002869:	00
   14000286a:	4c 89 e9             	mov    %r13,%rcx
   14000286d:	48 8d 15 57 57 0a 00 	lea    0xa5757(%rip),%rdx        # 1400a7fcb <.rdata+0x1fcb>
   140002874:	e8 27 1d 03 00       	call   1400345a0 <txrt_value_require_type>
   140002879:	85 c0                	test   %eax,%eax
   14000287b:	0f 85 79 06 00 00    	jne    140002efa <tx_fn_m0_arithmetic_0+0xc5a>
   140002881:	4c 89 e9             	mov    %r13,%rcx
   140002884:	48 8d 15 b0 57 0a 00 	lea    0xa57b0(%rip),%rdx        # 1400a803b <.rdata+0x203b>
   14000288b:	e8 10 1d 03 00       	call   1400345a0 <txrt_value_require_type>
   140002890:	85 c0                	test   %eax,%eax
   140002892:	0f 85 6b 06 00 00    	jne    140002f03 <tx_fn_m0_arithmetic_0+0xc63>
   140002898:	4c 89 e9             	mov    %r13,%rcx
   14000289b:	48 8d 94 24 00 01 00 	lea    0x100(%rsp),%rdx
   1400028a2:	00
   1400028a3:	e8 68 cb 02 00       	call   14002f410 <txrt_value_clone>
   1400028a8:	85 c0                	test   %eax,%eax
   1400028aa:	0f 85 5c 06 00 00    	jne    140002f0c <tx_fn_m0_arithmetic_0+0xc6c>
   1400028b0:	4c 8b ac 24 00 01 00 	mov    0x100(%rsp),%r13
   1400028b7:	00
   1400028b8:	ba 03 00 00 00       	mov    $0x3,%edx
   1400028bd:	48 89 f9             	mov    %rdi,%rcx
   1400028c0:	4c 8d 84 24 f8 00 00 	lea    0xf8(%rsp),%r8
   1400028c7:	00
   1400028c8:	e8 a3 5c 02 00       	call   140028570 <txrt_array_element_address>
   1400028cd:	85 c0                	test   %eax,%eax
   1400028cf:	0f 85 40 06 00 00    	jne    140002f15 <tx_fn_m0_arithmetic_0+0xc75>
   1400028d5:	48 8b ac 24 f8 00 00 	mov    0xf8(%rsp),%rbp
   1400028dc:	00
   1400028dd:	48 89 e9             	mov    %rbp,%rcx
   1400028e0:	48 8d 15 84 58 0a 00 	lea    0xa5884(%rip),%rdx        # 1400a816b <.rdata+0x216b>
   1400028e7:	e8 b4 1c 03 00       	call   1400345a0 <txrt_value_require_type>
   1400028ec:	85 c0                	test   %eax,%eax
   1400028ee:	0f 85 2a 06 00 00    	jne    140002f1e <tx_fn_m0_arithmetic_0+0xc7e>
   1400028f4:	48 89 e9             	mov    %rbp,%rcx
   1400028f7:	48 8d 15 cd 58 0a 00 	lea    0xa58cd(%rip),%rdx        # 1400a81cb <.rdata+0x21cb>
   1400028fe:	e8 9d 1c 03 00       	call   1400345a0 <txrt_value_require_type>
   140002903:	85 c0                	test   %eax,%eax
   140002905:	0f 85 1c 06 00 00    	jne    140002f27 <tx_fn_m0_arithmetic_0+0xc87>
   14000290b:	48 89 e9             	mov    %rbp,%rcx
   14000290e:	48 8d 94 24 f0 00 00 	lea    0xf0(%rsp),%rdx
   140002915:	00
   140002916:	e8 f5 ca 02 00       	call   14002f410 <txrt_value_clone>
   14000291b:	85 c0                	test   %eax,%eax
   14000291d:	0f 85 0d 06 00 00    	jne    140002f30 <tx_fn_m0_arithmetic_0+0xc90>
   140002923:	48 8b b4 24 f0 00 00 	mov    0xf0(%rsp),%rsi
   14000292a:	00
   14000292b:	48 89 f9             	mov    %rdi,%rcx
   14000292e:	e8 6d cc 02 00       	call   14002f5a0 <txrt_value_release>
   140002933:	48 8d 05 56 59 0a 00 	lea    0xa5956(%rip),%rax        # 1400a8290 <.rdata+0x2290>
   14000293a:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   14000293f:	48 c7 44 24 50 5d 00 	movq   $0x5d,0x50(%rsp)
   140002946:	00 00
   140002948:	48 c7 44 24 58 09 00 	movq   $0x9,0x58(%rsp)
   14000294f:	00 00
   140002951:	48 89 74 24 20       	mov    %rsi,0x20(%rsp)
   140002956:	4c 8b 7c 24 38       	mov    0x38(%rsp),%r15
   14000295b:	4c 89 f9             	mov    %r15,%rcx
   14000295e:	4c 89 f2             	mov    %r14,%rdx
   140002961:	4d 89 e0             	mov    %r12,%r8
   140002964:	4d 89 e9             	mov    %r13,%r9
   140002967:	e8 f4 f7 ff ff       	call   140002160 <tx_fn_m0_combine_0>
   14000296c:	48 8b 4c 24 68       	mov    0x68(%rsp),%rcx
   140002971:	48 89 cf             	mov    %rcx,%rdi
   140002974:	48 01 c7             	add    %rax,%rdi
   140002977:	0f 80 d9 05 00 00    	jo     140002f56 <tx_fn_m0_arithmetic_0+0xcb6>
   14000297d:	4c 89 f9             	mov    %r15,%rcx
   140002980:	e8 1b 46 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002985:	85 c0                	test   %eax,%eax
   140002987:	4d 89 fc             	mov    %r15,%r12
   14000298a:	0f 85 fa 05 00 00    	jne    140002f8a <tx_fn_m0_arithmetic_0+0xcea>
   140002990:	48 ff c3             	inc    %rbx
   140002993:	48 81 fb 21 4e 00 00 	cmp    $0x4e21,%rbx
   14000299a:	74 3d                	je     1400029d9 <tx_fn_m0_arithmetic_0+0x739>
   14000299c:	31 c9                	xor    %ecx,%ecx
   14000299e:	48 8d 54 24 78       	lea    0x78(%rsp),%rdx
   1400029a3:	e8 a8 6f 02 00       	call   140029950 <txrt_array_new>
   1400029a8:	48 89 fe             	mov    %rdi,%rsi
   1400029ab:	85 c0                	test   %eax,%eax
   1400029ad:	0f 84 1d fc ff ff    	je     1400025d0 <tx_fn_m0_arithmetic_0+0x330>
   1400029b3:	89 c7                	mov    %eax,%edi
   1400029b5:	48 8d 15 e4 4e 0a 00 	lea    0xa4ee4(%rip),%rdx        # 1400a78a0 <.rdata+0x18a0>
   1400029bc:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   1400029c2:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400029c8:	48 8b 4c 24 38       	mov    0x38(%rsp),%rcx
   1400029cd:	e8 5e 3e 00 00       	call   140006830 <txrt_stack_error_location>
   1400029d2:	89 f9                	mov    %edi,%ecx
   1400029d4:	e8 a7 14 00 00       	call   140003e80 <txrt_require_success>
   1400029d9:	48 8d 0d cb 59 0a 00 	lea    0xa59cb(%rip),%rcx        # 1400a83ab <.rdata+0x23ab>
   1400029e0:	4c 8d 84 24 e8 00 00 	lea    0xe8(%rsp),%r8
   1400029e7:	00
   1400029e8:	ba 0d 00 00 00       	mov    $0xd,%edx
   1400029ed:	e8 9e 1a 00 00       	call   140004490 <txrt_str_new>
   1400029f2:	85 c0                	test   %eax,%eax
   1400029f4:	0f 85 c3 06 00 00    	jne    1400030bd <tx_fn_m0_arithmetic_0+0xe1d>
   1400029fa:	48 8b b4 24 e8 00 00 	mov    0xe8(%rsp),%rsi
   140002a01:	00
   140002a02:	48 8d 05 17 5a 0a 00 	lea    0xa5a17(%rip),%rax        # 1400a8420 <.rdata+0x2420>
   140002a09:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   140002a0e:	48 c7 44 24 50 5f 00 	movq   $0x5f,0x50(%rsp)
   140002a15:	00 00
   140002a17:	48 c7 44 24 58 05 00 	movq   $0x5,0x58(%rsp)
   140002a1e:	00 00
   140002a20:	4c 89 e1             	mov    %r12,%rcx
   140002a23:	48 89 f2             	mov    %rsi,%rdx
   140002a26:	4c 8b 84 24 90 00 00 	mov    0x90(%rsp),%r8
   140002a2d:	00
   140002a2e:	49 89 f9             	mov    %rdi,%r9
   140002a31:	e8 aa eb ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140002a36:	48 89 f1             	mov    %rsi,%rcx
   140002a39:	e8 02 1e 00 00       	call   140004840 <txrt_str_release>
   140002a3e:	4c 89 e1             	mov    %r12,%rcx
   140002a41:	e8 5a 45 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002a46:	85 c0                	test   %eax,%eax
   140002a48:	0f 85 81 06 00 00    	jne    1400030cf <tx_fn_m0_arithmetic_0+0xe2f>
   140002a4e:	48 8d 94 24 e0 00 00 	lea    0xe0(%rsp),%rdx
   140002a55:	00
   140002a56:	b9 01 00 00 00       	mov    $0x1,%ecx
   140002a5b:	e8 f0 6e 02 00       	call   140029950 <txrt_array_new>
   140002a60:	85 c0                	test   %eax,%eax
   140002a62:	0f 85 79 06 00 00    	jne    1400030e1 <tx_fn_m0_arithmetic_0+0xe41>
   140002a68:	4c 8b bc 24 e0 00 00 	mov    0xe0(%rsp),%r15
   140002a6f:	00
   140002a70:	4c 89 f9             	mov    %r15,%rcx
   140002a73:	e8 78 51 02 00       	call   140027bf0 <txrt_array_ref>
   140002a78:	41 b8 03 00 00 00    	mov    $0x3,%r8d
   140002a7e:	48 89 c1             	mov    %rax,%rcx
   140002a81:	31 d2                	xor    %edx,%edx
   140002a83:	e8 a8 5e 02 00       	call   140028930 <txrt_array_ref_set_i64>
   140002a88:	85 c0                	test   %eax,%eax
   140002a8a:	0f 85 63 06 00 00    	jne    1400030f3 <tx_fn_m0_arithmetic_0+0xe53>
   140002a90:	4c 89 f9             	mov    %r15,%rcx
   140002a93:	e8 58 51 02 00       	call   140027bf0 <txrt_array_ref>
   140002a98:	4c 89 e1             	mov    %r12,%rcx
   140002a9b:	e8 00 45 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002aa0:	85 c0                	test   %eax,%eax
   140002aa2:	0f 85 5d 06 00 00    	jne    140003105 <tx_fn_m0_arithmetic_0+0xe65>
   140002aa8:	48 8d 8c 24 d8 00 00 	lea    0xd8(%rsp),%rcx
   140002aaf:	00
   140002ab0:	e8 9b 7b 04 00       	call   14004a650 <txrt_dict_new>
   140002ab5:	85 c0                	test   %eax,%eax
   140002ab7:	0f 85 5a 06 00 00    	jne    140003117 <tx_fn_m0_arithmetic_0+0xe77>
   140002abd:	48 8b ac 24 d8 00 00 	mov    0xd8(%rsp),%rbp
   140002ac4:	00
   140002ac5:	48 8d 0d 8f 5b 0a 00 	lea    0xa5b8f(%rip),%rcx        # 1400a865b <.rdata+0x265b>
   140002acc:	4c 8d 84 24 d0 00 00 	lea    0xd0(%rsp),%r8
   140002ad3:	00
   140002ad4:	ba 05 00 00 00       	mov    $0x5,%edx
   140002ad9:	e8 b2 19 00 00       	call   140004490 <txrt_str_new>
   140002ade:	85 c0                	test   %eax,%eax
   140002ae0:	0f 85 43 06 00 00    	jne    140003129 <tx_fn_m0_arithmetic_0+0xe89>
   140002ae6:	48 8b bc 24 d0 00 00 	mov    0xd0(%rsp),%rdi
   140002aed:	00
   140002aee:	48 8d 94 24 c8 00 00 	lea    0xc8(%rsp),%rdx
   140002af5:	00
   140002af6:	48 89 f9             	mov    %rdi,%rcx
   140002af9:	e8 12 c8 02 00       	call   14002f310 <txrt_value_box_str>
   140002afe:	85 c0                	test   %eax,%eax
   140002b00:	0f 85 32 06 00 00    	jne    140003138 <tx_fn_m0_arithmetic_0+0xe98>
   140002b06:	4c 8b b4 24 c8 00 00 	mov    0xc8(%rsp),%r14
   140002b0d:	00
   140002b0e:	48 8d 94 24 c0 00 00 	lea    0xc0(%rsp),%rdx
   140002b15:	00
   140002b16:	b9 04 00 00 00       	mov    $0x4,%ecx
   140002b1b:	e8 50 c4 02 00       	call   14002ef70 <txrt_value_box_i64>
   140002b20:	85 c0                	test   %eax,%eax
   140002b22:	0f 85 1f 06 00 00    	jne    140003147 <tx_fn_m0_arithmetic_0+0xea7>
   140002b28:	48 8b b4 24 c0 00 00 	mov    0xc0(%rsp),%rsi
   140002b2f:	00
   140002b30:	48 89 e9             	mov    %rbp,%rcx
   140002b33:	4c 89 f2             	mov    %r14,%rdx
   140002b36:	49 89 f0             	mov    %rsi,%r8
   140002b39:	e8 32 5d 04 00       	call   140048870 <txrt_dict_set>
   140002b3e:	85 c0                	test   %eax,%eax
   140002b40:	0f 85 10 06 00 00    	jne    140003156 <tx_fn_m0_arithmetic_0+0xeb6>
   140002b46:	4c 89 f1             	mov    %r14,%rcx
   140002b49:	e8 52 ca 02 00       	call   14002f5a0 <txrt_value_release>
   140002b4e:	48 89 f1             	mov    %rsi,%rcx
   140002b51:	e8 4a ca 02 00       	call   14002f5a0 <txrt_value_release>
   140002b56:	48 89 f9             	mov    %rdi,%rcx
   140002b59:	e8 e2 1c 00 00       	call   140004840 <txrt_str_release>
   140002b5e:	48 89 e9             	mov    %rbp,%rcx
   140002b61:	e8 1a 7d 04 00       	call   14004a880 <txrt_dict_ref>
   140002b66:	4c 89 e1             	mov    %r12,%rcx
   140002b69:	e8 32 44 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002b6e:	85 c0                	test   %eax,%eax
   140002b70:	0f 85 ef 05 00 00    	jne    140003165 <tx_fn_m0_arithmetic_0+0xec5>
   140002b76:	48 8d 8c 24 b8 00 00 	lea    0xb8(%rsp),%rcx
   140002b7d:	00
   140002b7e:	e8 fd b9 02 00       	call   14002e580 <txrt_time_monotonic_micros>
   140002b83:	85 c0                	test   %eax,%eax
   140002b85:	0f 85 e9 05 00 00    	jne    140003174 <tx_fn_m0_arithmetic_0+0xed4>
   140002b8b:	48 8b b4 24 b8 00 00 	mov    0xb8(%rsp),%rsi
   140002b92:	00
   140002b93:	4c 89 e1             	mov    %r12,%rcx
   140002b96:	e8 05 44 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002b9b:	85 c0                	test   %eax,%eax
   140002b9d:	0f 85 e0 05 00 00    	jne    140003183 <tx_fn_m0_arithmetic_0+0xee3>
   140002ba3:	48 89 74 24 68       	mov    %rsi,0x68(%rsp)
   140002ba8:	48 8d 54 24 70       	lea    0x70(%rsp),%rdx
   140002bad:	4c 89 fb             	mov    %r15,%rbx
   140002bb0:	4c 89 f9             	mov    %r15,%rcx
   140002bb3:	e8 58 c8 02 00       	call   14002f410 <txrt_value_clone>
   140002bb8:	85 c0                	test   %eax,%eax
   140002bba:	0f 85 36 01 00 00    	jne    140002cf6 <tx_fn_m0_arithmetic_0+0xa56>
   140002bc0:	41 bf 01 00 00 00    	mov    $0x1,%r15d
   140002bc6:	31 ff                	xor    %edi,%edi
   140002bc8:	48 8d 05 fe 5d 0a 00 	lea    0xa5dfe(%rip),%rax        # 1400a89cd <.rdata+0x29cd>
   140002bcf:	66 48 0f 6e c0       	movq   %rax,%xmm0
   140002bd4:	48 8d 05 f0 5d 0a 00 	lea    0xa5df0(%rip),%rax        # 1400a89cb <.rdata+0x29cb>
   140002bdb:	66 48 0f 6e f0       	movq   %rax,%xmm6
   140002be0:	66 0f 6c f0          	punpcklqdq %xmm0,%xmm6
   140002be4:	4c 8d ac 24 80 01 00 	lea    0x180(%rsp),%r13
   140002beb:	00
   140002bec:	48 8d 35 3d 5e 0a 00 	lea    0xa5e3d(%rip),%rsi        # 1400a8a30 <.rdata+0x2a30>
   140002bf3:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140002bfa:	84 00 00 00 00 00
   140002c00:	4c 8b 74 24 70       	mov    0x70(%rsp),%r14
   140002c05:	48 89 e9             	mov    %rbp,%rcx
   140002c08:	48 8d 94 24 b0 00 00 	lea    0xb0(%rsp),%rdx
   140002c0f:	00
   140002c10:	e8 fb c7 02 00       	call   14002f410 <txrt_value_clone>
   140002c15:	85 c0                	test   %eax,%eax
   140002c17:	0f 85 85 03 00 00    	jne    140002fa2 <tx_fn_m0_arithmetic_0+0xd02>
   140002c1d:	4c 8b a4 24 b0 00 00 	mov    0xb0(%rsp),%r12
   140002c24:	00
   140002c25:	66 0f 7f b4 24 80 01 	movdqa %xmm6,0x180(%rsp)
   140002c2c:	00 00
   140002c2e:	48 8d 84 24 a0 00 00 	lea    0xa0(%rsp),%rax
   140002c35:	00
   140002c36:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   140002c3b:	48 8d 84 24 a8 00 00 	lea    0xa8(%rsp),%rax
   140002c42:	00
   140002c43:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   140002c48:	41 b9 02 00 00 00    	mov    $0x2,%r9d
   140002c4e:	4c 89 f1             	mov    %r14,%rcx
   140002c51:	4c 89 e2             	mov    %r12,%rdx
   140002c54:	4d 89 e8             	mov    %r13,%r8
   140002c57:	e8 54 a3 04 00       	call   14004cfb0 <txrt_call_split_spreads>
   140002c5c:	85 c0                	test   %eax,%eax
   140002c5e:	0f 85 4d 03 00 00    	jne    140002fb1 <tx_fn_m0_arithmetic_0+0xd11>
   140002c64:	4c 89 f1             	mov    %r14,%rcx
   140002c67:	e8 34 c9 02 00       	call   14002f5a0 <txrt_value_release>
   140002c6c:	4c 89 e1             	mov    %r12,%rcx
   140002c6f:	e8 2c c9 02 00       	call   14002f5a0 <txrt_value_release>
   140002c74:	4c 8b 8c 24 a8 00 00 	mov    0xa8(%rsp),%r9
   140002c7b:	00
   140002c7c:	48 8b 84 24 a0 00 00 	mov    0xa0(%rsp),%rax
   140002c83:	00
   140002c84:	48 89 74 24 48       	mov    %rsi,0x48(%rsp)
   140002c89:	48 c7 44 24 50 66 00 	movq   $0x66,0x50(%rsp)
   140002c90:	00 00
   140002c92:	48 c7 44 24 58 09 00 	movq   $0x9,0x58(%rsp)
   140002c99:	00 00
   140002c9b:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   140002ca0:	41 b8 02 00 00 00    	mov    $0x2,%r8d
   140002ca6:	4c 8b 64 24 38       	mov    0x38(%rsp),%r12
   140002cab:	4c 89 e1             	mov    %r12,%rcx
   140002cae:	4c 89 fa             	mov    %r15,%rdx
   140002cb1:	e8 aa f4 ff ff       	call   140002160 <tx_fn_m0_combine_0>
   140002cb6:	49 89 fe             	mov    %rdi,%r14
   140002cb9:	49 01 c6             	add    %rax,%r14
   140002cbc:	0f 80 01 03 00 00    	jo     140002fc3 <tx_fn_m0_arithmetic_0+0xd23>
   140002cc2:	4c 89 e1             	mov    %r12,%rcx
   140002cc5:	e8 d6 42 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002cca:	85 c0                	test   %eax,%eax
   140002ccc:	0f 85 28 03 00 00    	jne    140002ffa <tx_fn_m0_arithmetic_0+0xd5a>
   140002cd2:	49 ff c7             	inc    %r15
   140002cd5:	49 81 ff 21 4e 00 00 	cmp    $0x4e21,%r15
   140002cdc:	74 40                	je     140002d1e <tx_fn_m0_arithmetic_0+0xa7e>
   140002cde:	48 89 d9             	mov    %rbx,%rcx
   140002ce1:	48 8d 54 24 70       	lea    0x70(%rsp),%rdx
   140002ce6:	e8 25 c7 02 00       	call   14002f410 <txrt_value_clone>
   140002ceb:	4c 89 f7             	mov    %r14,%rdi
   140002cee:	85 c0                	test   %eax,%eax
   140002cf0:	0f 84 0a ff ff ff    	je     140002c00 <tx_fn_m0_arithmetic_0+0x960>
   140002cf6:	41 89 c4             	mov    %eax,%r12d
   140002cf9:	48 8d 15 10 5c 0a 00 	lea    0xa5c10(%rip),%rdx        # 1400a8910 <.rdata+0x2910>
   140002d00:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002d06:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002d0c:	48 8b 4c 24 38       	mov    0x38(%rsp),%rcx
   140002d11:	e8 1a 3b 00 00       	call   140006830 <txrt_stack_error_location>
   140002d16:	44 89 e1             	mov    %r12d,%ecx
   140002d19:	e8 62 11 00 00       	call   140003e80 <txrt_require_success>
   140002d1e:	48 8d 0d 26 5e 0a 00 	lea    0xa5e26(%rip),%rcx        # 1400a8b4b <.rdata+0x2b4b>
   140002d25:	4c 8d 84 24 98 00 00 	lea    0x98(%rsp),%r8
   140002d2c:	00
   140002d2d:	ba 0e 00 00 00       	mov    $0xe,%edx
   140002d32:	e8 59 17 00 00       	call   140004490 <txrt_str_new>
   140002d37:	85 c0                	test   %eax,%eax
   140002d39:	0f 85 5c 04 00 00    	jne    14000319b <tx_fn_m0_arithmetic_0+0xefb>
   140002d3f:	48 8b b4 24 98 00 00 	mov    0x98(%rsp),%rsi
   140002d46:	00
   140002d47:	48 8d 05 72 5e 0a 00 	lea    0xa5e72(%rip),%rax        # 1400a8bc0 <.rdata+0x2bc0>
   140002d4e:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   140002d53:	48 c7 44 24 50 68 00 	movq   $0x68,0x50(%rsp)
   140002d5a:	00 00
   140002d5c:	48 c7 44 24 58 05 00 	movq   $0x5,0x58(%rsp)
   140002d63:	00 00
   140002d65:	4c 89 e1             	mov    %r12,%rcx
   140002d68:	48 89 f2             	mov    %rsi,%rdx
   140002d6b:	4c 8b 44 24 68       	mov    0x68(%rsp),%r8
   140002d70:	4d 89 f1             	mov    %r14,%r9
   140002d73:	e8 68 e8 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140002d78:	48 89 f1             	mov    %rsi,%rcx
   140002d7b:	e8 c0 1a 00 00       	call   140004840 <txrt_str_release>
   140002d80:	4c 89 e1             	mov    %r12,%rcx
   140002d83:	e8 18 42 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140002d88:	85 c0                	test   %eax,%eax
   140002d8a:	0f 85 14 04 00 00    	jne    1400031a4 <tx_fn_m0_arithmetic_0+0xf04>
   140002d90:	48 89 e9             	mov    %rbp,%rcx
   140002d93:	e8 08 c8 02 00       	call   14002f5a0 <txrt_value_release>
   140002d98:	48 89 d9             	mov    %rbx,%rcx
   140002d9b:	e8 00 c8 02 00       	call   14002f5a0 <txrt_value_release>
   140002da0:	48 8b 84 24 88 00 00 	mov    0x88(%rsp),%rax
   140002da7:	00
   140002da8:	49 89 04 24          	mov    %rax,(%r12)
   140002dac:	0f 28 b4 24 c0 01 00 	movaps 0x1c0(%rsp),%xmm6
   140002db3:	00
   140002db4:	48 81 c4 d8 01 00 00 	add    $0x1d8,%rsp
   140002dbb:	5b                   	pop    %rbx
   140002dbc:	5d                   	pop    %rbp
   140002dbd:	5f                   	pop    %rdi
   140002dbe:	5e                   	pop    %rsi
   140002dbf:	41 5c                	pop    %r12
   140002dc1:	41 5d                	pop    %r13
   140002dc3:	41 5e                	pop    %r14
   140002dc5:	41 5f                	pop    %r15
   140002dc7:	c3                   	ret
   140002dc8:	4c 8d 84 24 b8 01 00 	lea    0x1b8(%rsp),%r8
   140002dcf:	00
   140002dd0:	e8 ab 21 00 00       	call   140004f80 <txrt_add_i64>
   140002dd5:	89 c6                	mov    %eax,%esi
   140002dd7:	48 8d 15 62 45 0a 00 	lea    0xa4562(%rip),%rdx        # 1400a7340 <.rdata+0x1340>
   140002dde:	41 b8 4f 00 00 00    	mov    $0x4f,%r8d
   140002de4:	e9 fc 01 00 00       	jmp    140002fe5 <tx_fn_m0_arithmetic_0+0xd45>
   140002de9:	48 8d 15 60 48 0a 00 	lea    0xa4860(%rip),%rdx        # 1400a7650 <.rdata+0x1650>
   140002df0:	41 b8 56 00 00 00    	mov    $0x56,%r8d
   140002df6:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002dfc:	e9 b6 03 00 00       	jmp    1400031b7 <tx_fn_m0_arithmetic_0+0xf17>
   140002e01:	48 8d 15 f8 4a 0a 00 	lea    0xa4af8(%rip),%rdx        # 1400a7900 <.rdata+0x1900>
   140002e08:	e9 84 01 00 00       	jmp    140002f91 <tx_fn_m0_arithmetic_0+0xcf1>
   140002e0d:	48 8d 15 4c 4b 0a 00 	lea    0xa4b4c(%rip),%rdx        # 1400a7960 <.rdata+0x1960>
   140002e14:	e9 1e 01 00 00       	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002e19:	48 8d 15 a0 4b 0a 00 	lea    0xa4ba0(%rip),%rdx        # 1400a79c0 <.rdata+0x19c0>
   140002e20:	e9 12 01 00 00       	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002e25:	48 8d 15 f4 4b 0a 00 	lea    0xa4bf4(%rip),%rdx        # 1400a7a20 <.rdata+0x1a20>
   140002e2c:	e9 06 01 00 00       	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002e31:	48 8d 15 48 4c 0a 00 	lea    0xa4c48(%rip),%rdx        # 1400a7a80 <.rdata+0x1a80>
   140002e38:	e9 fa 00 00 00       	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002e3d:	48 8d 15 9c 4c 0a 00 	lea    0xa4c9c(%rip),%rdx        # 1400a7ae0 <.rdata+0x1ae0>
   140002e44:	e9 ee 00 00 00       	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002e49:	48 8d 15 f0 4c 0a 00 	lea    0xa4cf0(%rip),%rdx        # 1400a7b40 <.rdata+0x1b40>
   140002e50:	e9 e2 00 00 00       	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002e55:	48 8d 15 54 4d 0a 00 	lea    0xa4d54(%rip),%rdx        # 1400a7bb0 <.rdata+0x1bb0>
   140002e5c:	e9 d6 00 00 00       	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002e61:	48 8d 15 a8 4d 0a 00 	lea    0xa4da8(%rip),%rdx        # 1400a7c10 <.rdata+0x1c10>
   140002e68:	e9 ca 00 00 00       	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002e6d:	89 c6                	mov    %eax,%esi
   140002e6f:	48 8d 15 fa 4d 0a 00 	lea    0xa4dfa(%rip),%rdx        # 1400a7c70 <.rdata+0x1c70>
   140002e76:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   140002e7c:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002e82:	48 8b 4c 24 38       	mov    0x38(%rsp),%rcx
   140002e87:	e8 a4 39 00 00       	call   140006830 <txrt_stack_error_location>
   140002e8c:	89 f1                	mov    %esi,%ecx
   140002e8e:	e8 ed 0f 00 00       	call   140003e80 <txrt_require_success>
   140002e93:	41 89 c7             	mov    %eax,%r15d
   140002e96:	48 8d 15 33 4e 0a 00 	lea    0xa4e33(%rip),%rdx        # 1400a7cd0 <.rdata+0x1cd0>
   140002e9d:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   140002ea3:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002ea9:	48 8b 4c 24 38       	mov    0x38(%rsp),%rcx
   140002eae:	e8 7d 39 00 00       	call   140006830 <txrt_stack_error_location>
   140002eb3:	44 89 f9             	mov    %r15d,%ecx
   140002eb6:	e8 c5 0f 00 00       	call   140003e80 <txrt_require_success>
   140002ebb:	48 8d 15 6e 4e 0a 00 	lea    0xa4e6e(%rip),%rdx        # 1400a7d30 <.rdata+0x1d30>
   140002ec2:	eb 73                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002ec4:	48 8d 15 c5 4e 0a 00 	lea    0xa4ec5(%rip),%rdx        # 1400a7d90 <.rdata+0x1d90>
   140002ecb:	eb 6a                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002ecd:	48 8d 15 1c 4f 0a 00 	lea    0xa4f1c(%rip),%rdx        # 1400a7df0 <.rdata+0x1df0>
   140002ed4:	eb 61                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002ed6:	48 8d 15 73 4f 0a 00 	lea    0xa4f73(%rip),%rdx        # 1400a7e50 <.rdata+0x1e50>
   140002edd:	eb 58                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002edf:	48 8d 15 ca 4f 0a 00 	lea    0xa4fca(%rip),%rdx        # 1400a7eb0 <.rdata+0x1eb0>
   140002ee6:	eb 4f                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002ee8:	48 8d 15 21 50 0a 00 	lea    0xa5021(%rip),%rdx        # 1400a7f10 <.rdata+0x1f10>
   140002eef:	eb 46                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002ef1:	48 8d 15 78 50 0a 00 	lea    0xa5078(%rip),%rdx        # 1400a7f70 <.rdata+0x1f70>
   140002ef8:	eb 3d                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002efa:	48 8d 15 df 50 0a 00 	lea    0xa50df(%rip),%rdx        # 1400a7fe0 <.rdata+0x1fe0>
   140002f01:	eb 34                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002f03:	48 8d 15 46 51 0a 00 	lea    0xa5146(%rip),%rdx        # 1400a8050 <.rdata+0x2050>
   140002f0a:	eb 2b                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002f0c:	48 8d 15 9d 51 0a 00 	lea    0xa519d(%rip),%rdx        # 1400a80b0 <.rdata+0x20b0>
   140002f13:	eb 22                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002f15:	48 8d 15 f4 51 0a 00 	lea    0xa51f4(%rip),%rdx        # 1400a8110 <.rdata+0x2110>
   140002f1c:	eb 19                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002f1e:	48 8d 15 4b 52 0a 00 	lea    0xa524b(%rip),%rdx        # 1400a8170 <.rdata+0x2170>
   140002f25:	eb 10                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002f27:	48 8d 15 a2 52 0a 00 	lea    0xa52a2(%rip),%rdx        # 1400a81d0 <.rdata+0x21d0>
   140002f2e:	eb 07                	jmp    140002f37 <tx_fn_m0_arithmetic_0+0xc97>
   140002f30:	48 8d 15 f9 52 0a 00 	lea    0xa52f9(%rip),%rdx        # 1400a8230 <.rdata+0x2230>
   140002f37:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   140002f3d:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002f43:	48 8b 4c 24 38       	mov    0x38(%rsp),%rcx
   140002f48:	89 c6                	mov    %eax,%esi
   140002f4a:	e8 e1 38 00 00       	call   140006830 <txrt_stack_error_location>
   140002f4f:	89 f1                	mov    %esi,%ecx
   140002f51:	e8 2a 0f 00 00       	call   140003e80 <txrt_require_success>
   140002f56:	4c 8d 84 24 a8 01 00 	lea    0x1a8(%rsp),%r8
   140002f5d:	00
   140002f5e:	48 89 c2             	mov    %rax,%rdx
   140002f61:	e8 1a 20 00 00       	call   140004f80 <txrt_add_i64>
   140002f66:	89 c6                	mov    %eax,%esi
   140002f68:	48 8d 15 81 53 0a 00 	lea    0xa5381(%rip),%rdx        # 1400a82f0 <.rdata+0x22f0>
   140002f6f:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   140002f75:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002f7b:	4c 89 f9             	mov    %r15,%rcx
   140002f7e:	e8 ad 38 00 00       	call   140006830 <txrt_stack_error_location>
   140002f83:	89 f1                	mov    %esi,%ecx
   140002f85:	e8 f6 0e 00 00       	call   140003e80 <txrt_require_success>
   140002f8a:	48 8d 15 bf 53 0a 00 	lea    0xa53bf(%rip),%rdx        # 1400a8350 <.rdata+0x2350>
   140002f91:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   140002f97:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002f9d:	e9 15 02 00 00       	jmp    1400031b7 <tx_fn_m0_arithmetic_0+0xf17>
   140002fa2:	48 8d 15 c7 59 0a 00 	lea    0xa59c7(%rip),%rdx        # 1400a8970 <.rdata+0x2970>
   140002fa9:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002faf:	eb 8c                	jmp    140002f3d <tx_fn_m0_arithmetic_0+0xc9d>
   140002fb1:	48 8d 15 18 5a 0a 00 	lea    0xa5a18(%rip),%rdx        # 1400a89d0 <.rdata+0x29d0>
   140002fb8:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002fbe:	e9 7a ff ff ff       	jmp    140002f3d <tx_fn_m0_arithmetic_0+0xc9d>
   140002fc3:	4c 8d 84 24 a0 01 00 	lea    0x1a0(%rsp),%r8
   140002fca:	00
   140002fcb:	48 89 f9             	mov    %rdi,%rcx
   140002fce:	48 89 c2             	mov    %rax,%rdx
   140002fd1:	e8 aa 1f 00 00       	call   140004f80 <txrt_add_i64>
   140002fd6:	89 c6                	mov    %eax,%esi
   140002fd8:	48 8d 15 b1 5a 0a 00 	lea    0xa5ab1(%rip),%rdx        # 1400a8a90 <.rdata+0x2a90>
   140002fdf:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002fe5:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002feb:	4c 89 e1             	mov    %r12,%rcx
   140002fee:	e8 3d 38 00 00       	call   140006830 <txrt_stack_error_location>
   140002ff3:	89 f1                	mov    %esi,%ecx
   140002ff5:	e8 86 0e 00 00       	call   140003e80 <txrt_require_success>
   140002ffa:	48 8d 15 ef 5a 0a 00 	lea    0xa5aef(%rip),%rdx        # 1400a8af0 <.rdata+0x2af0>
   140003001:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140003007:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000300d:	e9 a5 01 00 00       	jmp    1400031b7 <tx_fn_m0_arithmetic_0+0xf17>
   140003012:	48 8d 15 07 42 0a 00 	lea    0xa4207(%rip),%rdx        # 1400a7220 <.rdata+0x1220>
   140003019:	eb 07                	jmp    140003022 <tx_fn_m0_arithmetic_0+0xd82>
   14000301b:	48 8d 15 5e 42 0a 00 	lea    0xa425e(%rip),%rdx        # 1400a7280 <.rdata+0x1280>
   140003022:	41 b8 4c 00 00 00    	mov    $0x4c,%r8d
   140003028:	e9 84 01 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   14000302d:	48 8d 15 7c 43 0a 00 	lea    0xa437c(%rip),%rdx        # 1400a73b0 <.rdata+0x13b0>
   140003034:	41 b8 51 00 00 00    	mov    $0x51,%r8d
   14000303a:	e9 72 01 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   14000303f:	48 8d 15 2a 44 0a 00 	lea    0xa442a(%rip),%rdx        # 1400a7470 <.rdata+0x1470>
   140003046:	41 b8 51 00 00 00    	mov    $0x51,%r8d
   14000304c:	e9 60 01 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003051:	48 8d 15 78 44 0a 00 	lea    0xa4478(%rip),%rdx        # 1400a74d0 <.rdata+0x14d0>
   140003058:	41 b8 53 00 00 00    	mov    $0x53,%r8d
   14000305e:	e9 4e 01 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003063:	48 8d 15 c6 44 0a 00 	lea    0xa44c6(%rip),%rdx        # 1400a7530 <.rdata+0x1530>
   14000306a:	41 b8 53 00 00 00    	mov    $0x53,%r8d
   140003070:	e9 3c 01 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003075:	48 8d 15 44 46 0a 00 	lea    0xa4644(%rip),%rdx        # 1400a76c0 <.rdata+0x16c0>
   14000307c:	41 b8 58 00 00 00    	mov    $0x58,%r8d
   140003082:	e9 2a 01 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003087:	48 8d 15 f2 46 0a 00 	lea    0xa46f2(%rip),%rdx        # 1400a7780 <.rdata+0x1780>
   14000308e:	41 b8 58 00 00 00    	mov    $0x58,%r8d
   140003094:	e9 18 01 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003099:	48 8d 15 40 47 0a 00 	lea    0xa4740(%rip),%rdx        # 1400a77e0 <.rdata+0x17e0>
   1400030a0:	41 b8 5a 00 00 00    	mov    $0x5a,%r8d
   1400030a6:	e9 06 01 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   1400030ab:	48 8d 15 8e 47 0a 00 	lea    0xa478e(%rip),%rdx        # 1400a7840 <.rdata+0x1840>
   1400030b2:	41 b8 5a 00 00 00    	mov    $0x5a,%r8d
   1400030b8:	e9 f4 00 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   1400030bd:	48 8d 15 fc 52 0a 00 	lea    0xa52fc(%rip),%rdx        # 1400a83c0 <.rdata+0x23c0>
   1400030c4:	41 b8 5f 00 00 00    	mov    $0x5f,%r8d
   1400030ca:	e9 e2 00 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   1400030cf:	48 8d 15 aa 53 0a 00 	lea    0xa53aa(%rip),%rdx        # 1400a8480 <.rdata+0x2480>
   1400030d6:	41 b8 5f 00 00 00    	mov    $0x5f,%r8d
   1400030dc:	e9 d0 00 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   1400030e1:	48 8d 15 f8 53 0a 00 	lea    0xa53f8(%rip),%rdx        # 1400a84e0 <.rdata+0x24e0>
   1400030e8:	41 b8 60 00 00 00    	mov    $0x60,%r8d
   1400030ee:	e9 be 00 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   1400030f3:	48 8d 15 46 54 0a 00 	lea    0xa5446(%rip),%rdx        # 1400a8540 <.rdata+0x2540>
   1400030fa:	41 b8 60 00 00 00    	mov    $0x60,%r8d
   140003100:	e9 ac 00 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003105:	48 8d 15 94 54 0a 00 	lea    0xa5494(%rip),%rdx        # 1400a85a0 <.rdata+0x25a0>
   14000310c:	41 b8 60 00 00 00    	mov    $0x60,%r8d
   140003112:	e9 9a 00 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003117:	48 8d 15 e2 54 0a 00 	lea    0xa54e2(%rip),%rdx        # 1400a8600 <.rdata+0x2600>
   14000311e:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140003124:	e9 88 00 00 00       	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003129:	48 8d 15 40 55 0a 00 	lea    0xa5540(%rip),%rdx        # 1400a8670 <.rdata+0x2670>
   140003130:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140003136:	eb 79                	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003138:	48 8d 15 91 55 0a 00 	lea    0xa5591(%rip),%rdx        # 1400a86d0 <.rdata+0x26d0>
   14000313f:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140003145:	eb 6a                	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003147:	48 8d 15 e2 55 0a 00 	lea    0xa55e2(%rip),%rdx        # 1400a8730 <.rdata+0x2730>
   14000314e:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140003154:	eb 5b                	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003156:	48 8d 15 33 56 0a 00 	lea    0xa5633(%rip),%rdx        # 1400a8790 <.rdata+0x2790>
   14000315d:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140003163:	eb 4c                	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003165:	48 8d 15 84 56 0a 00 	lea    0xa5684(%rip),%rdx        # 1400a87f0 <.rdata+0x27f0>
   14000316c:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140003172:	eb 3d                	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003174:	48 8d 15 d5 56 0a 00 	lea    0xa56d5(%rip),%rdx        # 1400a8850 <.rdata+0x2850>
   14000317b:	41 b8 63 00 00 00    	mov    $0x63,%r8d
   140003181:	eb 2e                	jmp    1400031b1 <tx_fn_m0_arithmetic_0+0xf11>
   140003183:	48 8d 15 26 57 0a 00 	lea    0xa5726(%rip),%rdx        # 1400a88b0 <.rdata+0x28b0>
   14000318a:	41 b8 63 00 00 00    	mov    $0x63,%r8d
   140003190:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140003196:	e9 a8 fd ff ff       	jmp    140002f43 <tx_fn_m0_arithmetic_0+0xca3>
   14000319b:	48 8d 15 be 59 0a 00 	lea    0xa59be(%rip),%rdx        # 1400a8b60 <.rdata+0x2b60>
   1400031a2:	eb 07                	jmp    1400031ab <tx_fn_m0_arithmetic_0+0xf0b>
   1400031a4:	48 8d 15 75 5a 0a 00 	lea    0xa5a75(%rip),%rdx        # 1400a8c20 <.rdata+0x2c20>
   1400031ab:	41 b8 68 00 00 00    	mov    $0x68,%r8d
   1400031b1:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   1400031b7:	4c 89 e1             	mov    %r12,%rcx
   1400031ba:	89 c6                	mov    %eax,%esi
   1400031bc:	e8 6f 36 00 00       	call   140006830 <txrt_stack_error_location>
   1400031c1:	89 f1                	mov    %esi,%ecx
   1400031c3:	e8 b8 0c 00 00       	call   140003e80 <txrt_require_success>
   1400031c8:	cc                   	int3
   1400031c9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)


E:\Project\other\Compilation\tx_build\performance_12_14\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140002050 <tx_fn_m0_recursive_0>:
   140002050:	56                   	push   %rsi
   140002051:	57                   	push   %rdi
   140002052:	53                   	push   %rbx
   140002053:	48 83 ec 50          	sub    $0x50,%rsp
   140002057:	48 89 ce             	mov    %rcx,%rsi
   14000205a:	48 8b 19             	mov    (%rcx),%rbx
   14000205d:	48 8d 05 17 50 0a 00 	lea    0xa5017(%rip),%rax        # 1400a707b <.rdata+0x107b>
   140002064:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   140002069:	48 89 5c 24 40       	mov    %rbx,0x40(%rsp)
   14000206e:	48 8d 44 24 20       	lea    0x20(%rsp),%rax
   140002073:	48 89 01             	mov    %rax,(%rcx)
   140002076:	48 85 d2             	test   %rdx,%rdx
   140002079:	7e 68                	jle    1400020e3 <tx_fn_m0_recursive_0+0x93>
   14000207b:	48 89 d7             	mov    %rdx,%rdi
   14000207e:	48 ff ca             	dec    %rdx
   140002081:	48 8d 05 38 4f 0a 00 	lea    0xa4f38(%rip),%rax        # 1400a6fc0 <.rdata+0xfc0>
   140002088:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   14000208d:	48 c7 44 24 30 3f 00 	movq   $0x3f,0x30(%rsp)
   140002094:	00 00
   140002096:	48 c7 44 24 38 09 00 	movq   $0x9,0x38(%rsp)
   14000209d:	00 00
   14000209f:	48 89 f1             	mov    %rsi,%rcx
   1400020a2:	e8 a9 ff ff ff       	call   140002050 <tx_fn_m0_recursive_0>
   1400020a7:	48 89 c2             	mov    %rax,%rdx
   1400020aa:	48 89 f8             	mov    %rdi,%rax
   1400020ad:	48 01 d0             	add    %rdx,%rax
   1400020b0:	71 33                	jno    1400020e5 <tx_fn_m0_recursive_0+0x95>
   1400020b2:	4c 8d 44 24 48       	lea    0x48(%rsp),%r8
   1400020b7:	48 89 f9             	mov    %rdi,%rcx
   1400020ba:	e8 c1 2e 00 00       	call   140004f80 <txrt_add_i64>
   1400020bf:	89 c7                	mov    %eax,%edi
   1400020c1:	48 8d 15 58 4f 0a 00 	lea    0xa4f58(%rip),%rdx        # 1400a7020 <.rdata+0x1020>
   1400020c8:	41 b8 3f 00 00 00    	mov    $0x3f,%r8d
   1400020ce:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400020d4:	48 89 f1             	mov    %rsi,%rcx
   1400020d7:	e8 54 47 00 00       	call   140006830 <txrt_stack_error_location>
   1400020dc:	89 f9                	mov    %edi,%ecx
   1400020de:	e8 9d 1d 00 00       	call   140003e80 <txrt_require_success>
   1400020e3:	31 c0                	xor    %eax,%eax
   1400020e5:	48 89 1e             	mov    %rbx,(%rsi)
   1400020e8:	48 83 c4 50          	add    $0x50,%rsp
   1400020ec:	5b                   	pop    %rbx
   1400020ed:	5f                   	pop    %rdi
   1400020ee:	5e                   	pop    %rsi
   1400020ef:	c3                   	ret


E:\Project\other\Compilation\tx_build\performance_12_14\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140003220 <tx_fn_m0_copies_0>:
   140003220:	41 57                	push   %r15
   140003222:	41 56                	push   %r14
   140003224:	41 55                	push   %r13
   140003226:	41 54                	push   %r12
   140003228:	56                   	push   %rsi
   140003229:	57                   	push   %rdi
   14000322a:	55                   	push   %rbp
   14000322b:	53                   	push   %rbx
   14000322c:	48 81 ec a8 00 00 00 	sub    $0xa8,%rsp
   140003233:	48 89 ce             	mov    %rcx,%rsi
   140003236:	4c 8b 21             	mov    (%rcx),%r12
   140003239:	48 8d 05 6b 60 0a 00 	lea    0xa606b(%rip),%rax        # 1400a92ab <.rdata+0x32ab>
   140003240:	48 89 44 24 70       	mov    %rax,0x70(%rsp)
   140003245:	48 8d 05 74 60 0a 00 	lea    0xa6074(%rip),%rax        # 1400a92c0 <.rdata+0x32c0>
   14000324c:	48 89 44 24 78       	mov    %rax,0x78(%rsp)
   140003251:	48 c7 84 24 80 00 00 	movq   $0x6b,0x80(%rsp)
   140003258:	00 6b 00 00 00
   14000325d:	48 c7 84 24 88 00 00 	movq   $0x1,0x88(%rsp)
   140003264:	00 01 00 00 00
   140003269:	4c 89 a4 24 90 00 00 	mov    %r12,0x90(%rsp)
   140003270:	00
   140003271:	48 8d 44 24 70       	lea    0x70(%rsp),%rax
   140003276:	48 89 01             	mov    %rax,(%rcx)
   140003279:	48 8d 0d 6b 5a 0a 00 	lea    0xa5a6b(%rip),%rcx        # 1400a8ceb <.rdata+0x2ceb>
   140003280:	4c 8d 44 24 68       	lea    0x68(%rsp),%r8
   140003285:	ba 06 00 00 00       	mov    $0x6,%edx
   14000328a:	e8 01 12 00 00       	call   140004490 <txrt_str_new>
   14000328f:	85 c0                	test   %eax,%eax
   140003291:	0f 85 b5 02 00 00    	jne    14000354c <tx_fn_m0_copies_0+0x32c>
   140003297:	48 8b 5c 24 68       	mov    0x68(%rsp),%rbx
   14000329c:	48 8d 0d 3d 2e 0a 00 	lea    0xa2e3d(%rip),%rcx        # 1400a60e0 <.rdata+0xe0>
   1400032a3:	48 8d 54 24 60       	lea    0x60(%rsp),%rdx
   1400032a8:	e8 b3 05 02 00       	call   140023860 <txrt_record_struct_new>
   1400032ad:	85 c0                	test   %eax,%eax
   1400032af:	0f 85 a0 02 00 00    	jne    140003555 <tx_fn_m0_copies_0+0x335>
   1400032b5:	48 8b 7c 24 60       	mov    0x60(%rsp),%rdi
   1400032ba:	48 89 f9             	mov    %rdi,%rcx
   1400032bd:	e8 2e f1 01 00       	call   1400223f0 <txrt_record_struct_view>
   1400032c2:	48 8b 00             	mov    (%rax),%rax
   1400032c5:	48 c7 00 03 00 00 00 	movq   $0x3,(%rax)
   1400032cc:	48 c7 40 08 07 00 00 	movq   $0x7,0x8(%rax)
   1400032d3:	00
   1400032d4:	48 b9 00 00 00 00 00 	movabs $0x3ff8000000000000,%rcx
   1400032db:	00 f8 3f
   1400032de:	48 89 48 10          	mov    %rcx,0x10(%rax)
   1400032e2:	c6 40 18 01          	movb   $0x1,0x18(%rax)
   1400032e6:	4c 8b 70 20          	mov    0x20(%rax),%r14
   1400032ea:	48 8d 54 24 58       	lea    0x58(%rsp),%rdx
   1400032ef:	48 89 d9             	mov    %rbx,%rcx
   1400032f2:	e8 19 c0 02 00       	call   14002f310 <txrt_value_box_str>
   1400032f7:	85 c0                	test   %eax,%eax
   1400032f9:	0f 85 5f 02 00 00    	jne    14000355e <tx_fn_m0_copies_0+0x33e>
   1400032ff:	4c 8b 7c 24 58       	mov    0x58(%rsp),%r15
   140003304:	4c 89 f1             	mov    %r14,%rcx
   140003307:	4c 89 fa             	mov    %r15,%rdx
   14000330a:	e8 b1 c2 02 00       	call   14002f5c0 <txrt_value_assign>
   14000330f:	85 c0                	test   %eax,%eax
   140003311:	0f 85 50 02 00 00    	jne    140003567 <tx_fn_m0_copies_0+0x347>
   140003317:	4c 89 f9             	mov    %r15,%rcx
   14000331a:	e8 81 c2 02 00       	call   14002f5a0 <txrt_value_release>
   14000331f:	48 89 d9             	mov    %rbx,%rcx
   140003322:	e8 19 15 00 00       	call   140004840 <txrt_str_release>
   140003327:	48 89 f9             	mov    %rdi,%rcx
   14000332a:	e8 c1 f0 01 00       	call   1400223f0 <txrt_record_struct_view>
   14000332f:	48 89 f1             	mov    %rsi,%rcx
   140003332:	e8 69 3c 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   140003337:	85 c0                	test   %eax,%eax
   140003339:	0f 85 31 02 00 00    	jne    140003570 <tx_fn_m0_copies_0+0x350>
   14000333f:	48 8d 4c 24 50       	lea    0x50(%rsp),%rcx
   140003344:	e8 37 b2 02 00       	call   14002e580 <txrt_time_monotonic_micros>
   140003349:	85 c0                	test   %eax,%eax
   14000334b:	0f 85 37 02 00 00    	jne    140003588 <tx_fn_m0_copies_0+0x368>
   140003351:	48 8b 5c 24 50       	mov    0x50(%rsp),%rbx
   140003356:	48 89 f1             	mov    %rsi,%rcx
   140003359:	e8 42 3c 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   14000335e:	85 c0                	test   %eax,%eax
   140003360:	0f 85 31 02 00 00    	jne    140003597 <tx_fn_m0_copies_0+0x377>
   140003366:	48 89 5c 24 30       	mov    %rbx,0x30(%rsp)
   14000336b:	4c 89 64 24 38       	mov    %r12,0x38(%rsp)
   140003370:	48 8d 54 24 28       	lea    0x28(%rsp),%rdx
   140003375:	48 89 f9             	mov    %rdi,%rcx
   140003378:	e8 93 c0 02 00       	call   14002f410 <txrt_value_clone>
   14000337d:	85 c0                	test   %eax,%eax
   14000337f:	0f 85 a1 00 00 00    	jne    140003426 <tx_fn_m0_copies_0+0x206>
   140003385:	bb a0 86 01 00       	mov    $0x186a0,%ebx
   14000338a:	45 31 e4             	xor    %r12d,%r12d
   14000338d:	4c 8d 74 24 48       	lea    0x48(%rsp),%r14
   140003392:	4c 8d 7c 24 28       	lea    0x28(%rsp),%r15
   140003397:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
   14000339e:	00 00
   1400033a0:	4c 8b 6c 24 28       	mov    0x28(%rsp),%r13
   1400033a5:	4c 89 e9             	mov    %r13,%rcx
   1400033a8:	4c 89 f2             	mov    %r14,%rdx
   1400033ab:	e8 80 57 03 00       	call   140038b30 <txrt_value_deep_copy>
   1400033b0:	85 c0                	test   %eax,%eax
   1400033b2:	0f 85 18 01 00 00    	jne    1400034d0 <tx_fn_m0_copies_0+0x2b0>
   1400033b8:	4c 89 e9             	mov    %r13,%rcx
   1400033bb:	e8 e0 c1 02 00       	call   14002f5a0 <txrt_value_release>
   1400033c0:	48 8b 6c 24 48       	mov    0x48(%rsp),%rbp
   1400033c5:	48 89 e9             	mov    %rbp,%rcx
   1400033c8:	e8 23 f0 01 00       	call   1400223f0 <txrt_record_struct_view>
   1400033cd:	49 89 c5             	mov    %rax,%r13
   1400033d0:	48 89 f1             	mov    %rsi,%rcx
   1400033d3:	e8 c8 3b 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   1400033d8:	85 c0                	test   %eax,%eax
   1400033da:	0f 85 f9 00 00 00    	jne    1400034d9 <tx_fn_m0_copies_0+0x2b9>
   1400033e0:	49 8b 45 00          	mov    0x0(%r13),%rax
   1400033e4:	48 8b 08             	mov    (%rax),%rcx
   1400033e7:	48 8b 40 08          	mov    0x8(%rax),%rax
   1400033eb:	48 89 ca             	mov    %rcx,%rdx
   1400033ee:	48 01 c2             	add    %rax,%rdx
   1400033f1:	0f 80 06 01 00 00    	jo     1400034fd <tx_fn_m0_copies_0+0x2dd>
   1400033f7:	4d 89 e5             	mov    %r12,%r13
   1400033fa:	49 01 d5             	add    %rdx,%r13
   1400033fd:	0f 80 15 01 00 00    	jo     140003518 <tx_fn_m0_copies_0+0x2f8>
   140003403:	48 89 e9             	mov    %rbp,%rcx
   140003406:	e8 95 c1 02 00       	call   14002f5a0 <txrt_value_release>
   14000340b:	48 ff cb             	dec    %rbx
   14000340e:	74 2a                	je     14000343a <tx_fn_m0_copies_0+0x21a>
   140003410:	48 89 f9             	mov    %rdi,%rcx
   140003413:	4c 89 fa             	mov    %r15,%rdx
   140003416:	e8 f5 bf 02 00       	call   14002f410 <txrt_value_clone>
   14000341b:	4d 89 ec             	mov    %r13,%r12
   14000341e:	85 c0                	test   %eax,%eax
   140003420:	0f 84 7a ff ff ff    	je     1400033a0 <tx_fn_m0_copies_0+0x180>
   140003426:	89 c7                	mov    %eax,%edi
   140003428:	48 8d 15 71 5b 0a 00 	lea    0xa5b71(%rip),%rdx        # 1400a8fa0 <.rdata+0x2fa0>
   14000342f:	41 b8 72 00 00 00    	mov    $0x72,%r8d
   140003435:	e9 fd 00 00 00       	jmp    140003537 <tx_fn_m0_copies_0+0x317>
   14000343a:	48 8d 0d 3a 5d 0a 00 	lea    0xa5d3a(%rip),%rcx        # 1400a917b <.rdata+0x317b>
   140003441:	4c 8d 44 24 40       	lea    0x40(%rsp),%r8
   140003446:	ba 09 00 00 00       	mov    $0x9,%edx
   14000344b:	e8 40 10 00 00       	call   140004490 <txrt_str_new>
   140003450:	85 c0                	test   %eax,%eax
   140003452:	0f 85 4e 01 00 00    	jne    1400035a6 <tx_fn_m0_copies_0+0x386>
   140003458:	4c 8b 74 24 40       	mov    0x40(%rsp),%r14
   14000345d:	48 8d 05 8c 5d 0a 00 	lea    0xa5d8c(%rip),%rax        # 1400a91f0 <.rdata+0x31f0>
   140003464:	48 89 44 24 78       	mov    %rax,0x78(%rsp)
   140003469:	48 c7 84 24 80 00 00 	movq   $0x75,0x80(%rsp)
   140003470:	00 75 00 00 00
   140003475:	48 c7 84 24 88 00 00 	movq   $0x5,0x88(%rsp)
   14000347c:	00 05 00 00 00
   140003481:	48 89 f1             	mov    %rsi,%rcx
   140003484:	4c 89 f2             	mov    %r14,%rdx
   140003487:	4c 8b 44 24 30       	mov    0x30(%rsp),%r8
   14000348c:	4d 89 e9             	mov    %r13,%r9
   14000348f:	e8 4c e1 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140003494:	4c 89 f1             	mov    %r14,%rcx
   140003497:	e8 a4 13 00 00       	call   140004840 <txrt_str_release>
   14000349c:	48 89 f1             	mov    %rsi,%rcx
   14000349f:	e8 fc 3a 02 00       	call   140026fa0 <txrt_gc_safepoint_context>
   1400034a4:	85 c0                	test   %eax,%eax
   1400034a6:	0f 85 09 01 00 00    	jne    1400035b5 <tx_fn_m0_copies_0+0x395>
   1400034ac:	48 89 f9             	mov    %rdi,%rcx
   1400034af:	e8 ec c0 02 00       	call   14002f5a0 <txrt_value_release>
   1400034b4:	48 8b 44 24 38       	mov    0x38(%rsp),%rax
   1400034b9:	48 89 06             	mov    %rax,(%rsi)
   1400034bc:	48 81 c4 a8 00 00 00 	add    $0xa8,%rsp
   1400034c3:	5b                   	pop    %rbx
   1400034c4:	5d                   	pop    %rbp
   1400034c5:	5f                   	pop    %rdi
   1400034c6:	5e                   	pop    %rsi
   1400034c7:	41 5c                	pop    %r12
   1400034c9:	41 5d                	pop    %r13
   1400034cb:	41 5e                	pop    %r14
   1400034cd:	41 5f                	pop    %r15
   1400034cf:	c3                   	ret
   1400034d0:	48 8d 15 29 5b 0a 00 	lea    0xa5b29(%rip),%rdx        # 1400a9000 <.rdata+0x3000>
   1400034d7:	eb 07                	jmp    1400034e0 <tx_fn_m0_copies_0+0x2c0>
   1400034d9:	48 8d 15 80 5b 0a 00 	lea    0xa5b80(%rip),%rdx        # 1400a9060 <.rdata+0x3060>
   1400034e0:	41 b8 72 00 00 00    	mov    $0x72,%r8d
   1400034e6:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400034ec:	48 89 f1             	mov    %rsi,%rcx
   1400034ef:	89 c6                	mov    %eax,%esi
   1400034f1:	e8 3a 33 00 00       	call   140006830 <txrt_stack_error_location>
   1400034f6:	89 f1                	mov    %esi,%ecx
   1400034f8:	e8 83 09 00 00       	call   140003e80 <txrt_require_success>
   1400034fd:	4c 8d 84 24 a0 00 00 	lea    0xa0(%rsp),%r8
   140003504:	00
   140003505:	48 89 c2             	mov    %rax,%rdx
   140003508:	e8 73 1a 00 00       	call   140004f80 <txrt_add_i64>
   14000350d:	89 c7                	mov    %eax,%edi
   14000350f:	48 8d 15 aa 5b 0a 00 	lea    0xa5baa(%rip),%rdx        # 1400a90c0 <.rdata+0x30c0>
   140003516:	eb 19                	jmp    140003531 <tx_fn_m0_copies_0+0x311>
   140003518:	4c 8d 84 24 98 00 00 	lea    0x98(%rsp),%r8
   14000351f:	00
   140003520:	4c 89 e1             	mov    %r12,%rcx
   140003523:	e8 58 1a 00 00       	call   140004f80 <txrt_add_i64>
   140003528:	89 c7                	mov    %eax,%edi
   14000352a:	48 8d 15 ef 5b 0a 00 	lea    0xa5bef(%rip),%rdx        # 1400a9120 <.rdata+0x3120>
   140003531:	41 b8 73 00 00 00    	mov    $0x73,%r8d
   140003537:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000353d:	48 89 f1             	mov    %rsi,%rcx
   140003540:	e8 eb 32 00 00       	call   140006830 <txrt_stack_error_location>
   140003545:	89 f9                	mov    %edi,%ecx
   140003547:	e8 34 09 00 00       	call   140003e80 <txrt_require_success>
   14000354c:	48 8d 15 ad 57 0a 00 	lea    0xa57ad(%rip),%rdx        # 1400a8d00 <.rdata+0x2d00>
   140003553:	eb 22                	jmp    140003577 <tx_fn_m0_copies_0+0x357>
   140003555:	48 8d 15 04 58 0a 00 	lea    0xa5804(%rip),%rdx        # 1400a8d60 <.rdata+0x2d60>
   14000355c:	eb 19                	jmp    140003577 <tx_fn_m0_copies_0+0x357>
   14000355e:	48 8d 15 5b 58 0a 00 	lea    0xa585b(%rip),%rdx        # 1400a8dc0 <.rdata+0x2dc0>
   140003565:	eb 10                	jmp    140003577 <tx_fn_m0_copies_0+0x357>
   140003567:	48 8d 15 b2 58 0a 00 	lea    0xa58b2(%rip),%rdx        # 1400a8e20 <.rdata+0x2e20>
   14000356e:	eb 07                	jmp    140003577 <tx_fn_m0_copies_0+0x357>
   140003570:	48 8d 15 09 59 0a 00 	lea    0xa5909(%rip),%rdx        # 1400a8e80 <.rdata+0x2e80>
   140003577:	41 b8 6d 00 00 00    	mov    $0x6d,%r8d
   14000357d:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140003583:	e9 64 ff ff ff       	jmp    1400034ec <tx_fn_m0_copies_0+0x2cc>
   140003588:	48 8d 15 51 59 0a 00 	lea    0xa5951(%rip),%rdx        # 1400a8ee0 <.rdata+0x2ee0>
   14000358f:	41 b8 6f 00 00 00    	mov    $0x6f,%r8d
   140003595:	eb e6                	jmp    14000357d <tx_fn_m0_copies_0+0x35d>
   140003597:	48 8d 15 a2 59 0a 00 	lea    0xa59a2(%rip),%rdx        # 1400a8f40 <.rdata+0x2f40>
   14000359e:	41 b8 6f 00 00 00    	mov    $0x6f,%r8d
   1400035a4:	eb d7                	jmp    14000357d <tx_fn_m0_copies_0+0x35d>
   1400035a6:	48 8d 15 e3 5b 0a 00 	lea    0xa5be3(%rip),%rdx        # 1400a9190 <.rdata+0x3190>
   1400035ad:	41 b8 75 00 00 00    	mov    $0x75,%r8d
   1400035b3:	eb c8                	jmp    14000357d <tx_fn_m0_copies_0+0x35d>
   1400035b5:	48 8d 15 94 5c 0a 00 	lea    0xa5c94(%rip),%rdx        # 1400a9250 <.rdata+0x3250>
   1400035bc:	41 b8 75 00 00 00    	mov    $0x75,%r8d
   1400035c2:	eb b9                	jmp    14000357d <tx_fn_m0_copies_0+0x35d>
   1400035c4:	66 66 66 2e 0f 1f 84 	data16 data16 cs nopw 0x0(%rax,%rax,1)
   1400035cb:	00 00 00 00 00
