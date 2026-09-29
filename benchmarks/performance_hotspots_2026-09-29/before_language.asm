
E:\Project\other\Compilation\tx_build\performance_hotspots\before\language.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140005160 <tx_fn_m0_bench_virtual_interface_0>:
   140005160:	push   %r15
   140005162:	push   %r14
   140005164:	push   %r13
   140005166:	push   %r12
   140005168:	push   %rsi
   140005169:	push   %rdi
   14000516a:	push   %rbp
   14000516b:	push   %rbx
   14000516c:	sub    $0xb8,%rsp
   140005173:	mov    %rcx,%rsi
   140005176:	mov    (%rcx),%r15
   140005179:	lea    0x9e0c0(%rip),%rax        # 1400a3240 <.rdata+0x5240>
   140005180:	mov    %rax,0x20(%rsp)
   140005185:	lea    0x9e0d4(%rip),%rax        # 1400a3260 <.rdata+0x5260>
   14000518c:	mov    %rax,0x28(%rsp)
   140005191:	movq   $0x16e,0x30(%rsp)
   14000519a:	movq   $0x1,0x38(%rsp)
   1400051a3:	mov    %r15,0x40(%rsp)
   1400051a8:	lea    0x20(%rsp),%rax
   1400051ad:	mov    %rax,(%rcx)
   1400051b0:	lea    0x99159(%rip),%rcx        # 14009e310 <.rdata+0x310>
   1400051b7:	lea    0x78(%rsp),%rdx
   1400051bc:	call   140012760 <txrt_record_class_new>
   1400051c1:	test   %eax,%eax
   1400051c3:	jne    1400054ce <tx_fn_m0_bench_virtual_interface_0+0x36e>
   1400051c9:	mov    0x78(%rsp),%rdi
   1400051ce:	lea    0x9daab(%rip),%rax        # 1400a2c80 <.rdata+0x4c80>
   1400051d5:	mov    %rax,0x28(%rsp)
   1400051da:	movq   $0x170,0x30(%rsp)
   1400051e3:	movq   $0x5,0x38(%rsp)
   1400051ec:	mov    %rdi,%rcx
   1400051ef:	call   1400316a0 <txrt_record_class_view>
   1400051f4:	mov    (%rsi),%rcx
   1400051f7:	lea    0x90(%rsp),%r14
   1400051ff:	mov    %r14,(%rsi)
   140005202:	mov    (%rax),%rax
   140005205:	movq   $0x7,(%rax)
   14000520c:	mov    %rcx,(%rsi)
   14000520f:	mov    %rdi,%rcx
   140005212:	call   1400316a0 <txrt_record_class_view>
   140005217:	mov    %rsi,%rcx
   14000521a:	call   14002e180 <txrt_gc_safepoint_context>
   14000521f:	test   %eax,%eax
   140005221:	jne    1400054d7 <tx_fn_m0_bench_virtual_interface_0+0x377>
   140005227:	lea    0x70(%rsp),%rcx
   14000522c:	call   140021c40 <txrt_time_unix_millis>
   140005231:	test   %eax,%eax
   140005233:	jne    1400054e6 <tx_fn_m0_bench_virtual_interface_0+0x386>
   140005239:	cmpq   $0x0,0x70(%rsp)
   14000523f:	jg     1400052bb <tx_fn_m0_bench_virtual_interface_0+0x15b>
   140005241:	lea    0x99258(%rip),%rcx        # 14009e4a0 <.rdata+0x4a0>
   140005248:	lea    0x68(%rsp),%rdx
   14000524d:	call   140012760 <txrt_record_class_new>
   140005252:	test   %eax,%eax
   140005254:	jne    14000556d <tx_fn_m0_bench_virtual_interface_0+0x40d>
   14000525a:	mov    0x68(%rsp),%rbx
   14000525f:	lea    0x9db5a(%rip),%rax        # 1400a2dc0 <.rdata+0x4dc0>
   140005266:	mov    %rax,0x28(%rsp)
   14000526b:	movq   $0x173,0x30(%rsp)
   140005274:	movq   $0x9,0x38(%rsp)
   14000527d:	mov    %rbx,%rcx
   140005280:	call   1400316a0 <txrt_record_class_view>
   140005285:	mov    (%rsi),%rcx
   140005288:	mov    %r14,(%rsi)
   14000528b:	mov    (%rax),%rax
   14000528e:	movq   $0x7,(%rax)
   140005295:	mov    %rcx,(%rsi)
   140005298:	mov    %rdi,%rcx
   14000529b:	call   14001ca50 <txrt_value_release>
   1400052a0:	mov    %rbx,%rcx
   1400052a3:	call   1400316a0 <txrt_record_class_view>
   1400052a8:	mov    %rsi,%rcx
   1400052ab:	call   14002e180 <txrt_gc_safepoint_context>
   1400052b0:	mov    %rbx,%rdi
   1400052b3:	test   %eax,%eax
   1400052b5:	jne    140005576 <tx_fn_m0_bench_virtual_interface_0+0x416>
   1400052bb:	mov    %rsi,%rcx
   1400052be:	call   14002e180 <txrt_gc_safepoint_context>
   1400052c3:	test   %eax,%eax
   1400052c5:	jne    1400054f5 <tx_fn_m0_bench_virtual_interface_0+0x395>
   1400052cb:	lea    0x60(%rsp),%rdx
   1400052d0:	mov    %rdi,%rcx
   1400052d3:	call   14001c8c0 <txrt_value_clone>
   1400052d8:	test   %eax,%eax
   1400052da:	jne    140005504 <tx_fn_m0_bench_virtual_interface_0+0x3a4>
   1400052e0:	mov    0x60(%rsp),%rbx
   1400052e5:	lea    0x98e94(%rip),%rdx        # 14009e180 <.rdata+0x180>
   1400052ec:	mov    %rbx,%rcx
   1400052ef:	call   140031d40 <txrt_record_require_type>
   1400052f4:	mov    %rbx,%rcx
   1400052f7:	call   1400316a0 <txrt_record_class_view>
   1400052fc:	mov    %rsi,%rcx
   1400052ff:	call   14002e180 <txrt_gc_safepoint_context>
   140005304:	test   %eax,%eax
   140005306:	jne    140005513 <tx_fn_m0_bench_virtual_interface_0+0x3b3>
   14000530c:	lea    0x58(%rsp),%rcx
   140005311:	call   140021e20 <txrt_time_monotonic_micros>
   140005316:	test   %eax,%eax
   140005318:	jne    140005522 <tx_fn_m0_bench_virtual_interface_0+0x3c2>
   14000531e:	mov    %r15,0x48(%rsp)
   140005323:	mov    0x58(%rsp),%r14
   140005328:	mov    %rsi,%rcx
   14000532b:	call   14002e180 <txrt_gc_safepoint_context>
   140005330:	test   %eax,%eax
   140005332:	jne    140005531 <tx_fn_m0_bench_virtual_interface_0+0x3d1>
   140005338:	mov    $0x30d40,%ebp
   14000533d:	xor    %r15d,%r15d
   140005340:	lea    0x9dca9(%rip),%r13        # 1400a2ff0 <.rdata+0x4ff0>
   140005347:	nopw   0x0(%rax,%rax,1)
   140005350:	mov    %rdi,%rcx
   140005353:	call   1400316a0 <txrt_record_class_view>
   140005358:	mov    0x8(%rax),%rax
   14000535c:	mov    0x38(%rax),%rax
   140005360:	mov    (%rax),%rax
   140005363:	mov    %r13,0x28(%rsp)
   140005368:	movq   $0x17a,0x30(%rsp)
   140005371:	movq   $0x9,0x38(%rsp)
   14000537a:	mov    %rsi,%rcx
   14000537d:	mov    %rdi,%rdx
   140005380:	call   *%rax
   140005382:	mov    %rax,%r12
   140005385:	mov    %rbx,%rcx
   140005388:	call   1400316a0 <txrt_record_class_view>
   14000538d:	mov    0x8(%rax),%rax
   140005391:	mov    0x38(%rax),%rax
   140005395:	mov    %rsi,%rcx
   140005398:	mov    %rbx,%rdx
   14000539b:	call   *0x8(%rax)
   14000539e:	mov    %r12,%rdx
   1400053a1:	add    %rax,%rdx
   1400053a4:	jo     140005464 <tx_fn_m0_bench_virtual_interface_0+0x304>
   1400053aa:	mov    %r15,%r12
   1400053ad:	add    %rdx,%r12
   1400053b0:	jo     140005482 <tx_fn_m0_bench_virtual_interface_0+0x322>
   1400053b6:	mov    %rsi,%rcx
   1400053b9:	call   14002e180 <txrt_gc_safepoint_context>
   1400053be:	test   %eax,%eax
   1400053c0:	jne    1400054b6 <tx_fn_m0_bench_virtual_interface_0+0x356>
   1400053c6:	mov    %r12,%r15
   1400053c9:	dec    %rbp
   1400053cc:	jne    140005350 <tx_fn_m0_bench_virtual_interface_0+0x1f0>
   1400053ce:	lea    0x9dd5b(%rip),%rcx        # 1400a3130 <.rdata+0x5130>
   1400053d5:	lea    0x50(%rsp),%r8
   1400053da:	mov    $0x11,%edx
   1400053df:	call   140039bd0 <txrt_str_new>
   1400053e4:	test   %eax,%eax
   1400053e6:	jne    140005540 <tx_fn_m0_bench_virtual_interface_0+0x3e0>
   1400053ec:	mov    0x50(%rsp),%r15
   1400053f1:	lea    0x9dda8(%rip),%rax        # 1400a31a0 <.rdata+0x51a0>
   1400053f8:	mov    %rax,0x28(%rsp)
   1400053fd:	movq   $0x17c,0x30(%rsp)
   140005406:	movq   $0x5,0x38(%rsp)
   14000540f:	mov    %rsi,%rcx
   140005412:	mov    %r15,%rdx
   140005415:	mov    %r14,%r8
   140005418:	mov    %r12,%r9
   14000541b:	call   140001930 <tx_fn_m0_report_0>
   140005420:	mov    %r15,%rcx
   140005423:	call   140039f80 <txrt_str_release>
   140005428:	mov    %rsi,%rcx
   14000542b:	call   14002e180 <txrt_gc_safepoint_context>
   140005430:	test   %eax,%eax
   140005432:	jne    140005549 <tx_fn_m0_bench_virtual_interface_0+0x3e9>
   140005438:	mov    %rbx,%rcx
   14000543b:	call   14001ca50 <txrt_value_release>
   140005440:	mov    %rdi,%rcx
   140005443:	call   14001ca50 <txrt_value_release>
   140005448:	mov    0x48(%rsp),%rax
   14000544d:	mov    %rax,(%rsi)
   140005450:	add    $0xb8,%rsp
   140005457:	pop    %rbx
   140005458:	pop    %rbp
   140005459:	pop    %rdi
   14000545a:	pop    %rsi
   14000545b:	pop    %r12
   14000545d:	pop    %r13
   14000545f:	pop    %r14
   140005461:	pop    %r15
   140005463:	ret
   140005464:	lea    0x88(%rsp),%r8
   14000546c:	mov    %r12,%rcx
   14000546f:	mov    %rax,%rdx
   140005472:	call   14003a6c0 <txrt_add_i64>
   140005477:	mov    %eax,%edi
   140005479:	lea    0x9dbc0(%rip),%rdx        # 1400a3040 <.rdata+0x5040>
   140005480:	jmp    14000549b <tx_fn_m0_bench_virtual_interface_0+0x33b>
   140005482:	lea    0x80(%rsp),%r8
   14000548a:	mov    %r15,%rcx
   14000548d:	call   14003a6c0 <txrt_add_i64>
   140005492:	mov    %eax,%edi
   140005494:	lea    0x9dbf5(%rip),%rdx        # 1400a3090 <.rdata+0x5090>
   14000549b:	mov    $0x17a,%r8d
   1400054a1:	mov    $0x9,%r9d
   1400054a7:	mov    %rsi,%rcx
   1400054aa:	call   140034e40 <txrt_stack_error_location>
   1400054af:	mov    %edi,%ecx
   1400054b1:	call   1400395c0 <txrt_require_success>
   1400054b6:	lea    0x9dc23(%rip),%rdx        # 1400a30e0 <.rdata+0x50e0>
   1400054bd:	mov    $0x17a,%r8d
   1400054c3:	mov    $0x9,%r9d
   1400054c9:	jmp    14000555c <tx_fn_m0_bench_virtual_interface_0+0x3fc>
   1400054ce:	lea    0x9d75b(%rip),%rdx        # 1400a2c30 <.rdata+0x4c30>
   1400054d5:	jmp    1400054de <tx_fn_m0_bench_virtual_interface_0+0x37e>
   1400054d7:	lea    0x9d7f2(%rip),%rdx        # 1400a2cd0 <.rdata+0x4cd0>
   1400054de:	mov    $0x170,%r8d
   1400054e4:	jmp    140005556 <tx_fn_m0_bench_virtual_interface_0+0x3f6>
   1400054e6:	lea    0x9d833(%rip),%rdx        # 1400a2d20 <.rdata+0x4d20>
   1400054ed:	mov    $0x171,%r8d
   1400054f3:	jmp    140005556 <tx_fn_m0_bench_virtual_interface_0+0x3f6>
   1400054f5:	lea    0x9d964(%rip),%rdx        # 1400a2e60 <.rdata+0x4e60>
   1400054fc:	mov    $0x171,%r8d
   140005502:	jmp    140005556 <tx_fn_m0_bench_virtual_interface_0+0x3f6>
   140005504:	lea    0x9d9a5(%rip),%rdx        # 1400a2eb0 <.rdata+0x4eb0>
   14000550b:	mov    $0x175,%r8d
   140005511:	jmp    140005556 <tx_fn_m0_bench_virtual_interface_0+0x3f6>
   140005513:	lea    0x9d9e6(%rip),%rdx        # 1400a2f00 <.rdata+0x4f00>
   14000551a:	mov    $0x175,%r8d
   140005520:	jmp    140005556 <tx_fn_m0_bench_virtual_interface_0+0x3f6>
   140005522:	lea    0x9da27(%rip),%rdx        # 1400a2f50 <.rdata+0x4f50>
   140005529:	mov    $0x177,%r8d
   14000552f:	jmp    140005556 <tx_fn_m0_bench_virtual_interface_0+0x3f6>
   140005531:	lea    0x9da68(%rip),%rdx        # 1400a2fa0 <.rdata+0x4fa0>
   140005538:	mov    $0x177,%r8d
   14000553e:	jmp    140005556 <tx_fn_m0_bench_virtual_interface_0+0x3f6>
   140005540:	lea    0x9dc09(%rip),%rdx        # 1400a3150 <.rdata+0x5150>
   140005547:	jmp    140005550 <tx_fn_m0_bench_virtual_interface_0+0x3f0>
   140005549:	lea    0x9dca0(%rip),%rdx        # 1400a31f0 <.rdata+0x51f0>
   140005550:	mov    $0x17c,%r8d
   140005556:	mov    $0x5,%r9d
   14000555c:	mov    %rsi,%rcx
   14000555f:	mov    %eax,%esi
   140005561:	call   140034e40 <txrt_stack_error_location>
   140005566:	mov    %esi,%ecx
   140005568:	call   1400395c0 <txrt_require_success>
   14000556d:	lea    0x9d7fc(%rip),%rdx        # 1400a2d70 <.rdata+0x4d70>
   140005574:	jmp    14000557d <tx_fn_m0_bench_virtual_interface_0+0x41d>
   140005576:	lea    0x9d893(%rip),%rdx        # 1400a2e10 <.rdata+0x4e10>
   14000557d:	mov    $0x173,%r8d
   140005583:	mov    $0x9,%r9d
   140005589:	jmp    14000555c <tx_fn_m0_bench_virtual_interface_0+0x3fc>
   14000558b:	nopl   0x0(%rax,%rax,1)
