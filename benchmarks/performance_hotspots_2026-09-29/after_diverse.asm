
E:\Project\other\Compilation\tx_build\performance_hotspots\after\diverse.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001840 <tx_fn_m0_bench_vector_scale_0>:
   140001840:	push   %r15
   140001842:	push   %r14
   140001844:	push   %r13
   140001846:	push   %r12
   140001848:	push   %rsi
   140001849:	push   %rdi
   14000184a:	push   %rbp
   14000184b:	push   %rbx
   14000184c:	sub    $0x88,%rsp
   140001853:	mov    %rcx,%rsi
   140001856:	mov    (%rcx),%rbp
   140001859:	lea    0xd7f60(%rip),%rax        # 1400d97c0 <.rdata+0x7c0>
   140001860:	mov    %rax,0x20(%rsp)
   140001865:	lea    0xd7f74(%rip),%rax        # 1400d97e0 <.rdata+0x7e0>
   14000186c:	mov    %rax,0x28(%rsp)
   140001871:	movq   $0x15,0x30(%rsp)
   14000187a:	movq   $0x1,0x38(%rsp)
   140001883:	mov    %rbp,0x40(%rsp)
   140001888:	lea    0x20(%rsp),%rax
   14000188d:	mov    %rax,(%rcx)
   140001890:	lea    0x70(%rsp),%r8
   140001895:	mov    $0x3e8,%ecx
   14000189a:	mov    $0x3,%edx
   14000189f:	call   140049bf0 <txrt_vector_new_i64>
   1400018a4:	test   %eax,%eax
   1400018a6:	jne    140001b9c <tx_fn_m0_bench_vector_scale_0+0x35c>
   1400018ac:	mov    0x70(%rsp),%rdi
   1400018b1:	mov    %rdi,%rcx
   1400018b4:	call   140049660 <txrt_vector_ref_i64>
   1400018b9:	mov    %rax,%r12
   1400018bc:	mov    %rsi,%rcx
   1400018bf:	call   14003b100 <txrt_gc_safepoint_context>
   1400018c4:	test   %eax,%eax
   1400018c6:	jne    140001ba5 <tx_fn_m0_bench_vector_scale_0+0x365>
   1400018cc:	lea    0x68(%rsp),%r8
   1400018d1:	mov    $0x186a0,%ecx
   1400018d6:	mov    $0x3,%edx
   1400018db:	call   140049bf0 <txrt_vector_new_i64>
   1400018e0:	test   %eax,%eax
   1400018e2:	jne    140001bb7 <tx_fn_m0_bench_vector_scale_0+0x377>
   1400018e8:	mov    0x68(%rsp),%rbx
   1400018ed:	mov    %rbx,%rcx
   1400018f0:	call   140049660 <txrt_vector_ref_i64>
   1400018f5:	mov    %rax,%r14
   1400018f8:	mov    %rsi,%rcx
   1400018fb:	call   14003b100 <txrt_gc_safepoint_context>
   140001900:	test   %eax,%eax
   140001902:	jne    140001bc6 <tx_fn_m0_bench_vector_scale_0+0x386>
   140001908:	lea    0x60(%rsp),%rcx
   14000190d:	call   140030d40 <txrt_time_monotonic_micros>
   140001912:	test   %eax,%eax
   140001914:	jne    140001bd5 <tx_fn_m0_bench_vector_scale_0+0x395>
   14000191a:	mov    0x60(%rsp),%r15
   14000191f:	mov    %rsi,%rcx
   140001922:	call   14003b100 <txrt_gc_safepoint_context>
   140001927:	test   %eax,%eax
   140001929:	jne    140001be4 <tx_fn_m0_bench_vector_scale_0+0x3a4>
   14000192f:	mov    0x8(%r12),%rax
   140001934:	test   %rax,%rax
   140001937:	jle    1400019a1 <tx_fn_m0_bench_vector_scale_0+0x161>
   140001939:	mov    (%r12),%rcx
   14000193d:	mov    $0x1,%edx
   140001942:	xor    %r12d,%r12d
   140001945:	data16 cs nopw 0x0(%rax,%rax,1)
   140001950:	xor    %r8d,%r8d
   140001953:	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001960:	add    (%rcx,%r8,8),%r12
   140001964:	jo     140001b32 <tx_fn_m0_bench_vector_scale_0+0x2f2>
   14000196a:	inc    %r8
   14000196d:	cmp    %r8,%rax
   140001970:	jne    140001960 <tx_fn_m0_bench_vector_scale_0+0x120>
   140001972:	xor    %r8d,%r8d
   140001975:	data16 cs nopw 0x0(%rax,%rax,1)
   140001980:	add    (%rcx,%r8,8),%r12
   140001984:	jo     140001b32 <tx_fn_m0_bench_vector_scale_0+0x2f2>
   14000198a:	inc    %r8
   14000198d:	cmp    %r8,%rax
   140001990:	jne    140001980 <tx_fn_m0_bench_vector_scale_0+0x140>
   140001992:	add    $0x2,%rdx
   140001996:	cmp    $0x3e9,%rdx
   14000199d:	jne    140001950 <tx_fn_m0_bench_vector_scale_0+0x110>
   14000199f:	jmp    1400019a4 <tx_fn_m0_bench_vector_scale_0+0x164>
   1400019a1:	xor    %r12d,%r12d
   1400019a4:	lea    0xd7ba4(%rip),%rcx        # 1400d954f <.rdata+0x54f>
   1400019ab:	lea    0x58(%rsp),%r8
   1400019b0:	mov    $0xe,%edx
   1400019b5:	call   1400545f0 <txrt_str_new>
   1400019ba:	test   %eax,%eax
   1400019bc:	jne    140001bf3 <tx_fn_m0_bench_vector_scale_0+0x3b3>
   1400019c2:	mov    0x58(%rsp),%r13
   1400019c7:	lea    0xd7bd2(%rip),%rax        # 1400d95a0 <.rdata+0x5a0>
   1400019ce:	mov    %rax,0x28(%rsp)
   1400019d3:	movq   $0x22,0x30(%rsp)
   1400019dc:	movq   $0x5,0x38(%rsp)
   1400019e5:	mov    %rsi,%rcx
   1400019e8:	mov    %r13,%rdx
   1400019eb:	mov    %r15,%r8
   1400019ee:	mov    %r12,%r9
   1400019f1:	call   1400015e0 <tx_fn_m0_report_0>
   1400019f6:	mov    %r13,%rcx
   1400019f9:	call   1400549a0 <txrt_str_release>
   1400019fe:	mov    %rsi,%rcx
   140001a01:	call   14003b100 <txrt_gc_safepoint_context>
   140001a06:	test   %eax,%eax
   140001a08:	jne    140001c02 <tx_fn_m0_bench_vector_scale_0+0x3c2>
   140001a0e:	lea    0x50(%rsp),%rcx
   140001a13:	call   140030d40 <txrt_time_monotonic_micros>
   140001a18:	test   %eax,%eax
   140001a1a:	jne    140001c11 <tx_fn_m0_bench_vector_scale_0+0x3d1>
   140001a20:	mov    0x50(%rsp),%r15
   140001a25:	mov    %rsi,%rcx
   140001a28:	call   14003b100 <txrt_gc_safepoint_context>
   140001a2d:	test   %eax,%eax
   140001a2f:	jne    140001c20 <tx_fn_m0_bench_vector_scale_0+0x3e0>
   140001a35:	mov    0x8(%r14),%rax
   140001a39:	test   %rax,%rax
   140001a3c:	jle    140001a9e <tx_fn_m0_bench_vector_scale_0+0x25e>
   140001a3e:	mov    (%r14),%rcx
   140001a41:	mov    $0x1,%edx
   140001a46:	xor    %r14d,%r14d
   140001a49:	nopl   0x0(%rax)
   140001a50:	xor    %r8d,%r8d
   140001a53:	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001a60:	add    (%rcx,%r8,8),%r14
   140001a64:	jo     140001b5f <tx_fn_m0_bench_vector_scale_0+0x31f>
   140001a6a:	inc    %r8
   140001a6d:	cmp    %r8,%rax
   140001a70:	jne    140001a60 <tx_fn_m0_bench_vector_scale_0+0x220>
   140001a72:	xor    %r8d,%r8d
   140001a75:	data16 cs nopw 0x0(%rax,%rax,1)
   140001a80:	add    (%rcx,%r8,8),%r14
   140001a84:	jo     140001b5f <tx_fn_m0_bench_vector_scale_0+0x31f>
   140001a8a:	inc    %r8
   140001a8d:	cmp    %r8,%rax
   140001a90:	jne    140001a80 <tx_fn_m0_bench_vector_scale_0+0x240>
   140001a92:	add    $0x2,%rdx
   140001a96:	cmp    $0x65,%rdx
   140001a9a:	jne    140001a50 <tx_fn_m0_bench_vector_scale_0+0x210>
   140001a9c:	jmp    140001aa1 <tx_fn_m0_bench_vector_scale_0+0x261>
   140001a9e:	xor    %r14d,%r14d
   140001aa1:	lea    0xd7c38(%rip),%rcx        # 1400d96e0 <.rdata+0x6e0>
   140001aa8:	lea    0x48(%rsp),%r8
   140001aad:	mov    $0x10,%edx
   140001ab2:	call   1400545f0 <txrt_str_new>
   140001ab7:	test   %eax,%eax
   140001ab9:	jne    140001c2f <tx_fn_m0_bench_vector_scale_0+0x3ef>
   140001abf:	mov    0x48(%rsp),%r12
   140001ac4:	lea    0xd7c75(%rip),%rax        # 1400d9740 <.rdata+0x740>
   140001acb:	mov    %rax,0x28(%rsp)
   140001ad0:	movq   $0x2d,0x30(%rsp)
   140001ad9:	movq   $0x5,0x38(%rsp)
   140001ae2:	mov    %rsi,%rcx
   140001ae5:	mov    %r12,%rdx
   140001ae8:	mov    %r15,%r8
   140001aeb:	mov    %r14,%r9
   140001aee:	call   1400015e0 <tx_fn_m0_report_0>
   140001af3:	mov    %r12,%rcx
   140001af6:	call   1400549a0 <txrt_str_release>
   140001afb:	mov    %rsi,%rcx
   140001afe:	call   14003b100 <txrt_gc_safepoint_context>
   140001b03:	test   %eax,%eax
   140001b05:	jne    140001c38 <tx_fn_m0_bench_vector_scale_0+0x3f8>
   140001b0b:	mov    %rbx,%rcx
   140001b0e:	call   14002b970 <txrt_value_release>
   140001b13:	mov    %rdi,%rcx
   140001b16:	call   14002b970 <txrt_value_release>
   140001b1b:	mov    %rbp,(%rsi)
   140001b1e:	add    $0x88,%rsp
   140001b25:	pop    %rbx
   140001b26:	pop    %rbp
   140001b27:	pop    %rdi
   140001b28:	pop    %rsi
   140001b29:	pop    %r12
   140001b2b:	pop    %r13
   140001b2d:	pop    %r14
   140001b2f:	pop    %r15
   140001b31:	ret
   140001b32:	movabs $0x7fffffffffffffff,%rcx
   140001b3c:	lea    0x80(%rsp),%r8
   140001b44:	mov    $0x1,%edx
   140001b49:	call   1400550e0 <txrt_add_i64>
   140001b4e:	mov    %eax,%edi
   140001b50:	lea    0xd79b9(%rip),%rdx        # 1400d9510 <.rdata+0x510>
   140001b57:	mov    $0x1f,%r8d
   140001b5d:	jmp    140001b87 <tx_fn_m0_bench_vector_scale_0+0x347>
   140001b5f:	movabs $0x7fffffffffffffff,%rcx
   140001b69:	lea    0x78(%rsp),%r8
   140001b6e:	mov    $0x1,%edx
   140001b73:	call   1400550e0 <txrt_add_i64>
   140001b78:	mov    %eax,%edi
   140001b7a:	lea    0xd7b1f(%rip),%rdx        # 1400d96a0 <.rdata+0x6a0>
   140001b81:	mov    $0x2a,%r8d
   140001b87:	mov    $0xd,%r9d
   140001b8d:	mov    %rsi,%rcx
   140001b90:	call   14004f860 <txrt_stack_error_location>
   140001b95:	mov    %edi,%ecx
   140001b97:	call   140053fe0 <txrt_require_success>
   140001b9c:	lea    0xd77ed(%rip),%rdx        # 1400d9390 <.rdata+0x390>
   140001ba3:	jmp    140001bac <tx_fn_m0_bench_vector_scale_0+0x36c>
   140001ba5:	lea    0xd7824(%rip),%rdx        # 1400d93d0 <.rdata+0x3d0>
   140001bac:	mov    $0x17,%r8d
   140001bb2:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001bb7:	lea    0xd7852(%rip),%rdx        # 1400d9410 <.rdata+0x410>
   140001bbe:	mov    $0x18,%r8d
   140001bc4:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001bc6:	lea    0xd7883(%rip),%rdx        # 1400d9450 <.rdata+0x450>
   140001bcd:	mov    $0x18,%r8d
   140001bd3:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001bd5:	lea    0xd78b4(%rip),%rdx        # 1400d9490 <.rdata+0x490>
   140001bdc:	mov    $0x1a,%r8d
   140001be2:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001be4:	lea    0xd78e5(%rip),%rdx        # 1400d94d0 <.rdata+0x4d0>
   140001beb:	mov    $0x1a,%r8d
   140001bf1:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001bf3:	lea    0xd7966(%rip),%rdx        # 1400d9560 <.rdata+0x560>
   140001bfa:	mov    $0x22,%r8d
   140001c00:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001c02:	lea    0xd79d7(%rip),%rdx        # 1400d95e0 <.rdata+0x5e0>
   140001c09:	mov    $0x22,%r8d
   140001c0f:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001c11:	lea    0xd7a08(%rip),%rdx        # 1400d9620 <.rdata+0x620>
   140001c18:	mov    $0x25,%r8d
   140001c1e:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001c20:	lea    0xd7a39(%rip),%rdx        # 1400d9660 <.rdata+0x660>
   140001c27:	mov    $0x25,%r8d
   140001c2d:	jmp    140001c45 <tx_fn_m0_bench_vector_scale_0+0x405>
   140001c2f:	lea    0xd7aca(%rip),%rdx        # 1400d9700 <.rdata+0x700>
   140001c36:	jmp    140001c3f <tx_fn_m0_bench_vector_scale_0+0x3ff>
   140001c38:	lea    0xd7b41(%rip),%rdx        # 1400d9780 <.rdata+0x780>
   140001c3f:	mov    $0x2d,%r8d
   140001c45:	mov    $0x5,%r9d
   140001c4b:	mov    %rsi,%rcx
   140001c4e:	mov    %eax,%esi
   140001c50:	call   14004f860 <txrt_stack_error_location>
   140001c55:	mov    %esi,%ecx
   140001c57:	call   140053fe0 <txrt_require_success>
   140001c5c:	int3
   140001c5d:	nopl   (%rax)
