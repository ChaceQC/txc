
E:\Project\other\Compilation\tx_build\performance_09_11\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001f60 <tx_fn_m0_bench_fields_0>:
   140001f60:	41 57                	push   %r15
   140001f62:	41 56                	push   %r14
   140001f64:	41 55                	push   %r13
   140001f66:	41 54                	push   %r12
   140001f68:	56                   	push   %rsi
   140001f69:	57                   	push   %rdi
   140001f6a:	55                   	push   %rbp
   140001f6b:	53                   	push   %rbx
   140001f6c:	48 81 ec 38 01 00 00 	sub    $0x138,%rsp
   140001f73:	66 0f 7f b4 24 20 01 	movdqa %xmm6,0x120(%rsp)
   140001f7a:	00 00
   140001f7c:	48 89 ce             	mov    %rcx,%rsi
   140001f7f:	4c 8b 29             	mov    (%rcx),%r13
   140001f82:	48 8d 05 72 b5 08 00 	lea    0x8b572(%rip),%rax        # 14008d4fb <.rdata+0x14fb>
   140001f89:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   140001f8e:	48 8d 05 7b b5 08 00 	lea    0x8b57b(%rip),%rax        # 14008d510 <.rdata+0x1510>
   140001f95:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   140001f9a:	48 c7 44 24 30 2a 00 	movq   $0x2a,0x30(%rsp)
   140001fa1:	00 00
   140001fa3:	48 c7 44 24 38 01 00 	movq   $0x1,0x38(%rsp)
   140001faa:	00 00
   140001fac:	4c 89 6c 24 40       	mov    %r13,0x40(%rsp)
   140001fb1:	48 8d 44 24 20       	lea    0x20(%rsp),%rax
   140001fb6:	48 89 01             	mov    %rax,(%rcx)
   140001fb9:	48 8d 0d 60 a2 08 00 	lea    0x8a260(%rip),%rcx        # 14008c220 <.rdata+0x220>
   140001fc0:	48 8d 94 24 e8 00 00 	lea    0xe8(%rsp),%rdx
   140001fc7:	00
   140001fc8:	e8 43 5a 01 00       	call   140017a10 <txrt_record_struct_new>
   140001fcd:	85 c0                	test   %eax,%eax
   140001fcf:	0f 85 bc 06 00 00    	jne    140002691 <tx_fn_m0_bench_fields_0+0x731>
   140001fd5:	48 8b ac 24 e8 00 00 	mov    0xe8(%rsp),%rbp
   140001fdc:	00
   140001fdd:	48 89 e9             	mov    %rbp,%rcx
   140001fe0:	e8 bb 45 01 00       	call   1400165a0 <txrt_record_struct_view>
   140001fe5:	48 8b 00             	mov    (%rax),%rax
   140001fe8:	48 c7 00 01 00 00 00 	movq   $0x1,(%rax)
   140001fef:	48 c7 40 08 02 00 00 	movq   $0x2,0x8(%rax)
   140001ff6:	00
   140001ff7:	48 89 e9             	mov    %rbp,%rcx
   140001ffa:	e8 a1 45 01 00       	call   1400165a0 <txrt_record_struct_view>
   140001fff:	49 89 c6             	mov    %rax,%r14
   140002002:	48 89 f1             	mov    %rsi,%rcx
   140002005:	e8 d6 b8 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   14000200a:	85 c0                	test   %eax,%eax
   14000200c:	0f 85 88 06 00 00    	jne    14000269a <tx_fn_m0_bench_fields_0+0x73a>
   140002012:	48 8d 94 24 e0 00 00 	lea    0xe0(%rsp),%rdx
   140002019:	00
   14000201a:	48 89 e9             	mov    %rbp,%rcx
   14000201d:	e8 7e e8 01 00       	call   1400208a0 <txrt_value_clone>
   140002022:	85 c0                	test   %eax,%eax
   140002024:	0f 85 82 06 00 00    	jne    1400026ac <tx_fn_m0_bench_fields_0+0x74c>
   14000202a:	48 8b 9c 24 e0 00 00 	mov    0xe0(%rsp),%rbx
   140002031:	00
   140002032:	48 89 d9             	mov    %rbx,%rcx
   140002035:	e8 66 45 01 00       	call   1400165a0 <txrt_record_struct_view>
   14000203a:	49 89 c4             	mov    %rax,%r12
   14000203d:	48 8d 8c 24 d8 00 00 	lea    0xd8(%rsp),%rcx
   140002044:	00
   140002045:	e8 c6 d9 01 00       	call   14001fa10 <txrt_time_monotonic_micros>
   14000204a:	85 c0                	test   %eax,%eax
   14000204c:	0f 85 6c 06 00 00    	jne    1400026be <tx_fn_m0_bench_fields_0+0x75e>
   140002052:	4c 8b bc 24 d8 00 00 	mov    0xd8(%rsp),%r15
   140002059:	00
   14000205a:	48 89 f1             	mov    %rsi,%rcx
   14000205d:	e8 7e b8 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002062:	85 c0                	test   %eax,%eax
   140002064:	0f 85 66 06 00 00    	jne    1400026d0 <tx_fn_m0_bench_fields_0+0x770>
   14000206a:	49 8b 0e             	mov    (%r14),%rcx
   14000206d:	48 8b 11             	mov    (%rcx),%rdx
   140002070:	48 ff c2             	inc    %rdx
   140002073:	70 37                	jo     1400020ac <tx_fn_m0_bench_fields_0+0x14c>
   140002075:	b8 40 42 0f 00       	mov    $0xf4240,%eax
   14000207a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
   140002080:	48 89 11             	mov    %rdx,(%rcx)
   140002083:	49 8b 14 24          	mov    (%r12),%rdx
   140002087:	48 8b 4a 08          	mov    0x8(%rdx),%rcx
   14000208b:	49 89 c8             	mov    %rcx,%r8
   14000208e:	49 83 c0 02          	add    $0x2,%r8
   140002092:	0f 80 e4 04 00 00    	jo     14000257c <tx_fn_m0_bench_fields_0+0x61c>
   140002098:	4c 89 42 08          	mov    %r8,0x8(%rdx)
   14000209c:	48 ff c8             	dec    %rax
   14000209f:	74 3b                	je     1400020dc <tx_fn_m0_bench_fields_0+0x17c>
   1400020a1:	49 8b 0e             	mov    (%r14),%rcx
   1400020a4:	48 8b 11             	mov    (%rcx),%rdx
   1400020a7:	48 ff c2             	inc    %rdx
   1400020aa:	71 d4                	jno    140002080 <tx_fn_m0_bench_fields_0+0x120>
   1400020ac:	48 b9 ff ff ff ff ff 	movabs $0x7fffffffffffffff,%rcx
   1400020b3:	ff ff 7f
   1400020b6:	4c 8d 84 24 18 01 00 	lea    0x118(%rsp),%r8
   1400020bd:	00
   1400020be:	ba 01 00 00 00       	mov    $0x1,%edx
   1400020c3:	e8 b8 29 00 00       	call   140004a80 <txrt_add_i64>
   1400020c8:	89 c7                	mov    %eax,%edi
   1400020ca:	48 8d 15 3f aa 08 00 	lea    0x8aa3f(%rip),%rdx        # 14008cb10 <.rdata+0xb10>
   1400020d1:	41 b8 31 00 00 00    	mov    $0x31,%r8d
   1400020d7:	e9 88 05 00 00       	jmp    140002664 <tx_fn_m0_bench_fields_0+0x704>
   1400020dc:	48 8d 0d e8 aa 08 00 	lea    0x8aae8(%rip),%rcx        # 14008cbcb <.rdata+0xbcb>
   1400020e3:	4c 8d 84 24 d0 00 00 	lea    0xd0(%rsp),%r8
   1400020ea:	00
   1400020eb:	ba 0d 00 00 00       	mov    $0xd,%edx
   1400020f0:	e8 9b 1e 00 00       	call   140003f90 <txrt_str_new>
   1400020f5:	85 c0                	test   %eax,%eax
   1400020f7:	0f 85 e5 05 00 00    	jne    1400026e2 <tx_fn_m0_bench_fields_0+0x782>
   1400020fd:	49 8b 06             	mov    (%r14),%rax
   140002100:	48 8b 08             	mov    (%rax),%rcx
   140002103:	48 8b 50 08          	mov    0x8(%rax),%rdx
   140002107:	49 89 c9             	mov    %rcx,%r9
   14000210a:	49 01 d1             	add    %rdx,%r9
   14000210d:	0f 80 e1 05 00 00    	jo     1400026f4 <tx_fn_m0_bench_fields_0+0x794>
   140002113:	48 8b bc 24 d0 00 00 	mov    0xd0(%rsp),%rdi
   14000211a:	00
   14000211b:	48 8d 05 7e ab 08 00 	lea    0x8ab7e(%rip),%rax        # 14008cca0 <.rdata+0xca0>
   140002122:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   140002127:	48 c7 44 24 30 34 00 	movq   $0x34,0x30(%rsp)
   14000212e:	00 00
   140002130:	48 c7 44 24 38 05 00 	movq   $0x5,0x38(%rsp)
   140002137:	00 00
   140002139:	48 89 f1             	mov    %rsi,%rcx
   14000213c:	48 89 fa             	mov    %rdi,%rdx
   14000213f:	4d 89 f8             	mov    %r15,%r8
   140002142:	e8 e9 f7 ff ff       	call   140001930 <tx_fn_m0_report_0>
   140002147:	48 89 f9             	mov    %rdi,%rcx
   14000214a:	e8 f1 21 00 00       	call   140004340 <txrt_str_release>
   14000214f:	48 89 f1             	mov    %rsi,%rcx
   140002152:	e8 89 b7 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002157:	85 c0                	test   %eax,%eax
   140002159:	0f 85 b6 05 00 00    	jne    140002715 <tx_fn_m0_bench_fields_0+0x7b5>
   14000215f:	48 8d 0d 0a 9f 08 00 	lea    0x89f0a(%rip),%rcx        # 14008c070 <.rdata+0x70>
   140002166:	48 8d 94 24 c8 00 00 	lea    0xc8(%rsp),%rdx
   14000216d:	00
   14000216e:	e8 ad 58 02 00       	call   140027a20 <txrt_record_class_new>
   140002173:	85 c0                	test   %eax,%eax
   140002175:	0f 85 ac 05 00 00    	jne    140002727 <tx_fn_m0_bench_fields_0+0x7c7>
   14000217b:	48 8b bc 24 c8 00 00 	mov    0xc8(%rsp),%rdi
   140002182:	00
   140002183:	48 8d 05 36 ac 08 00 	lea    0x8ac36(%rip),%rax        # 14008cdc0 <.rdata+0xdc0>
   14000218a:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   14000218f:	48 c7 44 24 30 35 00 	movq   $0x35,0x30(%rsp)
   140002196:	00 00
   140002198:	48 c7 44 24 38 05 00 	movq   $0x5,0x38(%rsp)
   14000219f:	00 00
   1400021a1:	48 89 f9             	mov    %rdi,%rcx
   1400021a4:	e8 c7 45 01 00       	call   140016770 <txrt_record_class_view>
   1400021a9:	48 8b 0e             	mov    (%rsi),%rcx
   1400021ac:	48 8d 54 24 50       	lea    0x50(%rsp),%rdx
   1400021b1:	48 89 16             	mov    %rdx,(%rsi)
   1400021b4:	48 8b 00             	mov    (%rax),%rax
   1400021b7:	48 c7 00 00 00 00 00 	movq   $0x0,(%rax)
   1400021be:	48 89 0e             	mov    %rcx,(%rsi)
   1400021c1:	48 89 f9             	mov    %rdi,%rcx
   1400021c4:	e8 a7 45 01 00       	call   140016770 <txrt_record_class_view>
   1400021c9:	49 89 c4             	mov    %rax,%r12
   1400021cc:	48 89 f1             	mov    %rsi,%rcx
   1400021cf:	e8 0c b7 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   1400021d4:	85 c0                	test   %eax,%eax
   1400021d6:	0f 85 5a 05 00 00    	jne    140002736 <tx_fn_m0_bench_fields_0+0x7d6>
   1400021dc:	48 8d 8c 24 c0 00 00 	lea    0xc0(%rsp),%rcx
   1400021e3:	00
   1400021e4:	e8 27 d8 01 00       	call   14001fa10 <txrt_time_monotonic_micros>
   1400021e9:	85 c0                	test   %eax,%eax
   1400021eb:	0f 85 54 05 00 00    	jne    140002745 <tx_fn_m0_bench_fields_0+0x7e5>
   1400021f1:	48 89 bc 24 88 00 00 	mov    %rdi,0x88(%rsp)
   1400021f8:	00
   1400021f9:	48 89 9c 24 90 00 00 	mov    %rbx,0x90(%rsp)
   140002200:	00
   140002201:	4c 8b bc 24 c0 00 00 	mov    0xc0(%rsp),%r15
   140002208:	00
   140002209:	48 89 f1             	mov    %rsi,%rcx
   14000220c:	e8 cf b6 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002211:	85 c0                	test   %eax,%eax
   140002213:	0f 85 3b 05 00 00    	jne    140002754 <tx_fn_m0_bench_fields_0+0x7f4>
   140002219:	bf 40 42 0f 00       	mov    $0xf4240,%edi
   14000221e:	31 c9                	xor    %ecx,%ecx
   140002220:	4c 8d 35 19 ad 08 00 	lea    0x8ad19(%rip),%r14        # 14008cf40 <.rdata+0xf40>
   140002227:	48 8d 05 82 c2 08 00 	lea    0x8c282(%rip),%rax        # 14008e4b0 <.rdata+0x24b0>
   14000222e:	66 48 0f 6e c0       	movq   %rax,%xmm0
   140002233:	48 8d 05 61 c2 08 00 	lea    0x8c261(%rip),%rax        # 14008e49b <.rdata+0x249b>
   14000223a:	66 48 0f 6e f0       	movq   %rax,%xmm6
   14000223f:	66 0f 6c f0          	punpcklqdq %xmm0,%xmm6
   140002243:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   14000224a:	84 00 00 00 00 00
   140002250:	4c 89 74 24 28       	mov    %r14,0x28(%rsp)
   140002255:	48 c7 44 24 30 3a 00 	movq   $0x3a,0x30(%rsp)
   14000225c:	00 00
   14000225e:	48 c7 44 24 38 09 00 	movq   $0x9,0x38(%rsp)
   140002265:	00 00
   140002267:	4c 8b 06             	mov    (%rsi),%r8
   14000226a:	66 0f 7f 74 24 50    	movdqa %xmm6,0x50(%rsp)
   140002270:	48 c7 44 24 60 12 00 	movq   $0x12,0x60(%rsp)
   140002277:	00 00
   140002279:	48 c7 44 24 68 09 00 	movq   $0x9,0x68(%rsp)
   140002280:	00 00
   140002282:	4c 89 44 24 70       	mov    %r8,0x70(%rsp)
   140002287:	48 8d 44 24 50       	lea    0x50(%rsp),%rax
   14000228c:	48 89 06             	mov    %rax,(%rsi)
   14000228f:	49 8b 14 24          	mov    (%r12),%rdx
   140002293:	48 8b 02             	mov    (%rdx),%rax
   140002296:	49 89 c1             	mov    %rax,%r9
   140002299:	49 ff c1             	inc    %r9
   14000229c:	0f 80 00 03 00 00    	jo     1400025a2 <tx_fn_m0_bench_fields_0+0x642>
   1400022a2:	4c 89 0a             	mov    %r9,(%rdx)
   1400022a5:	49 8b 04 24          	mov    (%r12),%rax
   1400022a9:	48 8b 10             	mov    (%rax),%rdx
   1400022ac:	4c 89 06             	mov    %r8,(%rsi)
   1400022af:	48 89 cb             	mov    %rcx,%rbx
   1400022b2:	48 01 d3             	add    %rdx,%rbx
   1400022b5:	0f 80 20 03 00 00    	jo     1400025db <tx_fn_m0_bench_fields_0+0x67b>
   1400022bb:	48 89 f1             	mov    %rsi,%rcx
   1400022be:	e8 1d b6 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   1400022c3:	85 c0                	test   %eax,%eax
   1400022c5:	0f 85 2e 03 00 00    	jne    1400025f9 <tx_fn_m0_bench_fields_0+0x699>
   1400022cb:	48 89 d9             	mov    %rbx,%rcx
   1400022ce:	48 ff cf             	dec    %rdi
   1400022d1:	0f 85 79 ff ff ff    	jne    140002250 <tx_fn_m0_bench_fields_0+0x2f0>
   1400022d7:	48 8d 0d 7d ad 08 00 	lea    0x8ad7d(%rip),%rcx        # 14008d05b <.rdata+0x105b>
   1400022de:	4c 8d 84 24 b8 00 00 	lea    0xb8(%rsp),%r8
   1400022e5:	00
   1400022e6:	ba 0d 00 00 00       	mov    $0xd,%edx
   1400022eb:	e8 a0 1c 00 00       	call   140003f90 <txrt_str_new>
   1400022f0:	85 c0                	test   %eax,%eax
   1400022f2:	0f 85 6b 04 00 00    	jne    140002763 <tx_fn_m0_bench_fields_0+0x803>
   1400022f8:	48 8b bc 24 b8 00 00 	mov    0xb8(%rsp),%rdi
   1400022ff:	00
   140002300:	48 8d 05 c9 ad 08 00 	lea    0x8adc9(%rip),%rax        # 14008d0d0 <.rdata+0x10d0>
   140002307:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   14000230c:	48 c7 44 24 30 3c 00 	movq   $0x3c,0x30(%rsp)
   140002313:	00 00
   140002315:	48 c7 44 24 38 05 00 	movq   $0x5,0x38(%rsp)
   14000231c:	00 00
   14000231e:	48 89 f1             	mov    %rsi,%rcx
   140002321:	48 89 fa             	mov    %rdi,%rdx
   140002324:	4d 89 f8             	mov    %r15,%r8
   140002327:	49 89 d9             	mov    %rbx,%r9
   14000232a:	e8 01 f6 ff ff       	call   140001930 <tx_fn_m0_report_0>
   14000232f:	48 89 f9             	mov    %rdi,%rcx
   140002332:	e8 09 20 00 00       	call   140004340 <txrt_str_release>
   140002337:	48 89 f1             	mov    %rsi,%rcx
   14000233a:	e8 a1 b5 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   14000233f:	85 c0                	test   %eax,%eax
   140002341:	0f 85 2b 04 00 00    	jne    140002772 <tx_fn_m0_bench_fields_0+0x812>
   140002347:	48 8d 8c 24 b0 00 00 	lea    0xb0(%rsp),%rcx
   14000234e:	00
   14000234f:	e8 bc d6 01 00       	call   14001fa10 <txrt_time_monotonic_micros>
   140002354:	85 c0                	test   %eax,%eax
   140002356:	0f 85 25 04 00 00    	jne    140002781 <tx_fn_m0_bench_fields_0+0x821>
   14000235c:	48 8b bc 24 b0 00 00 	mov    0xb0(%rsp),%rdi
   140002363:	00
   140002364:	48 89 f1             	mov    %rsi,%rcx
   140002367:	e8 74 b5 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   14000236c:	85 c0                	test   %eax,%eax
   14000236e:	0f 85 1c 04 00 00    	jne    140002790 <tx_fn_m0_bench_fields_0+0x830>
   140002374:	48 89 bc 24 80 00 00 	mov    %rdi,0x80(%rsp)
   14000237b:	00
   14000237c:	48 89 ac 24 98 00 00 	mov    %rbp,0x98(%rsp)
   140002383:	00
   140002384:	4c 89 ac 24 a0 00 00 	mov    %r13,0xa0(%rsp)
   14000238b:	00
   14000238c:	48 8d 0d bd 9d 08 00 	lea    0x89dbd(%rip),%rcx        # 14008c150 <.rdata+0x150>
   140002393:	48 8d 54 24 48       	lea    0x48(%rsp),%rdx
   140002398:	e8 73 56 01 00       	call   140017a10 <txrt_record_struct_new>
   14000239d:	85 c0                	test   %eax,%eax
   14000239f:	0f 85 03 01 00 00    	jne    1400024a8 <tx_fn_m0_bench_fields_0+0x548>
   1400023a5:	31 db                	xor    %ebx,%ebx
   1400023a7:	48 8d 05 d2 9f 08 00 	lea    0x89fd2(%rip),%rax        # 14008c380 <.rdata+0x380>
   1400023ae:	66 48 0f 6e c0       	movq   %rax,%xmm0
   1400023b3:	48 8d 05 bc 9f 08 00 	lea    0x89fbc(%rip),%rax        # 14008c376 <.rdata+0x376>
   1400023ba:	66 48 0f 6e f0       	movq   %rax,%xmm6
   1400023bf:	66 0f 6c f0          	punpcklqdq %xmm0,%xmm6
   1400023c3:	4c 8d 25 86 9d 08 00 	lea    0x89d86(%rip),%r12        # 14008c150 <.rdata+0x150>
   1400023ca:	4c 8d 6c 24 48       	lea    0x48(%rsp),%r13
   1400023cf:	45 31 f6             	xor    %r14d,%r14d
   1400023d2:	66 66 66 66 66 2e 0f 	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   1400023d9:	1f 84 00 00 00 00 00
   1400023e0:	48 8b 6c 24 48       	mov    0x48(%rsp),%rbp
   1400023e5:	48 89 e9             	mov    %rbp,%rcx
   1400023e8:	e8 b3 41 01 00       	call   1400165a0 <txrt_record_struct_view>
   1400023ed:	48 8b 00             	mov    (%rax),%rax
   1400023f0:	4c 89 30             	mov    %r14,(%rax)
   1400023f3:	48 c7 40 08 07 00 00 	movq   $0x7,0x8(%rax)
   1400023fa:	00
   1400023fb:	48 8d 05 ae ae 08 00 	lea    0x8aeae(%rip),%rax        # 14008d2b0 <.rdata+0x12b0>
   140002402:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   140002407:	48 c7 44 24 30 41 00 	movq   $0x41,0x30(%rsp)
   14000240e:	00 00
   140002410:	48 c7 44 24 38 09 00 	movq   $0x9,0x38(%rsp)
   140002417:	00 00
   140002419:	4c 8b 3e             	mov    (%rsi),%r15
   14000241c:	66 0f 7f 74 24 50    	movdqa %xmm6,0x50(%rsp)
   140002422:	48 c7 44 24 60 06 00 	movq   $0x6,0x60(%rsp)
   140002429:	00 00
   14000242b:	48 c7 44 24 68 01 00 	movq   $0x1,0x68(%rsp)
   140002432:	00 00
   140002434:	4c 89 7c 24 70       	mov    %r15,0x70(%rsp)
   140002439:	48 8d 44 24 50       	lea    0x50(%rsp),%rax
   14000243e:	48 89 06             	mov    %rax,(%rsi)
   140002441:	48 89 e9             	mov    %rbp,%rcx
   140002444:	e8 57 41 01 00       	call   1400165a0 <txrt_record_struct_view>
   140002449:	48 8b 00             	mov    (%rax),%rax
   14000244c:	48 8b 08             	mov    (%rax),%rcx
   14000244f:	48 8b 50 08          	mov    0x8(%rax),%rdx
   140002453:	48 89 cf             	mov    %rcx,%rdi
   140002456:	48 01 d7             	add    %rdx,%rdi
   140002459:	0f 80 b2 01 00 00    	jo     140002611 <tx_fn_m0_bench_fields_0+0x6b1>
   14000245f:	4c 89 3e             	mov    %r15,(%rsi)
   140002462:	48 89 e9             	mov    %rbp,%rcx
   140002465:	e8 c6 e5 01 00       	call   140020a30 <txrt_value_release>
   14000246a:	48 89 dd             	mov    %rbx,%rbp
   14000246d:	48 01 fd             	add    %rdi,%rbp
   140002470:	0f 80 cc 01 00 00    	jo     140002642 <tx_fn_m0_bench_fields_0+0x6e2>
   140002476:	48 89 f1             	mov    %rsi,%rcx
   140002479:	e8 62 b4 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   14000247e:	85 c0                	test   %eax,%eax
   140002480:	0f 85 f3 01 00 00    	jne    140002679 <tx_fn_m0_bench_fields_0+0x719>
   140002486:	49 ff c6             	inc    %r14
   140002489:	49 81 fe a0 86 01 00 	cmp    $0x186a0,%r14
   140002490:	74 24                	je     1400024b6 <tx_fn_m0_bench_fields_0+0x556>
   140002492:	4c 89 e1             	mov    %r12,%rcx
   140002495:	4c 89 ea             	mov    %r13,%rdx
   140002498:	e8 73 55 01 00       	call   140017a10 <txrt_record_struct_new>
   14000249d:	48 89 eb             	mov    %rbp,%rbx
   1400024a0:	85 c0                	test   %eax,%eax
   1400024a2:	0f 84 38 ff ff ff    	je     1400023e0 <tx_fn_m0_bench_fields_0+0x480>
   1400024a8:	89 c7                	mov    %eax,%edi
   1400024aa:	48 8d 15 9f ad 08 00 	lea    0x8ad9f(%rip),%rdx        # 14008d250 <.rdata+0x1250>
   1400024b1:	e9 a8 01 00 00       	jmp    14000265e <tx_fn_m0_bench_fields_0+0x6fe>
   1400024b6:	48 8d 0d 0e af 08 00 	lea    0x8af0e(%rip),%rcx        # 14008d3cb <.rdata+0x13cb>
   1400024bd:	4c 8d 84 24 a8 00 00 	lea    0xa8(%rsp),%r8
   1400024c4:	00
   1400024c5:	ba 0b 00 00 00       	mov    $0xb,%edx
   1400024ca:	e8 c1 1a 00 00       	call   140003f90 <txrt_str_new>
   1400024cf:	85 c0                	test   %eax,%eax
   1400024d1:	0f 85 c8 02 00 00    	jne    14000279f <tx_fn_m0_bench_fields_0+0x83f>
   1400024d7:	48 8b bc 24 a8 00 00 	mov    0xa8(%rsp),%rdi
   1400024de:	00
   1400024df:	48 8d 05 5a af 08 00 	lea    0x8af5a(%rip),%rax        # 14008d440 <.rdata+0x1440>
   1400024e6:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   1400024eb:	48 c7 44 24 30 43 00 	movq   $0x43,0x30(%rsp)
   1400024f2:	00 00
   1400024f4:	48 c7 44 24 38 05 00 	movq   $0x5,0x38(%rsp)
   1400024fb:	00 00
   1400024fd:	48 89 f1             	mov    %rsi,%rcx
   140002500:	48 89 fa             	mov    %rdi,%rdx
   140002503:	4c 8b 84 24 80 00 00 	mov    0x80(%rsp),%r8
   14000250a:	00
   14000250b:	49 89 e9             	mov    %rbp,%r9
   14000250e:	e8 1d f4 ff ff       	call   140001930 <tx_fn_m0_report_0>
   140002513:	48 89 f9             	mov    %rdi,%rcx
   140002516:	e8 25 1e 00 00       	call   140004340 <txrt_str_release>
   14000251b:	48 89 f1             	mov    %rsi,%rcx
   14000251e:	e8 bd b3 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002523:	85 c0                	test   %eax,%eax
   140002525:	48 8b bc 24 90 00 00 	mov    0x90(%rsp),%rdi
   14000252c:	00
   14000252d:	0f 85 75 02 00 00    	jne    1400027a8 <tx_fn_m0_bench_fields_0+0x848>
   140002533:	48 8b 8c 24 88 00 00 	mov    0x88(%rsp),%rcx
   14000253a:	00
   14000253b:	e8 f0 e4 01 00       	call   140020a30 <txrt_value_release>
   140002540:	48 89 f9             	mov    %rdi,%rcx
   140002543:	e8 e8 e4 01 00       	call   140020a30 <txrt_value_release>
   140002548:	48 8b 8c 24 98 00 00 	mov    0x98(%rsp),%rcx
   14000254f:	00
   140002550:	e8 db e4 01 00       	call   140020a30 <txrt_value_release>
   140002555:	48 8b 84 24 a0 00 00 	mov    0xa0(%rsp),%rax
   14000255c:	00
   14000255d:	48 89 06             	mov    %rax,(%rsi)
   140002560:	0f 28 b4 24 20 01 00 	movaps 0x120(%rsp),%xmm6
   140002567:	00
   140002568:	48 81 c4 38 01 00 00 	add    $0x138,%rsp
   14000256f:	5b                   	pop    %rbx
   140002570:	5d                   	pop    %rbp
   140002571:	5f                   	pop    %rdi
   140002572:	5e                   	pop    %rsi
   140002573:	41 5c                	pop    %r12
   140002575:	41 5d                	pop    %r13
   140002577:	41 5e                	pop    %r14
   140002579:	41 5f                	pop    %r15
   14000257b:	c3                   	ret
   14000257c:	4c 8d 84 24 10 01 00 	lea    0x110(%rsp),%r8
   140002583:	00
   140002584:	ba 02 00 00 00       	mov    $0x2,%edx
   140002589:	e8 f2 24 00 00       	call   140004a80 <txrt_add_i64>
   14000258e:	89 c7                	mov    %eax,%edi
   140002590:	48 8d 15 d9 a5 08 00 	lea    0x8a5d9(%rip),%rdx        # 14008cb70 <.rdata+0xb70>
   140002597:	41 b8 32 00 00 00    	mov    $0x32,%r8d
   14000259d:	e9 c2 00 00 00       	jmp    140002664 <tx_fn_m0_bench_fields_0+0x704>
   1400025a2:	4c 8d 84 24 f0 00 00 	lea    0xf0(%rsp),%r8
   1400025a9:	00
   1400025aa:	ba 01 00 00 00       	mov    $0x1,%edx
   1400025af:	48 89 c1             	mov    %rax,%rcx
   1400025b2:	e8 c9 24 00 00       	call   140004a80 <txrt_add_i64>
   1400025b7:	89 c7                	mov    %eax,%edi
   1400025b9:	48 8d 15 80 be 08 00 	lea    0x8be80(%rip),%rdx        # 14008e440 <.rdata+0x2440>
   1400025c0:	41 b8 14 00 00 00    	mov    $0x14,%r8d
   1400025c6:	41 b9 0d 00 00 00    	mov    $0xd,%r9d
   1400025cc:	48 89 f1             	mov    %rsi,%rcx
   1400025cf:	e8 5c 3d 00 00       	call   140006330 <txrt_stack_error_location>
   1400025d4:	89 f9                	mov    %edi,%ecx
   1400025d6:	e8 a5 13 00 00       	call   140003980 <txrt_require_success>
   1400025db:	4c 8d 84 24 00 01 00 	lea    0x100(%rsp),%r8
   1400025e2:	00
   1400025e3:	e8 98 24 00 00       	call   140004a80 <txrt_add_i64>
   1400025e8:	89 c7                	mov    %eax,%edi
   1400025ea:	48 8d 15 af a9 08 00 	lea    0x8a9af(%rip),%rdx        # 14008cfa0 <.rdata+0xfa0>
   1400025f1:	41 b8 3a 00 00 00    	mov    $0x3a,%r8d
   1400025f7:	eb 6b                	jmp    140002664 <tx_fn_m0_bench_fields_0+0x704>
   1400025f9:	48 8d 15 00 aa 08 00 	lea    0x8aa00(%rip),%rdx        # 14008d000 <.rdata+0x1000>
   140002600:	41 b8 3a 00 00 00    	mov    $0x3a,%r8d
   140002606:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000260c:	e9 aa 01 00 00       	jmp    1400027bb <tx_fn_m0_bench_fields_0+0x85b>
   140002611:	4c 8d 84 24 f0 00 00 	lea    0xf0(%rsp),%r8
   140002618:	00
   140002619:	e8 62 24 00 00       	call   140004a80 <txrt_add_i64>
   14000261e:	89 c7                	mov    %eax,%edi
   140002620:	48 8d 15 09 9d 08 00 	lea    0x89d09(%rip),%rdx        # 14008c330 <.rdata+0x330>
   140002627:	41 b8 08 00 00 00    	mov    $0x8,%r8d
   14000262d:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140002633:	48 89 f1             	mov    %rsi,%rcx
   140002636:	e8 f5 3c 00 00       	call   140006330 <txrt_stack_error_location>
   14000263b:	89 f9                	mov    %edi,%ecx
   14000263d:	e8 3e 13 00 00       	call   140003980 <txrt_require_success>
   140002642:	4c 8d 84 24 f8 00 00 	lea    0xf8(%rsp),%r8
   140002649:	00
   14000264a:	48 89 d9             	mov    %rbx,%rcx
   14000264d:	48 89 fa             	mov    %rdi,%rdx
   140002650:	e8 2b 24 00 00       	call   140004a80 <txrt_add_i64>
   140002655:	89 c7                	mov    %eax,%edi
   140002657:	48 8d 15 b2 ac 08 00 	lea    0x8acb2(%rip),%rdx        # 14008d310 <.rdata+0x1310>
   14000265e:	41 b8 41 00 00 00    	mov    $0x41,%r8d
   140002664:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000266a:	48 89 f1             	mov    %rsi,%rcx
   14000266d:	e8 be 3c 00 00       	call   140006330 <txrt_stack_error_location>
   140002672:	89 f9                	mov    %edi,%ecx
   140002674:	e8 07 13 00 00       	call   140003980 <txrt_require_success>
   140002679:	48 8d 15 f0 ac 08 00 	lea    0x8acf0(%rip),%rdx        # 14008d370 <.rdata+0x1370>
   140002680:	41 b8 41 00 00 00    	mov    $0x41,%r8d
   140002686:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   14000268c:	e9 2a 01 00 00       	jmp    1400027bb <tx_fn_m0_bench_fields_0+0x85b>
   140002691:	48 8d 15 98 a2 08 00 	lea    0x8a298(%rip),%rdx        # 14008c930 <.rdata+0x930>
   140002698:	eb 07                	jmp    1400026a1 <tx_fn_m0_bench_fields_0+0x741>
   14000269a:	48 8d 15 ef a2 08 00 	lea    0x8a2ef(%rip),%rdx        # 14008c990 <.rdata+0x990>
   1400026a1:	41 b8 2c 00 00 00    	mov    $0x2c,%r8d
   1400026a7:	e9 09 01 00 00       	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   1400026ac:	48 8d 15 3d a3 08 00 	lea    0x8a33d(%rip),%rdx        # 14008c9f0 <.rdata+0x9f0>
   1400026b3:	41 b8 2d 00 00 00    	mov    $0x2d,%r8d
   1400026b9:	e9 f7 00 00 00       	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   1400026be:	48 8d 15 8b a3 08 00 	lea    0x8a38b(%rip),%rdx        # 14008ca50 <.rdata+0xa50>
   1400026c5:	41 b8 2e 00 00 00    	mov    $0x2e,%r8d
   1400026cb:	e9 e5 00 00 00       	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   1400026d0:	48 8d 15 d9 a3 08 00 	lea    0x8a3d9(%rip),%rdx        # 14008cab0 <.rdata+0xab0>
   1400026d7:	41 b8 2e 00 00 00    	mov    $0x2e,%r8d
   1400026dd:	e9 d3 00 00 00       	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   1400026e2:	48 8d 15 f7 a4 08 00 	lea    0x8a4f7(%rip),%rdx        # 14008cbe0 <.rdata+0xbe0>
   1400026e9:	41 b8 34 00 00 00    	mov    $0x34,%r8d
   1400026ef:	e9 c1 00 00 00       	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   1400026f4:	4c 8d 84 24 08 01 00 	lea    0x108(%rsp),%r8
   1400026fb:	00
   1400026fc:	e8 7f 23 00 00       	call   140004a80 <txrt_add_i64>
   140002701:	89 c7                	mov    %eax,%edi
   140002703:	48 8d 15 36 a5 08 00 	lea    0x8a536(%rip),%rdx        # 14008cc40 <.rdata+0xc40>
   14000270a:	41 b8 34 00 00 00    	mov    $0x34,%r8d
   140002710:	e9 18 ff ff ff       	jmp    14000262d <tx_fn_m0_bench_fields_0+0x6cd>
   140002715:	48 8d 15 e4 a5 08 00 	lea    0x8a5e4(%rip),%rdx        # 14008cd00 <.rdata+0xd00>
   14000271c:	41 b8 34 00 00 00    	mov    $0x34,%r8d
   140002722:	e9 8e 00 00 00       	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   140002727:	48 8d 15 32 a6 08 00 	lea    0x8a632(%rip),%rdx        # 14008cd60 <.rdata+0xd60>
   14000272e:	41 b8 35 00 00 00    	mov    $0x35,%r8d
   140002734:	eb 7f                	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   140002736:	48 8d 15 e3 a6 08 00 	lea    0x8a6e3(%rip),%rdx        # 14008ce20 <.rdata+0xe20>
   14000273d:	41 b8 35 00 00 00    	mov    $0x35,%r8d
   140002743:	eb 70                	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   140002745:	48 8d 15 34 a7 08 00 	lea    0x8a734(%rip),%rdx        # 14008ce80 <.rdata+0xe80>
   14000274c:	41 b8 37 00 00 00    	mov    $0x37,%r8d
   140002752:	eb 61                	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   140002754:	48 8d 15 85 a7 08 00 	lea    0x8a785(%rip),%rdx        # 14008cee0 <.rdata+0xee0>
   14000275b:	41 b8 37 00 00 00    	mov    $0x37,%r8d
   140002761:	eb 52                	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   140002763:	48 8d 15 06 a9 08 00 	lea    0x8a906(%rip),%rdx        # 14008d070 <.rdata+0x1070>
   14000276a:	41 b8 3c 00 00 00    	mov    $0x3c,%r8d
   140002770:	eb 43                	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   140002772:	48 8d 15 b7 a9 08 00 	lea    0x8a9b7(%rip),%rdx        # 14008d130 <.rdata+0x1130>
   140002779:	41 b8 3c 00 00 00    	mov    $0x3c,%r8d
   14000277f:	eb 34                	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   140002781:	48 8d 15 08 aa 08 00 	lea    0x8aa08(%rip),%rdx        # 14008d190 <.rdata+0x1190>
   140002788:	41 b8 3e 00 00 00    	mov    $0x3e,%r8d
   14000278e:	eb 25                	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   140002790:	48 8d 15 59 aa 08 00 	lea    0x8aa59(%rip),%rdx        # 14008d1f0 <.rdata+0x11f0>
   140002797:	41 b8 3e 00 00 00    	mov    $0x3e,%r8d
   14000279d:	eb 16                	jmp    1400027b5 <tx_fn_m0_bench_fields_0+0x855>
   14000279f:	48 8d 15 3a ac 08 00 	lea    0x8ac3a(%rip),%rdx        # 14008d3e0 <.rdata+0x13e0>
   1400027a6:	eb 07                	jmp    1400027af <tx_fn_m0_bench_fields_0+0x84f>
   1400027a8:	48 8d 15 f1 ac 08 00 	lea    0x8acf1(%rip),%rdx        # 14008d4a0 <.rdata+0x14a0>
   1400027af:	41 b8 43 00 00 00    	mov    $0x43,%r8d
   1400027b5:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   1400027bb:	48 89 f1             	mov    %rsi,%rcx
   1400027be:	89 c6                	mov    %eax,%esi
   1400027c0:	e8 6b 3b 00 00       	call   140006330 <txrt_stack_error_location>
   1400027c5:	89 f1                	mov    %esi,%ecx
   1400027c7:	e8 b4 11 00 00       	call   140003980 <txrt_require_success>
   1400027cc:	cc                   	int3
   1400027cd:	0f 1f 00             	nopl   (%rax)


E:\Project\other\Compilation\tx_build\performance_09_11\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140002880 <tx_fn_m0_bench_calls_0>:
   140002880:	41 57                	push   %r15
   140002882:	41 56                	push   %r14
   140002884:	41 55                	push   %r13
   140002886:	41 54                	push   %r12
   140002888:	56                   	push   %rsi
   140002889:	57                   	push   %rdi
   14000288a:	55                   	push   %rbp
   14000288b:	53                   	push   %rbx
   14000288c:	48 81 ec d8 00 00 00 	sub    $0xd8,%rsp
   140002893:	48 89 ce             	mov    %rcx,%rsi
   140002896:	4c 8b 29             	mov    (%rcx),%r13
   140002899:	48 8d 05 4b b4 08 00 	lea    0x8b44b(%rip),%rax        # 14008dceb <.rdata+0x1ceb>
   1400028a0:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   1400028a5:	48 8d 05 54 b4 08 00 	lea    0x8b454(%rip),%rax        # 14008dd00 <.rdata+0x1d00>
   1400028ac:	48 89 44 24 50       	mov    %rax,0x50(%rsp)
   1400028b1:	48 c7 44 24 58 46 00 	movq   $0x46,0x58(%rsp)
   1400028b8:	00 00
   1400028ba:	48 c7 44 24 60 01 00 	movq   $0x1,0x60(%rsp)
   1400028c1:	00 00
   1400028c3:	4c 89 6c 24 68       	mov    %r13,0x68(%rsp)
   1400028c8:	48 8d 44 24 48       	lea    0x48(%rsp),%rax
   1400028cd:	48 89 01             	mov    %rax,(%rcx)
   1400028d0:	48 8d 0d f9 f3 ff ff 	lea    -0xc07(%rip),%rcx        # 140001cd0 <tx_callback_m0_double_value_0>
   1400028d7:	48 8d 15 42 f3 ff ff 	lea    -0xcbe(%rip),%rdx        # 140001c20 <tx_callback_m0_double_value_0_internal>
   1400028de:	4c 8d 05 86 ac 08 00 	lea    0x8ac86(%rip),%r8        # 14008d56b <.rdata+0x156b>
   1400028e5:	4c 8d 8c 24 c0 00 00 	lea    0xc0(%rsp),%r9
   1400028ec:	00
   1400028ed:	e8 fe 00 01 00       	call   1400129f0 <txrt_closure_new_internal>
   1400028f2:	85 c0                	test   %eax,%eax
   1400028f4:	0f 85 91 03 00 00    	jne    140002c8b <tx_fn_m0_bench_calls_0+0x40b>
   1400028fa:	48 8b ac 24 c0 00 00 	mov    0xc0(%rsp),%rbp
   140002901:	00
   140002902:	48 89 e9             	mov    %rbp,%rcx
   140002905:	e8 86 e7 00 00       	call   140011090 <txrt_closure_view>
   14000290a:	48 89 f1             	mov    %rsi,%rcx
   14000290d:	e8 ce af 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002912:	85 c0                	test   %eax,%eax
   140002914:	0f 85 7a 03 00 00    	jne    140002c94 <tx_fn_m0_bench_calls_0+0x414>
   14000291a:	48 8d 8c 24 b8 00 00 	lea    0xb8(%rsp),%rcx
   140002921:	00
   140002922:	e8 e9 d0 01 00       	call   14001fa10 <txrt_time_monotonic_micros>
   140002927:	85 c0                	test   %eax,%eax
   140002929:	0f 85 77 03 00 00    	jne    140002ca6 <tx_fn_m0_bench_calls_0+0x426>
   14000292f:	48 8b 9c 24 b8 00 00 	mov    0xb8(%rsp),%rbx
   140002936:	00
   140002937:	48 89 f1             	mov    %rsi,%rcx
   14000293a:	e8 a1 af 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   14000293f:	85 c0                	test   %eax,%eax
   140002941:	0f 85 71 03 00 00    	jne    140002cb8 <tx_fn_m0_bench_calls_0+0x438>
   140002947:	45 31 f6             	xor    %r14d,%r14d
   14000294a:	48 8d 3d af ad 08 00 	lea    0x8adaf(%rip),%rdi        # 14008d700 <.rdata+0x1700>
   140002951:	45 31 ff             	xor    %r15d,%r15d
   140002954:	45 31 e4             	xor    %r12d,%r12d
   140002957:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
   14000295e:	00 00
   140002960:	48 89 7c 24 50       	mov    %rdi,0x50(%rsp)
   140002965:	48 c7 44 24 58 4d 00 	movq   $0x4d,0x58(%rsp)
   14000296c:	00 00
   14000296e:	48 c7 44 24 60 09 00 	movq   $0x9,0x60(%rsp)
   140002975:	00 00
   140002977:	8b 4e 08             	mov    0x8(%rsi),%ecx
   14000297a:	e8 01 10 00 00       	call   140003980 <txrt_require_success>
   14000297f:	4d 01 f4             	add    %r14,%r12
   140002982:	0f 80 a8 02 00 00    	jo     140002c30 <tx_fn_m0_bench_calls_0+0x3b0>
   140002988:	49 83 c6 02          	add    $0x2,%r14
   14000298c:	4d 89 e7             	mov    %r12,%r15
   14000298f:	49 81 fe 80 84 1e 00 	cmp    $0x1e8480,%r14
   140002996:	75 c8                	jne    140002960 <tx_fn_m0_bench_calls_0+0xe0>
   140002998:	48 8d 0d 1c ae 08 00 	lea    0x8ae1c(%rip),%rcx        # 14008d7bb <.rdata+0x17bb>
   14000299f:	4c 8d 84 24 b0 00 00 	lea    0xb0(%rsp),%r8
   1400029a6:	00
   1400029a7:	ba 0e 00 00 00       	mov    $0xe,%edx
   1400029ac:	e8 df 15 00 00       	call   140003f90 <txrt_str_new>
   1400029b1:	85 c0                	test   %eax,%eax
   1400029b3:	0f 85 0e 03 00 00    	jne    140002cc7 <tx_fn_m0_bench_calls_0+0x447>
   1400029b9:	4c 8b b4 24 b0 00 00 	mov    0xb0(%rsp),%r14
   1400029c0:	00
   1400029c1:	48 8d 05 68 ae 08 00 	lea    0x8ae68(%rip),%rax        # 14008d830 <.rdata+0x1830>
   1400029c8:	48 89 44 24 50       	mov    %rax,0x50(%rsp)
   1400029cd:	48 c7 44 24 58 4f 00 	movq   $0x4f,0x58(%rsp)
   1400029d4:	00 00
   1400029d6:	48 c7 44 24 60 05 00 	movq   $0x5,0x60(%rsp)
   1400029dd:	00 00
   1400029df:	49 b9 c0 cd 95 d4 e8 	movabs $0xe8d495cdc0,%r9
   1400029e6:	00 00 00
   1400029e9:	48 89 f1             	mov    %rsi,%rcx
   1400029ec:	4c 89 f2             	mov    %r14,%rdx
   1400029ef:	49 89 d8             	mov    %rbx,%r8
   1400029f2:	e8 39 ef ff ff       	call   140001930 <tx_fn_m0_report_0>
   1400029f7:	4c 89 f1             	mov    %r14,%rcx
   1400029fa:	e8 41 19 00 00       	call   140004340 <txrt_str_release>
   1400029ff:	48 89 f1             	mov    %rsi,%rcx
   140002a02:	e8 d9 ae 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002a07:	85 c0                	test   %eax,%eax
   140002a09:	0f 85 c7 02 00 00    	jne    140002cd6 <tx_fn_m0_bench_calls_0+0x456>
   140002a0f:	48 8d 0d 9a f4 ff ff 	lea    -0xb66(%rip),%rcx        # 140001eb0 <tx_callback_m0_add_0>
   140002a16:	48 8d 15 f3 f3 ff ff 	lea    -0xc0d(%rip),%rdx        # 140001e10 <tx_callback_m0_add_0_internal>
   140002a1d:	4c 8d 05 cc ae 08 00 	lea    0x8aecc(%rip),%r8        # 14008d8f0 <.rdata+0x18f0>
   140002a24:	4c 8d 8c 24 a8 00 00 	lea    0xa8(%rsp),%r9
   140002a2b:	00
   140002a2c:	e8 bf ff 00 00       	call   1400129f0 <txrt_closure_new_internal>
   140002a31:	85 c0                	test   %eax,%eax
   140002a33:	0f 85 ac 02 00 00    	jne    140002ce5 <tx_fn_m0_bench_calls_0+0x465>
   140002a39:	48 8b 9c 24 a8 00 00 	mov    0xa8(%rsp),%rbx
   140002a40:	00
   140002a41:	48 c7 84 24 a0 00 00 	movq   $0x5,0xa0(%rsp)
   140002a48:	00 05 00 00 00
   140002a4d:	48 8d 84 24 98 00 00 	lea    0x98(%rsp),%rax
   140002a54:	00
   140002a55:	48 89 44 24 38       	mov    %rax,0x38(%rsp)
   140002a5a:	48 8d 84 24 a0 00 00 	lea    0xa0(%rsp),%rax
   140002a61:	00
   140002a62:	48 89 44 24 28       	mov    %rax,0x28(%rsp)
   140002a67:	48 8d 05 fd ae 08 00 	lea    0x8aefd(%rip),%rax        # 14008d96b <.rdata+0x196b>
   140002a6e:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
   140002a73:	48 c7 44 24 30 01 00 	movq   $0x1,0x30(%rsp)
   140002a7a:	00 00
   140002a7c:	48 8d 15 bd fd ff ff 	lea    -0x243(%rip),%rdx        # 140002840 <tx_bind_0>
   140002a83:	4c 8d 05 96 fd ff ff 	lea    -0x26a(%rip),%r8        # 140002820 <tx_bind_0_internal>
   140002a8a:	4c 8d 0d dc ae 08 00 	lea    0x8aedc(%rip),%r9        # 14008d96d <.rdata+0x196d>
   140002a91:	48 89 d9             	mov    %rbx,%rcx
   140002a94:	e8 47 08 01 00       	call   1400132e0 <txrt_closure_bind_internal>
   140002a99:	85 c0                	test   %eax,%eax
   140002a9b:	0f 85 53 02 00 00    	jne    140002cf4 <tx_fn_m0_bench_calls_0+0x474>
   140002aa1:	48 89 d9             	mov    %rbx,%rcx
   140002aa4:	e8 87 df 01 00       	call   140020a30 <txrt_value_release>
   140002aa9:	48 8b 9c 24 98 00 00 	mov    0x98(%rsp),%rbx
   140002ab0:	00
   140002ab1:	48 89 d9             	mov    %rbx,%rcx
   140002ab4:	e8 d7 e5 00 00       	call   140011090 <txrt_closure_view>
   140002ab9:	49 89 c7             	mov    %rax,%r15
   140002abc:	48 8b 38             	mov    (%rax),%rdi
   140002abf:	48 89 f1             	mov    %rsi,%rcx
   140002ac2:	e8 19 ae 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002ac7:	85 c0                	test   %eax,%eax
   140002ac9:	0f 85 34 02 00 00    	jne    140002d03 <tx_fn_m0_bench_calls_0+0x483>
   140002acf:	48 8d 8c 24 90 00 00 	lea    0x90(%rsp),%rcx
   140002ad6:	00
   140002ad7:	e8 34 cf 01 00       	call   14001fa10 <txrt_time_monotonic_micros>
   140002adc:	85 c0                	test   %eax,%eax
   140002ade:	0f 85 2e 02 00 00    	jne    140002d12 <tx_fn_m0_bench_calls_0+0x492>
   140002ae4:	48 89 6c 24 78       	mov    %rbp,0x78(%rsp)
   140002ae9:	4c 89 ac 24 80 00 00 	mov    %r13,0x80(%rsp)
   140002af0:	00
   140002af1:	48 8b 84 24 90 00 00 	mov    0x90(%rsp),%rax
   140002af8:	00
   140002af9:	48 89 44 24 70       	mov    %rax,0x70(%rsp)
   140002afe:	48 89 f1             	mov    %rsi,%rcx
   140002b01:	e8 da ad 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002b06:	85 c0                	test   %eax,%eax
   140002b08:	0f 85 13 02 00 00    	jne    140002d21 <tx_fn_m0_bench_calls_0+0x4a1>
   140002b0e:	4c 8d 35 eb af 08 00 	lea    0x8afeb(%rip),%r14        # 14008db00 <.rdata+0x1b00>
   140002b15:	4c 89 74 24 50       	mov    %r14,0x50(%rsp)
   140002b1a:	48 c7 44 24 58 55 00 	movq   $0x55,0x58(%rsp)
   140002b21:	00 00
   140002b23:	48 c7 44 24 60 09 00 	movq   $0x9,0x60(%rsp)
   140002b2a:	00 00
   140002b2c:	48 89 f1             	mov    %rsi,%rcx
   140002b2f:	4c 89 fa             	mov    %r15,%rdx
   140002b32:	45 31 c0             	xor    %r8d,%r8d
   140002b35:	ff d7                	call   *%rdi
   140002b37:	48 89 c5             	mov    %rax,%rbp
   140002b3a:	41 bd 01 00 00 00    	mov    $0x1,%r13d
   140002b40:	49 89 c4             	mov    %rax,%r12
   140002b43:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140002b4a:	84 00 00 00 00 00
   140002b50:	4c 89 74 24 50       	mov    %r14,0x50(%rsp)
   140002b55:	48 c7 44 24 58 55 00 	movq   $0x55,0x58(%rsp)
   140002b5c:	00 00
   140002b5e:	48 c7 44 24 60 09 00 	movq   $0x9,0x60(%rsp)
   140002b65:	00 00
   140002b67:	48 89 f1             	mov    %rsi,%rcx
   140002b6a:	4c 89 fa             	mov    %r15,%rdx
   140002b6d:	4d 89 e8             	mov    %r13,%r8
   140002b70:	ff d7                	call   *%rdi
   140002b72:	49 01 c4             	add    %rax,%r12
   140002b75:	0f 80 d9 00 00 00    	jo     140002c54 <tx_fn_m0_bench_calls_0+0x3d4>
   140002b7b:	49 ff c5             	inc    %r13
   140002b7e:	4c 89 e5             	mov    %r12,%rbp
   140002b81:	49 81 fd 40 42 0f 00 	cmp    $0xf4240,%r13
   140002b88:	75 c6                	jne    140002b50 <tx_fn_m0_bench_calls_0+0x2d0>
   140002b8a:	48 8d 0d 2a b0 08 00 	lea    0x8b02a(%rip),%rcx        # 14008dbbb <.rdata+0x1bbb>
   140002b91:	4c 8d 84 24 88 00 00 	lea    0x88(%rsp),%r8
   140002b98:	00
   140002b99:	ba 0c 00 00 00       	mov    $0xc,%edx
   140002b9e:	e8 ed 13 00 00       	call   140003f90 <txrt_str_new>
   140002ba3:	85 c0                	test   %eax,%eax
   140002ba5:	0f 85 85 01 00 00    	jne    140002d30 <tx_fn_m0_bench_calls_0+0x4b0>
   140002bab:	4c 8b bc 24 88 00 00 	mov    0x88(%rsp),%r15
   140002bb2:	00
   140002bb3:	48 8d 05 76 b0 08 00 	lea    0x8b076(%rip),%rax        # 14008dc30 <.rdata+0x1c30>
   140002bba:	48 89 44 24 50       	mov    %rax,0x50(%rsp)
   140002bbf:	48 c7 44 24 58 57 00 	movq   $0x57,0x58(%rsp)
   140002bc6:	00 00
   140002bc8:	48 c7 44 24 60 05 00 	movq   $0x5,0x60(%rsp)
   140002bcf:	00 00
   140002bd1:	48 89 f1             	mov    %rsi,%rcx
   140002bd4:	4c 89 fa             	mov    %r15,%rdx
   140002bd7:	4c 8b 44 24 70       	mov    0x70(%rsp),%r8
   140002bdc:	4d 89 e1             	mov    %r12,%r9
   140002bdf:	e8 4c ed ff ff       	call   140001930 <tx_fn_m0_report_0>
   140002be4:	4c 89 f9             	mov    %r15,%rcx
   140002be7:	e8 54 17 00 00       	call   140004340 <txrt_str_release>
   140002bec:	48 89 f1             	mov    %rsi,%rcx
   140002bef:	e8 ec ac 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002bf4:	85 c0                	test   %eax,%eax
   140002bf6:	48 8b bc 24 80 00 00 	mov    0x80(%rsp),%rdi
   140002bfd:	00
   140002bfe:	4c 8b 74 24 78       	mov    0x78(%rsp),%r14
   140002c03:	0f 85 30 01 00 00    	jne    140002d39 <tx_fn_m0_bench_calls_0+0x4b9>
   140002c09:	48 89 d9             	mov    %rbx,%rcx
   140002c0c:	e8 1f de 01 00       	call   140020a30 <txrt_value_release>
   140002c11:	4c 89 f1             	mov    %r14,%rcx
   140002c14:	e8 17 de 01 00       	call   140020a30 <txrt_value_release>
   140002c19:	48 89 3e             	mov    %rdi,(%rsi)
   140002c1c:	48 81 c4 d8 00 00 00 	add    $0xd8,%rsp
   140002c23:	5b                   	pop    %rbx
   140002c24:	5d                   	pop    %rbp
   140002c25:	5f                   	pop    %rdi
   140002c26:	5e                   	pop    %rsi
   140002c27:	41 5c                	pop    %r12
   140002c29:	41 5d                	pop    %r13
   140002c2b:	41 5e                	pop    %r14
   140002c2d:	41 5f                	pop    %r15
   140002c2f:	c3                   	ret
   140002c30:	4c 8d 84 24 d0 00 00 	lea    0xd0(%rsp),%r8
   140002c37:	00
   140002c38:	4c 89 f9             	mov    %r15,%rcx
   140002c3b:	4c 89 f2             	mov    %r14,%rdx
   140002c3e:	e8 3d 1e 00 00       	call   140004a80 <txrt_add_i64>
   140002c43:	89 c7                	mov    %eax,%edi
   140002c45:	48 8d 15 14 ab 08 00 	lea    0x8ab14(%rip),%rdx        # 14008d760 <.rdata+0x1760>
   140002c4c:	41 b8 4d 00 00 00    	mov    $0x4d,%r8d
   140002c52:	eb 22                	jmp    140002c76 <tx_fn_m0_bench_calls_0+0x3f6>
   140002c54:	4c 8d 84 24 c8 00 00 	lea    0xc8(%rsp),%r8
   140002c5b:	00
   140002c5c:	48 89 e9             	mov    %rbp,%rcx
   140002c5f:	48 89 c2             	mov    %rax,%rdx
   140002c62:	e8 19 1e 00 00       	call   140004a80 <txrt_add_i64>
   140002c67:	89 c7                	mov    %eax,%edi
   140002c69:	48 8d 15 f0 ae 08 00 	lea    0x8aef0(%rip),%rdx        # 14008db60 <.rdata+0x1b60>
   140002c70:	41 b8 55 00 00 00    	mov    $0x55,%r8d
   140002c76:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002c7c:	48 89 f1             	mov    %rsi,%rcx
   140002c7f:	e8 ac 36 00 00       	call   140006330 <txrt_stack_error_location>
   140002c84:	89 f9                	mov    %edi,%ecx
   140002c86:	e8 f5 0c 00 00       	call   140003980 <txrt_require_success>
   140002c8b:	48 8d 15 ee a8 08 00 	lea    0x8a8ee(%rip),%rdx        # 14008d580 <.rdata+0x1580>
   140002c92:	eb 07                	jmp    140002c9b <tx_fn_m0_bench_calls_0+0x41b>
   140002c94:	48 8d 15 45 a9 08 00 	lea    0x8a945(%rip),%rdx        # 14008d5e0 <.rdata+0x15e0>
   140002c9b:	41 b8 48 00 00 00    	mov    $0x48,%r8d
   140002ca1:	e9 a0 00 00 00       	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002ca6:	48 8d 15 93 a9 08 00 	lea    0x8a993(%rip),%rdx        # 14008d640 <.rdata+0x1640>
   140002cad:	41 b8 4a 00 00 00    	mov    $0x4a,%r8d
   140002cb3:	e9 8e 00 00 00       	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002cb8:	48 8d 15 e1 a9 08 00 	lea    0x8a9e1(%rip),%rdx        # 14008d6a0 <.rdata+0x16a0>
   140002cbf:	41 b8 4a 00 00 00    	mov    $0x4a,%r8d
   140002cc5:	eb 7f                	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002cc7:	48 8d 15 02 ab 08 00 	lea    0x8ab02(%rip),%rdx        # 14008d7d0 <.rdata+0x17d0>
   140002cce:	41 b8 4f 00 00 00    	mov    $0x4f,%r8d
   140002cd4:	eb 70                	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002cd6:	48 8d 15 b3 ab 08 00 	lea    0x8abb3(%rip),%rdx        # 14008d890 <.rdata+0x1890>
   140002cdd:	41 b8 4f 00 00 00    	mov    $0x4f,%r8d
   140002ce3:	eb 61                	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002ce5:	48 8d 15 24 ac 08 00 	lea    0x8ac24(%rip),%rdx        # 14008d910 <.rdata+0x1910>
   140002cec:	41 b8 50 00 00 00    	mov    $0x50,%r8d
   140002cf2:	eb 52                	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002cf4:	48 8d 15 85 ac 08 00 	lea    0x8ac85(%rip),%rdx        # 14008d980 <.rdata+0x1980>
   140002cfb:	41 b8 50 00 00 00    	mov    $0x50,%r8d
   140002d01:	eb 43                	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002d03:	48 8d 15 d6 ac 08 00 	lea    0x8acd6(%rip),%rdx        # 14008d9e0 <.rdata+0x19e0>
   140002d0a:	41 b8 50 00 00 00    	mov    $0x50,%r8d
   140002d10:	eb 34                	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002d12:	48 8d 15 27 ad 08 00 	lea    0x8ad27(%rip),%rdx        # 14008da40 <.rdata+0x1a40>
   140002d19:	41 b8 52 00 00 00    	mov    $0x52,%r8d
   140002d1f:	eb 25                	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002d21:	48 8d 15 78 ad 08 00 	lea    0x8ad78(%rip),%rdx        # 14008daa0 <.rdata+0x1aa0>
   140002d28:	41 b8 52 00 00 00    	mov    $0x52,%r8d
   140002d2e:	eb 16                	jmp    140002d46 <tx_fn_m0_bench_calls_0+0x4c6>
   140002d30:	48 8d 15 99 ae 08 00 	lea    0x8ae99(%rip),%rdx        # 14008dbd0 <.rdata+0x1bd0>
   140002d37:	eb 07                	jmp    140002d40 <tx_fn_m0_bench_calls_0+0x4c0>
   140002d39:	48 8d 15 50 af 08 00 	lea    0x8af50(%rip),%rdx        # 14008dc90 <.rdata+0x1c90>
   140002d40:	41 b8 57 00 00 00    	mov    $0x57,%r8d
   140002d46:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   140002d4c:	48 89 f1             	mov    %rsi,%rcx
   140002d4f:	89 c6                	mov    %eax,%esi
   140002d51:	e8 da 35 00 00       	call   140006330 <txrt_stack_error_location>
   140002d56:	89 f1                	mov    %esi,%ecx
   140002d58:	e8 23 0c 00 00       	call   140003980 <txrt_require_success>
   140002d5d:	cc                   	int3
   140002d5e:	66 90                	xchg   %ax,%ax


E:\Project\other\Compilation\tx_build\performance_09_11\candidate\paths.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140002db0 <tx_fn_m0_bench_heap_0>:
   140002db0:	41 57                	push   %r15
   140002db2:	41 56                	push   %r14
   140002db4:	41 55                	push   %r13
   140002db6:	41 54                	push   %r12
   140002db8:	56                   	push   %rsi
   140002db9:	57                   	push   %rdi
   140002dba:	53                   	push   %rbx
   140002dbb:	48 83 ec 70          	sub    $0x70,%rsp
   140002dbf:	48 89 ce             	mov    %rcx,%rsi
   140002dc2:	4c 8b 21             	mov    (%rcx),%r12
   140002dc5:	48 8d 05 bf b3 08 00 	lea    0x8b3bf(%rip),%rax        # 14008e18b <.rdata+0x218b>
   140002dcc:	48 89 44 24 40       	mov    %rax,0x40(%rsp)
   140002dd1:	48 8d 05 c8 b3 08 00 	lea    0x8b3c8(%rip),%rax        # 14008e1a0 <.rdata+0x21a0>
   140002dd8:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   140002ddd:	48 c7 44 24 50 5a 00 	movq   $0x5a,0x50(%rsp)
   140002de4:	00 00
   140002de6:	48 c7 44 24 58 01 00 	movq   $0x1,0x58(%rsp)
   140002ded:	00 00
   140002def:	4c 89 64 24 60       	mov    %r12,0x60(%rsp)
   140002df4:	48 8d 44 24 40       	lea    0x40(%rsp),%rax
   140002df9:	48 89 01             	mov    %rax,(%rcx)
   140002dfc:	48 8d 54 24 38       	lea    0x38(%rsp),%rdx
   140002e01:	31 c9                	xor    %ecx,%ecx
   140002e03:	e8 98 4e 01 00       	call   140017ca0 <txrt_heap_new_i64>
   140002e08:	85 c0                	test   %eax,%eax
   140002e0a:	0f 85 fb 01 00 00    	jne    14000300b <tx_fn_m0_bench_heap_0+0x25b>
   140002e10:	48 8b 7c 24 38       	mov    0x38(%rsp),%rdi
   140002e15:	48 89 f1             	mov    %rsi,%rcx
   140002e18:	e8 c3 aa 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002e1d:	85 c0                	test   %eax,%eax
   140002e1f:	0f 85 ef 01 00 00    	jne    140003014 <tx_fn_m0_bench_heap_0+0x264>
   140002e25:	48 8d 4c 24 30       	lea    0x30(%rsp),%rcx
   140002e2a:	e8 e1 cb 01 00       	call   14001fa10 <txrt_time_monotonic_micros>
   140002e2f:	85 c0                	test   %eax,%eax
   140002e31:	0f 85 ec 01 00 00    	jne    140003023 <tx_fn_m0_bench_heap_0+0x273>
   140002e37:	48 8b 5c 24 30       	mov    0x30(%rsp),%rbx
   140002e3c:	48 89 f1             	mov    %rsi,%rcx
   140002e3f:	e8 9c aa 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002e44:	85 c0                	test   %eax,%eax
   140002e46:	0f 85 e6 01 00 00    	jne    140003032 <tx_fn_m0_bench_heap_0+0x282>
   140002e4c:	48 89 f9             	mov    %rdi,%rcx
   140002e4f:	31 d2                	xor    %edx,%edx
   140002e51:	e8 9a 60 01 00       	call   140018ef0 <txrt_heap_push_i64>
   140002e56:	85 c0                	test   %eax,%eax
   140002e58:	0f 85 4a 01 00 00    	jne    140002fa8 <tx_fn_m0_bench_heap_0+0x1f8>
   140002e5e:	41 be 4f c3 00 00    	mov    $0xc34f,%r14d
   140002e64:	41 bf 01 00 00 00    	mov    $0x1,%r15d
   140002e6a:	49 bd cf f7 53 e3 a5 	movabs $0x20c49ba5e353f7cf,%r13
   140002e71:	9b c4 20
   140002e74:	66 66 66 2e 0f 1f 84 	data16 data16 cs nopw 0x0(%rax,%rax,1)
   140002e7b:	00 00 00 00 00
   140002e80:	4c 89 f8             	mov    %r15,%rax
   140002e83:	48 c1 e8 03          	shr    $0x3,%rax
   140002e87:	49 f7 e5             	mul    %r13
   140002e8a:	48 c1 ea 04          	shr    $0x4,%rdx
   140002e8e:	48 69 c2 e8 03 00 00 	imul   $0x3e8,%rdx,%rax
   140002e95:	4c 89 fa             	mov    %r15,%rdx
   140002e98:	48 29 c2             	sub    %rax,%rdx
   140002e9b:	48 89 f9             	mov    %rdi,%rcx
   140002e9e:	e8 4d 60 01 00       	call   140018ef0 <txrt_heap_push_i64>
   140002ea3:	85 c0                	test   %eax,%eax
   140002ea5:	0f 85 fd 00 00 00    	jne    140002fa8 <tx_fn_m0_bench_heap_0+0x1f8>
   140002eab:	49 ff c7             	inc    %r15
   140002eae:	49 ff ce             	dec    %r14
   140002eb1:	75 cd                	jne    140002e80 <tx_fn_m0_bench_heap_0+0xd0>
   140002eb3:	48 8d 54 24 20       	lea    0x20(%rsp),%rdx
   140002eb8:	48 89 f9             	mov    %rdi,%rcx
   140002ebb:	e8 80 5e 01 00       	call   140018d40 <txrt_heap_top_i64>
   140002ec0:	85 c0                	test   %eax,%eax
   140002ec2:	75 51                	jne    140002f15 <tx_fn_m0_bench_heap_0+0x165>
   140002ec4:	41 bd 50 c3 00 00    	mov    $0xc350,%r13d
   140002eca:	31 c9                	xor    %ecx,%ecx
   140002ecc:	4c 8d 7c 24 20       	lea    0x20(%rsp),%r15
   140002ed1:	45 31 f6             	xor    %r14d,%r14d
   140002ed4:	66 66 66 2e 0f 1f 84 	data16 data16 cs nopw 0x0(%rax,%rax,1)
   140002edb:	00 00 00 00 00
   140002ee0:	48 8b 54 24 20       	mov    0x20(%rsp),%rdx
   140002ee5:	49 01 d6             	add    %rdx,%r14
   140002ee8:	0f 80 cb 00 00 00    	jo     140002fb9 <tx_fn_m0_bench_heap_0+0x209>
   140002eee:	48 89 f9             	mov    %rdi,%rcx
   140002ef1:	e8 9a 71 01 00       	call   14001a090 <txrt_heap_pop_i64>
   140002ef6:	85 c0                	test   %eax,%eax
   140002ef8:	0f 85 e9 00 00 00    	jne    140002fe7 <tx_fn_m0_bench_heap_0+0x237>
   140002efe:	49 ff cd             	dec    %r13
   140002f01:	74 20                	je     140002f23 <tx_fn_m0_bench_heap_0+0x173>
   140002f03:	48 89 f9             	mov    %rdi,%rcx
   140002f06:	4c 89 fa             	mov    %r15,%rdx
   140002f09:	e8 32 5e 01 00       	call   140018d40 <txrt_heap_top_i64>
   140002f0e:	4c 89 f1             	mov    %r14,%rcx
   140002f11:	85 c0                	test   %eax,%eax
   140002f13:	74 cb                	je     140002ee0 <tx_fn_m0_bench_heap_0+0x130>
   140002f15:	89 c7                	mov    %eax,%edi
   140002f17:	48 8d 15 22 b0 08 00 	lea    0x8b022(%rip),%rdx        # 14008df40 <.rdata+0x1f40>
   140002f1e:	e9 a9 00 00 00       	jmp    140002fcc <tx_fn_m0_bench_heap_0+0x21c>
   140002f23:	48 8d 0d 31 b1 08 00 	lea    0x8b131(%rip),%rcx        # 14008e05b <.rdata+0x205b>
   140002f2a:	4c 8d 44 24 28       	lea    0x28(%rsp),%r8
   140002f2f:	ba 0d 00 00 00       	mov    $0xd,%edx
   140002f34:	e8 57 10 00 00       	call   140003f90 <txrt_str_new>
   140002f39:	85 c0                	test   %eax,%eax
   140002f3b:	0f 85 00 01 00 00    	jne    140003041 <tx_fn_m0_bench_heap_0+0x291>
   140002f41:	4c 8b 7c 24 28       	mov    0x28(%rsp),%r15
   140002f46:	48 8d 05 83 b1 08 00 	lea    0x8b183(%rip),%rax        # 14008e0d0 <.rdata+0x20d0>
   140002f4d:	48 89 44 24 48       	mov    %rax,0x48(%rsp)
   140002f52:	48 c7 44 24 50 68 00 	movq   $0x68,0x50(%rsp)
   140002f59:	00 00
   140002f5b:	48 c7 44 24 58 05 00 	movq   $0x5,0x58(%rsp)
   140002f62:	00 00
   140002f64:	48 89 f1             	mov    %rsi,%rcx
   140002f67:	4c 89 fa             	mov    %r15,%rdx
   140002f6a:	49 89 d8             	mov    %rbx,%r8
   140002f6d:	4d 89 f1             	mov    %r14,%r9
   140002f70:	e8 bb e9 ff ff       	call   140001930 <tx_fn_m0_report_0>
   140002f75:	4c 89 f9             	mov    %r15,%rcx
   140002f78:	e8 c3 13 00 00       	call   140004340 <txrt_str_release>
   140002f7d:	48 89 f1             	mov    %rsi,%rcx
   140002f80:	e8 5b a9 01 00       	call   14001d8e0 <txrt_gc_safepoint_context>
   140002f85:	85 c0                	test   %eax,%eax
   140002f87:	0f 85 bd 00 00 00    	jne    14000304a <tx_fn_m0_bench_heap_0+0x29a>
   140002f8d:	48 89 f9             	mov    %rdi,%rcx
   140002f90:	e8 9b da 01 00       	call   140020a30 <txrt_value_release>
   140002f95:	4c 89 26             	mov    %r12,(%rsi)
   140002f98:	48 83 c4 70          	add    $0x70,%rsp
   140002f9c:	5b                   	pop    %rbx
   140002f9d:	5f                   	pop    %rdi
   140002f9e:	5e                   	pop    %rsi
   140002f9f:	41 5c                	pop    %r12
   140002fa1:	41 5d                	pop    %r13
   140002fa3:	41 5e                	pop    %r14
   140002fa5:	41 5f                	pop    %r15
   140002fa7:	c3                   	ret
   140002fa8:	89 c7                	mov    %eax,%edi
   140002faa:	48 8d 15 2f af 08 00 	lea    0x8af2f(%rip),%rdx        # 14008dee0 <.rdata+0x1ee0>
   140002fb1:	41 b8 61 00 00 00    	mov    $0x61,%r8d
   140002fb7:	eb 19                	jmp    140002fd2 <tx_fn_m0_bench_heap_0+0x222>
   140002fb9:	4c 8d 44 24 68       	lea    0x68(%rsp),%r8
   140002fbe:	e8 bd 1a 00 00       	call   140004a80 <txrt_add_i64>
   140002fc3:	89 c7                	mov    %eax,%edi
   140002fc5:	48 8d 15 d4 af 08 00 	lea    0x8afd4(%rip),%rdx        # 14008dfa0 <.rdata+0x1fa0>
   140002fcc:	41 b8 65 00 00 00    	mov    $0x65,%r8d
   140002fd2:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002fd8:	48 89 f1             	mov    %rsi,%rcx
   140002fdb:	e8 50 33 00 00       	call   140006330 <txrt_stack_error_location>
   140002fe0:	89 f9                	mov    %edi,%ecx
   140002fe2:	e8 99 09 00 00       	call   140003980 <txrt_require_success>
   140002fe7:	48 8d 15 12 b0 08 00 	lea    0x8b012(%rip),%rdx        # 14008e000 <.rdata+0x2000>
   140002fee:	41 b8 66 00 00 00    	mov    $0x66,%r8d
   140002ff4:	41 b9 09 00 00 00    	mov    $0x9,%r9d
   140002ffa:	48 89 f1             	mov    %rsi,%rcx
   140002ffd:	89 c6                	mov    %eax,%esi
   140002fff:	e8 2c 33 00 00       	call   140006330 <txrt_stack_error_location>
   140003004:	89 f1                	mov    %esi,%ecx
   140003006:	e8 75 09 00 00       	call   140003980 <txrt_require_success>
   14000300b:	48 8d 15 4e ad 08 00 	lea    0x8ad4e(%rip),%rdx        # 14008dd60 <.rdata+0x1d60>
   140003012:	eb 07                	jmp    14000301b <tx_fn_m0_bench_heap_0+0x26b>
   140003014:	48 8d 15 a5 ad 08 00 	lea    0x8ada5(%rip),%rdx        # 14008ddc0 <.rdata+0x1dc0>
   14000301b:	41 b8 5c 00 00 00    	mov    $0x5c,%r8d
   140003021:	eb 34                	jmp    140003057 <tx_fn_m0_bench_heap_0+0x2a7>
   140003023:	48 8d 15 f6 ad 08 00 	lea    0x8adf6(%rip),%rdx        # 14008de20 <.rdata+0x1e20>
   14000302a:	41 b8 5e 00 00 00    	mov    $0x5e,%r8d
   140003030:	eb 25                	jmp    140003057 <tx_fn_m0_bench_heap_0+0x2a7>
   140003032:	48 8d 15 47 ae 08 00 	lea    0x8ae47(%rip),%rdx        # 14008de80 <.rdata+0x1e80>
   140003039:	41 b8 5e 00 00 00    	mov    $0x5e,%r8d
   14000303f:	eb 16                	jmp    140003057 <tx_fn_m0_bench_heap_0+0x2a7>
   140003041:	48 8d 15 28 b0 08 00 	lea    0x8b028(%rip),%rdx        # 14008e070 <.rdata+0x2070>
   140003048:	eb 07                	jmp    140003051 <tx_fn_m0_bench_heap_0+0x2a1>
   14000304a:	48 8d 15 df b0 08 00 	lea    0x8b0df(%rip),%rdx        # 14008e130 <.rdata+0x2130>
   140003051:	41 b8 68 00 00 00    	mov    $0x68,%r8d
   140003057:	41 b9 05 00 00 00    	mov    $0x5,%r9d
   14000305d:	eb 9b                	jmp    140002ffa <tx_fn_m0_bench_heap_0+0x24a>
   14000305f:	90                   	nop
