
E:\Project\other\Compilation\tx_build\performance_12_14\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001b40 <tx_fn_m0_snapshot_0>:
   140001b40:	41 57                	push   %r15
   140001b42:	41 56                	push   %r14
   140001b44:	41 55                	push   %r13
   140001b46:	41 54                	push   %r12
   140001b48:	56                   	push   %rsi
   140001b49:	57                   	push   %rdi
   140001b4a:	55                   	push   %rbp
   140001b4b:	53                   	push   %rbx
   140001b4c:	48 81 ec 88 00 00 00 	sub    $0x88,%rsp
   140001b53:	48 89 ce             	mov    %rcx,%rsi
   140001b56:	4c 8b 31             	mov    (%rcx),%r14
   140001b59:	48 8d 05 fb 62 0a 00 	lea    0xa62fb(%rip),%rax        # 1400a7e5b <.rdata+0xe5b>
   140001b60:	48 89 44 24 58       	mov    %rax,0x58(%rsp)
   140001b65:	48 8d 05 04 63 0a 00 	lea    0xa6304(%rip),%rax        # 1400a7e70 <.rdata+0xe70>
   140001b6c:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   140001b71:	48 c7 44 24 68 26 00 	movq   $0x26,0x68(%rsp)
   140001b78:	00 00
   140001b7a:	48 c7 44 24 70 01 00 	movq   $0x1,0x70(%rsp)
   140001b81:	00 00
   140001b83:	4c 89 74 24 78       	mov    %r14,0x78(%rsp)
   140001b88:	48 8d 44 24 58       	lea    0x58(%rsp),%rax
   140001b8d:	48 89 01             	mov    %rax,(%rcx)
   140001b90:	4c 8d 44 24 50       	lea    0x50(%rsp),%r8
   140001b95:	31 c9                	xor    %ecx,%ecx
   140001b97:	31 d2                	xor    %edx,%edx
   140001b99:	e8 c2 5e 03 00       	call   140037a60 <txrt_vector_new_i64>
   140001b9e:	85 c0                	test   %eax,%eax
   140001ba0:	0f 85 f6 02 00 00    	jne    140001e9c <tx_fn_m0_snapshot_0+0x35c>
   140001ba6:	48 8b 7c 24 50       	mov    0x50(%rsp),%rdi
   140001bab:	48 89 f9             	mov    %rdi,%rcx
   140001bae:	e8 1d 59 03 00       	call   1400374d0 <txrt_vector_ref_i64>
   140001bb3:	48 89 f1             	mov    %rsi,%rcx
   140001bb6:	e8 35 15 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140001bbb:	85 c0                	test   %eax,%eax
   140001bbd:	0f 85 e2 02 00 00    	jne    140001ea5 <tx_fn_m0_snapshot_0+0x365>
   140001bc3:	ba 01 00 00 00       	mov    $0x1,%edx
   140001bc8:	48 89 f9             	mov    %rdi,%rcx
   140001bcb:	e8 20 63 03 00       	call   140037ef0 <txrt_vector_push_back_i64>
   140001bd0:	85 c0                	test   %eax,%eax
   140001bd2:	75 37                	jne    140001c0b <tx_fn_m0_snapshot_0+0xcb>
   140001bd4:	bb 02 00 00 00       	mov    $0x2,%ebx
   140001bd9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
   140001be0:	48 89 f1             	mov    %rsi,%rcx
   140001be3:	e8 08 15 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140001be8:	85 c0                	test   %eax,%eax
   140001bea:	0f 85 82 02 00 00    	jne    140001e72 <tx_fn_m0_snapshot_0+0x332>
   140001bf0:	48 81 fb a1 86 01 00 	cmp    $0x186a1,%rbx
   140001bf7:	74 26                	je     140001c1f <tx_fn_m0_snapshot_0+0xdf>
   140001bf9:	48 89 f9             	mov    %rdi,%rcx
   140001bfc:	48 89 da             	mov    %rbx,%rdx
   140001bff:	e8 ec 62 03 00       	call   140037ef0 <txrt_vector_push_back_i64>
   140001c04:	48 ff c3             	inc    %rbx
   140001c07:	85 c0                	test   %eax,%eax
   140001c09:	74 d5                	je     140001be0 <tx_fn_m0_snapshot_0+0xa0>
   140001c0b:	89 c7                	mov    %eax,%edi
   140001c0d:	48 8d 15 bc 5d 0a 00 	lea    0xa5dbc(%rip),%rdx        # 1400a79d0 <.rdata+0x9d0>
   140001c14:	41 b8 2b 00 00 00    	mov    $0x2b,%r8d
   140001c1a:	e9 40 01 00 00       	jmp    140001d5f <tx_fn_m0_snapshot_0+0x21f>
   140001c1f:	48 8d 4c 24 48       	lea    0x48(%rsp),%rcx
   140001c24:	e8 47 66 01 00       	call   140018270 <txrt_time_monotonic_micros>
   140001c29:	85 c0                	test   %eax,%eax
   140001c2b:	0f 85 83 02 00 00    	jne    140001eb4 <tx_fn_m0_snapshot_0+0x374>
   140001c31:	48 8b 5c 24 48       	mov    0x48(%rsp),%rbx
   140001c36:	48 89 f1             	mov    %rsi,%rcx
   140001c39:	e8 b2 14 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140001c3e:	85 c0                	test   %eax,%eax
   140001c40:	0f 85 7d 02 00 00    	jne    140001ec3 <tx_fn_m0_snapshot_0+0x383>
   140001c46:	4c 89 74 24 38       	mov    %r14,0x38(%rsp)
   140001c4b:	48 8d 15 f9 5e 0a 00 	lea    0xa5ef9(%rip),%rdx        # 1400a7b4b <.rdata+0xb4b>
   140001c52:	4c 8d 4c 24 28       	lea    0x28(%rsp),%r9
   140001c57:	48 89 f9             	mov    %rdi,%rcx
   140001c5a:	41 b0 01             	mov    $0x1,%r8b
   140001c5d:	e8 ae 8a 02 00       	call   14002a710 <txrt_iterator_new_i64>
   140001c62:	85 c0                	test   %eax,%eax
   140001c64:	0f 85 e6 00 00 00    	jne    140001d50 <tx_fn_m0_snapshot_0+0x210>
   140001c6a:	48 89 5c 24 30       	mov    %rbx,0x30(%rsp)
   140001c6f:	41 be 01 00 00 00    	mov    $0x1,%r14d
   140001c75:	45 31 e4             	xor    %r12d,%r12d
   140001c78:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
   140001c7f:	00
   140001c80:	4c 8b 6c 24 28       	mov    0x28(%rsp),%r13
   140001c85:	4c 89 e9             	mov    %r13,%rcx
   140001c88:	e8 a3 6c 02 00       	call   140028930 <txrt_iterator_snapshot_cursor>
   140001c8d:	48 89 c5             	mov    %rax,%rbp
   140001c90:	48 89 f1             	mov    %rsi,%rcx
   140001c93:	e8 58 14 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140001c98:	85 c0                	test   %eax,%eax
   140001c9a:	0f 85 e7 01 00 00    	jne    140001e87 <tx_fn_m0_snapshot_0+0x347>
   140001ca0:	80 7d 19 00          	cmpb   $0x0,0x19(%rbp)
   140001ca4:	0f 85 7b 01 00 00    	jne    140001e25 <tx_fn_m0_snapshot_0+0x2e5>
   140001caa:	41 bf a0 86 01 00    	mov    $0x186a0,%r15d
   140001cb0:	4c 89 e3             	mov    %r12,%rbx
   140001cb3:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001cba:	84 00 00 00 00 00
   140001cc0:	48 8b 45 10          	mov    0x10(%rbp),%rax
   140001cc4:	48 3b 45 08          	cmp    0x8(%rbp),%rax
   140001cc8:	73 16                	jae    140001ce0 <tx_fn_m0_snapshot_0+0x1a0>
   140001cca:	48 8b 4d 00          	mov    0x0(%rbp),%rcx
   140001cce:	48 8b 14 c1          	mov    (%rcx,%rax,8),%rdx
   140001cd2:	48 ff c0             	inc    %rax
   140001cd5:	48 89 45 10          	mov    %rax,0x10(%rbp)
   140001cd9:	eb 18                	jmp    140001cf3 <tx_fn_m0_snapshot_0+0x1b3>
   140001cdb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
   140001ce0:	c6 45 18 01          	movb   $0x1,0x18(%rbp)
   140001ce4:	e8 d1 29 0a 00       	call   1400a46ba <txrt_option_empty_error>
   140001ce9:	85 c0                	test   %eax,%eax
   140001ceb:	0f 85 5d 01 00 00    	jne    140001e4e <tx_fn_m0_snapshot_0+0x30e>
   140001cf1:	31 d2                	xor    %edx,%edx
   140001cf3:	49 89 dc             	mov    %rbx,%r12
   140001cf6:	49 01 d4             	add    %rdx,%r12
   140001cf9:	0f 80 05 01 00 00    	jo     140001e04 <tx_fn_m0_snapshot_0+0x2c4>
   140001cff:	49 ff cf             	dec    %r15
   140001d02:	74 1c                	je     140001d20 <tx_fn_m0_snapshot_0+0x1e0>
   140001d04:	80 7d 19 00          	cmpb   $0x0,0x19(%rbp)
   140001d08:	4c 89 e3             	mov    %r12,%rbx
   140001d0b:	74 b3                	je     140001cc0 <tx_fn_m0_snapshot_0+0x180>
   140001d0d:	e9 13 01 00 00       	jmp    140001e25 <tx_fn_m0_snapshot_0+0x2e5>
   140001d12:	66 66 66 66 66 2e 0f 	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001d19:	1f 84 00 00 00 00 00
   140001d20:	4c 89 e9             	mov    %r13,%rcx
   140001d23:	e8 78 11 01 00       	call   140012ea0 <txrt_value_release>
   140001d28:	49 83 fe 0a          	cmp    $0xa,%r14
   140001d2c:	74 46                	je     140001d74 <tx_fn_m0_snapshot_0+0x234>
   140001d2e:	49 ff c6             	inc    %r14
   140001d31:	48 89 f9             	mov    %rdi,%rcx
   140001d34:	48 8d 15 10 5e 0a 00 	lea    0xa5e10(%rip),%rdx        # 1400a7b4b <.rdata+0xb4b>
   140001d3b:	41 b0 01             	mov    $0x1,%r8b
   140001d3e:	4c 8d 4c 24 28       	lea    0x28(%rsp),%r9
   140001d43:	e8 c8 89 02 00       	call   14002a710 <txrt_iterator_new_i64>
   140001d48:	85 c0                	test   %eax,%eax
   140001d4a:	0f 84 30 ff ff ff    	je     140001c80 <tx_fn_m0_snapshot_0+0x140>
   140001d50:	89 c7                	mov    %eax,%edi
   140001d52:	48 8d 15 f7 5d 0a 00 	lea    0xa5df7(%rip),%rdx        # 1400a7b50 <.rdata+0xb50>
   140001d59:	41 b8 31 00 00 00    	mov    $0x31,%r8d
   140001d5f:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001d65:	48 89 f1             	mov    %rsi,%rcx
   140001d68:	e8 13 7f 03 00       	call   140039c80 <txrt_stack_error_location>
   140001d6d:	89 f9                	mov    %edi,%ecx
   140001d6f:	e8 8c c6 03 00       	call   14003e400 <txrt_require_success>
   140001d74:	48 8d 0d b0 5f 0a 00 	lea    0xa5fb0(%rip),%rcx        # 1400a7d2b <.rdata+0xd2b>
   140001d7b:	4c 8d 44 24 40       	lea    0x40(%rsp),%r8
   140001d80:	ba 08 00 00 00       	mov    $0x8,%edx
   140001d85:	e8 86 cc 03 00       	call   14003ea10 <txrt_str_new>
   140001d8a:	85 c0                	test   %eax,%eax
   140001d8c:	0f 85 40 01 00 00    	jne    140001ed2 <tx_fn_m0_snapshot_0+0x392>
   140001d92:	48 8b 5c 24 40       	mov    0x40(%rsp),%rbx
   140001d97:	48 8d 05 02 60 0a 00 	lea    0xa6002(%rip),%rax        # 1400a7da0 <.rdata+0xda0>
   140001d9e:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   140001da3:	48 c7 44 24 68 38 00 	movq   $0x38,0x68(%rsp)
   140001daa:	00 00
   140001dac:	48 c7 44 24 70 05 00 	movq   $0x5,0x70(%rsp)
   140001db3:	00 00
   140001db5:	48 89 f1             	mov    %rsi,%rcx
   140001db8:	48 89 da             	mov    %rbx,%rdx
   140001dbb:	4c 8b 44 24 30       	mov    0x30(%rsp),%r8
   140001dc0:	4d 89 e1             	mov    %r12,%r9
   140001dc3:	e8 18 f8 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140001dc8:	48 89 d9             	mov    %rbx,%rcx
   140001dcb:	e8 f0 cf 03 00       	call   14003edc0 <txrt_str_release>
   140001dd0:	48 89 f1             	mov    %rsi,%rcx
   140001dd3:	e8 18 13 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140001dd8:	85 c0                	test   %eax,%eax
   140001dda:	0f 85 fb 00 00 00    	jne    140001edb <tx_fn_m0_snapshot_0+0x39b>
   140001de0:	48 89 f9             	mov    %rdi,%rcx
   140001de3:	e8 b8 10 01 00       	call   140012ea0 <txrt_value_release>
   140001de8:	48 8b 44 24 38       	mov    0x38(%rsp),%rax
   140001ded:	48 89 06             	mov    %rax,(%rsi)
   140001df0:	48 81 c4 88 00 00 00 	add    $0x88,%rsp
   140001df7:	5b                   	pop    %rbx
   140001df8:	5d                   	pop    %rbp
   140001df9:	5f                   	pop    %rdi
   140001dfa:	5e                   	pop    %rsi
   140001dfb:	41 5c                	pop    %r12
   140001dfd:	41 5d                	pop    %r13
   140001dff:	41 5e                	pop    %r14
   140001e01:	41 5f                	pop    %r15
   140001e03:	c3                   	ret
   140001e04:	4c 8d 84 24 80 00 00 	lea    0x80(%rsp),%r8
   140001e0b:	00
   140001e0c:	48 89 d9             	mov    %rbx,%rcx
   140001e0f:	e8 ec d6 03 00       	call   14003f500 <txrt_add_i64>
   140001e14:	89 c7                	mov    %eax,%edi
   140001e16:	48 8d 15 b3 5e 0a 00 	lea    0xa5eb3(%rip),%rdx        # 1400a7cd0 <.rdata+0xcd0>
   140001e1d:	41 b8 35 00 00 00    	mov    $0x35,%r8d
   140001e23:	eb 14                	jmp    140001e39 <tx_fn_m0_snapshot_0+0x2f9>
   140001e25:	e8 80 26 0a 00       	call   1400a44aa <txrt_iterator_closed_error>
   140001e2a:	89 c7                	mov    %eax,%edi
   140001e2c:	48 8d 15 dd 5d 0a 00 	lea    0xa5ddd(%rip),%rdx        # 1400a7c10 <.rdata+0xc10>
   140001e33:	41 b8 34 00 00 00    	mov    $0x34,%r8d
   140001e39:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140001e3f:	48 89 f1             	mov    %rsi,%rcx
   140001e42:	e8 39 7e 03 00       	call   140039c80 <txrt_stack_error_location>
   140001e47:	89 f9                	mov    %edi,%ecx
   140001e49:	e8 b2 c5 03 00       	call   14003e400 <txrt_require_success>
   140001e4e:	48 8d 15 1b 5e 0a 00 	lea    0xa5e1b(%rip),%rdx        # 1400a7c70 <.rdata+0xc70>
   140001e55:	41 b8 35 00 00 00    	mov    $0x35,%r8d
   140001e5b:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140001e61:	48 89 f1             	mov    %rsi,%rcx
   140001e64:	89 c6                	mov    %eax,%esi
   140001e66:	e8 15 7e 03 00       	call   140039c80 <txrt_stack_error_location>
   140001e6b:	89 f1                	mov    %esi,%ecx
   140001e6d:	e8 8e c5 03 00       	call   14003e400 <txrt_require_success>
   140001e72:	48 8d 15 b7 5b 0a 00 	lea    0xa5bb7(%rip),%rdx        # 1400a7a30 <.rdata+0xa30>
   140001e79:	41 b8 2b 00 00 00    	mov    $0x2b,%r8d
   140001e7f:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001e85:	eb da                	jmp    140001e61 <tx_fn_m0_snapshot_0+0x321>
   140001e87:	48 8d 15 22 5d 0a 00 	lea    0xa5d22(%rip),%rdx        # 1400a7bb0 <.rdata+0xbb0>
   140001e8e:	41 b8 31 00 00 00    	mov    $0x31,%r8d
   140001e94:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001e9a:	eb c5                	jmp    140001e61 <tx_fn_m0_snapshot_0+0x321>
   140001e9c:	48 8d 15 6d 5a 0a 00 	lea    0xa5a6d(%rip),%rdx        # 1400a7910 <.rdata+0x910>
   140001ea3:	eb 07                	jmp    140001eac <tx_fn_m0_snapshot_0+0x36c>
   140001ea5:	48 8d 15 c4 5a 0a 00 	lea    0xa5ac4(%rip),%rdx        # 1400a7970 <.rdata+0x970>
   140001eac:	41 b8 28 00 00 00    	mov    $0x28,%r8d
   140001eb2:	eb 34                	jmp    140001ee8 <tx_fn_m0_snapshot_0+0x3a8>
   140001eb4:	48 8d 15 d5 5b 0a 00 	lea    0xa5bd5(%rip),%rdx        # 1400a7a90 <.rdata+0xa90>
   140001ebb:	41 b8 2e 00 00 00    	mov    $0x2e,%r8d
   140001ec1:	eb 25                	jmp    140001ee8 <tx_fn_m0_snapshot_0+0x3a8>
   140001ec3:	48 8d 15 26 5c 0a 00 	lea    0xa5c26(%rip),%rdx        # 1400a7af0 <.rdata+0xaf0>
   140001eca:	41 b8 2e 00 00 00    	mov    $0x2e,%r8d
   140001ed0:	eb 16                	jmp    140001ee8 <tx_fn_m0_snapshot_0+0x3a8>
   140001ed2:	48 8d 15 67 5e 0a 00 	lea    0xa5e67(%rip),%rdx        # 1400a7d40 <.rdata+0xd40>
   140001ed9:	eb 07                	jmp    140001ee2 <tx_fn_m0_snapshot_0+0x3a2>
   140001edb:	48 8d 15 1e 5f 0a 00 	lea    0xa5f1e(%rip),%rdx        # 1400a7e00 <.rdata+0xe00>
   140001ee2:	41 b8 38 00 00 00    	mov    $0x38,%r8d
   140001ee8:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001eee:	e9 6e ff ff ff       	jmp    140001e61 <tx_fn_m0_snapshot_0+0x321>
   140001ef3:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001efa:	84 00 00 00 00 00


E:\Project\other\Compilation\tx_build\performance_12_14\candidate\paths.exe:     file format pei-x86-64


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
   14000183c:	48 83 ec 68          	sub    $0x68,%rsp
   140001840:	4c 89 cf             	mov    %r9,%rdi
   140001843:	4d 89 c6             	mov    %r8,%r14
   140001846:	49 89 d4             	mov    %rdx,%r12
   140001849:	48 89 ce             	mov    %rcx,%rsi
   14000184c:	4c 8b 29             	mov    (%rcx),%r13
   14000184f:	48 8d 05 55 60 0a 00 	lea    0xa6055(%rip),%rax        # 1400a78ab <.rdata+0x8ab>
   140001856:	48 89 44 24 38       	mov    %rax,0x38(%rsp)
   14000185b:	48 8d 05 4e 60 0a 00 	lea    0xa604e(%rip),%rax        # 1400a78b0 <.rdata+0x8b0>
   140001862:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   140001867:	48 c7 44 24 48 13 00 	movq   $0x13,0x48(%rsp)
   14000186e:	00 00
   140001870:	48 c7 44 24 50 01 00 	movq   $0x1,0x50(%rsp)
   140001877:	00 00
   140001879:	4c 89 6c 24 58       	mov    %r13,0x58(%rsp)
   14000187e:	48 8d 44 24 38       	lea    0x38(%rsp),%rax
   140001883:	48 89 01             	mov    %rax,(%rcx)
   140001886:	4c 8d 44 24 30       	lea    0x30(%rsp),%r8
   14000188b:	31 c9                	xor    %ecx,%ecx
   14000188d:	31 d2                	xor    %edx,%edx
   14000188f:	e8 cc 61 03 00       	call   140037a60 <txrt_vector_new_i64>
   140001894:	85 c0                	test   %eax,%eax
   140001896:	0f 85 c0 01 00 00    	jne    140001a5c <tx_fn_m0_scan_0+0x22c>
   14000189c:	48 8b 5c 24 30       	mov    0x30(%rsp),%rbx
   1400018a1:	48 89 d9             	mov    %rbx,%rcx
   1400018a4:	e8 27 5c 03 00       	call   1400374d0 <txrt_vector_ref_i64>
   1400018a9:	49 89 c7             	mov    %rax,%r15
   1400018ac:	48 89 f1             	mov    %rsi,%rcx
   1400018af:	e8 3c 18 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   1400018b4:	85 c0                	test   %eax,%eax
   1400018b6:	0f 85 a9 01 00 00    	jne    140001a65 <tx_fn_m0_scan_0+0x235>
   1400018bc:	4d 85 e4             	test   %r12,%r12
   1400018bf:	7e 6f                	jle    140001930 <tx_fn_m0_scan_0+0x100>
   1400018c1:	bd 01 00 00 00       	mov    $0x1,%ebp
   1400018c6:	ba 01 00 00 00       	mov    $0x1,%edx
   1400018cb:	48 89 d9             	mov    %rbx,%rcx
   1400018ce:	e8 1d 66 03 00       	call   140037ef0 <txrt_vector_push_back_i64>
   1400018d3:	85 c0                	test   %eax,%eax
   1400018d5:	75 35                	jne    14000190c <tx_fn_m0_scan_0+0xdc>
   1400018d7:	4c 29 e5             	sub    %r12,%rbp
   1400018da:	41 bc 02 00 00 00    	mov    $0x2,%r12d
   1400018e0:	48 89 f1             	mov    %rsi,%rcx
   1400018e3:	e8 08 18 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   1400018e8:	85 c0                	test   %eax,%eax
   1400018ea:	0f 85 48 01 00 00    	jne    140001a38 <tx_fn_m0_scan_0+0x208>
   1400018f0:	49 8d 04 2c          	lea    (%r12,%rbp,1),%rax
   1400018f4:	48 83 f8 02          	cmp    $0x2,%rax
   1400018f8:	74 36                	je     140001930 <tx_fn_m0_scan_0+0x100>
   1400018fa:	48 89 d9             	mov    %rbx,%rcx
   1400018fd:	4c 89 e2             	mov    %r12,%rdx
   140001900:	e8 eb 65 03 00       	call   140037ef0 <txrt_vector_push_back_i64>
   140001905:	49 ff c4             	inc    %r12
   140001908:	85 c0                	test   %eax,%eax
   14000190a:	74 d4                	je     1400018e0 <tx_fn_m0_scan_0+0xb0>
   14000190c:	89 c7                	mov    %eax,%edi
   14000190e:	48 8d 15 fb 5c 0a 00 	lea    0xa5cfb(%rip),%rdx        # 1400a7610 <.rdata+0x610>
   140001915:	41 b8 18 00 00 00    	mov    $0x18,%r8d
   14000191b:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001921:	48 89 f1             	mov    %rsi,%rcx
   140001924:	e8 57 83 03 00       	call   140039c80 <txrt_stack_error_location>
   140001929:	89 f9                	mov    %edi,%ecx
   14000192b:	e8 d0 ca 03 00       	call   14003e400 <txrt_require_success>
   140001930:	48 8d 4c 24 28       	lea    0x28(%rsp),%rcx
   140001935:	e8 36 69 01 00       	call   140018270 <txrt_time_monotonic_micros>
   14000193a:	85 c0                	test   %eax,%eax
   14000193c:	0f 85 32 01 00 00    	jne    140001a74 <tx_fn_m0_scan_0+0x244>
   140001942:	4c 8b 64 24 28       	mov    0x28(%rsp),%r12
   140001947:	48 89 f1             	mov    %rsi,%rcx
   14000194a:	e8 a1 17 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   14000194f:	85 c0                	test   %eax,%eax
   140001951:	0f 85 2c 01 00 00    	jne    140001a83 <tx_fn_m0_scan_0+0x253>
   140001957:	4d 85 f6             	test   %r14,%r14
   14000195a:	7e 53                	jle    1400019af <tx_fn_m0_scan_0+0x17f>
   14000195c:	49 8b 47 08          	mov    0x8(%r15),%rax
   140001960:	48 85 c0             	test   %rax,%rax
   140001963:	7e 4a                	jle    1400019af <tx_fn_m0_scan_0+0x17f>
   140001965:	4d 8b 07             	mov    (%r15),%r8
   140001968:	41 ba 01 00 00 00    	mov    $0x1,%r10d
   14000196e:	45 31 c9             	xor    %r9d,%r9d
   140001971:	66 66 66 66 66 66 2e 	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001978:	0f 1f 84 00 00 00 00
   14000197f:	00
   140001980:	45 31 db             	xor    %r11d,%r11d
   140001983:	4c 89 c9             	mov    %r9,%rcx
   140001986:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
   14000198d:	00 00 00
   140001990:	4b 8b 14 d8          	mov    (%r8,%r11,8),%rdx
   140001994:	49 01 d1             	add    %rdx,%r9
   140001997:	70 71                	jo     140001a0a <tx_fn_m0_scan_0+0x1da>
   140001999:	49 ff c3             	inc    %r11
   14000199c:	4c 89 c9             	mov    %r9,%rcx
   14000199f:	4c 39 d8             	cmp    %r11,%rax
   1400019a2:	75 ec                	jne    140001990 <tx_fn_m0_scan_0+0x160>
   1400019a4:	4d 39 f2             	cmp    %r14,%r10
   1400019a7:	4d 8d 52 01          	lea    0x1(%r10),%r10
   1400019ab:	75 d3                	jne    140001980 <tx_fn_m0_scan_0+0x150>
   1400019ad:	eb 03                	jmp    1400019b2 <tx_fn_m0_scan_0+0x182>
   1400019af:	45 31 c9             	xor    %r9d,%r9d
   1400019b2:	48 8d 05 37 5e 0a 00 	lea    0xa5e37(%rip),%rax        # 1400a77f0 <.rdata+0x7f0>
   1400019b9:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   1400019be:	48 c7 44 24 48 23 00 	movq   $0x23,0x48(%rsp)
   1400019c5:	00 00
   1400019c7:	48 c7 44 24 50 05 00 	movq   $0x5,0x50(%rsp)
   1400019ce:	00 00
   1400019d0:	48 89 f1             	mov    %rsi,%rcx
   1400019d3:	48 89 fa             	mov    %rdi,%rdx
   1400019d6:	4d 89 e0             	mov    %r12,%r8
   1400019d9:	e8 02 fc ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   1400019de:	48 89 f1             	mov    %rsi,%rcx
   1400019e1:	e8 0a 17 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   1400019e6:	85 c0                	test   %eax,%eax
   1400019e8:	0f 85 a4 00 00 00    	jne    140001a92 <tx_fn_m0_scan_0+0x262>
   1400019ee:	48 89 d9             	mov    %rbx,%rcx
   1400019f1:	e8 aa 14 01 00       	call   140012ea0 <txrt_value_release>
   1400019f6:	4c 89 2e             	mov    %r13,(%rsi)
   1400019f9:	48 83 c4 68          	add    $0x68,%rsp
   1400019fd:	5b                   	pop    %rbx
   1400019fe:	5d                   	pop    %rbp
   1400019ff:	5f                   	pop    %rdi
   140001a00:	5e                   	pop    %rsi
   140001a01:	41 5c                	pop    %r12
   140001a03:	41 5d                	pop    %r13
   140001a05:	41 5e                	pop    %r14
   140001a07:	41 5f                	pop    %r15
   140001a09:	c3                   	ret
   140001a0a:	4c 8d 44 24 60       	lea    0x60(%rsp),%r8
   140001a0f:	e8 ec da 03 00       	call   14003f500 <txrt_add_i64>
   140001a14:	89 c7                	mov    %eax,%edi
   140001a16:	48 8d 15 73 5d 0a 00 	lea    0xa5d73(%rip),%rdx        # 1400a7790 <.rdata+0x790>
   140001a1d:	41 b8 20 00 00 00    	mov    $0x20,%r8d
   140001a23:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   140001a29:	48 89 f1             	mov    %rsi,%rcx
   140001a2c:	e8 4f 82 03 00       	call   140039c80 <txrt_stack_error_location>
   140001a31:	89 f9                	mov    %edi,%ecx
   140001a33:	e8 c8 c9 03 00       	call   14003e400 <txrt_require_success>
   140001a38:	48 8d 15 31 5c 0a 00 	lea    0xa5c31(%rip),%rdx        # 1400a7670 <.rdata+0x670>
   140001a3f:	41 b8 18 00 00 00    	mov    $0x18,%r8d
   140001a45:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001a4b:	48 89 f1             	mov    %rsi,%rcx
   140001a4e:	89 c6                	mov    %eax,%esi
   140001a50:	e8 2b 82 03 00       	call   140039c80 <txrt_stack_error_location>
   140001a55:	89 f1                	mov    %esi,%ecx
   140001a57:	e8 a4 c9 03 00       	call   14003e400 <txrt_require_success>
   140001a5c:	48 8d 15 ed 5a 0a 00 	lea    0xa5aed(%rip),%rdx        # 1400a7550 <.rdata+0x550>
   140001a63:	eb 07                	jmp    140001a6c <tx_fn_m0_scan_0+0x23c>
   140001a65:	48 8d 15 44 5b 0a 00 	lea    0xa5b44(%rip),%rdx        # 1400a75b0 <.rdata+0x5b0>
   140001a6c:	41 b8 15 00 00 00    	mov    $0x15,%r8d
   140001a72:	eb 2b                	jmp    140001a9f <tx_fn_m0_scan_0+0x26f>
   140001a74:	48 8d 15 55 5c 0a 00 	lea    0xa5c55(%rip),%rdx        # 1400a76d0 <.rdata+0x6d0>
   140001a7b:	41 b8 1b 00 00 00    	mov    $0x1b,%r8d
   140001a81:	eb 1c                	jmp    140001a9f <tx_fn_m0_scan_0+0x26f>
   140001a83:	48 8d 15 a6 5c 0a 00 	lea    0xa5ca6(%rip),%rdx        # 1400a7730 <.rdata+0x730>
   140001a8a:	41 b8 1b 00 00 00    	mov    $0x1b,%r8d
   140001a90:	eb 0d                	jmp    140001a9f <tx_fn_m0_scan_0+0x26f>
   140001a92:	48 8d 15 b7 5d 0a 00 	lea    0xa5db7(%rip),%rdx        # 1400a7850 <.rdata+0x850>
   140001a99:	41 b8 23 00 00 00    	mov    $0x23,%r8d
   140001a9f:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140001aa5:	eb a4                	jmp    140001a4b <tx_fn_m0_scan_0+0x21b>
   140001aa7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
   140001aae:	00 00


E:\Project\other\Compilation\tx_build\performance_12_14\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

00000001400021a0 <tx_fn_m0_arithmetic_0>:
   1400021a0:	41 57                	push   %r15
   1400021a2:	41 56                	push   %r14
   1400021a4:	41 55                	push   %r13
   1400021a6:	41 54                	push   %r12
   1400021a8:	56                   	push   %rsi
   1400021a9:	57                   	push   %rdi
   1400021aa:	55                   	push   %rbp
   1400021ab:	53                   	push   %rbx
   1400021ac:	48 81 ec 38 01 00 00 	sub    $0x138,%rsp
   1400021b3:	48 89 ce             	mov    %rcx,%rsi
   1400021b6:	4c 8b 21             	mov    (%rcx),%r12
   1400021b9:	48 8d 05 db 71 0a 00 	lea    0xa71db(%rip),%rax        # 1400a939b <.rdata+0x239b>
   1400021c0:	48 89 44 24 38       	mov    %rax,0x38(%rsp)
   1400021c5:	48 8d 05 e4 71 0a 00 	lea    0xa71e4(%rip),%rax        # 1400a93b0 <.rdata+0x23b0>
   1400021cc:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   1400021d1:	48 c7 44 24 48 49 00 	movq   $0x49,0x48(%rsp)
   1400021d8:	00 00
   1400021da:	48 c7 44 24 50 01 00 	movq   $0x1,0x50(%rsp)
   1400021e1:	00 00
   1400021e3:	4c 89 64 24 58       	mov    %r12,0x58(%rsp)
   1400021e8:	48 8d 44 24 38       	lea    0x38(%rsp),%rax
   1400021ed:	48 89 01             	mov    %rax,(%rcx)
   1400021f0:	48 8d 8c 24 10 01 00 	lea    0x110(%rsp),%rcx
   1400021f7:	00
   1400021f8:	e8 73 60 01 00       	call   140018270 <txrt_time_monotonic_micros>
   1400021fd:	85 c0                	test   %eax,%eax
   1400021ff:	0f 85 84 08 00 00    	jne    140002a89 <tx_fn_m0_arithmetic_0+0x8e9>
   140002205:	48 8b bc 24 10 01 00 	mov    0x110(%rsp),%rdi
   14000220c:	00
   14000220d:	48 89 f1             	mov    %rsi,%rcx
   140002210:	e8 db 0e 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002215:	85 c0                	test   %eax,%eax
   140002217:	0f 85 75 08 00 00    	jne    140002a92 <tx_fn_m0_arithmetic_0+0x8f2>
   14000221d:	49 c7 c0 c2 bd f0 ff 	mov    $0xfffffffffff0bdc2,%r8
   140002224:	31 c9                	xor    %ecx,%ecx
   140002226:	49 b9 00 00 00 a0 24 	movabs $0x24924924a0000000,%r9
   14000222d:	49 92 24
   140002230:	41 8d 80 40 42 0f 00 	lea    0xf4240(%r8),%eax
   140002237:	49 f7 e1             	mul    %r9
   14000223a:	48 89 cb             	mov    %rcx,%rbx
   14000223d:	48 01 d3             	add    %rdx,%rbx
   140002240:	0f 80 12 08 00 00    	jo     140002a58 <tx_fn_m0_arithmetic_0+0x8b8>
   140002246:	4d 85 c0             	test   %r8,%r8
   140002249:	74 1c                	je     140002267 <tx_fn_m0_arithmetic_0+0xc7>
   14000224b:	41 8d 80 41 42 0f 00 	lea    0xf4241(%r8),%eax
   140002252:	49 f7 e1             	mul    %r9
   140002255:	48 89 d9             	mov    %rbx,%rcx
   140002258:	48 01 d1             	add    %rdx,%rcx
   14000225b:	0f 80 f4 07 00 00    	jo     140002a55 <tx_fn_m0_arithmetic_0+0x8b5>
   140002261:	49 83 c0 02          	add    $0x2,%r8
   140002265:	eb c9                	jmp    140002230 <tx_fn_m0_arithmetic_0+0x90>
   140002267:	48 8d 0d dd 5f 0a 00 	lea    0xa5fdd(%rip),%rcx        # 1400a824b <.rdata+0x124b>
   14000226e:	4c 8d 84 24 08 01 00 	lea    0x108(%rsp),%r8
   140002275:	00
   140002276:	ba 08 00 00 00       	mov    $0x8,%edx
   14000227b:	e8 90 c7 03 00       	call   14003ea10 <txrt_str_new>
   140002280:	85 c0                	test   %eax,%eax
   140002282:	0f 85 1c 08 00 00    	jne    140002aa4 <tx_fn_m0_arithmetic_0+0x904>
   140002288:	4c 8b b4 24 08 01 00 	mov    0x108(%rsp),%r14
   14000228f:	00
   140002290:	48 8d 05 29 60 0a 00 	lea    0xa6029(%rip),%rax        # 1400a82c0 <.rdata+0x12c0>
   140002297:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   14000229c:	48 c7 44 24 48 51 00 	movq   $0x51,0x48(%rsp)
   1400022a3:	00 00
   1400022a5:	48 c7 44 24 50 05 00 	movq   $0x5,0x50(%rsp)
   1400022ac:	00 00
   1400022ae:	48 89 f1             	mov    %rsi,%rcx
   1400022b1:	4c 89 f2             	mov    %r14,%rdx
   1400022b4:	49 89 f8             	mov    %rdi,%r8
   1400022b7:	49 89 d9             	mov    %rbx,%r9
   1400022ba:	e8 21 f3 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   1400022bf:	4c 89 f1             	mov    %r14,%rcx
   1400022c2:	e8 f9 ca 03 00       	call   14003edc0 <txrt_str_release>
   1400022c7:	48 89 f1             	mov    %rsi,%rcx
   1400022ca:	e8 21 0e 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   1400022cf:	85 c0                	test   %eax,%eax
   1400022d1:	0f 85 df 07 00 00    	jne    140002ab6 <tx_fn_m0_arithmetic_0+0x916>
   1400022d7:	48 8d 8c 24 00 01 00 	lea    0x100(%rsp),%rcx
   1400022de:	00
   1400022df:	e8 8c 5f 01 00       	call   140018270 <txrt_time_monotonic_micros>
   1400022e4:	85 c0                	test   %eax,%eax
   1400022e6:	0f 85 dc 07 00 00    	jne    140002ac8 <tx_fn_m0_arithmetic_0+0x928>
   1400022ec:	48 8b bc 24 00 01 00 	mov    0x100(%rsp),%rdi
   1400022f3:	00
   1400022f4:	48 89 f1             	mov    %rsi,%rcx
   1400022f7:	e8 f4 0d 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   1400022fc:	85 c0                	test   %eax,%eax
   1400022fe:	0f 85 d6 07 00 00    	jne    140002ada <tx_fn_m0_arithmetic_0+0x93a>
   140002304:	4c 8d 35 35 61 0a 00 	lea    0xa6135(%rip),%r14        # 1400a8440 <.rdata+0x1440>
   14000230b:	4c 89 74 24 40       	mov    %r14,0x40(%rsp)
   140002310:	48 c7 44 24 48 56 00 	movq   $0x56,0x48(%rsp)
   140002317:	00 00
   140002319:	48 c7 44 24 50 09 00 	movq   $0x9,0x50(%rsp)
   140002320:	00 00
   140002322:	ba 64 00 00 00       	mov    $0x64,%edx
   140002327:	48 89 f1             	mov    %rsi,%rcx
   14000232a:	e8 21 fc ff ff       	call   140001f50 <tx_fn_m0_recursive_0>
   14000232f:	41 bf 20 4e 00 00    	mov    $0x4e20,%r15d
   140002335:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
   14000233c:	00 00 00 00
   140002340:	48 89 c3             	mov    %rax,%rbx
   140002343:	48 89 f1             	mov    %rsi,%rcx
   140002346:	e8 a5 0d 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   14000234b:	85 c0                	test   %eax,%eax
   14000234d:	0f 85 f8 05 00 00    	jne    14000294b <tx_fn_m0_arithmetic_0+0x7ab>
   140002353:	49 ff cf             	dec    %r15
   140002356:	74 53                	je     1400023ab <tx_fn_m0_arithmetic_0+0x20b>
   140002358:	4c 89 74 24 40       	mov    %r14,0x40(%rsp)
   14000235d:	48 c7 44 24 48 56 00 	movq   $0x56,0x48(%rsp)
   140002364:	00 00
   140002366:	48 c7 44 24 50 09 00 	movq   $0x9,0x50(%rsp)
   14000236d:	00 00
   14000236f:	ba 64 00 00 00       	mov    $0x64,%edx
   140002374:	48 89 f1             	mov    %rsi,%rcx
   140002377:	e8 d4 fb ff ff       	call   140001f50 <tx_fn_m0_recursive_0>
   14000237c:	48 89 c2             	mov    %rax,%rdx
   14000237f:	48 89 d8             	mov    %rbx,%rax
   140002382:	48 01 d0             	add    %rdx,%rax
   140002385:	71 b9                	jno    140002340 <tx_fn_m0_arithmetic_0+0x1a0>
   140002387:	4c 8d 84 24 28 01 00 	lea    0x128(%rsp),%r8
   14000238e:	00
   14000238f:	48 89 d9             	mov    %rbx,%rcx
   140002392:	e8 69 d1 03 00       	call   14003f500 <txrt_add_i64>
   140002397:	89 c7                	mov    %eax,%edi
   140002399:	48 8d 15 00 61 0a 00 	lea    0xa6100(%rip),%rdx        # 1400a84a0 <.rdata+0x14a0>
   1400023a0:	41 b8 56 00 00 00    	mov    $0x56,%r8d
   1400023a6:	e9 c9 06 00 00       	jmp    140002a74 <tx_fn_m0_arithmetic_0+0x8d4>
   1400023ab:	48 8d 0d a9 61 0a 00 	lea    0xa61a9(%rip),%rcx        # 1400a855b <.rdata+0x155b>
   1400023b2:	4c 8d 84 24 f8 00 00 	lea    0xf8(%rsp),%r8
   1400023b9:	00
   1400023ba:	ba 09 00 00 00       	mov    $0x9,%edx
   1400023bf:	e8 4c c6 03 00       	call   14003ea10 <txrt_str_new>
   1400023c4:	85 c0                	test   %eax,%eax
   1400023c6:	0f 85 20 07 00 00    	jne    140002aec <tx_fn_m0_arithmetic_0+0x94c>
   1400023cc:	4c 8b b4 24 f8 00 00 	mov    0xf8(%rsp),%r14
   1400023d3:	00
   1400023d4:	48 8d 05 f5 61 0a 00 	lea    0xa61f5(%rip),%rax        # 1400a85d0 <.rdata+0x15d0>
   1400023db:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   1400023e0:	48 c7 44 24 48 58 00 	movq   $0x58,0x48(%rsp)
   1400023e7:	00 00
   1400023e9:	48 c7 44 24 50 05 00 	movq   $0x5,0x50(%rsp)
   1400023f0:	00 00
   1400023f2:	48 89 f1             	mov    %rsi,%rcx
   1400023f5:	4c 89 f2             	mov    %r14,%rdx
   1400023f8:	49 89 f8             	mov    %rdi,%r8
   1400023fb:	49 89 d9             	mov    %rbx,%r9
   1400023fe:	e8 dd f1 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140002403:	4c 89 f1             	mov    %r14,%rcx
   140002406:	e8 b5 c9 03 00       	call   14003edc0 <txrt_str_release>
   14000240b:	48 89 f1             	mov    %rsi,%rcx
   14000240e:	e8 dd 0c 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002413:	85 c0                	test   %eax,%eax
   140002415:	0f 85 e3 06 00 00    	jne    140002afe <tx_fn_m0_arithmetic_0+0x95e>
   14000241b:	48 8d 8c 24 f0 00 00 	lea    0xf0(%rsp),%rcx
   140002422:	00
   140002423:	e8 48 5e 01 00       	call   140018270 <txrt_time_monotonic_micros>
   140002428:	85 c0                	test   %eax,%eax
   14000242a:	0f 85 e0 06 00 00    	jne    140002b10 <tx_fn_m0_arithmetic_0+0x970>
   140002430:	48 8b bc 24 f0 00 00 	mov    0xf0(%rsp),%rdi
   140002437:	00
   140002438:	48 89 f1             	mov    %rsi,%rcx
   14000243b:	e8 b0 0c 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002440:	85 c0                	test   %eax,%eax
   140002442:	0f 85 da 06 00 00    	jne    140002b22 <tx_fn_m0_arithmetic_0+0x982>
   140002448:	48 89 7c 24 60       	mov    %rdi,0x60(%rsp)
   14000244d:	4c 89 64 24 78       	mov    %r12,0x78(%rsp)
   140002452:	48 8d 54 24 70       	lea    0x70(%rsp),%rdx
   140002457:	31 c9                	xor    %ecx,%ecx
   140002459:	e8 c2 a3 01 00       	call   14001c820 <txrt_array_new>
   14000245e:	85 c0                	test   %eax,%eax
   140002460:	0f 85 28 01 00 00    	jne    14000258e <tx_fn_m0_arithmetic_0+0x3ee>
   140002466:	bb 01 00 00 00       	mov    $0x1,%ebx
   14000246b:	31 ff                	xor    %edi,%edi
   14000246d:	4c 8d 2d b7 64 0a 00 	lea    0xa64b7(%rip),%r13        # 1400a892b <.rdata+0x192b>
   140002474:	4c 8d 25 25 65 0a 00 	lea    0xa6525(%rip),%r12        # 1400a89a0 <.rdata+0x19a0>
   14000247b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
   140002480:	4c 8b 74 24 70       	mov    0x70(%rsp),%r14
   140002485:	b9 03 00 00 00       	mov    $0x3,%ecx
   14000248a:	48 8d 94 24 e8 00 00 	lea    0xe8(%rsp),%rdx
   140002491:	00
   140002492:	e8 d9 03 01 00       	call   140012870 <txrt_value_box_i64>
   140002497:	85 c0                	test   %eax,%eax
   140002499:	0f 85 c4 04 00 00    	jne    140002963 <tx_fn_m0_arithmetic_0+0x7c3>
   14000249f:	4c 8b bc 24 e8 00 00 	mov    0xe8(%rsp),%r15
   1400024a6:	00
   1400024a7:	4c 89 f1             	mov    %r14,%rcx
   1400024aa:	4c 89 fa             	mov    %r15,%rdx
   1400024ad:	e8 be b1 01 00       	call   14001d670 <txrt_array_append>
   1400024b2:	85 c0                	test   %eax,%eax
   1400024b4:	0f 85 b2 04 00 00    	jne    14000296c <tx_fn_m0_arithmetic_0+0x7cc>
   1400024ba:	4c 89 f9             	mov    %r15,%rcx
   1400024bd:	e8 de 09 01 00       	call   140012ea0 <txrt_value_release>
   1400024c2:	48 8d 8c 24 e0 00 00 	lea    0xe0(%rsp),%rcx
   1400024c9:	00
   1400024ca:	e8 01 81 00 00       	call   14000a5d0 <txrt_dict_new>
   1400024cf:	85 c0                	test   %eax,%eax
   1400024d1:	0f 85 9e 04 00 00    	jne    140002975 <tx_fn_m0_arithmetic_0+0x7d5>
   1400024d7:	4c 8b bc 24 e0 00 00 	mov    0xe0(%rsp),%r15
   1400024de:	00
   1400024df:	b9 04 00 00 00       	mov    $0x4,%ecx
   1400024e4:	48 8d 94 24 d8 00 00 	lea    0xd8(%rsp),%rdx
   1400024eb:	00
   1400024ec:	e8 7f 03 01 00       	call   140012870 <txrt_value_box_i64>
   1400024f1:	85 c0                	test   %eax,%eax
   1400024f3:	0f 85 85 04 00 00    	jne    14000297e <tx_fn_m0_arithmetic_0+0x7de>
   1400024f9:	48 8b ac 24 d8 00 00 	mov    0xd8(%rsp),%rbp
   140002500:	00
   140002501:	4c 89 f9             	mov    %r15,%rcx
   140002504:	4c 89 ea             	mov    %r13,%rdx
   140002507:	49 89 e8             	mov    %rbp,%r8
   14000250a:	e8 21 15 00 00       	call   140003a30 <txrt_keyword_set>
   14000250f:	85 c0                	test   %eax,%eax
   140002511:	0f 85 8b 04 00 00    	jne    1400029a2 <tx_fn_m0_arithmetic_0+0x802>
   140002517:	48 89 e9             	mov    %rbp,%rcx
   14000251a:	e8 81 09 01 00       	call   140012ea0 <txrt_value_release>
   14000251f:	4c 89 64 24 40       	mov    %r12,0x40(%rsp)
   140002524:	48 c7 44 24 48 5d 00 	movq   $0x5d,0x48(%rsp)
   14000252b:	00 00
   14000252d:	48 c7 44 24 50 09 00 	movq   $0x9,0x50(%rsp)
   140002534:	00 00
   140002536:	4c 89 7c 24 20       	mov    %r15,0x20(%rsp)
   14000253b:	41 b8 02 00 00 00    	mov    $0x2,%r8d
   140002541:	48 89 f1             	mov    %rsi,%rcx
   140002544:	48 89 da             	mov    %rbx,%rdx
   140002547:	4d 89 f1             	mov    %r14,%r9
   14000254a:	e8 11 fb ff ff       	call   140002060 <tx_fn_m0_combine_0>
   14000254f:	49 89 fe             	mov    %rdi,%r14
   140002552:	49 01 c6             	add    %rax,%r14
   140002555:	0f 80 68 04 00 00    	jo     1400029c3 <tx_fn_m0_arithmetic_0+0x823>
   14000255b:	48 89 f1             	mov    %rsi,%rcx
   14000255e:	e8 8d 0b 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002563:	85 c0                	test   %eax,%eax
   140002565:	0f 85 40 04 00 00    	jne    1400029ab <tx_fn_m0_arithmetic_0+0x80b>
   14000256b:	48 ff c3             	inc    %rbx
   14000256e:	48 81 fb 21 4e 00 00 	cmp    $0x4e21,%rbx
   140002575:	74 25                	je     14000259c <tx_fn_m0_arithmetic_0+0x3fc>
   140002577:	31 c9                	xor    %ecx,%ecx
   140002579:	48 8d 54 24 70       	lea    0x70(%rsp),%rdx
   14000257e:	e8 9d a2 01 00       	call   14001c820 <txrt_array_new>
   140002583:	4c 89 f7             	mov    %r14,%rdi
   140002586:	85 c0                	test   %eax,%eax
   140002588:	0f 84 f2 fe ff ff    	je     140002480 <tx_fn_m0_arithmetic_0+0x2e0>
   14000258e:	89 c7                	mov    %eax,%edi
   140002590:	48 8d 15 b9 61 0a 00 	lea    0xa61b9(%rip),%rdx        # 1400a8750 <.rdata+0x1750>
   140002597:	e9 43 04 00 00       	jmp    1400029df <tx_fn_m0_arithmetic_0+0x83f>
   14000259c:	48 8d 0d 18 65 0a 00 	lea    0xa6518(%rip),%rcx        # 1400a8abb <.rdata+0x1abb>
   1400025a3:	4c 8d 84 24 d0 00 00 	lea    0xd0(%rsp),%r8
   1400025aa:	00
   1400025ab:	ba 0d 00 00 00       	mov    $0xd,%edx
   1400025b0:	e8 5b c4 03 00       	call   14003ea10 <txrt_str_new>
   1400025b5:	85 c0                	test   %eax,%eax
   1400025b7:	0f 85 77 05 00 00    	jne    140002b34 <tx_fn_m0_arithmetic_0+0x994>
   1400025bd:	48 8b bc 24 d0 00 00 	mov    0xd0(%rsp),%rdi
   1400025c4:	00
   1400025c5:	48 8d 05 64 65 0a 00 	lea    0xa6564(%rip),%rax        # 1400a8b30 <.rdata+0x1b30>
   1400025cc:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   1400025d1:	48 c7 44 24 48 5f 00 	movq   $0x5f,0x48(%rsp)
   1400025d8:	00 00
   1400025da:	48 c7 44 24 50 05 00 	movq   $0x5,0x50(%rsp)
   1400025e1:	00 00
   1400025e3:	48 89 f1             	mov    %rsi,%rcx
   1400025e6:	48 89 fa             	mov    %rdi,%rdx
   1400025e9:	4c 8b 44 24 60       	mov    0x60(%rsp),%r8
   1400025ee:	4d 89 f1             	mov    %r14,%r9
   1400025f1:	e8 ea ef ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   1400025f6:	48 89 f9             	mov    %rdi,%rcx
   1400025f9:	e8 c2 c7 03 00       	call   14003edc0 <txrt_str_release>
   1400025fe:	48 89 f1             	mov    %rsi,%rcx
   140002601:	e8 ea 0a 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002606:	85 c0                	test   %eax,%eax
   140002608:	0f 85 38 05 00 00    	jne    140002b46 <tx_fn_m0_arithmetic_0+0x9a6>
   14000260e:	48 8d 94 24 c8 00 00 	lea    0xc8(%rsp),%rdx
   140002615:	00
   140002616:	b9 01 00 00 00       	mov    $0x1,%ecx
   14000261b:	e8 00 a2 01 00       	call   14001c820 <txrt_array_new>
   140002620:	85 c0                	test   %eax,%eax
   140002622:	0f 85 30 05 00 00    	jne    140002b58 <tx_fn_m0_arithmetic_0+0x9b8>
   140002628:	4c 8b bc 24 c8 00 00 	mov    0xc8(%rsp),%r15
   14000262f:	00
   140002630:	4c 89 f9             	mov    %r15,%rcx
   140002633:	e8 88 84 01 00       	call   14001aac0 <txrt_array_ref>
   140002638:	41 b8 03 00 00 00    	mov    $0x3,%r8d
   14000263e:	48 89 c1             	mov    %rax,%rcx
   140002641:	31 d2                	xor    %edx,%edx
   140002643:	e8 b8 91 01 00       	call   14001b800 <txrt_array_ref_set_i64>
   140002648:	85 c0                	test   %eax,%eax
   14000264a:	0f 85 1a 05 00 00    	jne    140002b6a <tx_fn_m0_arithmetic_0+0x9ca>
   140002650:	4c 89 f9             	mov    %r15,%rcx
   140002653:	e8 68 84 01 00       	call   14001aac0 <txrt_array_ref>
   140002658:	48 89 f1             	mov    %rsi,%rcx
   14000265b:	e8 90 0a 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002660:	85 c0                	test   %eax,%eax
   140002662:	0f 85 14 05 00 00    	jne    140002b7c <tx_fn_m0_arithmetic_0+0x9dc>
   140002668:	48 8d 8c 24 c0 00 00 	lea    0xc0(%rsp),%rcx
   14000266f:	00
   140002670:	e8 5b 7f 00 00       	call   14000a5d0 <txrt_dict_new>
   140002675:	85 c0                	test   %eax,%eax
   140002677:	0f 85 11 05 00 00    	jne    140002b8e <tx_fn_m0_arithmetic_0+0x9ee>
   14000267d:	4c 8b a4 24 c0 00 00 	mov    0xc0(%rsp),%r12
   140002684:	00
   140002685:	48 8d 0d df 66 0a 00 	lea    0xa66df(%rip),%rcx        # 1400a8d6b <.rdata+0x1d6b>
   14000268c:	4c 8d 84 24 b8 00 00 	lea    0xb8(%rsp),%r8
   140002693:	00
   140002694:	ba 05 00 00 00       	mov    $0x5,%edx
   140002699:	e8 72 c3 03 00       	call   14003ea10 <txrt_str_new>
   14000269e:	85 c0                	test   %eax,%eax
   1400026a0:	0f 85 f7 04 00 00    	jne    140002b9d <tx_fn_m0_arithmetic_0+0x9fd>
   1400026a6:	48 8b bc 24 b8 00 00 	mov    0xb8(%rsp),%rdi
   1400026ad:	00
   1400026ae:	48 8d 94 24 b0 00 00 	lea    0xb0(%rsp),%rdx
   1400026b5:	00
   1400026b6:	48 89 f9             	mov    %rdi,%rcx
   1400026b9:	e8 52 05 01 00       	call   140012c10 <txrt_value_box_str>
   1400026be:	85 c0                	test   %eax,%eax
   1400026c0:	0f 85 e6 04 00 00    	jne    140002bac <tx_fn_m0_arithmetic_0+0xa0c>
   1400026c6:	48 8b 9c 24 b0 00 00 	mov    0xb0(%rsp),%rbx
   1400026cd:	00
   1400026ce:	48 8d 94 24 a8 00 00 	lea    0xa8(%rsp),%rdx
   1400026d5:	00
   1400026d6:	b9 04 00 00 00       	mov    $0x4,%ecx
   1400026db:	e8 90 01 01 00       	call   140012870 <txrt_value_box_i64>
   1400026e0:	85 c0                	test   %eax,%eax
   1400026e2:	0f 85 d3 04 00 00    	jne    140002bbb <tx_fn_m0_arithmetic_0+0xa1b>
   1400026e8:	4c 8b b4 24 a8 00 00 	mov    0xa8(%rsp),%r14
   1400026ef:	00
   1400026f0:	4c 89 e1             	mov    %r12,%rcx
   1400026f3:	48 89 da             	mov    %rbx,%rdx
   1400026f6:	4d 89 f0             	mov    %r14,%r8
   1400026f9:	e8 f2 60 00 00       	call   1400087f0 <txrt_dict_set>
   1400026fe:	85 c0                	test   %eax,%eax
   140002700:	0f 85 c4 04 00 00    	jne    140002bca <tx_fn_m0_arithmetic_0+0xa2a>
   140002706:	48 89 d9             	mov    %rbx,%rcx
   140002709:	e8 92 07 01 00       	call   140012ea0 <txrt_value_release>
   14000270e:	4c 89 f1             	mov    %r14,%rcx
   140002711:	e8 8a 07 01 00       	call   140012ea0 <txrt_value_release>
   140002716:	48 89 f9             	mov    %rdi,%rcx
   140002719:	e8 a2 c6 03 00       	call   14003edc0 <txrt_str_release>
   14000271e:	4c 89 e1             	mov    %r12,%rcx
   140002721:	e8 2a 48 00 00       	call   140006f50 <txrt_dict_ref>
   140002726:	48 89 f1             	mov    %rsi,%rcx
   140002729:	e8 c2 09 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   14000272e:	85 c0                	test   %eax,%eax
   140002730:	0f 85 a3 04 00 00    	jne    140002bd9 <tx_fn_m0_arithmetic_0+0xa39>
   140002736:	48 8d 8c 24 a0 00 00 	lea    0xa0(%rsp),%rcx
   14000273d:	00
   14000273e:	e8 2d 5b 01 00       	call   140018270 <txrt_time_monotonic_micros>
   140002743:	85 c0                	test   %eax,%eax
   140002745:	0f 85 9d 04 00 00    	jne    140002be8 <tx_fn_m0_arithmetic_0+0xa48>
   14000274b:	48 8b bc 24 a0 00 00 	mov    0xa0(%rsp),%rdi
   140002752:	00
   140002753:	48 89 f1             	mov    %rsi,%rcx
   140002756:	e8 95 09 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   14000275b:	85 c0                	test   %eax,%eax
   14000275d:	0f 85 94 04 00 00    	jne    140002bf7 <tx_fn_m0_arithmetic_0+0xa57>
   140002763:	48 89 7c 24 60       	mov    %rdi,0x60(%rsp)
   140002768:	48 8d 54 24 68       	lea    0x68(%rsp),%rdx
   14000276d:	4d 89 fd             	mov    %r15,%r13
   140002770:	4c 89 f9             	mov    %r15,%rcx
   140002773:	e8 98 05 01 00       	call   140012d10 <txrt_value_clone>
   140002778:	85 c0                	test   %eax,%eax
   14000277a:	0f 85 09 01 00 00    	jne    140002889 <tx_fn_m0_arithmetic_0+0x6e9>
   140002780:	41 bf 01 00 00 00    	mov    $0x1,%r15d
   140002786:	31 ff                	xor    %edi,%edi
   140002788:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
   14000278f:	00
   140002790:	4c 8b 74 24 68       	mov    0x68(%rsp),%r14
   140002795:	4c 89 e5             	mov    %r12,%rbp
   140002798:	4c 89 e1             	mov    %r12,%rcx
   14000279b:	48 8d 94 24 98 00 00 	lea    0x98(%rsp),%rdx
   1400027a2:	00
   1400027a3:	e8 68 05 01 00       	call   140012d10 <txrt_value_clone>
   1400027a8:	85 c0                	test   %eax,%eax
   1400027aa:	0f 85 3a 02 00 00    	jne    1400029ea <tx_fn_m0_arithmetic_0+0x84a>
   1400027b0:	48 8b 9c 24 98 00 00 	mov    0x98(%rsp),%rbx
   1400027b7:	00
   1400027b8:	48 8d 84 24 88 00 00 	lea    0x88(%rsp),%rax
   1400027bf:	00
   1400027c0:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   1400027c5:	48 8d 84 24 90 00 00 	lea    0x90(%rsp),%rax
   1400027cc:	00
   1400027cd:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   1400027d2:	41 b9 02 00 00 00    	mov    $0x2,%r9d
   1400027d8:	4c 89 f1             	mov    %r14,%rcx
   1400027db:	48 89 da             	mov    %rbx,%rdx
   1400027de:	4c 8d 05 fb 68 0a 00 	lea    0xa68fb(%rip),%r8        # 1400a90e0 <.rdata+0x20e0>
   1400027e5:	e8 76 1e 00 00       	call   140004660 <txrt_call_split_spreads>
   1400027ea:	85 c0                	test   %eax,%eax
   1400027ec:	0f 85 01 02 00 00    	jne    1400029f3 <tx_fn_m0_arithmetic_0+0x853>
   1400027f2:	4c 89 f1             	mov    %r14,%rcx
   1400027f5:	e8 a6 06 01 00       	call   140012ea0 <txrt_value_release>
   1400027fa:	48 89 d9             	mov    %rbx,%rcx
   1400027fd:	e8 9e 06 01 00       	call   140012ea0 <txrt_value_release>
   140002802:	4c 8b 8c 24 90 00 00 	mov    0x90(%rsp),%r9
   140002809:	00
   14000280a:	48 8b 84 24 88 00 00 	mov    0x88(%rsp),%rax
   140002811:	00
   140002812:	48 8d 0d 37 69 0a 00 	lea    0xa6937(%rip),%rcx        # 1400a9150 <.rdata+0x2150>
   140002819:	48 89 4c 24 40       	mov    %rcx,0x40(%rsp)
   14000281e:	48 c7 44 24 48 66 00 	movq   $0x66,0x48(%rsp)
   140002825:	00 00
   140002827:	48 c7 44 24 50 09 00 	movq   $0x9,0x50(%rsp)
   14000282e:	00 00
   140002830:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   140002835:	41 b8 02 00 00 00    	mov    $0x2,%r8d
   14000283b:	48 89 f1             	mov    %rsi,%rcx
   14000283e:	4c 89 fa             	mov    %r15,%rdx
   140002841:	e8 1a f8 ff ff       	call   140002060 <tx_fn_m0_combine_0>
   140002846:	49 89 fe             	mov    %rdi,%r14
   140002849:	49 01 c6             	add    %rax,%r14
   14000284c:	0f 80 c7 01 00 00    	jo     140002a19 <tx_fn_m0_arithmetic_0+0x879>
   140002852:	48 89 f1             	mov    %rsi,%rcx
   140002855:	e8 96 08 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   14000285a:	85 c0                	test   %eax,%eax
   14000285c:	0f 85 db 01 00 00    	jne    140002a3d <tx_fn_m0_arithmetic_0+0x89d>
   140002862:	49 89 ec             	mov    %rbp,%r12
   140002865:	49 ff c7             	inc    %r15
   140002868:	49 81 ff 21 4e 00 00 	cmp    $0x4e21,%r15
   14000286f:	74 3c                	je     1400028ad <tx_fn_m0_arithmetic_0+0x70d>
   140002871:	4c 89 e9             	mov    %r13,%rcx
   140002874:	48 8d 54 24 68       	lea    0x68(%rsp),%rdx
   140002879:	e8 92 04 01 00       	call   140012d10 <txrt_value_clone>
   14000287e:	4c 89 f7             	mov    %r14,%rdi
   140002881:	85 c0                	test   %eax,%eax
   140002883:	0f 84 07 ff ff ff    	je     140002790 <tx_fn_m0_arithmetic_0+0x5f0>
   140002889:	89 c3                	mov    %eax,%ebx
   14000288b:	48 8d 15 8e 67 0a 00 	lea    0xa678e(%rip),%rdx        # 1400a9020 <.rdata+0x2020>
   140002892:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002898:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000289e:	48 89 f1             	mov    %rsi,%rcx
   1400028a1:	e8 da 73 03 00       	call   140039c80 <txrt_stack_error_location>
   1400028a6:	89 d9                	mov    %ebx,%ecx
   1400028a8:	e8 53 bb 03 00       	call   14003e400 <txrt_require_success>
   1400028ad:	48 8d 0d b7 69 0a 00 	lea    0xa69b7(%rip),%rcx        # 1400a926b <.rdata+0x226b>
   1400028b4:	4c 8d 84 24 80 00 00 	lea    0x80(%rsp),%r8
   1400028bb:	00
   1400028bc:	ba 0e 00 00 00       	mov    $0xe,%edx
   1400028c1:	e8 4a c1 03 00       	call   14003ea10 <txrt_str_new>
   1400028c6:	85 c0                	test   %eax,%eax
   1400028c8:	0f 85 38 03 00 00    	jne    140002c06 <tx_fn_m0_arithmetic_0+0xa66>
   1400028ce:	48 8b bc 24 80 00 00 	mov    0x80(%rsp),%rdi
   1400028d5:	00
   1400028d6:	48 8d 05 03 6a 0a 00 	lea    0xa6a03(%rip),%rax        # 1400a92e0 <.rdata+0x22e0>
   1400028dd:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   1400028e2:	48 c7 44 24 48 68 00 	movq   $0x68,0x48(%rsp)
   1400028e9:	00 00
   1400028eb:	48 c7 44 24 50 05 00 	movq   $0x5,0x50(%rsp)
   1400028f2:	00 00
   1400028f4:	48 89 f1             	mov    %rsi,%rcx
   1400028f7:	48 89 fa             	mov    %rdi,%rdx
   1400028fa:	4c 8b 44 24 60       	mov    0x60(%rsp),%r8
   1400028ff:	4d 89 f1             	mov    %r14,%r9
   140002902:	e8 d9 ec ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140002907:	48 89 f9             	mov    %rdi,%rcx
   14000290a:	e8 b1 c4 03 00       	call   14003edc0 <txrt_str_release>
   14000290f:	48 89 f1             	mov    %rsi,%rcx
   140002912:	e8 d9 07 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002917:	85 c0                	test   %eax,%eax
   140002919:	48 8b 7c 24 78       	mov    0x78(%rsp),%rdi
   14000291e:	0f 85 eb 02 00 00    	jne    140002c0f <tx_fn_m0_arithmetic_0+0xa6f>
   140002924:	4c 89 e1             	mov    %r12,%rcx
   140002927:	e8 74 05 01 00       	call   140012ea0 <txrt_value_release>
   14000292c:	4c 89 e9             	mov    %r13,%rcx
   14000292f:	e8 6c 05 01 00       	call   140012ea0 <txrt_value_release>
   140002934:	48 89 3e             	mov    %rdi,(%rsi)
   140002937:	48 81 c4 38 01 00 00 	add    $0x138,%rsp
   14000293e:	5b                   	pop    %rbx
   14000293f:	5d                   	pop    %rbp
   140002940:	5f                   	pop    %rdi
   140002941:	5e                   	pop    %rsi
   140002942:	41 5c                	pop    %r12
   140002944:	41 5d                	pop    %r13
   140002946:	41 5e                	pop    %r14
   140002948:	41 5f                	pop    %r15
   14000294a:	c3                   	ret
   14000294b:	48 8d 15 ae 5b 0a 00 	lea    0xa5bae(%rip),%rdx        # 1400a8500 <.rdata+0x1500>
   140002952:	41 b8 56 00 00 00    	mov    $0x56,%r8d
   140002958:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000295e:	e9 bf 02 00 00       	jmp    140002c22 <tx_fn_m0_arithmetic_0+0xa82>
   140002963:	48 8d 15 46 5e 0a 00 	lea    0xa5e46(%rip),%rdx        # 1400a87b0 <.rdata+0x17b0>
   14000296a:	eb 46                	jmp    1400029b2 <tx_fn_m0_arithmetic_0+0x812>
   14000296c:	48 8d 15 9d 5e 0a 00 	lea    0xa5e9d(%rip),%rdx        # 1400a8810 <.rdata+0x1810>
   140002973:	eb 3d                	jmp    1400029b2 <tx_fn_m0_arithmetic_0+0x812>
   140002975:	48 8d 15 f4 5e 0a 00 	lea    0xa5ef4(%rip),%rdx        # 1400a8870 <.rdata+0x1870>
   14000297c:	eb 34                	jmp    1400029b2 <tx_fn_m0_arithmetic_0+0x812>
   14000297e:	89 c5                	mov    %eax,%ebp
   140002980:	48 8d 15 49 5f 0a 00 	lea    0xa5f49(%rip),%rdx        # 1400a88d0 <.rdata+0x18d0>
   140002987:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   14000298d:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002993:	48 89 f1             	mov    %rsi,%rcx
   140002996:	e8 e5 72 03 00       	call   140039c80 <txrt_stack_error_location>
   14000299b:	89 e9                	mov    %ebp,%ecx
   14000299d:	e8 5e ba 03 00       	call   14003e400 <txrt_require_success>
   1400029a2:	48 8d 15 97 5f 0a 00 	lea    0xa5f97(%rip),%rdx        # 1400a8940 <.rdata+0x1940>
   1400029a9:	eb 07                	jmp    1400029b2 <tx_fn_m0_arithmetic_0+0x812>
   1400029ab:	48 8d 15 ae 60 0a 00 	lea    0xa60ae(%rip),%rdx        # 1400a8a60 <.rdata+0x1a60>
   1400029b2:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   1400029b8:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400029be:	e9 5f 02 00 00       	jmp    140002c22 <tx_fn_m0_arithmetic_0+0xa82>
   1400029c3:	4c 8d 84 24 20 01 00 	lea    0x120(%rsp),%r8
   1400029ca:	00
   1400029cb:	48 89 f9             	mov    %rdi,%rcx
   1400029ce:	48 89 c2             	mov    %rax,%rdx
   1400029d1:	e8 2a cb 03 00       	call   14003f500 <txrt_add_i64>
   1400029d6:	89 c7                	mov    %eax,%edi
   1400029d8:	48 8d 15 21 60 0a 00 	lea    0xa6021(%rip),%rdx        # 1400a8a00 <.rdata+0x1a00>
   1400029df:	41 b8 5d 00 00 00    	mov    $0x5d,%r8d
   1400029e5:	e9 8a 00 00 00       	jmp    140002a74 <tx_fn_m0_arithmetic_0+0x8d4>
   1400029ea:	48 8d 15 8f 66 0a 00 	lea    0xa668f(%rip),%rdx        # 1400a9080 <.rdata+0x2080>
   1400029f1:	eb 51                	jmp    140002a44 <tx_fn_m0_arithmetic_0+0x8a4>
   1400029f3:	41 89 c4             	mov    %eax,%r12d
   1400029f6:	48 8d 15 f3 66 0a 00 	lea    0xa66f3(%rip),%rdx        # 1400a90f0 <.rdata+0x20f0>
   1400029fd:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002a03:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002a09:	48 89 f1             	mov    %rsi,%rcx
   140002a0c:	e8 6f 72 03 00       	call   140039c80 <txrt_stack_error_location>
   140002a11:	44 89 e1             	mov    %r12d,%ecx
   140002a14:	e8 e7 b9 03 00       	call   14003e400 <txrt_require_success>
   140002a19:	4c 8d 84 24 18 01 00 	lea    0x118(%rsp),%r8
   140002a20:	00
   140002a21:	48 89 f9             	mov    %rdi,%rcx
   140002a24:	48 89 c2             	mov    %rax,%rdx
   140002a27:	e8 d4 ca 03 00       	call   14003f500 <txrt_add_i64>
   140002a2c:	89 c7                	mov    %eax,%edi
   140002a2e:	48 8d 15 7b 67 0a 00 	lea    0xa677b(%rip),%rdx        # 1400a91b0 <.rdata+0x21b0>
   140002a35:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002a3b:	eb 37                	jmp    140002a74 <tx_fn_m0_arithmetic_0+0x8d4>
   140002a3d:	48 8d 15 cc 67 0a 00 	lea    0xa67cc(%rip),%rdx        # 1400a9210 <.rdata+0x2210>
   140002a44:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002a4a:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002a50:	e9 cd 01 00 00       	jmp    140002c22 <tx_fn_m0_arithmetic_0+0xa82>
   140002a55:	48 89 d9             	mov    %rbx,%rcx
   140002a58:	4c 8d 84 24 30 01 00 	lea    0x130(%rsp),%r8
   140002a5f:	00
   140002a60:	e8 9b ca 03 00       	call   14003f500 <txrt_add_i64>
   140002a65:	89 c7                	mov    %eax,%edi
   140002a67:	48 8d 15 82 57 0a 00 	lea    0xa5782(%rip),%rdx        # 1400a81f0 <.rdata+0x11f0>
   140002a6e:	41 b8 4f 00 00 00    	mov    $0x4f,%r8d
   140002a74:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002a7a:	48 89 f1             	mov    %rsi,%rcx
   140002a7d:	e8 fe 71 03 00       	call   140039c80 <txrt_stack_error_location>
   140002a82:	89 f9                	mov    %edi,%ecx
   140002a84:	e8 77 b9 03 00       	call   14003e400 <txrt_require_success>
   140002a89:	48 8d 15 a0 56 0a 00 	lea    0xa56a0(%rip),%rdx        # 1400a8130 <.rdata+0x1130>
   140002a90:	eb 07                	jmp    140002a99 <tx_fn_m0_arithmetic_0+0x8f9>
   140002a92:	48 8d 15 f7 56 0a 00 	lea    0xa56f7(%rip),%rdx        # 1400a8190 <.rdata+0x1190>
   140002a99:	41 b8 4c 00 00 00    	mov    $0x4c,%r8d
   140002a9f:	e9 78 01 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002aa4:	48 8d 15 b5 57 0a 00 	lea    0xa57b5(%rip),%rdx        # 1400a8260 <.rdata+0x1260>
   140002aab:	41 b8 51 00 00 00    	mov    $0x51,%r8d
   140002ab1:	e9 66 01 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002ab6:	48 8d 15 63 58 0a 00 	lea    0xa5863(%rip),%rdx        # 1400a8320 <.rdata+0x1320>
   140002abd:	41 b8 51 00 00 00    	mov    $0x51,%r8d
   140002ac3:	e9 54 01 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002ac8:	48 8d 15 b1 58 0a 00 	lea    0xa58b1(%rip),%rdx        # 1400a8380 <.rdata+0x1380>
   140002acf:	41 b8 53 00 00 00    	mov    $0x53,%r8d
   140002ad5:	e9 42 01 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002ada:	48 8d 15 ff 58 0a 00 	lea    0xa58ff(%rip),%rdx        # 1400a83e0 <.rdata+0x13e0>
   140002ae1:	41 b8 53 00 00 00    	mov    $0x53,%r8d
   140002ae7:	e9 30 01 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002aec:	48 8d 15 7d 5a 0a 00 	lea    0xa5a7d(%rip),%rdx        # 1400a8570 <.rdata+0x1570>
   140002af3:	41 b8 58 00 00 00    	mov    $0x58,%r8d
   140002af9:	e9 1e 01 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002afe:	48 8d 15 2b 5b 0a 00 	lea    0xa5b2b(%rip),%rdx        # 1400a8630 <.rdata+0x1630>
   140002b05:	41 b8 58 00 00 00    	mov    $0x58,%r8d
   140002b0b:	e9 0c 01 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b10:	48 8d 15 79 5b 0a 00 	lea    0xa5b79(%rip),%rdx        # 1400a8690 <.rdata+0x1690>
   140002b17:	41 b8 5a 00 00 00    	mov    $0x5a,%r8d
   140002b1d:	e9 fa 00 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b22:	48 8d 15 c7 5b 0a 00 	lea    0xa5bc7(%rip),%rdx        # 1400a86f0 <.rdata+0x16f0>
   140002b29:	41 b8 5a 00 00 00    	mov    $0x5a,%r8d
   140002b2f:	e9 e8 00 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b34:	48 8d 15 95 5f 0a 00 	lea    0xa5f95(%rip),%rdx        # 1400a8ad0 <.rdata+0x1ad0>
   140002b3b:	41 b8 5f 00 00 00    	mov    $0x5f,%r8d
   140002b41:	e9 d6 00 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b46:	48 8d 15 43 60 0a 00 	lea    0xa6043(%rip),%rdx        # 1400a8b90 <.rdata+0x1b90>
   140002b4d:	41 b8 5f 00 00 00    	mov    $0x5f,%r8d
   140002b53:	e9 c4 00 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b58:	48 8d 15 91 60 0a 00 	lea    0xa6091(%rip),%rdx        # 1400a8bf0 <.rdata+0x1bf0>
   140002b5f:	41 b8 60 00 00 00    	mov    $0x60,%r8d
   140002b65:	e9 b2 00 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b6a:	48 8d 15 df 60 0a 00 	lea    0xa60df(%rip),%rdx        # 1400a8c50 <.rdata+0x1c50>
   140002b71:	41 b8 60 00 00 00    	mov    $0x60,%r8d
   140002b77:	e9 a0 00 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b7c:	48 8d 15 2d 61 0a 00 	lea    0xa612d(%rip),%rdx        # 1400a8cb0 <.rdata+0x1cb0>
   140002b83:	41 b8 60 00 00 00    	mov    $0x60,%r8d
   140002b89:	e9 8e 00 00 00       	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b8e:	48 8d 15 7b 61 0a 00 	lea    0xa617b(%rip),%rdx        # 1400a8d10 <.rdata+0x1d10>
   140002b95:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002b9b:	eb 7f                	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002b9d:	48 8d 15 dc 61 0a 00 	lea    0xa61dc(%rip),%rdx        # 1400a8d80 <.rdata+0x1d80>
   140002ba4:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002baa:	eb 70                	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002bac:	48 8d 15 2d 62 0a 00 	lea    0xa622d(%rip),%rdx        # 1400a8de0 <.rdata+0x1de0>
   140002bb3:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002bb9:	eb 61                	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002bbb:	48 8d 15 7e 62 0a 00 	lea    0xa627e(%rip),%rdx        # 1400a8e40 <.rdata+0x1e40>
   140002bc2:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002bc8:	eb 52                	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002bca:	48 8d 15 cf 62 0a 00 	lea    0xa62cf(%rip),%rdx        # 1400a8ea0 <.rdata+0x1ea0>
   140002bd1:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002bd7:	eb 43                	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002bd9:	48 8d 15 20 63 0a 00 	lea    0xa6320(%rip),%rdx        # 1400a8f00 <.rdata+0x1f00>
   140002be0:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002be6:	eb 34                	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002be8:	48 8d 15 71 63 0a 00 	lea    0xa6371(%rip),%rdx        # 1400a8f60 <.rdata+0x1f60>
   140002bef:	41 b8 63 00 00 00    	mov    $0x63,%r8d
   140002bf5:	eb 25                	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002bf7:	48 8d 15 c2 63 0a 00 	lea    0xa63c2(%rip),%rdx        # 1400a8fc0 <.rdata+0x1fc0>
   140002bfe:	41 b8 63 00 00 00    	mov    $0x63,%r8d
   140002c04:	eb 16                	jmp    140002c1c <tx_fn_m0_arithmetic_0+0xa7c>
   140002c06:	48 8d 15 73 66 0a 00 	lea    0xa6673(%rip),%rdx        # 1400a9280 <.rdata+0x2280>
   140002c0d:	eb 07                	jmp    140002c16 <tx_fn_m0_arithmetic_0+0xa76>
   140002c0f:	48 8d 15 2a 67 0a 00 	lea    0xa672a(%rip),%rdx        # 1400a9340 <.rdata+0x2340>
   140002c16:	41 b8 68 00 00 00    	mov    $0x68,%r8d
   140002c1c:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140002c22:	48 89 f1             	mov    %rsi,%rcx
   140002c25:	89 c6                	mov    %eax,%esi
   140002c27:	e8 54 70 03 00       	call   140039c80 <txrt_stack_error_location>
   140002c2c:	89 f1                	mov    %esi,%ecx
   140002c2e:	e8 cd b7 03 00       	call   14003e400 <txrt_require_success>
   140002c33:	cc                   	int3
   140002c34:	66 66 66 2e 0f 1f 84 	data16 data16 cs nopw 0x0(%rax,%rax,1)
   140002c3b:	00 00 00 00 00


E:\Project\other\Compilation\tx_build\performance_12_14\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001f50 <tx_fn_m0_recursive_0>:
   140001f50:	56                   	push   %rsi
   140001f51:	57                   	push   %rdi
   140001f52:	53                   	push   %rbx
   140001f53:	48 83 ec 50          	sub    $0x50,%rsp
   140001f57:	48 89 ce             	mov    %rcx,%rsi
   140001f5a:	48 8b 19             	mov    (%rcx),%rbx
   140001f5d:	48 8d 05 27 60 0a 00 	lea    0xa6027(%rip),%rax        # 1400a7f8b <.rdata+0xf8b>
   140001f64:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   140001f69:	48 89 5c 24 40       	mov    %rbx,0x40(%rsp)
   140001f6e:	48 8d 44 24 20       	lea    0x20(%rsp),%rax
   140001f73:	48 89 01             	mov    %rax,(%rcx)
   140001f76:	48 85 d2             	test   %rdx,%rdx
   140001f79:	7e 68                	jle    140001fe3 <tx_fn_m0_recursive_0+0x93>
   140001f7b:	48 89 d7             	mov    %rdx,%rdi
   140001f7e:	48 ff ca             	dec    %rdx
   140001f81:	48 8d 05 48 5f 0a 00 	lea    0xa5f48(%rip),%rax        # 1400a7ed0 <.rdata+0xed0>
   140001f88:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   140001f8d:	48 c7 44 24 30 3f 00 	movq   $0x3f,0x30(%rsp)
   140001f94:	00 00
   140001f96:	48 c7 44 24 38 09 00 	movq   $0x9,0x38(%rsp)
   140001f9d:	00 00
   140001f9f:	48 89 f1             	mov    %rsi,%rcx
   140001fa2:	e8 a9 ff ff ff       	call   140001f50 <tx_fn_m0_recursive_0>
   140001fa7:	48 89 c2             	mov    %rax,%rdx
   140001faa:	48 89 f8             	mov    %rdi,%rax
   140001fad:	48 01 d0             	add    %rdx,%rax
   140001fb0:	71 33                	jno    140001fe5 <tx_fn_m0_recursive_0+0x95>
   140001fb2:	4c 8d 44 24 48       	lea    0x48(%rsp),%r8
   140001fb7:	48 89 f9             	mov    %rdi,%rcx
   140001fba:	e8 41 d5 03 00       	call   14003f500 <txrt_add_i64>
   140001fbf:	89 c7                	mov    %eax,%edi
   140001fc1:	48 8d 15 68 5f 0a 00 	lea    0xa5f68(%rip),%rdx        # 1400a7f30 <.rdata+0xf30>
   140001fc8:	41 b8 3f 00 00 00    	mov    $0x3f,%r8d
   140001fce:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140001fd4:	48 89 f1             	mov    %rsi,%rcx
   140001fd7:	e8 a4 7c 03 00       	call   140039c80 <txrt_stack_error_location>
   140001fdc:	89 f9                	mov    %edi,%ecx
   140001fde:	e8 1d c4 03 00       	call   14003e400 <txrt_require_success>
   140001fe3:	31 c0                	xor    %eax,%eax
   140001fe5:	48 89 1e             	mov    %rbx,(%rsi)
   140001fe8:	48 83 c4 50          	add    $0x50,%rsp
   140001fec:	5b                   	pop    %rbx
   140001fed:	5f                   	pop    %rdi
   140001fee:	5e                   	pop    %rsi
   140001fef:	c3                   	ret


E:\Project\other\Compilation\tx_build\performance_12_14\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140002c90 <tx_fn_m0_copies_0>:
   140002c90:	41 57                	push   %r15
   140002c92:	41 56                	push   %r14
   140002c94:	41 55                	push   %r13
   140002c96:	41 54                	push   %r12
   140002c98:	56                   	push   %rsi
   140002c99:	57                   	push   %rdi
   140002c9a:	55                   	push   %rbp
   140002c9b:	53                   	push   %rbx
   140002c9c:	48 81 ec a8 00 00 00 	sub    $0xa8,%rsp
   140002ca3:	48 89 ce             	mov    %rcx,%rsi
   140002ca6:	4c 8b 21             	mov    (%rcx),%r12
   140002ca9:	48 8d 05 1b 6d 0a 00 	lea    0xa6d1b(%rip),%rax        # 1400a99cb <.rdata+0x29cb>
   140002cb0:	48 89 44 24 70       	mov    %rax,0x70(%rsp)
   140002cb5:	48 8d 05 24 6d 0a 00 	lea    0xa6d24(%rip),%rax        # 1400a99e0 <.rdata+0x29e0>
   140002cbc:	48 89 44 24 78       	mov    %rax,0x78(%rsp)
   140002cc1:	48 c7 84 24 80 00 00 	movq   $0x6b,0x80(%rsp)
   140002cc8:	00 6b 00 00 00
   140002ccd:	48 c7 84 24 88 00 00 	movq   $0x1,0x88(%rsp)
   140002cd4:	00 01 00 00 00
   140002cd9:	4c 89 a4 24 90 00 00 	mov    %r12,0x90(%rsp)
   140002ce0:	00
   140002ce1:	48 8d 44 24 70       	lea    0x70(%rsp),%rax
   140002ce6:	48 89 01             	mov    %rax,(%rcx)
   140002ce9:	48 8d 0d 1b 67 0a 00 	lea    0xa671b(%rip),%rcx        # 1400a940b <.rdata+0x240b>
   140002cf0:	4c 8d 44 24 68       	lea    0x68(%rsp),%r8
   140002cf5:	ba 06 00 00 00       	mov    $0x6,%edx
   140002cfa:	e8 11 bd 03 00       	call   14003ea10 <txrt_str_new>
   140002cff:	85 c0                	test   %eax,%eax
   140002d01:	0f 85 b5 02 00 00    	jne    140002fbc <tx_fn_m0_copies_0+0x32c>
   140002d07:	48 8b 5c 24 68       	mov    0x68(%rsp),%rbx
   140002d0c:	48 8d 0d fd 43 0a 00 	lea    0xa43fd(%rip),%rcx        # 1400a7110 <.rdata+0x110>
   140002d13:	48 8d 54 24 60       	lea    0x60(%rsp),%rdx
   140002d18:	e8 93 4b 02 00       	call   1400278b0 <txrt_record_struct_new>
   140002d1d:	85 c0                	test   %eax,%eax
   140002d1f:	0f 85 a0 02 00 00    	jne    140002fc5 <tx_fn_m0_copies_0+0x335>
   140002d25:	48 8b 7c 24 60       	mov    0x60(%rsp),%rdi
   140002d2a:	48 89 f9             	mov    %rdi,%rcx
   140002d2d:	e8 0e 37 02 00       	call   140026440 <txrt_record_struct_view>
   140002d32:	48 8b 00             	mov    (%rax),%rax
   140002d35:	48 c7 00 03 00 00 00 	movq   $0x3,(%rax)
   140002d3c:	48 c7 40 08 07 00 00 	movq   $0x7,0x8(%rax)
   140002d43:	00
   140002d44:	48 b9 00 00 00 00 00 	movabs $0x3ff8000000000000,%rcx
   140002d4b:	00 f8 3f
   140002d4e:	48 89 48 10          	mov    %rcx,0x10(%rax)
   140002d52:	c6 40 18 01          	movb   $0x1,0x18(%rax)
   140002d56:	4c 8b 70 20          	mov    0x20(%rax),%r14
   140002d5a:	48 8d 54 24 58       	lea    0x58(%rsp),%rdx
   140002d5f:	48 89 d9             	mov    %rbx,%rcx
   140002d62:	e8 a9 fe 00 00       	call   140012c10 <txrt_value_box_str>
   140002d67:	85 c0                	test   %eax,%eax
   140002d69:	0f 85 5f 02 00 00    	jne    140002fce <tx_fn_m0_copies_0+0x33e>
   140002d6f:	4c 8b 7c 24 58       	mov    0x58(%rsp),%r15
   140002d74:	4c 89 f1             	mov    %r14,%rcx
   140002d77:	4c 89 fa             	mov    %r15,%rdx
   140002d7a:	e8 41 01 01 00       	call   140012ec0 <txrt_value_assign>
   140002d7f:	85 c0                	test   %eax,%eax
   140002d81:	0f 85 50 02 00 00    	jne    140002fd7 <tx_fn_m0_copies_0+0x347>
   140002d87:	4c 89 f9             	mov    %r15,%rcx
   140002d8a:	e8 11 01 01 00       	call   140012ea0 <txrt_value_release>
   140002d8f:	48 89 d9             	mov    %rbx,%rcx
   140002d92:	e8 29 c0 03 00       	call   14003edc0 <txrt_str_release>
   140002d97:	48 89 f9             	mov    %rdi,%rcx
   140002d9a:	e8 a1 36 02 00       	call   140026440 <txrt_record_struct_view>
   140002d9f:	48 89 f1             	mov    %rsi,%rcx
   140002da2:	e8 49 03 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002da7:	85 c0                	test   %eax,%eax
   140002da9:	0f 85 31 02 00 00    	jne    140002fe0 <tx_fn_m0_copies_0+0x350>
   140002daf:	48 8d 4c 24 50       	lea    0x50(%rsp),%rcx
   140002db4:	e8 b7 54 01 00       	call   140018270 <txrt_time_monotonic_micros>
   140002db9:	85 c0                	test   %eax,%eax
   140002dbb:	0f 85 37 02 00 00    	jne    140002ff8 <tx_fn_m0_copies_0+0x368>
   140002dc1:	48 8b 5c 24 50       	mov    0x50(%rsp),%rbx
   140002dc6:	48 89 f1             	mov    %rsi,%rcx
   140002dc9:	e8 22 03 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002dce:	85 c0                	test   %eax,%eax
   140002dd0:	0f 85 31 02 00 00    	jne    140003007 <tx_fn_m0_copies_0+0x377>
   140002dd6:	48 89 5c 24 30       	mov    %rbx,0x30(%rsp)
   140002ddb:	4c 89 64 24 38       	mov    %r12,0x38(%rsp)
   140002de0:	48 8d 54 24 28       	lea    0x28(%rsp),%rdx
   140002de5:	48 89 f9             	mov    %rdi,%rcx
   140002de8:	e8 23 ff 00 00       	call   140012d10 <txrt_value_clone>
   140002ded:	85 c0                	test   %eax,%eax
   140002def:	0f 85 a1 00 00 00    	jne    140002e96 <tx_fn_m0_copies_0+0x206>
   140002df5:	bb a0 86 01 00       	mov    $0x186a0,%ebx
   140002dfa:	45 31 e4             	xor    %r12d,%r12d
   140002dfd:	4c 8d 74 24 48       	lea    0x48(%rsp),%r14
   140002e02:	4c 8d 7c 24 28       	lea    0x28(%rsp),%r15
   140002e07:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
   140002e0e:	00 00
   140002e10:	4c 8b 6c 24 28       	mov    0x28(%rsp),%r13
   140002e15:	4c 89 e9             	mov    %r13,%rcx
   140002e18:	4c 89 f2             	mov    %r14,%rdx
   140002e1b:	e8 c0 bc 00 00       	call   14000eae0 <txrt_value_deep_copy>
   140002e20:	85 c0                	test   %eax,%eax
   140002e22:	0f 85 18 01 00 00    	jne    140002f40 <tx_fn_m0_copies_0+0x2b0>
   140002e28:	4c 89 e9             	mov    %r13,%rcx
   140002e2b:	e8 70 00 01 00       	call   140012ea0 <txrt_value_release>
   140002e30:	48 8b 6c 24 48       	mov    0x48(%rsp),%rbp
   140002e35:	48 89 e9             	mov    %rbp,%rcx
   140002e38:	e8 03 36 02 00       	call   140026440 <txrt_record_struct_view>
   140002e3d:	49 89 c5             	mov    %rax,%r13
   140002e40:	48 89 f1             	mov    %rsi,%rcx
   140002e43:	e8 a8 02 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002e48:	85 c0                	test   %eax,%eax
   140002e4a:	0f 85 f9 00 00 00    	jne    140002f49 <tx_fn_m0_copies_0+0x2b9>
   140002e50:	49 8b 45 00          	mov    0x0(%r13),%rax
   140002e54:	48 8b 08             	mov    (%rax),%rcx
   140002e57:	48 8b 40 08          	mov    0x8(%rax),%rax
   140002e5b:	48 89 ca             	mov    %rcx,%rdx
   140002e5e:	48 01 c2             	add    %rax,%rdx
   140002e61:	0f 80 06 01 00 00    	jo     140002f6d <tx_fn_m0_copies_0+0x2dd>
   140002e67:	4d 89 e5             	mov    %r12,%r13
   140002e6a:	49 01 d5             	add    %rdx,%r13
   140002e6d:	0f 80 15 01 00 00    	jo     140002f88 <tx_fn_m0_copies_0+0x2f8>
   140002e73:	48 89 e9             	mov    %rbp,%rcx
   140002e76:	e8 25 00 01 00       	call   140012ea0 <txrt_value_release>
   140002e7b:	48 ff cb             	dec    %rbx
   140002e7e:	74 2a                	je     140002eaa <tx_fn_m0_copies_0+0x21a>
   140002e80:	48 89 f9             	mov    %rdi,%rcx
   140002e83:	4c 89 fa             	mov    %r15,%rdx
   140002e86:	e8 85 fe 00 00       	call   140012d10 <txrt_value_clone>
   140002e8b:	4d 89 ec             	mov    %r13,%r12
   140002e8e:	85 c0                	test   %eax,%eax
   140002e90:	0f 84 7a ff ff ff    	je     140002e10 <tx_fn_m0_copies_0+0x180>
   140002e96:	89 c7                	mov    %eax,%edi
   140002e98:	48 8d 15 21 68 0a 00 	lea    0xa6821(%rip),%rdx        # 1400a96c0 <.rdata+0x26c0>
   140002e9f:	41 b8 72 00 00 00    	mov    $0x72,%r8d
   140002ea5:	e9 fd 00 00 00       	jmp    140002fa7 <tx_fn_m0_copies_0+0x317>
   140002eaa:	48 8d 0d ea 69 0a 00 	lea    0xa69ea(%rip),%rcx        # 1400a989b <.rdata+0x289b>
   140002eb1:	4c 8d 44 24 40       	lea    0x40(%rsp),%r8
   140002eb6:	ba 09 00 00 00       	mov    $0x9,%edx
   140002ebb:	e8 50 bb 03 00       	call   14003ea10 <txrt_str_new>
   140002ec0:	85 c0                	test   %eax,%eax
   140002ec2:	0f 85 4e 01 00 00    	jne    140003016 <tx_fn_m0_copies_0+0x386>
   140002ec8:	4c 8b 74 24 40       	mov    0x40(%rsp),%r14
   140002ecd:	48 8d 05 3c 6a 0a 00 	lea    0xa6a3c(%rip),%rax        # 1400a9910 <.rdata+0x2910>
   140002ed4:	48 89 44 24 78       	mov    %rax,0x78(%rsp)
   140002ed9:	48 c7 84 24 80 00 00 	movq   $0x75,0x80(%rsp)
   140002ee0:	00 75 00 00 00
   140002ee5:	48 c7 84 24 88 00 00 	movq   $0x5,0x88(%rsp)
   140002eec:	00 05 00 00 00
   140002ef1:	48 89 f1             	mov    %rsi,%rcx
   140002ef4:	4c 89 f2             	mov    %r14,%rdx
   140002ef7:	4c 8b 44 24 30       	mov    0x30(%rsp),%r8
   140002efc:	4d 89 e9             	mov    %r13,%r9
   140002eff:	e8 dc e6 ff ff       	call   1400015e0 <tx_fn_m0_report_0>
   140002f04:	4c 89 f1             	mov    %r14,%rcx
   140002f07:	e8 b4 be 03 00       	call   14003edc0 <txrt_str_release>
   140002f0c:	48 89 f1             	mov    %rsi,%rcx
   140002f0f:	e8 dc 01 02 00       	call   1400230f0 <txrt_gc_safepoint_context>
   140002f14:	85 c0                	test   %eax,%eax
   140002f16:	0f 85 09 01 00 00    	jne    140003025 <tx_fn_m0_copies_0+0x395>
   140002f1c:	48 89 f9             	mov    %rdi,%rcx
   140002f1f:	e8 7c ff 00 00       	call   140012ea0 <txrt_value_release>
   140002f24:	48 8b 44 24 38       	mov    0x38(%rsp),%rax
   140002f29:	48 89 06             	mov    %rax,(%rsi)
   140002f2c:	48 81 c4 a8 00 00 00 	add    $0xa8,%rsp
   140002f33:	5b                   	pop    %rbx
   140002f34:	5d                   	pop    %rbp
   140002f35:	5f                   	pop    %rdi
   140002f36:	5e                   	pop    %rsi
   140002f37:	41 5c                	pop    %r12
   140002f39:	41 5d                	pop    %r13
   140002f3b:	41 5e                	pop    %r14
   140002f3d:	41 5f                	pop    %r15
   140002f3f:	c3                   	ret
   140002f40:	48 8d 15 d9 67 0a 00 	lea    0xa67d9(%rip),%rdx        # 1400a9720 <.rdata+0x2720>
   140002f47:	eb 07                	jmp    140002f50 <tx_fn_m0_copies_0+0x2c0>
   140002f49:	48 8d 15 30 68 0a 00 	lea    0xa6830(%rip),%rdx        # 1400a9780 <.rdata+0x2780>
   140002f50:	41 b8 72 00 00 00    	mov    $0x72,%r8d
   140002f56:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002f5c:	48 89 f1             	mov    %rsi,%rcx
   140002f5f:	89 c6                	mov    %eax,%esi
   140002f61:	e8 1a 6d 03 00       	call   140039c80 <txrt_stack_error_location>
   140002f66:	89 f1                	mov    %esi,%ecx
   140002f68:	e8 93 b4 03 00       	call   14003e400 <txrt_require_success>
   140002f6d:	4c 8d 84 24 a0 00 00 	lea    0xa0(%rsp),%r8
   140002f74:	00
   140002f75:	48 89 c2             	mov    %rax,%rdx
   140002f78:	e8 83 c5 03 00       	call   14003f500 <txrt_add_i64>
   140002f7d:	89 c7                	mov    %eax,%edi
   140002f7f:	48 8d 15 5a 68 0a 00 	lea    0xa685a(%rip),%rdx        # 1400a97e0 <.rdata+0x27e0>
   140002f86:	eb 19                	jmp    140002fa1 <tx_fn_m0_copies_0+0x311>
   140002f88:	4c 8d 84 24 98 00 00 	lea    0x98(%rsp),%r8
   140002f8f:	00
   140002f90:	4c 89 e1             	mov    %r12,%rcx
   140002f93:	e8 68 c5 03 00       	call   14003f500 <txrt_add_i64>
   140002f98:	89 c7                	mov    %eax,%edi
   140002f9a:	48 8d 15 9f 68 0a 00 	lea    0xa689f(%rip),%rdx        # 1400a9840 <.rdata+0x2840>
   140002fa1:	41 b8 73 00 00 00    	mov    $0x73,%r8d
   140002fa7:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002fad:	48 89 f1             	mov    %rsi,%rcx
   140002fb0:	e8 cb 6c 03 00       	call   140039c80 <txrt_stack_error_location>
   140002fb5:	89 f9                	mov    %edi,%ecx
   140002fb7:	e8 44 b4 03 00       	call   14003e400 <txrt_require_success>
   140002fbc:	48 8d 15 5d 64 0a 00 	lea    0xa645d(%rip),%rdx        # 1400a9420 <.rdata+0x2420>
   140002fc3:	eb 22                	jmp    140002fe7 <tx_fn_m0_copies_0+0x357>
   140002fc5:	48 8d 15 b4 64 0a 00 	lea    0xa64b4(%rip),%rdx        # 1400a9480 <.rdata+0x2480>
   140002fcc:	eb 19                	jmp    140002fe7 <tx_fn_m0_copies_0+0x357>
   140002fce:	48 8d 15 0b 65 0a 00 	lea    0xa650b(%rip),%rdx        # 1400a94e0 <.rdata+0x24e0>
   140002fd5:	eb 10                	jmp    140002fe7 <tx_fn_m0_copies_0+0x357>
   140002fd7:	48 8d 15 62 65 0a 00 	lea    0xa6562(%rip),%rdx        # 1400a9540 <.rdata+0x2540>
   140002fde:	eb 07                	jmp    140002fe7 <tx_fn_m0_copies_0+0x357>
   140002fe0:	48 8d 15 b9 65 0a 00 	lea    0xa65b9(%rip),%rdx        # 1400a95a0 <.rdata+0x25a0>
   140002fe7:	41 b8 6d 00 00 00    	mov    $0x6d,%r8d
   140002fed:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140002ff3:	e9 64 ff ff ff       	jmp    140002f5c <tx_fn_m0_copies_0+0x2cc>
   140002ff8:	48 8d 15 01 66 0a 00 	lea    0xa6601(%rip),%rdx        # 1400a9600 <.rdata+0x2600>
   140002fff:	41 b8 6f 00 00 00    	mov    $0x6f,%r8d
   140003005:	eb e6                	jmp    140002fed <tx_fn_m0_copies_0+0x35d>
   140003007:	48 8d 15 52 66 0a 00 	lea    0xa6652(%rip),%rdx        # 1400a9660 <.rdata+0x2660>
   14000300e:	41 b8 6f 00 00 00    	mov    $0x6f,%r8d
   140003014:	eb d7                	jmp    140002fed <tx_fn_m0_copies_0+0x35d>
   140003016:	48 8d 15 93 68 0a 00 	lea    0xa6893(%rip),%rdx        # 1400a98b0 <.rdata+0x28b0>
   14000301d:	41 b8 75 00 00 00    	mov    $0x75,%r8d
   140003023:	eb c8                	jmp    140002fed <tx_fn_m0_copies_0+0x35d>
   140003025:	48 8d 15 44 69 0a 00 	lea    0xa6944(%rip),%rdx        # 1400a9970 <.rdata+0x2970>
   14000302c:	41 b8 75 00 00 00    	mov    $0x75,%r8d
   140003032:	eb b9                	jmp    140002fed <tx_fn_m0_copies_0+0x35d>
   140003034:	66 66 66 2e 0f 1f 84 	data16 data16 cs nopw 0x0(%rax,%rax,1)
   14000303b:	00 00 00 00 00
