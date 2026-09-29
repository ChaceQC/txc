
E:\Project\other\Compilation\tx_build\performance_09_11\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001d30 <tx_fn_m0_bench_fields_0>:
   140001d30:	41 57                	push   %r15
   140001d32:	41 56                	push   %r14
   140001d34:	41 55                	push   %r13
   140001d36:	41 54                	push   %r12
   140001d38:	56                   	push   %rsi
   140001d39:	57                   	push   %rdi
   140001d3a:	55                   	push   %rbp
   140001d3b:	53                   	push   %rbx
   140001d3c:	48 81 ec 68 01 00 00 	sub    $0x168,%rsp
   140001d43:	66 0f 7f b4 24 50 01 	movdqa %xmm6,0x150(%rsp)
   140001d4a:	00 00
   140001d4c:	48 89 ce             	mov    %rcx,%rsi
   140001d4f:	4c 8b 29             	mov    (%rcx),%r13
   140001d52:	48 8d 05 02 57 08 00 	lea    0x85702(%rip),%rax        # 14008745b <.rdata+0x145b>
   140001d59:	48 89 44 24 58       	mov    %rax,0x58(%rsp)
   140001d5e:	48 8d 05 0b 57 08 00 	lea    0x8570b(%rip),%rax        # 140087470 <.rdata+0x1470>
   140001d65:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   140001d6a:	48 c7 44 24 68 2a 00 	movq   $0x2a,0x68(%rsp)
   140001d71:	00 00
   140001d73:	48 c7 44 24 70 01 00 	movq   $0x1,0x70(%rsp)
   140001d7a:	00 00
   140001d7c:	4c 89 6c 24 78       	mov    %r13,0x78(%rsp)
   140001d81:	48 8d 44 24 58       	lea    0x58(%rsp),%rax
   140001d86:	48 89 01             	mov    %rax,(%rcx)
   140001d89:	48 8d 0d 41 49 08 00 	lea    0x84941(%rip),%rcx        # 1400866d1 <.rdata+0x6d1>
   140001d90:	48 8d 15 34 49 08 00 	lea    0x84934(%rip),%rdx        # 1400866cb <.rdata+0x6cb>
   140001d97:	4c 8d 8c 24 18 01 00 	lea    0x118(%rsp),%r9
   140001d9e:	00
   140001d9f:	41 b8 02 00 00 00    	mov    $0x2,%r8d
   140001da5:	e8 a6 67 00 00       	call   140008550 <txrt_struct_new>
   140001daa:	85 c0                	test   %eax,%eax
   140001dac:	0f 85 09 08 00 00    	jne    1400025bb <tx_fn_m0_bench_fields_0+0x88b>
   140001db2:	48 8b ac 24 18 01 00 	mov    0x118(%rsp),%rbp
   140001db9:	00
   140001dba:	4c 8d 05 7a 49 08 00 	lea    0x8497a(%rip),%r8        # 14008673b <.rdata+0x73b>
   140001dc1:	41 b9 01 00 00 00    	mov    $0x1,%r9d
   140001dc7:	48 89 e9             	mov    %rbp,%rcx
   140001dca:	31 d2                	xor    %edx,%edx
   140001dcc:	e8 9f 56 00 00       	call   140007470 <txrt_struct_set_field_i64>
   140001dd1:	85 c0                	test   %eax,%eax
   140001dd3:	0f 85 eb 07 00 00    	jne    1400025c4 <tx_fn_m0_bench_fields_0+0x894>
   140001dd9:	4c 8d 05 bb 49 08 00 	lea    0x849bb(%rip),%r8        # 14008679b <.rdata+0x79b>
   140001de0:	ba 01 00 00 00       	mov    $0x1,%edx
   140001de5:	41 b9 02 00 00 00    	mov    $0x2,%r9d
   140001deb:	48 89 e9             	mov    %rbp,%rcx
   140001dee:	e8 7d 56 00 00       	call   140007470 <txrt_struct_set_field_i64>
   140001df3:	85 c0                	test   %eax,%eax
   140001df5:	0f 85 d2 07 00 00    	jne    1400025cd <tx_fn_m0_bench_fields_0+0x89d>
   140001dfb:	48 89 f1             	mov    %rsi,%rcx
   140001dfe:	e8 ed ed 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140001e03:	85 c0                	test   %eax,%eax
   140001e05:	0f 85 cb 07 00 00    	jne    1400025d6 <tx_fn_m0_bench_fields_0+0x8a6>
   140001e0b:	48 8d 94 24 10 01 00 	lea    0x110(%rsp),%rdx
   140001e12:	00
   140001e13:	48 89 e9             	mov    %rbp,%rcx
   140001e16:	e8 a5 43 00 00       	call   1400061c0 <txrt_value_clone>
   140001e1b:	85 c0                	test   %eax,%eax
   140001e1d:	0f 85 c5 07 00 00    	jne    1400025e8 <tx_fn_m0_bench_fields_0+0x8b8>
   140001e23:	4c 8b bc 24 10 01 00 	mov    0x110(%rsp),%r15
   140001e2a:	00
   140001e2b:	48 8d 8c 24 08 01 00 	lea    0x108(%rsp),%rcx
   140001e32:	00
   140001e33:	e8 b8 96 00 00       	call   14000b4f0 <txrt_time_monotonic_micros>
   140001e38:	85 c0                	test   %eax,%eax
   140001e3a:	0f 85 ba 07 00 00    	jne    1400025fa <tx_fn_m0_bench_fields_0+0x8ca>
   140001e40:	4c 8b b4 24 08 01 00 	mov    0x108(%rsp),%r14
   140001e47:	00
   140001e48:	48 89 f1             	mov    %rsi,%rcx
   140001e4b:	e8 a0 ed 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140001e50:	85 c0                	test   %eax,%eax
   140001e52:	0f 85 b4 07 00 00    	jne    14000260c <tx_fn_m0_bench_fields_0+0x8dc>
   140001e58:	48 89 e9             	mov    %rbp,%rcx
   140001e5b:	31 d2                	xor    %edx,%edx
   140001e5d:	e8 ae 5f 00 00       	call   140007e10 <txrt_struct_field_i64_ptr>
   140001e62:	48 8b 08             	mov    (%rax),%rcx
   140001e65:	48 ff c1             	inc    %rcx
   140001e68:	70 40                	jo     140001eaa <tx_fn_m0_bench_fields_0+0x17a>
   140001e6a:	bf 40 42 0f 00       	mov    $0xf4240,%edi
   140001e6f:	90                   	nop
   140001e70:	48 89 08             	mov    %rcx,(%rax)
   140001e73:	ba 01 00 00 00       	mov    $0x1,%edx
   140001e78:	4c 89 f9             	mov    %r15,%rcx
   140001e7b:	e8 90 5f 00 00       	call   140007e10 <txrt_struct_field_i64_ptr>
   140001e80:	48 8b 08             	mov    (%rax),%rcx
   140001e83:	48 89 ca             	mov    %rcx,%rdx
   140001e86:	48 83 c2 02          	add    $0x2,%rdx
   140001e8a:	0f 80 fb 05 00 00    	jo     14000248b <tx_fn_m0_bench_fields_0+0x75b>
   140001e90:	48 89 10             	mov    %rdx,(%rax)
   140001e93:	48 ff cf             	dec    %rdi
   140001e96:	74 42                	je     140001eda <tx_fn_m0_bench_fields_0+0x1aa>
   140001e98:	48 89 e9             	mov    %rbp,%rcx
   140001e9b:	31 d2                	xor    %edx,%edx
   140001e9d:	e8 6e 5f 00 00       	call   140007e10 <txrt_struct_field_i64_ptr>
   140001ea2:	48 8b 08             	mov    (%rax),%rcx
   140001ea5:	48 ff c1             	inc    %rcx
   140001ea8:	71 c6                	jno    140001e70 <tx_fn_m0_bench_fields_0+0x140>
   140001eaa:	48 b9 ff ff ff ff ff 	movabs $0x7fffffffffffffff,%rcx
   140001eb1:	ff ff 7f
   140001eb4:	4c 8d 84 24 48 01 00 	lea    0x148(%rsp),%r8
   140001ebb:	00
   140001ebc:	ba 01 00 00 00       	mov    $0x1,%edx
   140001ec1:	e8 3a da 01 00       	call   14001f900 <txrt_add_i64>
   140001ec6:	89 c7                	mov    %eax,%edi
   140001ec8:	48 8d 15 b1 4a 08 00 	lea    0x84ab1(%rip),%rdx        # 140086980 <.rdata+0x980>
   140001ecf:	41 b8 31 00 00 00    	mov    $0x31,%r8d
   140001ed5:	e9 a8 06 00 00       	jmp    140002582 <tx_fn_m0_bench_fields_0+0x852>
   140001eda:	48 8d 0d 5a 4b 08 00 	lea    0x84b5a(%rip),%rcx        # 140086a3b <.rdata+0xa3b>
   140001ee1:	4c 8d 84 24 00 01 00 	lea    0x100(%rsp),%r8
   140001ee8:	00
   140001ee9:	ba 0d 00 00 00       	mov    $0xd,%edx
   140001eee:	e8 1d cf 01 00       	call   14001ee10 <txrt_str_new>
   140001ef3:	85 c0                	test   %eax,%eax
   140001ef5:	0f 85 23 07 00 00    	jne    14000261e <tx_fn_m0_bench_fields_0+0x8ee>
   140001efb:	48 8b bc 24 00 01 00 	mov    0x100(%rsp),%rdi
   140001f02:	00
   140001f03:	48 89 e9             	mov    %rbp,%rcx
   140001f06:	31 d2                	xor    %edx,%edx
   140001f08:	e8 03 5f 00 00       	call   140007e10 <txrt_struct_field_i64_ptr>
   140001f0d:	48 8b 18             	mov    (%rax),%rbx
   140001f10:	ba 01 00 00 00       	mov    $0x1,%edx
   140001f15:	48 89 e9             	mov    %rbp,%rcx
   140001f18:	e8 f3 5e 00 00       	call   140007e10 <txrt_struct_field_i64_ptr>
   140001f1d:	48 8b 10             	mov    (%rax),%rdx
   140001f20:	49 89 d9             	mov    %rbx,%r9
   140001f23:	49 01 d1             	add    %rdx,%r9
   140001f26:	0f 80 04 07 00 00    	jo     140002630 <tx_fn_m0_bench_fields_0+0x900>
   140001f2c:	48 8d 05 dd 4b 08 00 	lea    0x84bdd(%rip),%rax        # 140086b10 <.rdata+0xb10>
   140001f33:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   140001f38:	48 c7 44 24 68 34 00 	movq   $0x34,0x68(%rsp)
   140001f3f:	00 00
   140001f41:	48 c7 44 24 70 05 00 	movq   $0x5,0x70(%rsp)
   140001f48:	00 00
   140001f4a:	48 89 f1             	mov    %rsi,%rcx
   140001f4d:	48 89 fa             	mov    %rdi,%rdx
   140001f50:	4d 89 f0             	mov    %r14,%r8
   140001f53:	e8 38 f9 ff ff       	call   140001890 <tx_fn_m0_report_0>
   140001f58:	48 89 f9             	mov    %rdi,%rcx
   140001f5b:	e8 60 d2 01 00       	call   14001f1c0 <txrt_str_release>
   140001f60:	48 89 f1             	mov    %rsi,%rcx
   140001f63:	e8 88 ec 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140001f68:	85 c0                	test   %eax,%eax
   140001f6a:	0f 85 e4 06 00 00    	jne    140002654 <tx_fn_m0_bench_fields_0+0x924>
   140001f70:	48 8d 84 24 f8 00 00 	lea    0xf8(%rsp),%rax
   140001f77:	00
   140001f78:	48 89 44 24 50       	mov    %rax,0x50(%rsp)
   140001f7d:	66 0f ef c0          	pxor   %xmm0,%xmm0
   140001f81:	f3 0f 7f 44 24 40    	movdqu %xmm0,0x40(%rsp)
   140001f87:	f3 0f 7f 44 24 30    	movdqu %xmm0,0x30(%rsp)
   140001f8d:	48 8d 05 8c 40 08 00 	lea    0x8408c(%rip),%rax        # 140086020 <.rdata+0x20>
   140001f94:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   140001f99:	48 c7 44 24 28 01 00 	movq   $0x1,0x28(%rsp)
   140001fa0:	00 00
   140001fa2:	48 8d 0d 2a 4c 08 00 	lea    0x84c2a(%rip),%rcx        # 140086bd3 <.rdata+0xbd3>
   140001fa9:	48 8d 15 1b 4c 08 00 	lea    0x84c1b(%rip),%rdx        # 140086bcb <.rdata+0xbcb>
   140001fb0:	4c 8d 05 59 40 08 00 	lea    0x84059(%rip),%r8        # 140086010 <.rdata+0x10>
   140001fb7:	41 b9 01 00 00 00    	mov    $0x1,%r9d
   140001fbd:	e8 ee 25 00 00       	call   1400045b0 <txrt_class_new>
   140001fc2:	85 c0                	test   %eax,%eax
   140001fc4:	0f 85 9c 06 00 00    	jne    140002666 <tx_fn_m0_bench_fields_0+0x936>
   140001fca:	48 8b 8c 24 f8 00 00 	mov    0xf8(%rsp),%rcx
   140001fd1:	00
   140001fd2:	48 8d 05 67 4c 08 00 	lea    0x84c67(%rip),%rax        # 140086c40 <.rdata+0xc40>
   140001fd9:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   140001fde:	48 c7 44 24 68 35 00 	movq   $0x35,0x68(%rsp)
   140001fe5:	00 00
   140001fe7:	48 c7 44 24 70 05 00 	movq   $0x5,0x70(%rsp)
   140001fee:	00 00
   140001ff0:	48 8b 3e             	mov    (%rsi),%rdi
   140001ff3:	48 8d 05 06 67 08 00 	lea    0x86706(%rip),%rax        # 140088700 <.rdata+0x2700>
   140001ffa:	48 89 84 24 80 00 00 	mov    %rax,0x80(%rsp)
   140002001:	00
   140002002:	48 8d 05 07 67 08 00 	lea    0x86707(%rip),%rax        # 140088710 <.rdata+0x2710>
   140002009:	48 89 84 24 88 00 00 	mov    %rax,0x88(%rsp)
   140002010:	00
   140002011:	48 c7 84 24 90 00 00 	movq   $0xe,0x90(%rsp)
   140002018:	00 0e 00 00 00
   14000201d:	48 c7 84 24 98 00 00 	movq   $0x9,0x98(%rsp)
   140002024:	00 09 00 00 00
   140002029:	48 89 bc 24 a0 00 00 	mov    %rdi,0xa0(%rsp)
   140002030:	00
   140002031:	48 8d 84 24 80 00 00 	lea    0x80(%rsp),%rax
   140002038:	00
   140002039:	48 89 06             	mov    %rax,(%rsi)
   14000203c:	48 89 8c 24 a8 00 00 	mov    %rcx,0xa8(%rsp)
   140002043:	00
   140002044:	31 d2                	xor    %edx,%edx
   140002046:	e8 85 19 00 00       	call   1400039d0 <txrt_class_field_i64_ptr>
   14000204b:	48 c7 00 00 00 00 00 	movq   $0x0,(%rax)
   140002052:	48 89 3e             	mov    %rdi,(%rsi)
   140002055:	48 89 f1             	mov    %rsi,%rcx
   140002058:	e8 93 eb 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   14000205d:	85 c0                	test   %eax,%eax
   14000205f:	0f 85 10 06 00 00    	jne    140002675 <tx_fn_m0_bench_fields_0+0x945>
   140002065:	48 8d 8c 24 f0 00 00 	lea    0xf0(%rsp),%rcx
   14000206c:	00
   14000206d:	e8 7e 94 00 00       	call   14000b4f0 <txrt_time_monotonic_micros>
   140002072:	85 c0                	test   %eax,%eax
   140002074:	0f 85 0a 06 00 00    	jne    140002684 <tx_fn_m0_bench_fields_0+0x954>
   14000207a:	4c 89 bc 24 c0 00 00 	mov    %r15,0xc0(%rsp)
   140002081:	00
   140002082:	48 8b bc 24 f0 00 00 	mov    0xf0(%rsp),%rdi
   140002089:	00
   14000208a:	48 89 f1             	mov    %rsi,%rcx
   14000208d:	e8 5e eb 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002092:	85 c0                	test   %eax,%eax
   140002094:	0f 85 f9 05 00 00    	jne    140002693 <tx_fn_m0_bench_fields_0+0x963>
   14000209a:	41 bf 40 42 0f 00    	mov    $0xf4240,%r15d
   1400020a0:	31 db                	xor    %ebx,%ebx
   1400020a2:	4c 8d 25 17 4d 08 00 	lea    0x84d17(%rip),%r12        # 140086dc0 <.rdata+0xdc0>
   1400020a9:	48 8d 05 30 67 08 00 	lea    0x86730(%rip),%rax        # 1400887e0 <.rdata+0x27e0>
   1400020b0:	66 48 0f 6e c0       	movq   %rax,%xmm0
   1400020b5:	48 8d 05 0f 67 08 00 	lea    0x8670f(%rip),%rax        # 1400887cb <.rdata+0x27cb>
   1400020bc:	66 48 0f 6e f0       	movq   %rax,%xmm6
   1400020c1:	66 0f 6c f0          	punpcklqdq %xmm0,%xmm6
   1400020c5:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
   1400020cc:	00 00 00 00
   1400020d0:	4c 89 64 24 60       	mov    %r12,0x60(%rsp)
   1400020d5:	48 c7 44 24 68 3a 00 	movq   $0x3a,0x68(%rsp)
   1400020dc:	00 00
   1400020de:	48 c7 44 24 70 09 00 	movq   $0x9,0x70(%rsp)
   1400020e5:	00 00
   1400020e7:	4c 8b 36             	mov    (%rsi),%r14
   1400020ea:	66 0f 7f b4 24 80 00 	movdqa %xmm6,0x80(%rsp)
   1400020f1:	00 00
   1400020f3:	48 c7 84 24 90 00 00 	movq   $0x12,0x90(%rsp)
   1400020fa:	00 12 00 00 00
   1400020ff:	48 c7 84 24 98 00 00 	movq   $0x9,0x98(%rsp)
   140002106:	00 09 00 00 00
   14000210b:	4c 89 b4 24 a0 00 00 	mov    %r14,0xa0(%rsp)
   140002112:	00
   140002113:	48 8d 84 24 80 00 00 	lea    0x80(%rsp),%rax
   14000211a:	00
   14000211b:	48 89 06             	mov    %rax,(%rsi)
   14000211e:	48 8b 8c 24 a8 00 00 	mov    0xa8(%rsp),%rcx
   140002125:	00
   140002126:	31 d2                	xor    %edx,%edx
   140002128:	e8 a3 18 00 00       	call   1400039d0 <txrt_class_field_i64_ptr>
   14000212d:	48 8b 08             	mov    (%rax),%rcx
   140002130:	48 89 ca             	mov    %rcx,%rdx
   140002133:	48 ff c2             	inc    %rdx
   140002136:	0f 80 75 03 00 00    	jo     1400024b1 <tx_fn_m0_bench_fields_0+0x781>
   14000213c:	48 89 10             	mov    %rdx,(%rax)
   14000213f:	4c 89 36             	mov    %r14,(%rsi)
   140002142:	49 89 de             	mov    %rbx,%r14
   140002145:	49 01 d6             	add    %rdx,%r14
   140002148:	0f 80 99 03 00 00    	jo     1400024e7 <tx_fn_m0_bench_fields_0+0x7b7>
   14000214e:	48 89 f1             	mov    %rsi,%rcx
   140002151:	e8 9a ea 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002156:	85 c0                	test   %eax,%eax
   140002158:	0f 85 aa 03 00 00    	jne    140002508 <tx_fn_m0_bench_fields_0+0x7d8>
   14000215e:	4c 89 f3             	mov    %r14,%rbx
   140002161:	49 ff cf             	dec    %r15
   140002164:	0f 85 66 ff ff ff    	jne    1400020d0 <tx_fn_m0_bench_fields_0+0x3a0>
   14000216a:	48 8d 0d 6a 4d 08 00 	lea    0x84d6a(%rip),%rcx        # 140086edb <.rdata+0xedb>
   140002171:	4c 8d 84 24 e8 00 00 	lea    0xe8(%rsp),%r8
   140002178:	00
   140002179:	ba 0d 00 00 00       	mov    $0xd,%edx
   14000217e:	e8 8d cc 01 00       	call   14001ee10 <txrt_str_new>
   140002183:	85 c0                	test   %eax,%eax
   140002185:	0f 85 17 05 00 00    	jne    1400026a2 <tx_fn_m0_bench_fields_0+0x972>
   14000218b:	48 8b 9c 24 e8 00 00 	mov    0xe8(%rsp),%rbx
   140002192:	00
   140002193:	48 8d 05 b6 4d 08 00 	lea    0x84db6(%rip),%rax        # 140086f50 <.rdata+0xf50>
   14000219a:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   14000219f:	48 c7 44 24 68 3c 00 	movq   $0x3c,0x68(%rsp)
   1400021a6:	00 00
   1400021a8:	48 c7 44 24 70 05 00 	movq   $0x5,0x70(%rsp)
   1400021af:	00 00
   1400021b1:	48 89 f1             	mov    %rsi,%rcx
   1400021b4:	48 89 da             	mov    %rbx,%rdx
   1400021b7:	49 89 f8             	mov    %rdi,%r8
   1400021ba:	4d 89 f1             	mov    %r14,%r9
   1400021bd:	e8 ce f6 ff ff       	call   140001890 <tx_fn_m0_report_0>
   1400021c2:	48 89 d9             	mov    %rbx,%rcx
   1400021c5:	e8 f6 cf 01 00       	call   14001f1c0 <txrt_str_release>
   1400021ca:	48 89 f1             	mov    %rsi,%rcx
   1400021cd:	e8 1e ea 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   1400021d2:	85 c0                	test   %eax,%eax
   1400021d4:	0f 85 d7 04 00 00    	jne    1400026b1 <tx_fn_m0_bench_fields_0+0x981>
   1400021da:	48 8d 8c 24 e0 00 00 	lea    0xe0(%rsp),%rcx
   1400021e1:	00
   1400021e2:	e8 09 93 00 00       	call   14000b4f0 <txrt_time_monotonic_micros>
   1400021e7:	85 c0                	test   %eax,%eax
   1400021e9:	0f 85 d1 04 00 00    	jne    1400026c0 <tx_fn_m0_bench_fields_0+0x990>
   1400021ef:	48 89 ac 24 c8 00 00 	mov    %rbp,0xc8(%rsp)
   1400021f6:	00
   1400021f7:	48 8b bc 24 e0 00 00 	mov    0xe0(%rsp),%rdi
   1400021fe:	00
   1400021ff:	48 89 f1             	mov    %rsi,%rcx
   140002202:	e8 e9 e9 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002207:	85 c0                	test   %eax,%eax
   140002209:	0f 85 c0 04 00 00    	jne    1400026cf <tx_fn_m0_bench_fields_0+0x99f>
   14000220f:	48 89 bc 24 b8 00 00 	mov    %rdi,0xb8(%rsp)
   140002216:	00
   140002217:	4c 89 ac 24 d0 00 00 	mov    %r13,0xd0(%rsp)
   14000221e:	00
   14000221f:	48 8d 0d aa 4e 08 00 	lea    0x84eaa(%rip),%rcx        # 1400870d0 <.rdata+0x10d0>
   140002226:	48 8d 15 9e 4e 08 00 	lea    0x84e9e(%rip),%rdx        # 1400870cb <.rdata+0x10cb>
   14000222d:	4c 8d 8c 24 b0 00 00 	lea    0xb0(%rsp),%r9
   140002234:	00
   140002235:	41 b8 02 00 00 00    	mov    $0x2,%r8d
   14000223b:	e8 10 63 00 00       	call   140008550 <txrt_struct_new>
   140002240:	85 c0                	test   %eax,%eax
   140002242:	0f 85 56 01 00 00    	jne    14000239e <tx_fn_m0_bench_fields_0+0x66e>
   140002248:	45 31 e4             	xor    %r12d,%r12d
   14000224b:	48 8d 05 ce 3e 08 00 	lea    0x83ece(%rip),%rax        # 140086120 <.rdata+0x120>
   140002252:	66 48 0f 6e c0       	movq   %rax,%xmm0
   140002257:	48 8d 05 b8 3e 08 00 	lea    0x83eb8(%rip),%rax        # 140086116 <.rdata+0x116>
   14000225e:	66 48 0f 6e f0       	movq   %rax,%xmm6
   140002263:	66 0f 6c f0          	punpcklqdq %xmm0,%xmm6
   140002267:	48 8d bc 24 b0 00 00 	lea    0xb0(%rsp),%rdi
   14000226e:	00
   14000226f:	45 31 f6             	xor    %r14d,%r14d
   140002272:	66 66 66 66 66 2e 0f 	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140002279:	1f 84 00 00 00 00 00
   140002280:	4c 8b ac 24 b0 00 00 	mov    0xb0(%rsp),%r13
   140002287:	00
   140002288:	4c 89 e9             	mov    %r13,%rcx
   14000228b:	31 d2                	xor    %edx,%edx
   14000228d:	4c 8d 05 a7 4e 08 00 	lea    0x84ea7(%rip),%r8        # 14008713b <.rdata+0x113b>
   140002294:	4d 89 e1             	mov    %r12,%r9
   140002297:	e8 d4 51 00 00       	call   140007470 <txrt_struct_set_field_i64>
   14000229c:	85 c0                	test   %eax,%eax
   14000229e:	0f 85 76 02 00 00    	jne    14000251a <tx_fn_m0_bench_fields_0+0x7ea>
   1400022a4:	ba 01 00 00 00       	mov    $0x1,%edx
   1400022a9:	41 b9 07 00 00 00    	mov    $0x7,%r9d
   1400022af:	4c 89 e9             	mov    %r13,%rcx
   1400022b2:	4c 8d 05 e2 4e 08 00 	lea    0x84ee2(%rip),%r8        # 14008719b <.rdata+0x119b>
   1400022b9:	e8 b2 51 00 00       	call   140007470 <txrt_struct_set_field_i64>
   1400022be:	85 c0                	test   %eax,%eax
   1400022c0:	0f 85 5d 02 00 00    	jne    140002523 <tx_fn_m0_bench_fields_0+0x7f3>
   1400022c6:	48 8d 05 43 4f 08 00 	lea    0x84f43(%rip),%rax        # 140087210 <.rdata+0x1210>
   1400022cd:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   1400022d2:	48 c7 44 24 68 41 00 	movq   $0x41,0x68(%rsp)
   1400022d9:	00 00
   1400022db:	48 c7 44 24 70 09 00 	movq   $0x9,0x70(%rsp)
   1400022e2:	00 00
   1400022e4:	48 8b 1e             	mov    (%rsi),%rbx
   1400022e7:	66 0f 7f b4 24 80 00 	movdqa %xmm6,0x80(%rsp)
   1400022ee:	00 00
   1400022f0:	48 c7 84 24 90 00 00 	movq   $0x6,0x90(%rsp)
   1400022f7:	00 06 00 00 00
   1400022fc:	48 c7 84 24 98 00 00 	movq   $0x1,0x98(%rsp)
   140002303:	00 01 00 00 00
   140002308:	48 89 9c 24 a0 00 00 	mov    %rbx,0xa0(%rsp)
   14000230f:	00
   140002310:	48 8d 84 24 80 00 00 	lea    0x80(%rsp),%rax
   140002317:	00
   140002318:	48 89 06             	mov    %rax,(%rsi)
   14000231b:	4c 89 e9             	mov    %r13,%rcx
   14000231e:	31 d2                	xor    %edx,%edx
   140002320:	e8 eb 5a 00 00       	call   140007e10 <txrt_struct_field_i64_ptr>
   140002325:	4c 8b 38             	mov    (%rax),%r15
   140002328:	ba 01 00 00 00       	mov    $0x1,%edx
   14000232d:	4c 89 e9             	mov    %r13,%rcx
   140002330:	e8 db 5a 00 00       	call   140007e10 <txrt_struct_field_i64_ptr>
   140002335:	48 8b 10             	mov    (%rax),%rdx
   140002338:	4c 89 fd             	mov    %r15,%rbp
   14000233b:	48 01 d5             	add    %rdx,%rbp
   14000233e:	0f 80 e8 01 00 00    	jo     14000252c <tx_fn_m0_bench_fields_0+0x7fc>
   140002344:	48 89 1e             	mov    %rbx,(%rsi)
   140002347:	4c 89 e9             	mov    %r13,%rcx
   14000234a:	e8 01 40 00 00       	call   140006350 <txrt_value_release>
   14000234f:	4d 89 f5             	mov    %r14,%r13
   140002352:	49 01 ed             	add    %rbp,%r13
   140002355:	0f 80 05 02 00 00    	jo     140002560 <tx_fn_m0_bench_fields_0+0x830>
   14000235b:	48 89 f1             	mov    %rsi,%rcx
   14000235e:	e8 8d e8 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002363:	85 c0                	test   %eax,%eax
   140002365:	0f 85 2c 02 00 00    	jne    140002597 <tx_fn_m0_bench_fields_0+0x867>
   14000236b:	49 ff c4             	inc    %r12
   14000236e:	49 81 fc a0 86 01 00 	cmp    $0x186a0,%r12
   140002375:	74 4b                	je     1400023c2 <tx_fn_m0_bench_fields_0+0x692>
   140002377:	41 b8 02 00 00 00    	mov    $0x2,%r8d
   14000237d:	48 8d 0d 4c 4d 08 00 	lea    0x84d4c(%rip),%rcx        # 1400870d0 <.rdata+0x10d0>
   140002384:	48 8d 15 40 4d 08 00 	lea    0x84d40(%rip),%rdx        # 1400870cb <.rdata+0x10cb>
   14000238b:	49 89 f9             	mov    %rdi,%r9
   14000238e:	e8 bd 61 00 00       	call   140008550 <txrt_struct_new>
   140002393:	4d 89 ee             	mov    %r13,%r14
   140002396:	85 c0                	test   %eax,%eax
   140002398:	0f 84 e2 fe ff ff    	je     140002280 <tx_fn_m0_bench_fields_0+0x550>
   14000239e:	89 c5                	mov    %eax,%ebp
   1400023a0:	48 8d 15 39 4d 08 00 	lea    0x84d39(%rip),%rdx        # 1400870e0 <.rdata+0x10e0>
   1400023a7:	41 b8 41 00 00 00    	mov    $0x41,%r8d
   1400023ad:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400023b3:	48 89 f1             	mov    %rsi,%rcx
   1400023b6:	e8 35 7d 01 00       	call   14001a0f0 <txrt_stack_error_location>
   1400023bb:	89 e9                	mov    %ebp,%ecx
   1400023bd:	e8 3e c4 01 00       	call   14001e800 <txrt_require_success>
   1400023c2:	48 8d 0d 62 4f 08 00 	lea    0x84f62(%rip),%rcx        # 14008732b <.rdata+0x132b>
   1400023c9:	4c 8d 84 24 d8 00 00 	lea    0xd8(%rsp),%r8
   1400023d0:	00
   1400023d1:	ba 0b 00 00 00       	mov    $0xb,%edx
   1400023d6:	e8 35 ca 01 00       	call   14001ee10 <txrt_str_new>
   1400023db:	85 c0                	test   %eax,%eax
   1400023dd:	0f 85 fb 02 00 00    	jne    1400026de <tx_fn_m0_bench_fields_0+0x9ae>
   1400023e3:	48 8b bc 24 d8 00 00 	mov    0xd8(%rsp),%rdi
   1400023ea:	00
   1400023eb:	48 8d 05 ae 4f 08 00 	lea    0x84fae(%rip),%rax        # 1400873a0 <.rdata+0x13a0>
   1400023f2:	48 89 44 24 60       	mov    %rax,0x60(%rsp)
   1400023f7:	48 c7 44 24 68 43 00 	movq   $0x43,0x68(%rsp)
   1400023fe:	00 00
   140002400:	48 c7 44 24 70 05 00 	movq   $0x5,0x70(%rsp)
   140002407:	00 00
   140002409:	48 89 f1             	mov    %rsi,%rcx
   14000240c:	48 89 fa             	mov    %rdi,%rdx
   14000240f:	4c 8b 84 24 b8 00 00 	mov    0xb8(%rsp),%r8
   140002416:	00
   140002417:	4d 89 e9             	mov    %r13,%r9
   14000241a:	e8 71 f4 ff ff       	call   140001890 <tx_fn_m0_report_0>
   14000241f:	48 89 f9             	mov    %rdi,%rcx
   140002422:	e8 99 cd 01 00       	call   14001f1c0 <txrt_str_release>
   140002427:	48 89 f1             	mov    %rsi,%rcx
   14000242a:	e8 c1 e7 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   14000242f:	85 c0                	test   %eax,%eax
   140002431:	48 8b bc 24 c8 00 00 	mov    0xc8(%rsp),%rdi
   140002438:	00
   140002439:	48 8b 9c 24 c0 00 00 	mov    0xc0(%rsp),%rbx
   140002440:	00
   140002441:	0f 85 a0 02 00 00    	jne    1400026e7 <tx_fn_m0_bench_fields_0+0x9b7>
   140002447:	48 8b 8c 24 a8 00 00 	mov    0xa8(%rsp),%rcx
   14000244e:	00
   14000244f:	e8 fc 3e 00 00       	call   140006350 <txrt_value_release>
   140002454:	48 89 d9             	mov    %rbx,%rcx
   140002457:	e8 f4 3e 00 00       	call   140006350 <txrt_value_release>
   14000245c:	48 89 f9             	mov    %rdi,%rcx
   14000245f:	e8 ec 3e 00 00       	call   140006350 <txrt_value_release>
   140002464:	48 8b 84 24 d0 00 00 	mov    0xd0(%rsp),%rax
   14000246b:	00
   14000246c:	48 89 06             	mov    %rax,(%rsi)
   14000246f:	0f 28 b4 24 50 01 00 	movaps 0x150(%rsp),%xmm6
   140002476:	00
   140002477:	48 81 c4 68 01 00 00 	add    $0x168,%rsp
   14000247e:	5b                   	pop    %rbx
   14000247f:	5d                   	pop    %rbp
   140002480:	5f                   	pop    %rdi
   140002481:	5e                   	pop    %rsi
   140002482:	41 5c                	pop    %r12
   140002484:	41 5d                	pop    %r13
   140002486:	41 5e                	pop    %r14
   140002488:	41 5f                	pop    %r15
   14000248a:	c3                   	ret
   14000248b:	4c 8d 84 24 40 01 00 	lea    0x140(%rsp),%r8
   140002492:	00
   140002493:	ba 02 00 00 00       	mov    $0x2,%edx
   140002498:	e8 63 d4 01 00       	call   14001f900 <txrt_add_i64>
   14000249d:	89 c7                	mov    %eax,%edi
   14000249f:	48 8d 15 3a 45 08 00 	lea    0x8453a(%rip),%rdx        # 1400869e0 <.rdata+0x9e0>
   1400024a6:	41 b8 32 00 00 00    	mov    $0x32,%r8d
   1400024ac:	e9 d1 00 00 00       	jmp    140002582 <tx_fn_m0_bench_fields_0+0x852>
   1400024b1:	4c 8d 84 24 20 01 00 	lea    0x120(%rsp),%r8
   1400024b8:	00
   1400024b9:	ba 01 00 00 00       	mov    $0x1,%edx
   1400024be:	e8 3d d4 01 00       	call   14001f900 <txrt_add_i64>
   1400024c3:	89 c7                	mov    %eax,%edi
   1400024c5:	48 8d 15 a4 62 08 00 	lea    0x862a4(%rip),%rdx        # 140088770 <.rdata+0x2770>
   1400024cc:	41 b8 14 00 00 00    	mov    $0x14,%r8d
   1400024d2:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   1400024d8:	48 89 f1             	mov    %rsi,%rcx
   1400024db:	e8 10 7c 01 00       	call   14001a0f0 <txrt_stack_error_location>
   1400024e0:	89 f9                	mov    %edi,%ecx
   1400024e2:	e8 19 c3 01 00       	call   14001e800 <txrt_require_success>
   1400024e7:	4c 8d 84 24 30 01 00 	lea    0x130(%rsp),%r8
   1400024ee:	00
   1400024ef:	48 89 d9             	mov    %rbx,%rcx
   1400024f2:	e8 09 d4 01 00       	call   14001f900 <txrt_add_i64>
   1400024f7:	89 c7                	mov    %eax,%edi
   1400024f9:	48 8d 15 20 49 08 00 	lea    0x84920(%rip),%rdx        # 140086e20 <.rdata+0xe20>
   140002500:	41 b8 3a 00 00 00    	mov    $0x3a,%r8d
   140002506:	eb 7a                	jmp    140002582 <tx_fn_m0_bench_fields_0+0x852>
   140002508:	48 8d 15 71 49 08 00 	lea    0x84971(%rip),%rdx        # 140086e80 <.rdata+0xe80>
   14000250f:	41 b8 3a 00 00 00    	mov    $0x3a,%r8d
   140002515:	e9 8a 00 00 00       	jmp    1400025a4 <tx_fn_m0_bench_fields_0+0x874>
   14000251a:	48 8d 15 1f 4c 08 00 	lea    0x84c1f(%rip),%rdx        # 140087140 <.rdata+0x1140>
   140002521:	eb 7b                	jmp    14000259e <tx_fn_m0_bench_fields_0+0x86e>
   140002523:	48 8d 15 86 4c 08 00 	lea    0x84c86(%rip),%rdx        # 1400871b0 <.rdata+0x11b0>
   14000252a:	eb 72                	jmp    14000259e <tx_fn_m0_bench_fields_0+0x86e>
   14000252c:	4c 8d 84 24 20 01 00 	lea    0x120(%rsp),%r8
   140002533:	00
   140002534:	4c 89 f9             	mov    %r15,%rcx
   140002537:	e8 c4 d3 01 00       	call   14001f900 <txrt_add_i64>
   14000253c:	89 c7                	mov    %eax,%edi
   14000253e:	48 8d 15 8b 3b 08 00 	lea    0x83b8b(%rip),%rdx        # 1400860d0 <.rdata+0xd0>
   140002545:	41 b8 08 00 00 00    	mov    $0x8,%r8d
   14000254b:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140002551:	48 89 f1             	mov    %rsi,%rcx
   140002554:	e8 97 7b 01 00       	call   14001a0f0 <txrt_stack_error_location>
   140002559:	89 f9                	mov    %edi,%ecx
   14000255b:	e8 a0 c2 01 00       	call   14001e800 <txrt_require_success>
   140002560:	4c 8d 84 24 28 01 00 	lea    0x128(%rsp),%r8
   140002567:	00
   140002568:	4c 89 f1             	mov    %r14,%rcx
   14000256b:	48 89 ea             	mov    %rbp,%rdx
   14000256e:	e8 8d d3 01 00       	call   14001f900 <txrt_add_i64>
   140002573:	89 c7                	mov    %eax,%edi
   140002575:	48 8d 15 f4 4c 08 00 	lea    0x84cf4(%rip),%rdx        # 140087270 <.rdata+0x1270>
   14000257c:	41 b8 41 00 00 00    	mov    $0x41,%r8d
   140002582:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002588:	48 89 f1             	mov    %rsi,%rcx
   14000258b:	e8 60 7b 01 00       	call   14001a0f0 <txrt_stack_error_location>
   140002590:	89 f9                	mov    %edi,%ecx
   140002592:	e8 69 c2 01 00       	call   14001e800 <txrt_require_success>
   140002597:	48 8d 15 32 4d 08 00 	lea    0x84d32(%rip),%rdx        # 1400872d0 <.rdata+0x12d0>
   14000259e:	41 b8 41 00 00 00    	mov    $0x41,%r8d
   1400025a4:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   1400025aa:	48 89 f1             	mov    %rsi,%rcx
   1400025ad:	89 c6                	mov    %eax,%esi
   1400025af:	e8 3c 7b 01 00       	call   14001a0f0 <txrt_stack_error_location>
   1400025b4:	89 f1                	mov    %esi,%ecx
   1400025b6:	e8 45 c2 01 00       	call   14001e800 <txrt_require_success>
   1400025bb:	48 8d 15 1e 41 08 00 	lea    0x8411e(%rip),%rdx        # 1400866e0 <.rdata+0x6e0>
   1400025c2:	eb 19                	jmp    1400025dd <tx_fn_m0_bench_fields_0+0x8ad>
   1400025c4:	48 8d 15 75 41 08 00 	lea    0x84175(%rip),%rdx        # 140086740 <.rdata+0x740>
   1400025cb:	eb 10                	jmp    1400025dd <tx_fn_m0_bench_fields_0+0x8ad>
   1400025cd:	48 8d 15 cc 41 08 00 	lea    0x841cc(%rip),%rdx        # 1400867a0 <.rdata+0x7a0>
   1400025d4:	eb 07                	jmp    1400025dd <tx_fn_m0_bench_fields_0+0x8ad>
   1400025d6:	48 8d 15 23 42 08 00 	lea    0x84223(%rip),%rdx        # 140086800 <.rdata+0x800>
   1400025dd:	41 b8 2c 00 00 00    	mov    $0x2c,%r8d
   1400025e3:	e9 0c 01 00 00       	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   1400025e8:	48 8d 15 71 42 08 00 	lea    0x84271(%rip),%rdx        # 140086860 <.rdata+0x860>
   1400025ef:	41 b8 2d 00 00 00    	mov    $0x2d,%r8d
   1400025f5:	e9 fa 00 00 00       	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   1400025fa:	48 8d 15 bf 42 08 00 	lea    0x842bf(%rip),%rdx        # 1400868c0 <.rdata+0x8c0>
   140002601:	41 b8 2e 00 00 00    	mov    $0x2e,%r8d
   140002607:	e9 e8 00 00 00       	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   14000260c:	48 8d 15 0d 43 08 00 	lea    0x8430d(%rip),%rdx        # 140086920 <.rdata+0x920>
   140002613:	41 b8 2e 00 00 00    	mov    $0x2e,%r8d
   140002619:	e9 d6 00 00 00       	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   14000261e:	48 8d 15 2b 44 08 00 	lea    0x8442b(%rip),%rdx        # 140086a50 <.rdata+0xa50>
   140002625:	41 b8 34 00 00 00    	mov    $0x34,%r8d
   14000262b:	e9 c4 00 00 00       	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   140002630:	4c 8d 84 24 38 01 00 	lea    0x138(%rsp),%r8
   140002637:	00
   140002638:	48 89 d9             	mov    %rbx,%rcx
   14000263b:	e8 c0 d2 01 00       	call   14001f900 <txrt_add_i64>
   140002640:	89 c7                	mov    %eax,%edi
   140002642:	48 8d 15 67 44 08 00 	lea    0x84467(%rip),%rdx        # 140086ab0 <.rdata+0xab0>
   140002649:	41 b8 34 00 00 00    	mov    $0x34,%r8d
   14000264f:	e9 f7 fe ff ff       	jmp    14000254b <tx_fn_m0_bench_fields_0+0x81b>
   140002654:	48 8d 15 15 45 08 00 	lea    0x84515(%rip),%rdx        # 140086b70 <.rdata+0xb70>
   14000265b:	41 b8 34 00 00 00    	mov    $0x34,%r8d
   140002661:	e9 8e 00 00 00       	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   140002666:	48 8d 15 73 45 08 00 	lea    0x84573(%rip),%rdx        # 140086be0 <.rdata+0xbe0>
   14000266d:	41 b8 35 00 00 00    	mov    $0x35,%r8d
   140002673:	eb 7f                	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   140002675:	48 8d 15 24 46 08 00 	lea    0x84624(%rip),%rdx        # 140086ca0 <.rdata+0xca0>
   14000267c:	41 b8 35 00 00 00    	mov    $0x35,%r8d
   140002682:	eb 70                	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   140002684:	48 8d 15 75 46 08 00 	lea    0x84675(%rip),%rdx        # 140086d00 <.rdata+0xd00>
   14000268b:	41 b8 37 00 00 00    	mov    $0x37,%r8d
   140002691:	eb 61                	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   140002693:	48 8d 15 c6 46 08 00 	lea    0x846c6(%rip),%rdx        # 140086d60 <.rdata+0xd60>
   14000269a:	41 b8 37 00 00 00    	mov    $0x37,%r8d
   1400026a0:	eb 52                	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   1400026a2:	48 8d 15 47 48 08 00 	lea    0x84847(%rip),%rdx        # 140086ef0 <.rdata+0xef0>
   1400026a9:	41 b8 3c 00 00 00    	mov    $0x3c,%r8d
   1400026af:	eb 43                	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   1400026b1:	48 8d 15 f8 48 08 00 	lea    0x848f8(%rip),%rdx        # 140086fb0 <.rdata+0xfb0>
   1400026b8:	41 b8 3c 00 00 00    	mov    $0x3c,%r8d
   1400026be:	eb 34                	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   1400026c0:	48 8d 15 49 49 08 00 	lea    0x84949(%rip),%rdx        # 140087010 <.rdata+0x1010>
   1400026c7:	41 b8 3e 00 00 00    	mov    $0x3e,%r8d
   1400026cd:	eb 25                	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   1400026cf:	48 8d 15 9a 49 08 00 	lea    0x8499a(%rip),%rdx        # 140087070 <.rdata+0x1070>
   1400026d6:	41 b8 3e 00 00 00    	mov    $0x3e,%r8d
   1400026dc:	eb 16                	jmp    1400026f4 <tx_fn_m0_bench_fields_0+0x9c4>
   1400026de:	48 8d 15 5b 4c 08 00 	lea    0x84c5b(%rip),%rdx        # 140087340 <.rdata+0x1340>
   1400026e5:	eb 07                	jmp    1400026ee <tx_fn_m0_bench_fields_0+0x9be>
   1400026e7:	48 8d 15 12 4d 08 00 	lea    0x84d12(%rip),%rdx        # 140087400 <.rdata+0x1400>
   1400026ee:	41 b8 43 00 00 00    	mov    $0x43,%r8d
   1400026f4:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   1400026fa:	e9 ab fe ff ff       	jmp    1400025aa <tx_fn_m0_bench_fields_0+0x87a>
   1400026ff:	90                   	nop


E:\Project\other\Compilation\tx_build\performance_09_11\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

00000001400027c0 <tx_fn_m0_bench_calls_0>:
   1400027c0:	41 57                	push   %r15
   1400027c2:	41 56                	push   %r14
   1400027c4:	41 55                	push   %r13
   1400027c6:	41 54                	push   %r12
   1400027c8:	56                   	push   %rsi
   1400027c9:	57                   	push   %rdi
   1400027ca:	55                   	push   %rbp
   1400027cb:	53                   	push   %rbx
   1400027cc:	48 81 ec b8 00 00 00 	sub    $0xb8,%rsp
   1400027d3:	48 89 ce             	mov    %rcx,%rsi
   1400027d6:	4c 8b 29             	mov    (%rcx),%r13
   1400027d9:	48 8d 05 8b 55 08 00 	lea    0x8558b(%rip),%rax        # 140087d6b <.rdata+0x1d6b>
   1400027e0:	48 89 44 24 38       	mov    %rax,0x38(%rsp)
   1400027e5:	48 8d 05 94 55 08 00 	lea    0x85594(%rip),%rax        # 140087d80 <.rdata+0x1d80>
   1400027ec:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   1400027f1:	48 c7 44 24 48 46 00 	movq   $0x46,0x48(%rsp)
   1400027f8:	00 00
   1400027fa:	48 c7 44 24 50 01 00 	movq   $0x1,0x50(%rsp)
   140002801:	00 00
   140002803:	4c 89 6c 24 58       	mov    %r13,0x58(%rsp)
   140002808:	48 8d 44 24 38       	lea    0x38(%rsp),%rax
   14000280d:	48 89 01             	mov    %rax,(%rcx)
   140002810:	48 8d 0d 29 f3 ff ff 	lea    -0xcd7(%rip),%rcx        # 140001b40 <tx_callback_m0_double_value_0>
   140002817:	48 8d 15 ad 4c 08 00 	lea    0x84cad(%rip),%rdx        # 1400874cb <.rdata+0x14cb>
   14000281e:	4c 8d 84 24 a0 00 00 	lea    0xa0(%rsp),%r8
   140002825:	00
   140002826:	e8 b5 56 01 00       	call   140017ee0 <txrt_closure_new>
   14000282b:	85 c0                	test   %eax,%eax
   14000282d:	0f 85 b3 03 00 00    	jne    140002be6 <tx_fn_m0_bench_calls_0+0x426>
   140002833:	48 8b bc 24 a0 00 00 	mov    0xa0(%rsp),%rdi
   14000283a:	00
   14000283b:	48 89 f1             	mov    %rsi,%rcx
   14000283e:	e8 ad e3 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002843:	85 c0                	test   %eax,%eax
   140002845:	0f 85 a4 03 00 00    	jne    140002bef <tx_fn_m0_bench_calls_0+0x42f>
   14000284b:	48 8d 8c 24 98 00 00 	lea    0x98(%rsp),%rcx
   140002852:	00
   140002853:	e8 98 8c 00 00       	call   14000b4f0 <txrt_time_monotonic_micros>
   140002858:	85 c0                	test   %eax,%eax
   14000285a:	0f 85 a1 03 00 00    	jne    140002c01 <tx_fn_m0_bench_calls_0+0x441>
   140002860:	48 8b 9c 24 98 00 00 	mov    0x98(%rsp),%rbx
   140002867:	00
   140002868:	48 89 f1             	mov    %rsi,%rcx
   14000286b:	e8 80 e3 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002870:	85 c0                	test   %eax,%eax
   140002872:	0f 85 9b 03 00 00    	jne    140002c13 <tx_fn_m0_bench_calls_0+0x453>
   140002878:	48 89 f9             	mov    %rdi,%rcx
   14000287b:	e8 90 49 01 00       	call   140017210 <txrt_closure_code>
   140002880:	4c 8d 25 d9 4d 08 00 	lea    0x84dd9(%rip),%r12        # 140087660 <.rdata+0x1660>
   140002887:	4c 89 64 24 40       	mov    %r12,0x40(%rsp)
   14000288c:	48 c7 44 24 48 4d 00 	movq   $0x4d,0x48(%rsp)
   140002893:	00 00
   140002895:	48 c7 44 24 50 09 00 	movq   $0x9,0x50(%rsp)
   14000289c:	00 00
   14000289e:	48 89 f9             	mov    %rdi,%rcx
   1400028a1:	31 d2                	xor    %edx,%edx
   1400028a3:	ff d0                	call   *%rax
   1400028a5:	48 89 c1             	mov    %rax,%rcx
   1400028a8:	41 bf 01 00 00 00    	mov    $0x1,%r15d
   1400028ae:	66 90                	xchg   %ax,%ax
   1400028b0:	49 89 ce             	mov    %rcx,%r14
   1400028b3:	48 89 f1             	mov    %rsi,%rcx
   1400028b6:	e8 35 e3 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   1400028bb:	85 c0                	test   %eax,%eax
   1400028bd:	0f 85 f3 02 00 00    	jne    140002bb6 <tx_fn_m0_bench_calls_0+0x3f6>
   1400028c3:	49 81 ff 40 42 0f 00 	cmp    $0xf4240,%r15
   1400028ca:	74 59                	je     140002925 <tx_fn_m0_bench_calls_0+0x165>
   1400028cc:	48 89 f9             	mov    %rdi,%rcx
   1400028cf:	e8 3c 49 01 00       	call   140017210 <txrt_closure_code>
   1400028d4:	4c 89 64 24 40       	mov    %r12,0x40(%rsp)
   1400028d9:	48 c7 44 24 48 4d 00 	movq   $0x4d,0x48(%rsp)
   1400028e0:	00 00
   1400028e2:	48 c7 44 24 50 09 00 	movq   $0x9,0x50(%rsp)
   1400028e9:	00 00
   1400028eb:	48 89 f9             	mov    %rdi,%rcx
   1400028ee:	4c 89 fa             	mov    %r15,%rdx
   1400028f1:	ff d0                	call   *%rax
   1400028f3:	49 ff c7             	inc    %r15
   1400028f6:	4c 89 f1             	mov    %r14,%rcx
   1400028f9:	48 01 c1             	add    %rax,%rcx
   1400028fc:	71 b2                	jno    1400028b0 <tx_fn_m0_bench_calls_0+0xf0>
   1400028fe:	4c 8d 84 24 b0 00 00 	lea    0xb0(%rsp),%r8
   140002905:	00
   140002906:	4c 89 f1             	mov    %r14,%rcx
   140002909:	48 89 c2             	mov    %rax,%rdx
   14000290c:	e8 ef cf 01 00       	call   14001f900 <txrt_add_i64>
   140002911:	89 c7                	mov    %eax,%edi
   140002913:	48 8d 15 a6 4d 08 00 	lea    0x84da6(%rip),%rdx        # 1400876c0 <.rdata+0x16c0>
   14000291a:	41 b8 4d 00 00 00    	mov    $0x4d,%r8d
   140002920:	e9 eb 01 00 00       	jmp    140002b10 <tx_fn_m0_bench_calls_0+0x350>
   140002925:	48 8d 0d 4f 4e 08 00 	lea    0x84e4f(%rip),%rcx        # 14008777b <.rdata+0x177b>
   14000292c:	4c 8d 84 24 90 00 00 	lea    0x90(%rsp),%r8
   140002933:	00
   140002934:	ba 0e 00 00 00       	mov    $0xe,%edx
   140002939:	e8 d2 c4 01 00       	call   14001ee10 <txrt_str_new>
   14000293e:	85 c0                	test   %eax,%eax
   140002940:	0f 85 df 02 00 00    	jne    140002c25 <tx_fn_m0_bench_calls_0+0x465>
   140002946:	4c 8b bc 24 90 00 00 	mov    0x90(%rsp),%r15
   14000294d:	00
   14000294e:	48 8d 05 9b 4e 08 00 	lea    0x84e9b(%rip),%rax        # 1400877f0 <.rdata+0x17f0>
   140002955:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   14000295a:	48 c7 44 24 48 4f 00 	movq   $0x4f,0x48(%rsp)
   140002961:	00 00
   140002963:	48 c7 44 24 50 05 00 	movq   $0x5,0x50(%rsp)
   14000296a:	00 00
   14000296c:	48 89 f1             	mov    %rsi,%rcx
   14000296f:	4c 89 fa             	mov    %r15,%rdx
   140002972:	49 89 d8             	mov    %rbx,%r8
   140002975:	4d 89 f1             	mov    %r14,%r9
   140002978:	e8 13 ef ff ff       	call   140001890 <tx_fn_m0_report_0>
   14000297d:	4c 89 f9             	mov    %r15,%rcx
   140002980:	e8 3b c8 01 00       	call   14001f1c0 <txrt_str_release>
   140002985:	48 89 f1             	mov    %rsi,%rcx
   140002988:	e8 63 e2 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   14000298d:	85 c0                	test   %eax,%eax
   14000298f:	0f 85 9f 02 00 00    	jne    140002c34 <tx_fn_m0_bench_calls_0+0x474>
   140002995:	48 8d 0d e4 f2 ff ff 	lea    -0xd1c(%rip),%rcx        # 140001c80 <tx_callback_m0_add_0>
   14000299c:	48 8d 15 0d 4f 08 00 	lea    0x84f0d(%rip),%rdx        # 1400878b0 <.rdata+0x18b0>
   1400029a3:	4c 8d 84 24 88 00 00 	lea    0x88(%rsp),%r8
   1400029aa:	00
   1400029ab:	e8 30 55 01 00       	call   140017ee0 <txrt_closure_new>
   1400029b0:	85 c0                	test   %eax,%eax
   1400029b2:	0f 85 8b 02 00 00    	jne    140002c43 <tx_fn_m0_bench_calls_0+0x483>
   1400029b8:	48 8b 9c 24 88 00 00 	mov    0x88(%rsp),%rbx
   1400029bf:	00
   1400029c0:	48 8d 94 24 80 00 00 	lea    0x80(%rsp),%rdx
   1400029c7:	00
   1400029c8:	b9 05 00 00 00       	mov    $0x5,%ecx
   1400029cd:	e8 4e 33 00 00       	call   140005d20 <txrt_value_box_i64>
   1400029d2:	85 c0                	test   %eax,%eax
   1400029d4:	0f 85 78 02 00 00    	jne    140002c52 <tx_fn_m0_bench_calls_0+0x492>
   1400029da:	4c 8b b4 24 80 00 00 	mov    0x80(%rsp),%r14
   1400029e1:	00
   1400029e2:	4c 89 74 24 78       	mov    %r14,0x78(%rsp)
   1400029e7:	48 8d 44 24 70       	lea    0x70(%rsp),%rax
   1400029ec:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   1400029f1:	48 c7 44 24 20 01 00 	movq   $0x1,0x20(%rsp)
   1400029f8:	00 00
   1400029fa:	48 8d 15 2f fd ff ff 	lea    -0x2d1(%rip),%rdx        # 140002730 <tx_bind_0>
   140002a01:	4c 8d 05 83 4f 08 00 	lea    0x84f83(%rip),%r8        # 14008798b <.rdata+0x198b>
   140002a08:	4c 8d 4c 24 78       	lea    0x78(%rsp),%r9
   140002a0d:	48 89 d9             	mov    %rbx,%rcx
   140002a10:	e8 4b 5a 01 00       	call   140018460 <txrt_closure_bind>
   140002a15:	85 c0                	test   %eax,%eax
   140002a17:	0f 85 44 02 00 00    	jne    140002c61 <tx_fn_m0_bench_calls_0+0x4a1>
   140002a1d:	4c 89 f1             	mov    %r14,%rcx
   140002a20:	e8 2b 39 00 00       	call   140006350 <txrt_value_release>
   140002a25:	48 89 d9             	mov    %rbx,%rcx
   140002a28:	e8 23 39 00 00       	call   140006350 <txrt_value_release>
   140002a2d:	48 8b 5c 24 70       	mov    0x70(%rsp),%rbx
   140002a32:	48 89 f1             	mov    %rsi,%rcx
   140002a35:	e8 b6 e1 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002a3a:	85 c0                	test   %eax,%eax
   140002a3c:	0f 85 2e 02 00 00    	jne    140002c70 <tx_fn_m0_bench_calls_0+0x4b0>
   140002a42:	48 8d 4c 24 68       	lea    0x68(%rsp),%rcx
   140002a47:	e8 a4 8a 00 00       	call   14000b4f0 <txrt_time_monotonic_micros>
   140002a4c:	85 c0                	test   %eax,%eax
   140002a4e:	0f 85 2b 02 00 00    	jne    140002c7f <tx_fn_m0_bench_calls_0+0x4bf>
   140002a54:	4c 8b 74 24 68       	mov    0x68(%rsp),%r14
   140002a59:	48 89 f1             	mov    %rsi,%rcx
   140002a5c:	e8 8f e1 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002a61:	85 c0                	test   %eax,%eax
   140002a63:	0f 85 25 02 00 00    	jne    140002c8e <tx_fn_m0_bench_calls_0+0x4ce>
   140002a69:	48 89 d9             	mov    %rbx,%rcx
   140002a6c:	e8 9f 47 01 00       	call   140017210 <txrt_closure_code>
   140002a71:	48 8d 2d a8 50 08 00 	lea    0x850a8(%rip),%rbp        # 140087b20 <.rdata+0x1b20>
   140002a78:	48 89 6c 24 40       	mov    %rbp,0x40(%rsp)
   140002a7d:	48 c7 44 24 48 55 00 	movq   $0x55,0x48(%rsp)
   140002a84:	00 00
   140002a86:	48 c7 44 24 50 09 00 	movq   $0x9,0x50(%rsp)
   140002a8d:	00 00
   140002a8f:	48 89 d9             	mov    %rbx,%rcx
   140002a92:	31 d2                	xor    %edx,%edx
   140002a94:	ff d0                	call   *%rax
   140002a96:	48 89 c1             	mov    %rax,%rcx
   140002a99:	41 bc 01 00 00 00    	mov    $0x1,%r12d
   140002a9f:	90                   	nop
   140002aa0:	49 89 cf             	mov    %rcx,%r15
   140002aa3:	48 89 f1             	mov    %rsi,%rcx
   140002aa6:	e8 45 e1 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002aab:	85 c0                	test   %eax,%eax
   140002aad:	0f 85 1b 01 00 00    	jne    140002bce <tx_fn_m0_bench_calls_0+0x40e>
   140002ab3:	49 81 fc 40 42 0f 00 	cmp    $0xf4240,%r12
   140002aba:	74 69                	je     140002b25 <tx_fn_m0_bench_calls_0+0x365>
   140002abc:	48 89 d9             	mov    %rbx,%rcx
   140002abf:	e8 4c 47 01 00       	call   140017210 <txrt_closure_code>
   140002ac4:	48 89 6c 24 40       	mov    %rbp,0x40(%rsp)
   140002ac9:	48 c7 44 24 48 55 00 	movq   $0x55,0x48(%rsp)
   140002ad0:	00 00
   140002ad2:	48 c7 44 24 50 09 00 	movq   $0x9,0x50(%rsp)
   140002ad9:	00 00
   140002adb:	48 89 d9             	mov    %rbx,%rcx
   140002ade:	4c 89 e2             	mov    %r12,%rdx
   140002ae1:	ff d0                	call   *%rax
   140002ae3:	49 ff c4             	inc    %r12
   140002ae6:	4c 89 f9             	mov    %r15,%rcx
   140002ae9:	48 01 c1             	add    %rax,%rcx
   140002aec:	71 b2                	jno    140002aa0 <tx_fn_m0_bench_calls_0+0x2e0>
   140002aee:	4c 8d 84 24 a8 00 00 	lea    0xa8(%rsp),%r8
   140002af5:	00
   140002af6:	4c 89 f9             	mov    %r15,%rcx
   140002af9:	48 89 c2             	mov    %rax,%rdx
   140002afc:	e8 ff cd 01 00       	call   14001f900 <txrt_add_i64>
   140002b01:	89 c7                	mov    %eax,%edi
   140002b03:	48 8d 15 76 50 08 00 	lea    0x85076(%rip),%rdx        # 140087b80 <.rdata+0x1b80>
   140002b0a:	41 b8 55 00 00 00    	mov    $0x55,%r8d
   140002b10:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002b16:	48 89 f1             	mov    %rsi,%rcx
   140002b19:	e8 d2 75 01 00       	call   14001a0f0 <txrt_stack_error_location>
   140002b1e:	89 f9                	mov    %edi,%ecx
   140002b20:	e8 db bc 01 00       	call   14001e800 <txrt_require_success>
   140002b25:	48 8d 0d 0f 51 08 00 	lea    0x8510f(%rip),%rcx        # 140087c3b <.rdata+0x1c3b>
   140002b2c:	4c 8d 44 24 60       	lea    0x60(%rsp),%r8
   140002b31:	ba 0c 00 00 00       	mov    $0xc,%edx
   140002b36:	e8 d5 c2 01 00       	call   14001ee10 <txrt_str_new>
   140002b3b:	85 c0                	test   %eax,%eax
   140002b3d:	0f 85 5a 01 00 00    	jne    140002c9d <tx_fn_m0_bench_calls_0+0x4dd>
   140002b43:	4c 8b 64 24 60       	mov    0x60(%rsp),%r12
   140002b48:	48 8d 05 61 51 08 00 	lea    0x85161(%rip),%rax        # 140087cb0 <.rdata+0x1cb0>
   140002b4f:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   140002b54:	48 c7 44 24 48 57 00 	movq   $0x57,0x48(%rsp)
   140002b5b:	00 00
   140002b5d:	48 c7 44 24 50 05 00 	movq   $0x5,0x50(%rsp)
   140002b64:	00 00
   140002b66:	48 89 f1             	mov    %rsi,%rcx
   140002b69:	4c 89 e2             	mov    %r12,%rdx
   140002b6c:	4d 89 f0             	mov    %r14,%r8
   140002b6f:	4d 89 f9             	mov    %r15,%r9
   140002b72:	e8 19 ed ff ff       	call   140001890 <tx_fn_m0_report_0>
   140002b77:	4c 89 e1             	mov    %r12,%rcx
   140002b7a:	e8 41 c6 01 00       	call   14001f1c0 <txrt_str_release>
   140002b7f:	48 89 f1             	mov    %rsi,%rcx
   140002b82:	e8 69 e0 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002b87:	85 c0                	test   %eax,%eax
   140002b89:	0f 85 17 01 00 00    	jne    140002ca6 <tx_fn_m0_bench_calls_0+0x4e6>
   140002b8f:	48 89 d9             	mov    %rbx,%rcx
   140002b92:	e8 b9 37 00 00       	call   140006350 <txrt_value_release>
   140002b97:	48 89 f9             	mov    %rdi,%rcx
   140002b9a:	e8 b1 37 00 00       	call   140006350 <txrt_value_release>
   140002b9f:	4c 89 2e             	mov    %r13,(%rsi)
   140002ba2:	48 81 c4 b8 00 00 00 	add    $0xb8,%rsp
   140002ba9:	5b                   	pop    %rbx
   140002baa:	5d                   	pop    %rbp
   140002bab:	5f                   	pop    %rdi
   140002bac:	5e                   	pop    %rsi
   140002bad:	41 5c                	pop    %r12
   140002baf:	41 5d                	pop    %r13
   140002bb1:	41 5e                	pop    %r14
   140002bb3:	41 5f                	pop    %r15
   140002bb5:	c3                   	ret
   140002bb6:	48 8d 15 63 4b 08 00 	lea    0x84b63(%rip),%rdx        # 140087720 <.rdata+0x1720>
   140002bbd:	41 b8 4d 00 00 00    	mov    $0x4d,%r8d
   140002bc3:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002bc9:	e9 eb 00 00 00       	jmp    140002cb9 <tx_fn_m0_bench_calls_0+0x4f9>
   140002bce:	48 8d 15 0b 50 08 00 	lea    0x8500b(%rip),%rdx        # 140087be0 <.rdata+0x1be0>
   140002bd5:	41 b8 55 00 00 00    	mov    $0x55,%r8d
   140002bdb:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002be1:	e9 d3 00 00 00       	jmp    140002cb9 <tx_fn_m0_bench_calls_0+0x4f9>
   140002be6:	48 8d 15 f3 48 08 00 	lea    0x848f3(%rip),%rdx        # 1400874e0 <.rdata+0x14e0>
   140002bed:	eb 07                	jmp    140002bf6 <tx_fn_m0_bench_calls_0+0x436>
   140002bef:	48 8d 15 4a 49 08 00 	lea    0x8494a(%rip),%rdx        # 140087540 <.rdata+0x1540>
   140002bf6:	41 b8 48 00 00 00    	mov    $0x48,%r8d
   140002bfc:	e9 b2 00 00 00       	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c01:	48 8d 15 98 49 08 00 	lea    0x84998(%rip),%rdx        # 1400875a0 <.rdata+0x15a0>
   140002c08:	41 b8 4a 00 00 00    	mov    $0x4a,%r8d
   140002c0e:	e9 a0 00 00 00       	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c13:	48 8d 15 e6 49 08 00 	lea    0x849e6(%rip),%rdx        # 140087600 <.rdata+0x1600>
   140002c1a:	41 b8 4a 00 00 00    	mov    $0x4a,%r8d
   140002c20:	e9 8e 00 00 00       	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c25:	48 8d 15 64 4b 08 00 	lea    0x84b64(%rip),%rdx        # 140087790 <.rdata+0x1790>
   140002c2c:	41 b8 4f 00 00 00    	mov    $0x4f,%r8d
   140002c32:	eb 7f                	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c34:	48 8d 15 15 4c 08 00 	lea    0x84c15(%rip),%rdx        # 140087850 <.rdata+0x1850>
   140002c3b:	41 b8 4f 00 00 00    	mov    $0x4f,%r8d
   140002c41:	eb 70                	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c43:	48 8d 15 86 4c 08 00 	lea    0x84c86(%rip),%rdx        # 1400878d0 <.rdata+0x18d0>
   140002c4a:	41 b8 50 00 00 00    	mov    $0x50,%r8d
   140002c50:	eb 61                	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c52:	48 8d 15 d7 4c 08 00 	lea    0x84cd7(%rip),%rdx        # 140087930 <.rdata+0x1930>
   140002c59:	41 b8 50 00 00 00    	mov    $0x50,%r8d
   140002c5f:	eb 52                	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c61:	48 8d 15 38 4d 08 00 	lea    0x84d38(%rip),%rdx        # 1400879a0 <.rdata+0x19a0>
   140002c68:	41 b8 50 00 00 00    	mov    $0x50,%r8d
   140002c6e:	eb 43                	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c70:	48 8d 15 89 4d 08 00 	lea    0x84d89(%rip),%rdx        # 140087a00 <.rdata+0x1a00>
   140002c77:	41 b8 50 00 00 00    	mov    $0x50,%r8d
   140002c7d:	eb 34                	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c7f:	48 8d 15 da 4d 08 00 	lea    0x84dda(%rip),%rdx        # 140087a60 <.rdata+0x1a60>
   140002c86:	41 b8 52 00 00 00    	mov    $0x52,%r8d
   140002c8c:	eb 25                	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c8e:	48 8d 15 2b 4e 08 00 	lea    0x84e2b(%rip),%rdx        # 140087ac0 <.rdata+0x1ac0>
   140002c95:	41 b8 52 00 00 00    	mov    $0x52,%r8d
   140002c9b:	eb 16                	jmp    140002cb3 <tx_fn_m0_bench_calls_0+0x4f3>
   140002c9d:	48 8d 15 ac 4f 08 00 	lea    0x84fac(%rip),%rdx        # 140087c50 <.rdata+0x1c50>
   140002ca4:	eb 07                	jmp    140002cad <tx_fn_m0_bench_calls_0+0x4ed>
   140002ca6:	48 8d 15 63 50 08 00 	lea    0x85063(%rip),%rdx        # 140087d10 <.rdata+0x1d10>
   140002cad:	41 b8 57 00 00 00    	mov    $0x57,%r8d
   140002cb3:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140002cb9:	48 89 f1             	mov    %rsi,%rcx
   140002cbc:	89 c6                	mov    %eax,%esi
   140002cbe:	e8 2d 74 01 00       	call   14001a0f0 <txrt_stack_error_location>
   140002cc3:	89 f1                	mov    %esi,%ecx
   140002cc5:	e8 36 bb 01 00       	call   14001e800 <txrt_require_success>
   140002cca:	cc                   	int3
   140002ccb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)


E:\Project\other\Compilation\tx_build\performance_09_11\baseline\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140002d00 <tx_fn_m0_bench_heap_0>:
   140002d00:	41 57                	push   %r15
   140002d02:	41 56                	push   %r14
   140002d04:	41 55                	push   %r13
   140002d06:	41 54                	push   %r12
   140002d08:	56                   	push   %rsi
   140002d09:	57                   	push   %rdi
   140002d0a:	55                   	push   %rbp
   140002d0b:	53                   	push   %rbx
   140002d0c:	48 81 ec 98 00 00 00 	sub    $0x98,%rsp
   140002d13:	48 89 ce             	mov    %rcx,%rsi
   140002d16:	48 8b 19             	mov    (%rcx),%rbx
   140002d19:	48 8d 05 2b 57 08 00 	lea    0x8572b(%rip),%rax        # 14008844b <.rdata+0x244b>
   140002d20:	48 89 44 24 68       	mov    %rax,0x68(%rsp)
   140002d25:	48 8d 05 34 57 08 00 	lea    0x85734(%rip),%rax        # 140088460 <.rdata+0x2460>
   140002d2c:	48 89 44 24 70       	mov    %rax,0x70(%rsp)
   140002d31:	48 c7 44 24 78 5a 00 	movq   $0x5a,0x78(%rsp)
   140002d38:	00 00
   140002d3a:	48 c7 84 24 80 00 00 	movq   $0x1,0x80(%rsp)
   140002d41:	00 01 00 00 00
   140002d46:	48 89 9c 24 88 00 00 	mov    %rbx,0x88(%rsp)
   140002d4d:	00
   140002d4e:	48 8d 44 24 68       	lea    0x68(%rsp),%rax
   140002d53:	48 89 01             	mov    %rax,(%rcx)
   140002d56:	48 8d 54 24 60       	lea    0x60(%rsp),%rdx
   140002d5b:	31 c9                	xor    %ecx,%ecx
   140002d5d:	e8 9e 18 01 00       	call   140014600 <txrt_heap_new_i64>
   140002d62:	85 c0                	test   %eax,%eax
   140002d64:	0f 85 0a 03 00 00    	jne    140003074 <tx_fn_m0_bench_heap_0+0x374>
   140002d6a:	48 8b 7c 24 60       	mov    0x60(%rsp),%rdi
   140002d6f:	48 89 f1             	mov    %rsi,%rcx
   140002d72:	e8 79 de 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002d77:	85 c0                	test   %eax,%eax
   140002d79:	0f 85 fe 02 00 00    	jne    14000307d <tx_fn_m0_bench_heap_0+0x37d>
   140002d7f:	48 8d 4c 24 58       	lea    0x58(%rsp),%rcx
   140002d84:	e8 67 87 00 00       	call   14000b4f0 <txrt_time_monotonic_micros>
   140002d89:	85 c0                	test   %eax,%eax
   140002d8b:	0f 85 fb 02 00 00    	jne    14000308c <tx_fn_m0_bench_heap_0+0x38c>
   140002d91:	4c 8b 74 24 58       	mov    0x58(%rsp),%r14
   140002d96:	48 89 f1             	mov    %rsi,%rcx
   140002d99:	e8 52 de 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002d9e:	85 c0                	test   %eax,%eax
   140002da0:	0f 85 f5 02 00 00    	jne    14000309b <tx_fn_m0_bench_heap_0+0x39b>
   140002da6:	4c 89 74 24 30       	mov    %r14,0x30(%rsp)
   140002dab:	48 8d 54 24 28       	lea    0x28(%rsp),%rdx
   140002db0:	48 89 f9             	mov    %rdi,%rcx
   140002db3:	e8 08 34 00 00       	call   1400061c0 <txrt_value_clone>
   140002db8:	85 c0                	test   %eax,%eax
   140002dba:	0f 85 7f 00 00 00    	jne    140002e3f <tx_fn_m0_bench_heap_0+0x13f>
   140002dc0:	48 89 5c 24 38       	mov    %rbx,0x38(%rsp)
   140002dc5:	41 bf 50 c3 00 00    	mov    $0xc350,%r15d
   140002dcb:	45 31 e4             	xor    %r12d,%r12d
   140002dce:	49 bd cf f7 53 e3 a5 	movabs $0x20c49ba5e353f7cf,%r13
   140002dd5:	9b c4 20
   140002dd8:	4c 8d 74 24 28       	lea    0x28(%rsp),%r14
   140002ddd:	0f 1f 00             	nopl   (%rax)
   140002de0:	4c 89 e0             	mov    %r12,%rax
   140002de3:	48 c1 e8 03          	shr    $0x3,%rax
   140002de7:	49 f7 e5             	mul    %r13
   140002dea:	48 c1 ea 04          	shr    $0x4,%rdx
   140002dee:	48 69 c2 e8 03 00 00 	imul   $0x3e8,%rdx,%rax
   140002df5:	4c 89 e2             	mov    %r12,%rdx
   140002df8:	48 29 c2             	sub    %rax,%rdx
   140002dfb:	48 8b 5c 24 28       	mov    0x28(%rsp),%rbx
   140002e00:	48 89 d9             	mov    %rbx,%rcx
   140002e03:	e8 98 f5 00 00       	call   1400123a0 <txrt_heap_push_i64>
   140002e08:	85 c0                	test   %eax,%eax
   140002e0a:	0f 85 c7 01 00 00    	jne    140002fd7 <tx_fn_m0_bench_heap_0+0x2d7>
   140002e10:	48 89 d9             	mov    %rbx,%rcx
   140002e13:	e8 38 35 00 00       	call   140006350 <txrt_value_release>
   140002e18:	48 89 f1             	mov    %rsi,%rcx
   140002e1b:	e8 d0 dd 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002e20:	85 c0                	test   %eax,%eax
   140002e22:	0f 85 b8 01 00 00    	jne    140002fe0 <tx_fn_m0_bench_heap_0+0x2e0>
   140002e28:	49 ff cf             	dec    %r15
   140002e2b:	74 26                	je     140002e53 <tx_fn_m0_bench_heap_0+0x153>
   140002e2d:	49 ff c4             	inc    %r12
   140002e30:	48 89 f9             	mov    %rdi,%rcx
   140002e33:	4c 89 f2             	mov    %r14,%rdx
   140002e36:	e8 85 33 00 00       	call   1400061c0 <txrt_value_clone>
   140002e3b:	85 c0                	test   %eax,%eax
   140002e3d:	74 a1                	je     140002de0 <tx_fn_m0_bench_heap_0+0xe0>
   140002e3f:	89 c7                	mov    %eax,%edi
   140002e41:	48 8d 15 18 51 08 00 	lea    0x85118(%rip),%rdx        # 140087f60 <.rdata+0x1f60>
   140002e48:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002e4e:	e9 c7 01 00 00       	jmp    14000301a <tx_fn_m0_bench_heap_0+0x31a>
   140002e53:	48 8d 54 24 20       	lea    0x20(%rsp),%rdx
   140002e58:	48 89 f9             	mov    %rdi,%rcx
   140002e5b:	e8 60 33 00 00       	call   1400061c0 <txrt_value_clone>
   140002e60:	85 c0                	test   %eax,%eax
   140002e62:	0f 85 b8 00 00 00    	jne    140002f20 <tx_fn_m0_bench_heap_0+0x220>
   140002e68:	41 be 50 c3 00 00    	mov    $0xc350,%r14d
   140002e6e:	31 ed                	xor    %ebp,%ebp
   140002e70:	4c 8d 7c 24 48       	lea    0x48(%rsp),%r15
   140002e75:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
   140002e7c:	00 00 00 00
   140002e80:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx
   140002e85:	48 89 d9             	mov    %rbx,%rcx
   140002e88:	48 8d 54 24 50       	lea    0x50(%rsp),%rdx
   140002e8d:	e8 5e f3 00 00       	call   1400121f0 <txrt_heap_top_i64>
   140002e92:	85 c0                	test   %eax,%eax
   140002e94:	0f 85 55 01 00 00    	jne    140002fef <tx_fn_m0_bench_heap_0+0x2ef>
   140002e9a:	4c 8b 64 24 50       	mov    0x50(%rsp),%r12
   140002e9f:	48 89 d9             	mov    %rbx,%rcx
   140002ea2:	e8 a9 34 00 00       	call   140006350 <txrt_value_release>
   140002ea7:	49 89 ed             	mov    %rbp,%r13
   140002eaa:	4d 01 e5             	add    %r12,%r13
   140002ead:	0f 80 45 01 00 00    	jo     140002ff8 <tx_fn_m0_bench_heap_0+0x2f8>
   140002eb3:	48 89 f1             	mov    %rsi,%rcx
   140002eb6:	e8 35 dd 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002ebb:	85 c0                	test   %eax,%eax
   140002ebd:	0f 85 6c 01 00 00    	jne    14000302f <tx_fn_m0_bench_heap_0+0x32f>
   140002ec3:	48 89 f9             	mov    %rdi,%rcx
   140002ec6:	4c 89 fa             	mov    %r15,%rdx
   140002ec9:	e8 f2 32 00 00       	call   1400061c0 <txrt_value_clone>
   140002ece:	85 c0                	test   %eax,%eax
   140002ed0:	0f 85 68 01 00 00    	jne    14000303e <tx_fn_m0_bench_heap_0+0x33e>
   140002ed6:	48 8b 5c 24 48       	mov    0x48(%rsp),%rbx
   140002edb:	48 89 d9             	mov    %rbx,%rcx
   140002ede:	e8 6d 13 01 00       	call   140014250 <txrt_heap_pop_i64>
   140002ee3:	85 c0                	test   %eax,%eax
   140002ee5:	0f 85 5c 01 00 00    	jne    140003047 <tx_fn_m0_bench_heap_0+0x347>
   140002eeb:	48 89 d9             	mov    %rbx,%rcx
   140002eee:	e8 5d 34 00 00       	call   140006350 <txrt_value_release>
   140002ef3:	48 89 f1             	mov    %rsi,%rcx
   140002ef6:	e8 f5 dc 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002efb:	85 c0                	test   %eax,%eax
   140002efd:	0f 85 4d 01 00 00    	jne    140003050 <tx_fn_m0_bench_heap_0+0x350>
   140002f03:	49 ff ce             	dec    %r14
   140002f06:	74 3c                	je     140002f44 <tx_fn_m0_bench_heap_0+0x244>
   140002f08:	48 89 f9             	mov    %rdi,%rcx
   140002f0b:	48 8d 54 24 20       	lea    0x20(%rsp),%rdx
   140002f10:	e8 ab 32 00 00       	call   1400061c0 <txrt_value_clone>
   140002f15:	4c 89 ed             	mov    %r13,%rbp
   140002f18:	85 c0                	test   %eax,%eax
   140002f1a:	0f 84 60 ff ff ff    	je     140002e80 <tx_fn_m0_bench_heap_0+0x180>
   140002f20:	89 c3                	mov    %eax,%ebx
   140002f22:	48 8d 15 57 51 08 00 	lea    0x85157(%rip),%rdx        # 140088080 <.rdata+0x2080>
   140002f29:	41 b8 65 00 00 00    	mov    $0x65,%r8d
   140002f2f:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002f35:	48 89 f1             	mov    %rsi,%rcx
   140002f38:	e8 b3 71 01 00       	call   14001a0f0 <txrt_stack_error_location>
   140002f3d:	89 d9                	mov    %ebx,%ecx
   140002f3f:	e8 bc b8 01 00       	call   14001e800 <txrt_require_success>
   140002f44:	48 8d 0d d0 53 08 00 	lea    0x853d0(%rip),%rcx        # 14008831b <.rdata+0x231b>
   140002f4b:	4c 8d 44 24 40       	lea    0x40(%rsp),%r8
   140002f50:	ba 0d 00 00 00       	mov    $0xd,%edx
   140002f55:	e8 b6 be 01 00       	call   14001ee10 <txrt_str_new>
   140002f5a:	85 c0                	test   %eax,%eax
   140002f5c:	0f 85 48 01 00 00    	jne    1400030aa <tx_fn_m0_bench_heap_0+0x3aa>
   140002f62:	48 8b 5c 24 40       	mov    0x40(%rsp),%rbx
   140002f67:	48 8d 05 22 54 08 00 	lea    0x85422(%rip),%rax        # 140088390 <.rdata+0x2390>
   140002f6e:	48 89 44 24 70       	mov    %rax,0x70(%rsp)
   140002f73:	48 c7 44 24 78 68 00 	movq   $0x68,0x78(%rsp)
   140002f7a:	00 00
   140002f7c:	48 c7 84 24 80 00 00 	movq   $0x5,0x80(%rsp)
   140002f83:	00 05 00 00 00
   140002f88:	48 89 f1             	mov    %rsi,%rcx
   140002f8b:	48 89 da             	mov    %rbx,%rdx
   140002f8e:	4c 8b 44 24 30       	mov    0x30(%rsp),%r8
   140002f93:	4d 89 e9             	mov    %r13,%r9
   140002f96:	e8 f5 e8 ff ff       	call   140001890 <tx_fn_m0_report_0>
   140002f9b:	48 89 d9             	mov    %rbx,%rcx
   140002f9e:	e8 1d c2 01 00       	call   14001f1c0 <txrt_str_release>
   140002fa3:	48 89 f1             	mov    %rsi,%rcx
   140002fa6:	e8 45 dc 00 00       	call   140010bf0 <txrt_gc_safepoint_context>
   140002fab:	85 c0                	test   %eax,%eax
   140002fad:	0f 85 00 01 00 00    	jne    1400030b3 <tx_fn_m0_bench_heap_0+0x3b3>
   140002fb3:	48 89 f9             	mov    %rdi,%rcx
   140002fb6:	e8 95 33 00 00       	call   140006350 <txrt_value_release>
   140002fbb:	48 8b 44 24 38       	mov    0x38(%rsp),%rax
   140002fc0:	48 89 06             	mov    %rax,(%rsi)
   140002fc3:	48 81 c4 98 00 00 00 	add    $0x98,%rsp
   140002fca:	5b                   	pop    %rbx
   140002fcb:	5d                   	pop    %rbp
   140002fcc:	5f                   	pop    %rdi
   140002fcd:	5e                   	pop    %rsi
   140002fce:	41 5c                	pop    %r12
   140002fd0:	41 5d                	pop    %r13
   140002fd2:	41 5e                	pop    %r14
   140002fd4:	41 5f                	pop    %r15
   140002fd6:	c3                   	ret
   140002fd7:	48 8d 15 e2 4f 08 00 	lea    0x84fe2(%rip),%rdx        # 140087fc0 <.rdata+0x1fc0>
   140002fde:	eb 07                	jmp    140002fe7 <tx_fn_m0_bench_heap_0+0x2e7>
   140002fe0:	48 8d 15 39 50 08 00 	lea    0x85039(%rip),%rdx        # 140088020 <.rdata+0x2020>
   140002fe7:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002fed:	eb 6e                	jmp    14000305d <tx_fn_m0_bench_heap_0+0x35d>
   140002fef:	48 8d 15 ea 50 08 00 	lea    0x850ea(%rip),%rdx        # 1400880e0 <.rdata+0x20e0>
   140002ff6:	eb 3e                	jmp    140003036 <tx_fn_m0_bench_heap_0+0x336>
   140002ff8:	4c 8d 84 24 90 00 00 	lea    0x90(%rsp),%r8
   140002fff:	00
   140003000:	48 89 e9             	mov    %rbp,%rcx
   140003003:	4c 89 e2             	mov    %r12,%rdx
   140003006:	e8 f5 c8 01 00       	call   14001f900 <txrt_add_i64>
   14000300b:	89 c7                	mov    %eax,%edi
   14000300d:	48 8d 15 2c 51 08 00 	lea    0x8512c(%rip),%rdx        # 140088140 <.rdata+0x2140>
   140003014:	41 b8 65 00 00 00    	mov    $0x65,%r8d
   14000301a:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140003020:	48 89 f1             	mov    %rsi,%rcx
   140003023:	e8 c8 70 01 00       	call   14001a0f0 <txrt_stack_error_location>
   140003028:	89 f9                	mov    %edi,%ecx
   14000302a:	e8 d1 b7 01 00       	call   14001e800 <txrt_require_success>
   14000302f:	48 8d 15 6a 51 08 00 	lea    0x8516a(%rip),%rdx        # 1400881a0 <.rdata+0x21a0>
   140003036:	41 b8 65 00 00 00    	mov    $0x65,%r8d
   14000303c:	eb 1f                	jmp    14000305d <tx_fn_m0_bench_heap_0+0x35d>
   14000303e:	48 8d 15 bb 51 08 00 	lea    0x851bb(%rip),%rdx        # 140088200 <.rdata+0x2200>
   140003045:	eb 10                	jmp    140003057 <tx_fn_m0_bench_heap_0+0x357>
   140003047:	48 8d 15 12 52 08 00 	lea    0x85212(%rip),%rdx        # 140088260 <.rdata+0x2260>
   14000304e:	eb 07                	jmp    140003057 <tx_fn_m0_bench_heap_0+0x357>
   140003050:	48 8d 15 69 52 08 00 	lea    0x85269(%rip),%rdx        # 1400882c0 <.rdata+0x22c0>
   140003057:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   14000305d:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140003063:	48 89 f1             	mov    %rsi,%rcx
   140003066:	89 c6                	mov    %eax,%esi
   140003068:	e8 83 70 01 00       	call   14001a0f0 <txrt_stack_error_location>
   14000306d:	89 f1                	mov    %esi,%ecx
   14000306f:	e8 8c b7 01 00       	call   14001e800 <txrt_require_success>
   140003074:	48 8d 15 65 4d 08 00 	lea    0x84d65(%rip),%rdx        # 140087de0 <.rdata+0x1de0>
   14000307b:	eb 07                	jmp    140003084 <tx_fn_m0_bench_heap_0+0x384>
   14000307d:	48 8d 15 bc 4d 08 00 	lea    0x84dbc(%rip),%rdx        # 140087e40 <.rdata+0x1e40>
   140003084:	41 b8 5c 00 00 00    	mov    $0x5c,%r8d
   14000308a:	eb 34                	jmp    1400030c0 <tx_fn_m0_bench_heap_0+0x3c0>
   14000308c:	48 8d 15 0d 4e 08 00 	lea    0x84e0d(%rip),%rdx        # 140087ea0 <.rdata+0x1ea0>
   140003093:	41 b8 5e 00 00 00    	mov    $0x5e,%r8d
   140003099:	eb 25                	jmp    1400030c0 <tx_fn_m0_bench_heap_0+0x3c0>
   14000309b:	48 8d 15 5e 4e 08 00 	lea    0x84e5e(%rip),%rdx        # 140087f00 <.rdata+0x1f00>
   1400030a2:	41 b8 5e 00 00 00    	mov    $0x5e,%r8d
   1400030a8:	eb 16                	jmp    1400030c0 <tx_fn_m0_bench_heap_0+0x3c0>
   1400030aa:	48 8d 15 7f 52 08 00 	lea    0x8527f(%rip),%rdx        # 140088330 <.rdata+0x2330>
   1400030b1:	eb 07                	jmp    1400030ba <tx_fn_m0_bench_heap_0+0x3ba>
   1400030b3:	48 8d 15 36 53 08 00 	lea    0x85336(%rip),%rdx        # 1400883f0 <.rdata+0x23f0>
   1400030ba:	41 b8 68 00 00 00    	mov    $0x68,%r8d
   1400030c0:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   1400030c6:	eb 9b                	jmp    140003063 <tx_fn_m0_bench_heap_0+0x363>
   1400030c8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
   1400030cf:	00
