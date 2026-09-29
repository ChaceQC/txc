
E:\Project\other\Compilation\tx_build\performance_hotspots\before\stage.exe:     file format pei-x86-64


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
   140001aac:	sub    $0xe8,%rsp
   140001ab3:	mov    %rcx,%r14
   140001ab6:	mov    (%rcx),%rsi
   140001ab9:	lea    0x8004f(%rip),%rax        # 140081b0f <.rdata+0xb0f>
   140001ac0:	mov    %rax,0xb0(%rsp)
   140001ac8:	lea    0x80051(%rip),%rax        # 140081b20 <.rdata+0xb20>
   140001acf:	mov    %rax,0xb8(%rsp)
   140001ad7:	movq   $0x18,0xc0(%rsp)
   140001ae3:	movq   $0x1,0xc8(%rsp)
   140001aef:	mov    %rsi,0xd0(%rsp)
   140001af7:	lea    0xb0(%rsp),%rax
   140001aff:	mov    %rax,(%rcx)
   140001b02:	lea    0xa8(%rsp),%rdx
   140001b0a:	xor    %ecx,%ecx
   140001b0c:	call   140020a70 <txrt_input>
   140001b11:	test   %eax,%eax
   140001b13:	jne    140001fb6 <tx_fn_main_0+0x516>
   140001b19:	mov    0xa8(%rsp),%rdi
   140001b21:	lea    0xa0(%rsp),%rdx
   140001b29:	mov    %rdi,%rcx
   140001b2c:	call   140020250 <txrt_parse_int>
   140001b31:	test   %eax,%eax
   140001b33:	jne    140001fbf <tx_fn_main_0+0x51f>
   140001b39:	mov    %rdi,%rcx
   140001b3c:	call   14001fd10 <txrt_str_release>
   140001b41:	mov    0xa0(%rsp),%rbx
   140001b49:	mov    %r14,%rcx
   140001b4c:	call   1400158c0 <txrt_gc_safepoint_context>
   140001b51:	test   %eax,%eax
   140001b53:	jne    140001fc8 <tx_fn_main_0+0x528>
   140001b59:	lea    0x7f950(%rip),%rax        # 1400814b0 <.rdata+0x4b0>
   140001b60:	mov    %rax,0xb8(%rsp)
   140001b68:	movq   $0x1b,0xc0(%rsp)
   140001b74:	movq   $0x5,0xc8(%rsp)
   140001b80:	mov    %r14,%rcx
   140001b83:	mov    %rbx,%rdx
   140001b86:	call   1400015e0 <tx_fn_m0_read_earliest_0>
   140001b8b:	mov    %rax,%rdi
   140001b8e:	mov    %rax,%rcx
   140001b91:	call   14000b600 <txrt_array_ref>
   140001b96:	mov    %rax,0x60(%rsp)
   140001b9b:	mov    %r14,%rcx
   140001b9e:	call   1400158c0 <txrt_gc_safepoint_context>
   140001ba3:	test   %eax,%eax
   140001ba5:	jne    140001fda <tx_fn_main_0+0x53a>
   140001bab:	lea    0x98(%rsp),%rcx
   140001bb3:	call   140008bc0 <txrt_time_monotonic_micros>
   140001bb8:	test   %eax,%eax
   140001bba:	jne    140001fec <tx_fn_main_0+0x54c>
   140001bc0:	mov    0x98(%rsp),%r12
   140001bc8:	mov    %r14,%rcx
   140001bcb:	call   1400158c0 <txrt_gc_safepoint_context>
   140001bd0:	test   %eax,%eax
   140001bd2:	jne    140001ffe <tx_fn_main_0+0x55e>
   140001bd8:	mov    %rbx,%r15
   140001bdb:	inc    %r15
   140001bde:	jo     140002010 <tx_fn_main_0+0x570>
   140001be4:	lea    0x90(%rsp),%rdx
   140001bec:	mov    %r15,%rcx
   140001bef:	call   14000ac80 <txrt_local_scalar_array_new>
   140001bf4:	test   %eax,%eax
   140001bf6:	jne    140002036 <tx_fn_main_0+0x596>
   140001bfc:	mov    0x90(%rsp),%rax
   140001c04:	mov    %rax,0x30(%rsp)
   140001c09:	mov    %r14,%rcx
   140001c0c:	call   1400158c0 <txrt_gc_safepoint_context>
   140001c11:	test   %eax,%eax
   140001c13:	jne    140002048 <tx_fn_main_0+0x5a8>
   140001c19:	mov    %r12,0x38(%rsp)
   140001c1e:	lea    0x88(%rsp),%rdx
   140001c26:	mov    %rbx,%rcx
   140001c29:	call   14000d360 <txrt_array_new>
   140001c2e:	test   %eax,%eax
   140001c30:	jne    14000205a <tx_fn_main_0+0x5ba>
   140001c36:	mov    %rdi,0x48(%rsp)
   140001c3b:	mov    %rsi,0x50(%rsp)
   140001c40:	mov    0x88(%rsp),%rcx
   140001c48:	mov    %rcx,0x40(%rsp)
   140001c4d:	call   14000b600 <txrt_array_ref>
   140001c52:	mov    %rax,0x28(%rsp)
   140001c57:	mov    %r14,%rcx
   140001c5a:	call   1400158c0 <txrt_gc_safepoint_context>
   140001c5f:	test   %eax,%eax
   140001c61:	jne    14000206c <tx_fn_main_0+0x5cc>
   140001c67:	test   %rbx,%rbx
   140001c6a:	mov    %r14,%rbp
   140001c6d:	jle    140001d49 <tx_fn_main_0+0x2a9>
   140001c73:	mov    %rbx,%rax
   140001c76:	shl    $0x4,%rax
   140001c7a:	add    0x30(%rsp),%rax
   140001c7f:	mov    %rax,0x58(%rsp)
   140001c84:	mov    $0x1,%r12d
   140001c8a:	nopw   0x0(%rax,%rax,1)
   140001c90:	mov    0x58(%rsp),%rsi
   140001c95:	mov    %rbx,%rdi
   140001c98:	nopl   0x0(%rax,%rax,1)
   140001ca0:	cmp    %r15,%rbx
   140001ca3:	jge    140001f34 <tx_fn_main_0+0x494>
   140001ca9:	cmpb   $0x1,(%rsi)
   140001cac:	jne    140001cc0 <tx_fn_main_0+0x220>
   140001cae:	mov    0x60(%rsp),%rcx
   140001cb3:	mov    %rdi,%rdx
   140001cb6:	call   14000b810 <txrt_array_ref_get_i64>
   140001cbb:	cmp    %r12,%rax
   140001cbe:	jle    140001cd0 <tx_fn_main_0+0x230>
   140001cc0:	xor    %r13d,%r13d
   140001cc3:	jmp    140001cee <tx_fn_main_0+0x24e>
   140001cc5:	data16 cs nopw 0x0(%rax,%rax,1)
   140001cd0:	movb   $0x4,(%rsi)
   140001cd3:	movq   $0x1,0x8(%rsi)
   140001cdb:	mov    %r14,%rcx
   140001cde:	call   1400158c0 <txrt_gc_safepoint_context>
   140001ce3:	mov    %rdi,%r13
   140001ce6:	test   %eax,%eax
   140001ce8:	jne    140001f5d <tx_fn_main_0+0x4bd>
   140001cee:	mov    %rbp,%r14
   140001cf1:	mov    %rbp,%rcx
   140001cf4:	call   1400158c0 <txrt_gc_safepoint_context>
   140001cf9:	test   %eax,%eax
   140001cfb:	jne    140001f39 <tx_fn_main_0+0x499>
   140001d01:	cmp    $0x2,%rdi
   140001d05:	jl     140001d13 <tx_fn_main_0+0x273>
   140001d07:	dec    %rdi
   140001d0a:	add    $0xfffffffffffffff0,%rsi
   140001d0e:	test   %r13,%r13
   140001d11:	je     140001ca0 <tx_fn_main_0+0x200>
   140001d13:	lea    -0x1(%r12),%rdx
   140001d18:	mov    0x28(%rsp),%rcx
   140001d1d:	mov    %r13,%r8
   140001d20:	call   14000c340 <txrt_array_ref_set_i64>
   140001d25:	test   %eax,%eax
   140001d27:	jne    140001f83 <tx_fn_main_0+0x4e3>
   140001d2d:	mov    %r14,%rcx
   140001d30:	call   1400158c0 <txrt_gc_safepoint_context>
   140001d35:	test   %eax,%eax
   140001d37:	jne    140001f8c <tx_fn_main_0+0x4ec>
   140001d3d:	inc    %r12
   140001d40:	cmp    %r15,%r12
   140001d43:	jne    140001c90 <tx_fn_main_0+0x1f0>
   140001d49:	lea    0x80(%rsp),%rcx
   140001d51:	call   140008bc0 <txrt_time_monotonic_micros>
   140001d56:	test   %eax,%eax
   140001d58:	jne    14000207e <tx_fn_main_0+0x5de>
   140001d5e:	mov    0x80(%rsp),%rcx
   140001d66:	mov    %rcx,%r15
   140001d69:	mov    0x38(%rsp),%rdx
   140001d6e:	sub    %rdx,%r15
   140001d71:	jo     140002090 <tx_fn_main_0+0x5f0>
   140001d77:	mov    %r14,%rcx
   140001d7a:	call   1400158c0 <txrt_gc_safepoint_context>
   140001d7f:	test   %eax,%eax
   140001d81:	jne    1400020c1 <tx_fn_main_0+0x621>
   140001d87:	test   %rbx,%rbx
   140001d8a:	jle    140001e28 <tx_fn_main_0+0x388>
   140001d90:	mov    0x28(%rsp),%rsi
   140001d95:	mov    %rsi,%rcx
   140001d98:	xor    %edx,%edx
   140001d9a:	call   140010020 <txrt_array_ref_get_str>
   140001d9f:	mov    %rax,%r13
   140001da2:	mov    %rsi,%rcx
   140001da5:	xor    %edx,%edx
   140001da7:	mov    %rax,%r8
   140001daa:	call   14000f750 <txrt_array_ref_set_str>
   140001daf:	test   %eax,%eax
   140001db1:	jne    140001e04 <tx_fn_main_0+0x364>
   140001db3:	mov    $0x1,%edi
   140001db8:	mov    0x28(%rsp),%r12
   140001dbd:	nopl   (%rax)
   140001dc0:	mov    %r13,%rcx
   140001dc3:	call   14001fd10 <txrt_str_release>
   140001dc8:	mov    %r14,%rcx
   140001dcb:	call   1400158c0 <txrt_gc_safepoint_context>
   140001dd0:	test   %eax,%eax
   140001dd2:	jne    140001fa1 <tx_fn_main_0+0x501>
   140001dd8:	cmp    %rdi,%rbx
   140001ddb:	je     140001e28 <tx_fn_main_0+0x388>
   140001ddd:	lea    0x1(%rdi),%rsi
   140001de1:	mov    %r12,%rcx
   140001de4:	mov    %rdi,%rdx
   140001de7:	call   140010020 <txrt_array_ref_get_str>
   140001dec:	mov    %rax,%r13
   140001def:	mov    %r12,%rcx
   140001df2:	mov    %rdi,%rdx
   140001df5:	mov    %rax,%r8
   140001df8:	call   14000f750 <txrt_array_ref_set_str>
   140001dfd:	mov    %rsi,%rdi
   140001e00:	test   %eax,%eax
   140001e02:	je     140001dc0 <tx_fn_main_0+0x320>
   140001e04:	mov    %eax,%edi
   140001e06:	lea    0x7faa3(%rip),%rdx        # 1400818b0 <.rdata+0x8b0>
   140001e0d:	mov    $0x34,%r8d
   140001e13:	mov    $0x9,%r9d
   140001e19:	mov    %rbp,%rcx
   140001e1c:	call   14001abd0 <txrt_stack_error_location>
   140001e21:	mov    %edi,%ecx
   140001e23:	call   14001f350 <txrt_require_success>
   140001e28:	lea    0x7fb00(%rip),%rdx        # 14008192f <.rdata+0x92f>
   140001e2f:	lea    0x78(%rsp),%r9
   140001e34:	mov    $0x1,%r8d
   140001e3a:	mov    0x40(%rsp),%r12
   140001e3f:	mov    %r12,%rcx
   140001e42:	call   140011620 <txrt_string_join_literal>
   140001e47:	test   %eax,%eax
   140001e49:	jne    1400020d0 <tx_fn_main_0+0x630>
   140001e4f:	mov    0x78(%rsp),%rdi
   140001e54:	mov    %rdi,%rcx
   140001e57:	mov    $0x1,%dl
   140001e59:	call   14001ff30 <txrt_print_str>
   140001e5e:	test   %eax,%eax
   140001e60:	mov    0x50(%rsp),%rsi
   140001e65:	mov    0x48(%rsp),%rbx
   140001e6a:	jne    1400020df <tx_fn_main_0+0x63f>
   140001e70:	mov    %rdi,%rcx
   140001e73:	call   14001fd10 <txrt_str_release>
   140001e78:	mov    %r14,%rcx
   140001e7b:	call   1400158c0 <txrt_gc_safepoint_context>
   140001e80:	test   %eax,%eax
   140001e82:	jne    1400020ee <tx_fn_main_0+0x64e>
   140001e88:	lea    0x68(%rsp),%rdx
   140001e8d:	mov    %r15,%rcx
   140001e90:	call   140020bd0 <txrt_int_to_str>
   140001e95:	test   %eax,%eax
   140001e97:	jne    1400020fd <tx_fn_main_0+0x65d>
   140001e9d:	mov    0x68(%rsp),%rdi
   140001ea2:	lea    0x70(%rsp),%rax
   140001ea7:	mov    %rax,0x20(%rsp)
   140001eac:	lea    0x7fb8c(%rip),%rdx        # 140081a3f <.rdata+0xa3f>
   140001eb3:	mov    $0x1,%r8d
   140001eb9:	mov    %rdi,%rcx
   140001ebc:	xor    %r9d,%r9d
   140001ebf:	call   1400195d0 <txrt_str_concat_literal>
   140001ec4:	test   %eax,%eax
   140001ec6:	jne    140002106 <tx_fn_main_0+0x666>
   140001ecc:	mov    %rdi,%rcx
   140001ecf:	call   14001fd10 <txrt_str_release>
   140001ed4:	mov    0x70(%rsp),%rdi
   140001ed9:	mov    %rdi,%rcx
   140001edc:	call   140008da0 <txrt_io_write_error>
   140001ee1:	test   %eax,%eax
   140001ee3:	jne    14000210f <tx_fn_main_0+0x66f>
   140001ee9:	mov    %rdi,%rcx
   140001eec:	call   14001fd10 <txrt_str_release>
   140001ef1:	mov    %r14,%rcx
   140001ef4:	call   1400158c0 <txrt_gc_safepoint_context>
   140001ef9:	test   %eax,%eax
   140001efb:	jne    140002118 <tx_fn_main_0+0x678>
   140001f01:	mov    0x30(%rsp),%rcx
   140001f06:	call   14000ae50 <txrt_local_scalar_array_release>
   140001f0b:	mov    %rbx,%rcx
   140001f0e:	call   1400037f0 <txrt_value_release>
   140001f13:	mov    %r12,%rcx
   140001f16:	call   1400037f0 <txrt_value_release>
   140001f1b:	mov    %rsi,(%r14)
   140001f1e:	xor    %eax,%eax
   140001f20:	add    $0xe8,%rsp
   140001f27:	pop    %rbx
   140001f28:	pop    %rbp
   140001f29:	pop    %rdi
   140001f2a:	pop    %rsi
   140001f2b:	pop    %r12
   140001f2d:	pop    %r13
   140001f2f:	pop    %r14
   140001f31:	pop    %r15
   140001f33:	ret
   140001f34:	call   14000b6c0 <txrt_array_index_error>
   140001f39:	lea    0x7f7f0(%rip),%rdx        # 140081730 <.rdata+0x730>
   140001f40:	mov    $0x27,%r8d
   140001f46:	mov    $0xd,%r9d
   140001f4c:	mov    %r14,%rcx
   140001f4f:	mov    %eax,%esi
   140001f51:	call   14001abd0 <txrt_stack_error_location>
   140001f56:	mov    %esi,%ecx
   140001f58:	call   14001f350 <txrt_require_success>
   140001f5d:	mov    %eax,%r14d
   140001f60:	lea    0x7f789(%rip),%rdx        # 1400816f0 <.rdata+0x6f0>
   140001f67:	mov    $0x29,%r8d
   140001f6d:	mov    $0x11,%r9d
   140001f73:	mov    %rbp,%rcx
   140001f76:	call   14001abd0 <txrt_stack_error_location>
   140001f7b:	mov    %r14d,%ecx
   140001f7e:	call   14001f350 <txrt_require_success>
   140001f83:	lea    0x7f7e6(%rip),%rdx        # 140081770 <.rdata+0x770>
   140001f8a:	jmp    140001f93 <tx_fn_main_0+0x4f3>
   140001f8c:	lea    0x7f81d(%rip),%rdx        # 1400817b0 <.rdata+0x7b0>
   140001f93:	mov    $0x2e,%r8d
   140001f99:	mov    $0x9,%r9d
   140001f9f:	jmp    140001f4c <tx_fn_main_0+0x4ac>
   140001fa1:	lea    0x7f948(%rip),%rdx        # 1400818f0 <.rdata+0x8f0>
   140001fa8:	mov    $0x34,%r8d
   140001fae:	mov    $0x9,%r9d
   140001fb4:	jmp    140001f4c <tx_fn_main_0+0x4ac>
   140001fb6:	lea    0x7f433(%rip),%rdx        # 1400813f0 <.rdata+0x3f0>
   140001fbd:	jmp    140001fcf <tx_fn_main_0+0x52f>
   140001fbf:	lea    0x7f46a(%rip),%rdx        # 140081430 <.rdata+0x430>
   140001fc6:	jmp    140001fcf <tx_fn_main_0+0x52f>
   140001fc8:	lea    0x7f4a1(%rip),%rdx        # 140081470 <.rdata+0x470>
   140001fcf:	mov    $0x1a,%r8d
   140001fd5:	jmp    140002125 <tx_fn_main_0+0x685>
   140001fda:	lea    0x7f50f(%rip),%rdx        # 1400814f0 <.rdata+0x4f0>
   140001fe1:	mov    $0x1b,%r8d
   140001fe7:	jmp    140002125 <tx_fn_main_0+0x685>
   140001fec:	lea    0x7f53d(%rip),%rdx        # 140081530 <.rdata+0x530>
   140001ff3:	mov    $0x1e,%r8d
   140001ff9:	jmp    140002125 <tx_fn_main_0+0x685>
   140001ffe:	lea    0x7f56b(%rip),%rdx        # 140081570 <.rdata+0x570>
   140002005:	mov    $0x1e,%r8d
   14000200b:	jmp    140002125 <tx_fn_main_0+0x685>
   140002010:	lea    0xe0(%rsp),%r8
   140002018:	mov    $0x1,%edx
   14000201d:	mov    %rbx,%rcx
   140002020:	call   140020450 <txrt_add_i64>
   140002025:	mov    %eax,%edi
   140002027:	lea    0x7f582(%rip),%rdx        # 1400815b0 <.rdata+0x5b0>
   14000202e:	mov    $0x1f,%r8d
   140002034:	jmp    1400020ac <tx_fn_main_0+0x60c>
   140002036:	lea    0x7f5b3(%rip),%rdx        # 1400815f0 <.rdata+0x5f0>
   14000203d:	mov    $0x1f,%r8d
   140002043:	jmp    140002125 <tx_fn_main_0+0x685>
   140002048:	lea    0x7f5e1(%rip),%rdx        # 140081630 <.rdata+0x630>
   14000204f:	mov    $0x1f,%r8d
   140002055:	jmp    140002125 <tx_fn_main_0+0x685>
   14000205a:	lea    0x7f60f(%rip),%rdx        # 140081670 <.rdata+0x670>
   140002061:	mov    $0x20,%r8d
   140002067:	jmp    140002125 <tx_fn_main_0+0x685>
   14000206c:	lea    0x7f63d(%rip),%rdx        # 1400816b0 <.rdata+0x6b0>
   140002073:	mov    $0x20,%r8d
   140002079:	jmp    140002125 <tx_fn_main_0+0x685>
   14000207e:	lea    0x7f76b(%rip),%rdx        # 1400817f0 <.rdata+0x7f0>
   140002085:	mov    $0x30,%r8d
   14000208b:	jmp    140002125 <tx_fn_main_0+0x685>
   140002090:	lea    0xd8(%rsp),%r8
   140002098:	call   140020530 <txrt_sub_i64>
   14000209d:	mov    %eax,%edi
   14000209f:	lea    0x7f78a(%rip),%rdx        # 140081830 <.rdata+0x830>
   1400020a6:	mov    $0x30,%r8d
   1400020ac:	mov    $0x5,%r9d
   1400020b2:	mov    %r14,%rcx
   1400020b5:	call   14001abd0 <txrt_stack_error_location>
   1400020ba:	mov    %edi,%ecx
   1400020bc:	call   14001f350 <txrt_require_success>
   1400020c1:	lea    0x7f7a8(%rip),%rdx        # 140081870 <.rdata+0x870>
   1400020c8:	mov    $0x30,%r8d
   1400020ce:	jmp    140002125 <tx_fn_main_0+0x685>
   1400020d0:	lea    0x7f869(%rip),%rdx        # 140081940 <.rdata+0x940>
   1400020d7:	mov    $0x36,%r8d
   1400020dd:	jmp    140002125 <tx_fn_main_0+0x685>
   1400020df:	lea    0x7f89a(%rip),%rdx        # 140081980 <.rdata+0x980>
   1400020e6:	mov    $0x36,%r8d
   1400020ec:	jmp    140002125 <tx_fn_main_0+0x685>
   1400020ee:	lea    0x7f8cb(%rip),%rdx        # 1400819c0 <.rdata+0x9c0>
   1400020f5:	mov    $0x36,%r8d
   1400020fb:	jmp    140002125 <tx_fn_main_0+0x685>
   1400020fd:	lea    0x7f8fc(%rip),%rdx        # 140081a00 <.rdata+0xa00>
   140002104:	jmp    14000211f <tx_fn_main_0+0x67f>
   140002106:	lea    0x7f943(%rip),%rdx        # 140081a50 <.rdata+0xa50>
   14000210d:	jmp    14000211f <tx_fn_main_0+0x67f>
   14000210f:	lea    0x7f97a(%rip),%rdx        # 140081a90 <.rdata+0xa90>
   140002116:	jmp    14000211f <tx_fn_main_0+0x67f>
   140002118:	lea    0x7f9b1(%rip),%rdx        # 140081ad0 <.rdata+0xad0>
   14000211f:	mov    $0x37,%r8d
   140002125:	mov    $0x5,%r9d
   14000212b:	jmp    140001f4c <tx_fn_main_0+0x4ac>
