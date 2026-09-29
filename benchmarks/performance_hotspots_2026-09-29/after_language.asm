
E:\Project\other\Compilation\tx_build\performance_hotspots\after\language.exe:     file format pei-x86-64


Disassembly of section .text:

00000001400051a0 <tx_fn_m0_bench_virtual_interface_0>:
   1400051a0:	push   %r15
   1400051a2:	push   %r14
   1400051a4:	push   %r13
   1400051a6:	push   %r12
   1400051a8:	push   %rsi
   1400051a9:	push   %rdi
   1400051aa:	push   %rbp
   1400051ab:	push   %rbx
   1400051ac:	sub    $0xc8,%rsp
   1400051b3:	mov    %rcx,%rsi
   1400051b6:	mov    (%rcx),%rdi
   1400051b9:	lea    0x9cf90(%rip),%rax        # 1400a2150 <.rdata+0x5150>
   1400051c0:	mov    %rax,0x28(%rsp)
   1400051c5:	lea    0x9cfa4(%rip),%rax        # 1400a2170 <.rdata+0x5170>
   1400051cc:	mov    %rax,0x30(%rsp)
   1400051d1:	movq   $0x16e,0x38(%rsp)
   1400051da:	movq   $0x1,0x40(%rsp)
   1400051e3:	mov    %rdi,0x48(%rsp)
   1400051e8:	lea    0x28(%rsp),%rax
   1400051ed:	mov    %rax,(%rcx)
   1400051f0:	lea    0x98179(%rip),%rcx        # 14009d370 <.rdata+0x370>
   1400051f7:	lea    0x88(%rsp),%rdx
   1400051ff:	call   1400129c0 <txrt_record_class_new>
   140005204:	test   %eax,%eax
   140005206:	jne    14000551f <tx_fn_m0_bench_virtual_interface_0+0x37f>
   14000520c:	mov    0x88(%rsp),%rbx
   140005214:	lea    0x9c9c5(%rip),%rax        # 1400a1be0 <.rdata+0x4be0>
   14000521b:	mov    %rax,0x30(%rsp)
   140005220:	movq   $0x170,0x38(%rsp)
   140005229:	movq   $0x5,0x40(%rsp)
   140005232:	mov    %rbx,%rcx
   140005235:	call   140030420 <txrt_record_class_view>
   14000523a:	mov    (%rsi),%rcx
   14000523d:	lea    0xa0(%rsp),%r15
   140005245:	mov    %r15,(%rsi)
   140005248:	mov    (%rax),%rax
   14000524b:	movq   $0x7,(%rax)
   140005252:	mov    %rcx,(%rsi)
   140005255:	mov    %rbx,0x20(%rsp)
   14000525a:	mov    %rbx,%rcx
   14000525d:	call   140030420 <txrt_record_class_view>
   140005262:	mov    %rax,%rbx
   140005265:	mov    %rsi,%rcx
   140005268:	call   14002cf00 <txrt_gc_safepoint_context>
   14000526d:	test   %eax,%eax
   14000526f:	jne    140005528 <tx_fn_m0_bench_virtual_interface_0+0x388>
   140005275:	lea    0x80(%rsp),%rcx
   14000527d:	call   140021ea0 <txrt_time_unix_millis>
   140005282:	test   %eax,%eax
   140005284:	jne    140005537 <tx_fn_m0_bench_virtual_interface_0+0x397>
   14000528a:	cmpq   $0x0,0x80(%rsp)
   140005293:	jg     14000531a <tx_fn_m0_bench_virtual_interface_0+0x17a>
   140005299:	lea    0x98280(%rip),%rcx        # 14009d520 <.rdata+0x520>
   1400052a0:	lea    0x78(%rsp),%rdx
   1400052a5:	call   1400129c0 <txrt_record_class_new>
   1400052aa:	test   %eax,%eax
   1400052ac:	jne    1400055be <tx_fn_m0_bench_virtual_interface_0+0x41e>
   1400052b2:	mov    0x78(%rsp),%r14
   1400052b7:	lea    0x9ca62(%rip),%rax        # 1400a1d20 <.rdata+0x4d20>
   1400052be:	mov    %rax,0x30(%rsp)
   1400052c3:	movq   $0x173,0x38(%rsp)
   1400052cc:	movq   $0x9,0x40(%rsp)
   1400052d5:	mov    %r14,%rcx
   1400052d8:	call   140030420 <txrt_record_class_view>
   1400052dd:	mov    (%rsi),%rcx
   1400052e0:	mov    %r15,(%rsi)
   1400052e3:	mov    (%rax),%rax
   1400052e6:	movq   $0x7,(%rax)
   1400052ed:	mov    %rcx,(%rsi)
   1400052f0:	mov    0x20(%rsp),%rcx
   1400052f5:	call   14001ccb0 <txrt_value_release>
   1400052fa:	mov    %r14,%rcx
   1400052fd:	call   140030420 <txrt_record_class_view>
   140005302:	mov    %rax,%rbx
   140005305:	mov    %rsi,%rcx
   140005308:	call   14002cf00 <txrt_gc_safepoint_context>
   14000530d:	test   %eax,%eax
   14000530f:	jne    1400055c7 <tx_fn_m0_bench_virtual_interface_0+0x427>
   140005315:	mov    %r14,0x20(%rsp)
   14000531a:	mov    %rsi,%rcx
   14000531d:	call   14002cf00 <txrt_gc_safepoint_context>
   140005322:	test   %eax,%eax
   140005324:	jne    140005546 <tx_fn_m0_bench_virtual_interface_0+0x3a6>
   14000532a:	lea    0x70(%rsp),%rdx
   14000532f:	mov    0x20(%rsp),%rcx
   140005334:	call   14001cb20 <txrt_value_clone>
   140005339:	test   %eax,%eax
   14000533b:	jne    140005555 <tx_fn_m0_bench_virtual_interface_0+0x3b5>
   140005341:	mov    0x70(%rsp),%r14
   140005346:	lea    0x97e73(%rip),%rdx        # 14009d1c0 <.rdata+0x1c0>
   14000534d:	mov    %r14,%rcx
   140005350:	call   140030ac0 <txrt_record_require_type>
   140005355:	mov    %r14,%rcx
   140005358:	call   140030420 <txrt_record_class_view>
   14000535d:	mov    %rax,%r12
   140005360:	mov    %rsi,%rcx
   140005363:	call   14002cf00 <txrt_gc_safepoint_context>
   140005368:	test   %eax,%eax
   14000536a:	jne    140005564 <tx_fn_m0_bench_virtual_interface_0+0x3c4>
   140005370:	lea    0x68(%rsp),%rcx
   140005375:	call   140022080 <txrt_time_monotonic_micros>
   14000537a:	test   %eax,%eax
   14000537c:	jne    140005573 <tx_fn_m0_bench_virtual_interface_0+0x3d3>
   140005382:	mov    %rdi,0x58(%rsp)
   140005387:	mov    0x68(%rsp),%rax
   14000538c:	mov    %rax,0x50(%rsp)
   140005391:	mov    %rsi,%rcx
   140005394:	call   14002cf00 <txrt_gc_safepoint_context>
   140005399:	test   %eax,%eax
   14000539b:	jne    140005582 <tx_fn_m0_bench_virtual_interface_0+0x3e2>
   1400053a1:	mov    $0x30d40,%r15d
   1400053a7:	xor    %r13d,%r13d
   1400053aa:	lea    0x9cb9f(%rip),%rdi        # 1400a1f50 <.rdata+0x4f50>
   1400053b1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   1400053c0:	mov    0x8(%rbx),%rax
   1400053c4:	mov    0x70(%rax),%rax
   1400053c8:	mov    (%rax),%rax
   1400053cb:	mov    %rdi,0x30(%rsp)
   1400053d0:	movq   $0x17a,0x38(%rsp)
   1400053d9:	movq   $0x9,0x40(%rsp)
   1400053e2:	mov    %rsi,%rcx
   1400053e5:	mov    0x20(%rsp),%rdx
   1400053ea:	mov    %rbx,%r8
   1400053ed:	call   *%rax
   1400053ef:	mov    %rax,%rbp
   1400053f2:	mov    0x8(%r12),%rax
   1400053f7:	mov    0x70(%rax),%rax
   1400053fb:	mov    %rsi,%rcx
   1400053fe:	mov    %r14,%rdx
   140005401:	mov    %r12,%r8
   140005404:	call   *0x8(%rax)
   140005407:	add    %rbp,%rax
   14000540a:	jo     1400054b8 <tx_fn_m0_bench_virtual_interface_0+0x318>
   140005410:	add    %rax,%r13
   140005413:	jo     1400054df <tx_fn_m0_bench_virtual_interface_0+0x33f>
   140005419:	dec    %r15
   14000541c:	jne    1400053c0 <tx_fn_m0_bench_virtual_interface_0+0x220>
   14000541e:	lea    0x9cc1b(%rip),%rcx        # 1400a2040 <.rdata+0x5040>
   140005425:	lea    0x60(%rsp),%r8
   14000542a:	mov    $0x11,%edx
   14000542f:	call   140038950 <txrt_str_new>
   140005434:	test   %eax,%eax
   140005436:	jne    140005591 <tx_fn_m0_bench_virtual_interface_0+0x3f1>
   14000543c:	mov    0x60(%rsp),%rbx
   140005441:	lea    0x9cc68(%rip),%rax        # 1400a20b0 <.rdata+0x50b0>
   140005448:	mov    %rax,0x30(%rsp)
   14000544d:	movq   $0x17c,0x38(%rsp)
   140005456:	movq   $0x5,0x40(%rsp)
   14000545f:	mov    %rsi,%rcx
   140005462:	mov    %rbx,%rdx
   140005465:	mov    0x50(%rsp),%r8
   14000546a:	mov    %r13,%r9
   14000546d:	call   140001960 <tx_fn_m0_report_0>
   140005472:	mov    %rbx,%rcx
   140005475:	call   140038d00 <txrt_str_release>
   14000547a:	mov    %rsi,%rcx
   14000547d:	call   14002cf00 <txrt_gc_safepoint_context>
   140005482:	test   %eax,%eax
   140005484:	jne    14000559a <tx_fn_m0_bench_virtual_interface_0+0x3fa>
   14000548a:	mov    %r14,%rcx
   14000548d:	call   14001ccb0 <txrt_value_release>
   140005492:	mov    0x20(%rsp),%rcx
   140005497:	call   14001ccb0 <txrt_value_release>
   14000549c:	mov    0x58(%rsp),%rax
   1400054a1:	mov    %rax,(%rsi)
   1400054a4:	add    $0xc8,%rsp
   1400054ab:	pop    %rbx
   1400054ac:	pop    %rbp
   1400054ad:	pop    %rdi
   1400054ae:	pop    %rsi
   1400054af:	pop    %r12
   1400054b1:	pop    %r13
   1400054b3:	pop    %r14
   1400054b5:	pop    %r15
   1400054b7:	ret
   1400054b8:	movabs $0x7fffffffffffffff,%rcx
   1400054c2:	lea    0x98(%rsp),%r8
   1400054ca:	mov    $0x1,%edx
   1400054cf:	call   140039440 <txrt_add_i64>
   1400054d4:	mov    %eax,%edi
   1400054d6:	lea    0x9cac3(%rip),%rdx        # 1400a1fa0 <.rdata+0x4fa0>
   1400054dd:	jmp    140005504 <tx_fn_m0_bench_virtual_interface_0+0x364>
   1400054df:	movabs $0x7fffffffffffffff,%rcx
   1400054e9:	lea    0x90(%rsp),%r8
   1400054f1:	mov    $0x1,%edx
   1400054f6:	call   140039440 <txrt_add_i64>
   1400054fb:	mov    %eax,%edi
   1400054fd:	lea    0x9caec(%rip),%rdx        # 1400a1ff0 <.rdata+0x4ff0>
   140005504:	mov    $0x17a,%r8d
   14000550a:	mov    $0x9,%r9d
   140005510:	mov    %rsi,%rcx
   140005513:	call   140033bc0 <txrt_stack_error_location>
   140005518:	mov    %edi,%ecx
   14000551a:	call   140038340 <txrt_require_success>
   14000551f:	lea    0x9c66a(%rip),%rdx        # 1400a1b90 <.rdata+0x4b90>
   140005526:	jmp    14000552f <tx_fn_m0_bench_virtual_interface_0+0x38f>
   140005528:	lea    0x9c701(%rip),%rdx        # 1400a1c30 <.rdata+0x4c30>
   14000552f:	mov    $0x170,%r8d
   140005535:	jmp    1400055a7 <tx_fn_m0_bench_virtual_interface_0+0x407>
   140005537:	lea    0x9c742(%rip),%rdx        # 1400a1c80 <.rdata+0x4c80>
   14000553e:	mov    $0x171,%r8d
   140005544:	jmp    1400055a7 <tx_fn_m0_bench_virtual_interface_0+0x407>
   140005546:	lea    0x9c873(%rip),%rdx        # 1400a1dc0 <.rdata+0x4dc0>
   14000554d:	mov    $0x171,%r8d
   140005553:	jmp    1400055a7 <tx_fn_m0_bench_virtual_interface_0+0x407>
   140005555:	lea    0x9c8b4(%rip),%rdx        # 1400a1e10 <.rdata+0x4e10>
   14000555c:	mov    $0x175,%r8d
   140005562:	jmp    1400055a7 <tx_fn_m0_bench_virtual_interface_0+0x407>
   140005564:	lea    0x9c8f5(%rip),%rdx        # 1400a1e60 <.rdata+0x4e60>
   14000556b:	mov    $0x175,%r8d
   140005571:	jmp    1400055a7 <tx_fn_m0_bench_virtual_interface_0+0x407>
   140005573:	lea    0x9c936(%rip),%rdx        # 1400a1eb0 <.rdata+0x4eb0>
   14000557a:	mov    $0x177,%r8d
   140005580:	jmp    1400055a7 <tx_fn_m0_bench_virtual_interface_0+0x407>
   140005582:	lea    0x9c977(%rip),%rdx        # 1400a1f00 <.rdata+0x4f00>
   140005589:	mov    $0x177,%r8d
   14000558f:	jmp    1400055a7 <tx_fn_m0_bench_virtual_interface_0+0x407>
   140005591:	lea    0x9cac8(%rip),%rdx        # 1400a2060 <.rdata+0x5060>
   140005598:	jmp    1400055a1 <tx_fn_m0_bench_virtual_interface_0+0x401>
   14000559a:	lea    0x9cb5f(%rip),%rdx        # 1400a2100 <.rdata+0x5100>
   1400055a1:	mov    $0x17c,%r8d
   1400055a7:	mov    $0x5,%r9d
   1400055ad:	mov    %rsi,%rcx
   1400055b0:	mov    %eax,%esi
   1400055b2:	call   140033bc0 <txrt_stack_error_location>
   1400055b7:	mov    %esi,%ecx
   1400055b9:	call   140038340 <txrt_require_success>
   1400055be:	lea    0x9c70b(%rip),%rdx        # 1400a1cd0 <.rdata+0x4cd0>
   1400055c5:	jmp    1400055ce <tx_fn_m0_bench_virtual_interface_0+0x42e>
   1400055c7:	lea    0x9c7a2(%rip),%rdx        # 1400a1d70 <.rdata+0x4d70>
   1400055ce:	mov    $0x173,%r8d
   1400055d4:	mov    $0x9,%r9d
   1400055da:	jmp    1400055ad <tx_fn_m0_bench_virtual_interface_0+0x40d>
   1400055dc:	nopl   0x0(%rax)
