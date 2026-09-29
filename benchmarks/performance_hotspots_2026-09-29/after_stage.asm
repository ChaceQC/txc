
E:\Project\other\Compilation\tx_build\performance_hotspots\after\stage.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001aa0 <tx_fn_main_0>:
   140001aa0:	push   %r15
   140001aa2:	push   %r14
   140001aa4:	push   %r13
   140001aa6:	push   %r12
   140001aa8:	push   %rsi
   140001aa9:	push   %rdi
   140001aaa:	push   %rbp
   140001aab:	push   %rbx
   140001aac:	sub    $0xd8,%rsp
   140001ab3:	mov    %rcx,%rsi
   140001ab6:	mov    (%rcx),%rbx
   140001ab9:	lea    0x7ffcf(%rip),%rax        # 140081a8f <.rdata+0xa8f>
   140001ac0:	mov    %rax,0xa0(%rsp)
   140001ac8:	lea    0x7ffd1(%rip),%rax        # 140081aa0 <.rdata+0xaa0>
   140001acf:	mov    %rax,0xa8(%rsp)
   140001ad7:	movq   $0x18,0xb0(%rsp)
   140001ae3:	movq   $0x1,0xb8(%rsp)
   140001aef:	mov    %rbx,0xc0(%rsp)
   140001af7:	lea    0xa0(%rsp),%rax
   140001aff:	mov    %rax,(%rcx)
   140001b02:	lea    0x98(%rsp),%rdx
   140001b0a:	xor    %ecx,%ecx
   140001b0c:	call   140020a30 <txrt_input>
   140001b11:	test   %eax,%eax
   140001b13:	jne    140001f3f <tx_fn_main_0+0x49f>
   140001b19:	mov    0x98(%rsp),%rdi
   140001b21:	lea    0x90(%rsp),%rdx
   140001b29:	mov    %rdi,%rcx
   140001b2c:	call   140020210 <txrt_parse_int>
   140001b31:	test   %eax,%eax
   140001b33:	jne    140001f48 <tx_fn_main_0+0x4a8>
   140001b39:	mov    %rdi,%rcx
   140001b3c:	call   14001fcd0 <txrt_str_release>
   140001b41:	mov    0x90(%rsp),%rdi
   140001b49:	mov    %rsi,%rcx
   140001b4c:	call   140015880 <txrt_gc_safepoint_context>
   140001b51:	test   %eax,%eax
   140001b53:	jne    140001f51 <tx_fn_main_0+0x4b1>
   140001b59:	lea    0x7f950(%rip),%rax        # 1400814b0 <.rdata+0x4b0>
   140001b60:	mov    %rax,0xa8(%rsp)
   140001b68:	movq   $0x1b,0xb0(%rsp)
   140001b74:	movq   $0x5,0xb8(%rsp)
   140001b80:	mov    %rsi,%rcx
   140001b83:	mov    %rdi,%rdx
   140001b86:	call   1400015e0 <tx_fn_m0_read_earliest_0>
   140001b8b:	mov    %rax,%r14
   140001b8e:	mov    %rax,%rcx
   140001b91:	call   14000b5c0 <txrt_array_ref>
   140001b96:	mov    %rax,%r13
   140001b99:	mov    %rsi,%rcx
   140001b9c:	call   140015880 <txrt_gc_safepoint_context>
   140001ba1:	test   %eax,%eax
   140001ba3:	jne    140001f63 <tx_fn_main_0+0x4c3>
   140001ba9:	lea    0x88(%rsp),%rcx
   140001bb1:	call   140008b80 <txrt_time_monotonic_micros>
   140001bb6:	test   %eax,%eax
   140001bb8:	jne    140001f75 <tx_fn_main_0+0x4d5>
   140001bbe:	mov    0x88(%rsp),%r15
   140001bc6:	mov    %rsi,%rcx
   140001bc9:	call   140015880 <txrt_gc_safepoint_context>
   140001bce:	test   %eax,%eax
   140001bd0:	jne    140001f84 <tx_fn_main_0+0x4e4>
   140001bd6:	mov    %rdi,%rbp
   140001bd9:	inc    %rbp
   140001bdc:	jo     140001f93 <tx_fn_main_0+0x4f3>
   140001be2:	lea    0x80(%rsp),%rdx
   140001bea:	mov    %rbp,%rcx
   140001bed:	call   14000ac40 <txrt_local_scalar_array_new>
   140001bf2:	test   %eax,%eax
   140001bf4:	jne    140001fd3 <tx_fn_main_0+0x533>
   140001bfa:	mov    0x80(%rsp),%rax
   140001c02:	mov    %rax,0x30(%rsp)
   140001c07:	mov    %rsi,%rcx
   140001c0a:	call   140015880 <txrt_gc_safepoint_context>
   140001c0f:	test   %eax,%eax
   140001c11:	jne    140001fe2 <tx_fn_main_0+0x542>
   140001c17:	mov    %r15,0x50(%rsp)
   140001c1c:	lea    0x78(%rsp),%rdx
   140001c21:	mov    %rdi,%rcx
   140001c24:	call   14000d320 <txrt_array_new>
   140001c29:	test   %eax,%eax
   140001c2b:	jne    140001ff1 <tx_fn_main_0+0x551>
   140001c31:	mov    %r14,0x40(%rsp)
   140001c36:	mov    %rbx,0x48(%rsp)
   140001c3b:	mov    0x78(%rsp),%rcx
   140001c40:	mov    %rcx,0x38(%rsp)
   140001c45:	call   14000b5c0 <txrt_array_ref>
   140001c4a:	mov    %rax,%r12
   140001c4d:	mov    %rsi,0x28(%rsp)
   140001c52:	mov    %rsi,%rcx
   140001c55:	call   140015880 <txrt_gc_safepoint_context>
   140001c5a:	test   %eax,%eax
   140001c5c:	jne    140002015 <tx_fn_main_0+0x575>
   140001c62:	test   %rdi,%rdi
   140001c65:	jle    140001d12 <tx_fn_main_0+0x272>
   140001c6b:	mov    %rdi,%rsi
   140001c6e:	shl    $0x4,%rsi
   140001c72:	add    0x30(%rsp),%rsi
   140001c77:	mov    $0x1,%r14d
   140001c7d:	nopl   (%rax)
   140001c80:	mov    %rsi,%rbx
   140001c83:	mov    %rdi,%r15
   140001c86:	jmp    140001ca5 <tx_fn_main_0+0x205>
   140001c88:	nopl   0x0(%rax,%rax,1)
   140001c90:	xor    %r8d,%r8d
   140001c93:	cmp    $0x2,%r15
   140001c97:	jl     140001ce0 <tx_fn_main_0+0x240>
   140001c99:	dec    %r15
   140001c9c:	add    $0xfffffffffffffff0,%rbx
   140001ca0:	test   %r8,%r8
   140001ca3:	jne    140001ce0 <tx_fn_main_0+0x240>
   140001ca5:	cmp    %rbp,%rdi
   140001ca8:	jge    140001ef3 <tx_fn_main_0+0x453>
   140001cae:	cmpb   $0x1,(%rbx)
   140001cb1:	jne    140001c90 <tx_fn_main_0+0x1f0>
   140001cb3:	mov    %r13,%rcx
   140001cb6:	mov    %r15,%rdx
   140001cb9:	call   14000b7d0 <txrt_array_ref_get_i64>
   140001cbe:	cmp    %r14,%rax
   140001cc1:	jg     140001c90 <tx_fn_main_0+0x1f0>
   140001cc3:	movb   $0x4,(%rbx)
   140001cc6:	movq   $0x1,0x8(%rbx)
   140001cce:	mov    %r15,%r8
   140001cd1:	cmp    $0x2,%r15
   140001cd5:	jge    140001c99 <tx_fn_main_0+0x1f9>
   140001cd7:	nopw   0x0(%rax,%rax,1)
   140001ce0:	lea    -0x1(%r14),%rdx
   140001ce4:	mov    %r12,%rcx
   140001ce7:	call   14000c300 <txrt_array_ref_set_i64>
   140001cec:	test   %eax,%eax
   140001cee:	jne    140001ef8 <tx_fn_main_0+0x458>
   140001cf4:	mov    0x28(%rsp),%rcx
   140001cf9:	call   140015880 <txrt_gc_safepoint_context>
   140001cfe:	test   %eax,%eax
   140001d00:	jne    140001f01 <tx_fn_main_0+0x461>
   140001d06:	inc    %r14
   140001d09:	cmp    %rbp,%r14
   140001d0c:	jne    140001c80 <tx_fn_main_0+0x1e0>
   140001d12:	lea    0x70(%rsp),%rcx
   140001d17:	call   140008b80 <txrt_time_monotonic_micros>
   140001d1c:	test   %eax,%eax
   140001d1e:	jne    140002024 <tx_fn_main_0+0x584>
   140001d24:	mov    0x70(%rsp),%r13
   140001d29:	sub    0x50(%rsp),%r13
   140001d2e:	mov    0x28(%rsp),%r14
   140001d33:	jo     14000203c <tx_fn_main_0+0x59c>
   140001d39:	mov    %r14,%rcx
   140001d3c:	call   140015880 <txrt_gc_safepoint_context>
   140001d41:	test   %eax,%eax
   140001d43:	jne    14000207f <tx_fn_main_0+0x5df>
   140001d49:	test   %rdi,%rdi
   140001d4c:	jle    140001dea <tx_fn_main_0+0x34a>
   140001d52:	mov    %r12,%rcx
   140001d55:	xor    %edx,%edx
   140001d57:	call   14000ffe0 <txrt_array_ref_get_str>
   140001d5c:	mov    %rax,%rbp
   140001d5f:	mov    %r12,%rcx
   140001d62:	xor    %edx,%edx
   140001d64:	mov    %rax,%r8
   140001d67:	call   14000f710 <txrt_array_ref_set_str>
   140001d6c:	test   %eax,%eax
   140001d6e:	jne    140001dc4 <tx_fn_main_0+0x324>
   140001d70:	mov    $0x1,%r15d
   140001d76:	cs nopw 0x0(%rax,%rax,1)
   140001d80:	mov    %rbp,%rcx
   140001d83:	call   14001fcd0 <txrt_str_release>
   140001d88:	mov    %r14,%rcx
   140001d8b:	call   140015880 <txrt_gc_safepoint_context>
   140001d90:	test   %eax,%eax
   140001d92:	jne    140001f27 <tx_fn_main_0+0x487>
   140001d98:	cmp    %r15,%rdi
   140001d9b:	je     140001dea <tx_fn_main_0+0x34a>
   140001d9d:	lea    0x1(%r15),%rsi
   140001da1:	mov    %r12,%rcx
   140001da4:	mov    %r15,%rdx
   140001da7:	call   14000ffe0 <txrt_array_ref_get_str>
   140001dac:	mov    %rax,%rbp
   140001daf:	mov    %r12,%rcx
   140001db2:	mov    %r15,%rdx
   140001db5:	mov    %rax,%r8
   140001db8:	call   14000f710 <txrt_array_ref_set_str>
   140001dbd:	mov    %rsi,%r15
   140001dc0:	test   %eax,%eax
   140001dc2:	je     140001d80 <tx_fn_main_0+0x2e0>
   140001dc4:	mov    %eax,%ebx
   140001dc6:	lea    0x7fa63(%rip),%rdx        # 140081830 <.rdata+0x830>
   140001dcd:	mov    $0x34,%r8d
   140001dd3:	mov    $0x9,%r9d
   140001dd9:	mov    0x28(%rsp),%rcx
   140001dde:	call   14001ab90 <txrt_stack_error_location>
   140001de3:	mov    %ebx,%ecx
   140001de5:	call   14001f310 <txrt_require_success>
   140001dea:	lea    0x7fabe(%rip),%rdx        # 1400818af <.rdata+0x8af>
   140001df1:	lea    0x68(%rsp),%r9
   140001df6:	mov    $0x1,%r8d
   140001dfc:	mov    0x38(%rsp),%rsi
   140001e01:	mov    %rsi,%rcx
   140001e04:	call   1400115e0 <txrt_string_join_literal>
   140001e09:	test   %eax,%eax
   140001e0b:	jne    14000208e <tx_fn_main_0+0x5ee>
   140001e11:	mov    0x68(%rsp),%rdi
   140001e16:	mov    %rdi,%rcx
   140001e19:	mov    $0x1,%dl
   140001e1b:	call   14001fef0 <txrt_print_str>
   140001e20:	test   %eax,%eax
   140001e22:	jne    140002097 <tx_fn_main_0+0x5f7>
   140001e28:	mov    %rdi,%rcx
   140001e2b:	call   14001fcd0 <txrt_str_release>
   140001e30:	mov    %r14,%rcx
   140001e33:	call   140015880 <txrt_gc_safepoint_context>
   140001e38:	test   %eax,%eax
   140001e3a:	jne    1400020a0 <tx_fn_main_0+0x600>
   140001e40:	lea    0x58(%rsp),%rdx
   140001e45:	mov    %r13,%rcx
   140001e48:	call   140020b90 <txrt_int_to_str>
   140001e4d:	test   %eax,%eax
   140001e4f:	jne    1400020af <tx_fn_main_0+0x60f>
   140001e55:	mov    0x58(%rsp),%rdi
   140001e5a:	lea    0x60(%rsp),%rax
   140001e5f:	mov    %rax,0x20(%rsp)
   140001e64:	lea    0x7fb54(%rip),%rdx        # 1400819bf <.rdata+0x9bf>
   140001e6b:	mov    $0x1,%r8d
   140001e71:	mov    %rdi,%rcx
   140001e74:	xor    %r9d,%r9d
   140001e77:	call   140019590 <txrt_str_concat_literal>
   140001e7c:	test   %eax,%eax
   140001e7e:	jne    1400020b8 <tx_fn_main_0+0x618>
   140001e84:	mov    %rdi,%rcx
   140001e87:	call   14001fcd0 <txrt_str_release>
   140001e8c:	mov    0x60(%rsp),%rdi
   140001e91:	mov    %rdi,%rcx
   140001e94:	call   140008d60 <txrt_io_write_error>
   140001e99:	test   %eax,%eax
   140001e9b:	jne    1400020c1 <tx_fn_main_0+0x621>
   140001ea1:	mov    %rdi,%rcx
   140001ea4:	call   14001fcd0 <txrt_str_release>
   140001ea9:	mov    %r14,%rcx
   140001eac:	call   140015880 <txrt_gc_safepoint_context>
   140001eb1:	test   %eax,%eax
   140001eb3:	jne    1400020ca <tx_fn_main_0+0x62a>
   140001eb9:	mov    0x30(%rsp),%rcx
   140001ebe:	call   14000ae10 <txrt_local_scalar_array_release>
   140001ec3:	mov    0x40(%rsp),%rcx
   140001ec8:	call   1400037b0 <txrt_value_release>
   140001ecd:	mov    %rsi,%rcx
   140001ed0:	call   1400037b0 <txrt_value_release>
   140001ed5:	mov    0x48(%rsp),%rax
   140001eda:	mov    %rax,(%r14)
   140001edd:	xor    %eax,%eax
   140001edf:	add    $0xd8,%rsp
   140001ee6:	pop    %rbx
   140001ee7:	pop    %rbp
   140001ee8:	pop    %rdi
   140001ee9:	pop    %rsi
   140001eea:	pop    %r12
   140001eec:	pop    %r13
   140001eee:	pop    %r14
   140001ef0:	pop    %r15
   140001ef2:	ret
   140001ef3:	call   14000b680 <txrt_array_index_error>
   140001ef8:	lea    0x7f7f1(%rip),%rdx        # 1400816f0 <.rdata+0x6f0>
   140001eff:	jmp    140001f08 <tx_fn_main_0+0x468>
   140001f01:	lea    0x7f828(%rip),%rdx        # 140081730 <.rdata+0x730>
   140001f08:	mov    $0x2e,%r8d
   140001f0e:	mov    $0x9,%r9d
   140001f14:	mov    0x28(%rsp),%rcx
   140001f19:	mov    %eax,%esi
   140001f1b:	call   14001ab90 <txrt_stack_error_location>
   140001f20:	mov    %esi,%ecx
   140001f22:	call   14001f310 <txrt_require_success>
   140001f27:	lea    0x7f942(%rip),%rdx        # 140081870 <.rdata+0x870>
   140001f2e:	mov    $0x34,%r8d
   140001f34:	mov    $0x9,%r9d
   140001f3a:	jmp    1400020dd <tx_fn_main_0+0x63d>
   140001f3f:	lea    0x7f4aa(%rip),%rdx        # 1400813f0 <.rdata+0x3f0>
   140001f46:	jmp    140001f58 <tx_fn_main_0+0x4b8>
   140001f48:	lea    0x7f4e1(%rip),%rdx        # 140081430 <.rdata+0x430>
   140001f4f:	jmp    140001f58 <tx_fn_main_0+0x4b8>
   140001f51:	lea    0x7f518(%rip),%rdx        # 140081470 <.rdata+0x470>
   140001f58:	mov    $0x1a,%r8d
   140001f5e:	jmp    140001ffe <tx_fn_main_0+0x55e>
   140001f63:	lea    0x7f586(%rip),%rdx        # 1400814f0 <.rdata+0x4f0>
   140001f6a:	mov    $0x1b,%r8d
   140001f70:	jmp    140001ffe <tx_fn_main_0+0x55e>
   140001f75:	lea    0x7f5b4(%rip),%rdx        # 140081530 <.rdata+0x530>
   140001f7c:	mov    $0x1e,%r8d
   140001f82:	jmp    140001ffe <tx_fn_main_0+0x55e>
   140001f84:	lea    0x7f5e5(%rip),%rdx        # 140081570 <.rdata+0x570>
   140001f8b:	mov    $0x1e,%r8d
   140001f91:	jmp    140001ffe <tx_fn_main_0+0x55e>
   140001f93:	movabs $0x7fffffffffffffff,%rcx
   140001f9d:	lea    0xd0(%rsp),%r8
   140001fa5:	mov    $0x1,%edx
   140001faa:	call   140020410 <txrt_add_i64>
   140001faf:	mov    %eax,%edi
   140001fb1:	lea    0x7f5f8(%rip),%rdx        # 1400815b0 <.rdata+0x5b0>
   140001fb8:	mov    $0x1f,%r8d
   140001fbe:	mov    $0x5,%r9d
   140001fc4:	mov    %rsi,%rcx
   140001fc7:	call   14001ab90 <txrt_stack_error_location>
   140001fcc:	mov    %edi,%ecx
   140001fce:	call   14001f310 <txrt_require_success>
   140001fd3:	lea    0x7f616(%rip),%rdx        # 1400815f0 <.rdata+0x5f0>
   140001fda:	mov    $0x1f,%r8d
   140001fe0:	jmp    140001ffe <tx_fn_main_0+0x55e>
   140001fe2:	lea    0x7f647(%rip),%rdx        # 140081630 <.rdata+0x630>
   140001fe9:	mov    $0x1f,%r8d
   140001fef:	jmp    140001ffe <tx_fn_main_0+0x55e>
   140001ff1:	lea    0x7f678(%rip),%rdx        # 140081670 <.rdata+0x670>
   140001ff8:	mov    $0x20,%r8d
   140001ffe:	mov    $0x5,%r9d
   140002004:	mov    %rsi,%rcx
   140002007:	mov    %eax,%esi
   140002009:	call   14001ab90 <txrt_stack_error_location>
   14000200e:	mov    %esi,%ecx
   140002010:	call   14001f310 <txrt_require_success>
   140002015:	lea    0x7f694(%rip),%rdx        # 1400816b0 <.rdata+0x6b0>
   14000201c:	mov    $0x20,%r8d
   140002022:	jmp    140002031 <tx_fn_main_0+0x591>
   140002024:	lea    0x7f745(%rip),%rdx        # 140081770 <.rdata+0x770>
   14000202b:	mov    $0x30,%r8d
   140002031:	mov    $0x5,%r9d
   140002037:	jmp    140001f14 <tx_fn_main_0+0x474>
   14000203c:	movabs $0x7fffffffffffffff,%rcx
   140002046:	inc    %rcx
   140002049:	lea    0xc8(%rsp),%r8
   140002051:	mov    $0x1,%edx
   140002056:	call   1400204f0 <txrt_sub_i64>
   14000205b:	mov    %eax,%edi
   14000205d:	lea    0x7f74c(%rip),%rdx        # 1400817b0 <.rdata+0x7b0>
   140002064:	mov    $0x30,%r8d
   14000206a:	mov    $0x5,%r9d
   140002070:	mov    %r14,%rcx
   140002073:	call   14001ab90 <txrt_stack_error_location>
   140002078:	mov    %edi,%ecx
   14000207a:	call   14001f310 <txrt_require_success>
   14000207f:	lea    0x7f76a(%rip),%rdx        # 1400817f0 <.rdata+0x7f0>
   140002086:	mov    $0x30,%r8d
   14000208c:	jmp    1400020d7 <tx_fn_main_0+0x637>
   14000208e:	lea    0x7f82b(%rip),%rdx        # 1400818c0 <.rdata+0x8c0>
   140002095:	jmp    1400020a7 <tx_fn_main_0+0x607>
   140002097:	lea    0x7f862(%rip),%rdx        # 140081900 <.rdata+0x900>
   14000209e:	jmp    1400020a7 <tx_fn_main_0+0x607>
   1400020a0:	lea    0x7f899(%rip),%rdx        # 140081940 <.rdata+0x940>
   1400020a7:	mov    $0x36,%r8d
   1400020ad:	jmp    1400020d7 <tx_fn_main_0+0x637>
   1400020af:	lea    0x7f8ca(%rip),%rdx        # 140081980 <.rdata+0x980>
   1400020b6:	jmp    1400020d1 <tx_fn_main_0+0x631>
   1400020b8:	lea    0x7f911(%rip),%rdx        # 1400819d0 <.rdata+0x9d0>
   1400020bf:	jmp    1400020d1 <tx_fn_main_0+0x631>
   1400020c1:	lea    0x7f948(%rip),%rdx        # 140081a10 <.rdata+0xa10>
   1400020c8:	jmp    1400020d1 <tx_fn_main_0+0x631>
   1400020ca:	lea    0x7f97f(%rip),%rdx        # 140081a50 <.rdata+0xa50>
   1400020d1:	mov    $0x37,%r8d
   1400020d7:	mov    $0x5,%r9d
   1400020dd:	mov    %r14,%rcx
   1400020e0:	mov    %eax,%esi
   1400020e2:	call   14001ab90 <txrt_stack_error_location>
   1400020e7:	mov    %esi,%ecx
   1400020e9:	call   14001f310 <txrt_require_success>
   1400020ee:	int3
   1400020ef:	nop
