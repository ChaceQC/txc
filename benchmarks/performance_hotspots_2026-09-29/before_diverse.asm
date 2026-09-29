
E:\Project\other\Compilation\tx_build\performance_hotspots\before\diverse.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140001830 <tx_fn_m0_bench_vector_scale_0>:
   140001830:	push   %r15
   140001832:	push   %r14
   140001834:	push   %r13
   140001836:	push   %r12
   140001838:	push   %rsi
   140001839:	push   %rdi
   14000183a:	push   %rbp
   14000183b:	push   %rbx
   14000183c:	sub    $0x88,%rsp
   140001843:	mov    %rcx,%rsi
   140001846:	mov    (%rcx),%rbp
   140001849:	lea    0xd7f60(%rip),%rax        # 1400d97b0 <.rdata+0x7b0>
   140001850:	mov    %rax,0x20(%rsp)
   140001855:	lea    0xd7f74(%rip),%rax        # 1400d97d0 <.rdata+0x7d0>
   14000185c:	mov    %rax,0x28(%rsp)
   140001861:	movq   $0x15,0x30(%rsp)
   14000186a:	movq   $0x1,0x38(%rsp)
   140001873:	mov    %rbp,0x40(%rsp)
   140001878:	lea    0x20(%rsp),%rax
   14000187d:	mov    %rax,(%rcx)
   140001880:	lea    0x70(%rsp),%r8
   140001885:	mov    $0x3e8,%ecx
   14000188a:	mov    $0x3,%edx
   14000188f:	call   140049b10 <txrt_vector_new_i64>
   140001894:	test   %eax,%eax
   140001896:	jne    140001b74 <tx_fn_m0_bench_vector_scale_0+0x344>
   14000189c:	mov    0x70(%rsp),%rdi
   1400018a1:	mov    %rdi,%rcx
   1400018a4:	call   140049580 <txrt_vector_ref_i64>
   1400018a9:	mov    %rax,%r12
   1400018ac:	mov    %rsi,%rcx
   1400018af:	call   14003b020 <txrt_gc_safepoint_context>
   1400018b4:	test   %eax,%eax
   1400018b6:	jne    140001b7d <tx_fn_m0_bench_vector_scale_0+0x34d>
   1400018bc:	lea    0x68(%rsp),%r8
   1400018c1:	mov    $0x186a0,%ecx
   1400018c6:	mov    $0x3,%edx
   1400018cb:	call   140049b10 <txrt_vector_new_i64>
   1400018d0:	test   %eax,%eax
   1400018d2:	jne    140001b8f <tx_fn_m0_bench_vector_scale_0+0x35f>
   1400018d8:	mov    0x68(%rsp),%rbx
   1400018dd:	mov    %rbx,%rcx
   1400018e0:	call   140049580 <txrt_vector_ref_i64>
   1400018e5:	mov    %rax,%r14
   1400018e8:	mov    %rsi,%rcx
   1400018eb:	call   14003b020 <txrt_gc_safepoint_context>
   1400018f0:	test   %eax,%eax
   1400018f2:	jne    140001b9e <tx_fn_m0_bench_vector_scale_0+0x36e>
   1400018f8:	lea    0x60(%rsp),%rcx
   1400018fd:	call   140030c60 <txrt_time_monotonic_micros>
   140001902:	test   %eax,%eax
   140001904:	jne    140001bad <tx_fn_m0_bench_vector_scale_0+0x37d>
   14000190a:	mov    0x60(%rsp),%r15
   14000190f:	mov    %rsi,%rcx
   140001912:	call   14003b020 <txrt_gc_safepoint_context>
   140001917:	test   %eax,%eax
   140001919:	jne    140001bbc <tx_fn_m0_bench_vector_scale_0+0x38c>
   14000191f:	mov    0x8(%r12),%rax
   140001924:	test   %rax,%rax
   140001927:	jle    140001997 <tx_fn_m0_bench_vector_scale_0+0x167>
   140001929:	mov    (%r12),%r8
   14000192d:	mov    $0x1,%r9d
   140001933:	xor    %r12d,%r12d
   140001936:	cs nopw 0x0(%rax,%rax,1)
   140001940:	xor    %r10d,%r10d
   140001943:	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001950:	mov    %r12,%rcx
   140001953:	mov    (%r8,%r10,8),%rdx
   140001957:	add    %rdx,%r12
   14000195a:	jo     140001b28 <tx_fn_m0_bench_vector_scale_0+0x2f8>
   140001960:	inc    %r10
   140001963:	cmp    %r10,%rax
   140001966:	jne    140001950 <tx_fn_m0_bench_vector_scale_0+0x120>
   140001968:	xor    %r10d,%r10d
   14000196b:	nopl   0x0(%rax,%rax,1)
   140001970:	mov    %r12,%rcx
   140001973:	mov    (%r8,%r10,8),%rdx
   140001977:	add    %rdx,%r12
   14000197a:	jo     140001b28 <tx_fn_m0_bench_vector_scale_0+0x2f8>
   140001980:	inc    %r10
   140001983:	cmp    %r10,%rax
   140001986:	jne    140001970 <tx_fn_m0_bench_vector_scale_0+0x140>
   140001988:	add    $0x2,%r9
   14000198c:	cmp    $0x3e9,%r9
   140001993:	jne    140001940 <tx_fn_m0_bench_vector_scale_0+0x110>
   140001995:	jmp    14000199a <tx_fn_m0_bench_vector_scale_0+0x16a>
   140001997:	xor    %r12d,%r12d
   14000199a:	lea    0xd7b9e(%rip),%rcx        # 1400d953f <.rdata+0x53f>
   1400019a1:	lea    0x58(%rsp),%r8
   1400019a6:	mov    $0xe,%edx
   1400019ab:	call   140054510 <txrt_str_new>
   1400019b0:	test   %eax,%eax
   1400019b2:	jne    140001bcb <tx_fn_m0_bench_vector_scale_0+0x39b>
   1400019b8:	mov    0x58(%rsp),%r13
   1400019bd:	lea    0xd7bcc(%rip),%rax        # 1400d9590 <.rdata+0x590>
   1400019c4:	mov    %rax,0x28(%rsp)
   1400019c9:	movq   $0x22,0x30(%rsp)
   1400019d2:	movq   $0x5,0x38(%rsp)
   1400019db:	mov    %rsi,%rcx
   1400019de:	mov    %r13,%rdx
   1400019e1:	mov    %r15,%r8
   1400019e4:	mov    %r12,%r9
   1400019e7:	call   1400015e0 <tx_fn_m0_report_0>
   1400019ec:	mov    %r13,%rcx
   1400019ef:	call   1400548c0 <txrt_str_release>
   1400019f4:	mov    %rsi,%rcx
   1400019f7:	call   14003b020 <txrt_gc_safepoint_context>
   1400019fc:	test   %eax,%eax
   1400019fe:	jne    140001bda <tx_fn_m0_bench_vector_scale_0+0x3aa>
   140001a04:	lea    0x50(%rsp),%rcx
   140001a09:	call   140030c60 <txrt_time_monotonic_micros>
   140001a0e:	test   %eax,%eax
   140001a10:	jne    140001be9 <tx_fn_m0_bench_vector_scale_0+0x3b9>
   140001a16:	mov    0x50(%rsp),%r15
   140001a1b:	mov    %rsi,%rcx
   140001a1e:	call   14003b020 <txrt_gc_safepoint_context>
   140001a23:	test   %eax,%eax
   140001a25:	jne    140001bf8 <tx_fn_m0_bench_vector_scale_0+0x3c8>
   140001a2b:	mov    0x8(%r14),%rax
   140001a2f:	test   %rax,%rax
   140001a32:	jle    140001a94 <tx_fn_m0_bench_vector_scale_0+0x264>
   140001a34:	mov    (%r14),%r8
   140001a37:	mov    $0x1,%r9d
   140001a3d:	xor    %r14d,%r14d
   140001a40:	xor    %r10d,%r10d
   140001a43:	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
   140001a50:	mov    %r14,%rcx
   140001a53:	mov    (%r8,%r10,8),%rdx
   140001a57:	add    %rdx,%r14
   140001a5a:	jo     140001b46 <tx_fn_m0_bench_vector_scale_0+0x316>
   140001a60:	inc    %r10
   140001a63:	cmp    %r10,%rax
   140001a66:	jne    140001a50 <tx_fn_m0_bench_vector_scale_0+0x220>
   140001a68:	xor    %r10d,%r10d
   140001a6b:	nopl   0x0(%rax,%rax,1)
   140001a70:	mov    %r14,%rcx
   140001a73:	mov    (%r8,%r10,8),%rdx
   140001a77:	add    %rdx,%r14
   140001a7a:	jo     140001b46 <tx_fn_m0_bench_vector_scale_0+0x316>
   140001a80:	inc    %r10
   140001a83:	cmp    %r10,%rax
   140001a86:	jne    140001a70 <tx_fn_m0_bench_vector_scale_0+0x240>
   140001a88:	add    $0x2,%r9
   140001a8c:	cmp    $0x65,%r9
   140001a90:	jne    140001a40 <tx_fn_m0_bench_vector_scale_0+0x210>
   140001a92:	jmp    140001a97 <tx_fn_m0_bench_vector_scale_0+0x267>
   140001a94:	xor    %r14d,%r14d
   140001a97:	lea    0xd7c32(%rip),%rcx        # 1400d96d0 <.rdata+0x6d0>
   140001a9e:	lea    0x48(%rsp),%r8
   140001aa3:	mov    $0x10,%edx
   140001aa8:	call   140054510 <txrt_str_new>
   140001aad:	test   %eax,%eax
   140001aaf:	jne    140001c07 <tx_fn_m0_bench_vector_scale_0+0x3d7>
   140001ab5:	mov    0x48(%rsp),%r12
   140001aba:	lea    0xd7c6f(%rip),%rax        # 1400d9730 <.rdata+0x730>
   140001ac1:	mov    %rax,0x28(%rsp)
   140001ac6:	movq   $0x2d,0x30(%rsp)
   140001acf:	movq   $0x5,0x38(%rsp)
   140001ad8:	mov    %rsi,%rcx
   140001adb:	mov    %r12,%rdx
   140001ade:	mov    %r15,%r8
   140001ae1:	mov    %r14,%r9
   140001ae4:	call   1400015e0 <tx_fn_m0_report_0>
   140001ae9:	mov    %r12,%rcx
   140001aec:	call   1400548c0 <txrt_str_release>
   140001af1:	mov    %rsi,%rcx
   140001af4:	call   14003b020 <txrt_gc_safepoint_context>
   140001af9:	test   %eax,%eax
   140001afb:	jne    140001c10 <tx_fn_m0_bench_vector_scale_0+0x3e0>
   140001b01:	mov    %rbx,%rcx
   140001b04:	call   14002b890 <txrt_value_release>
   140001b09:	mov    %rdi,%rcx
   140001b0c:	call   14002b890 <txrt_value_release>
   140001b11:	mov    %rbp,(%rsi)
   140001b14:	add    $0x88,%rsp
   140001b1b:	pop    %rbx
   140001b1c:	pop    %rbp
   140001b1d:	pop    %rdi
   140001b1e:	pop    %rsi
   140001b1f:	pop    %r12
   140001b21:	pop    %r13
   140001b23:	pop    %r14
   140001b25:	pop    %r15
   140001b27:	ret
   140001b28:	lea    0x80(%rsp),%r8
   140001b30:	call   140055000 <txrt_add_i64>
   140001b35:	mov    %eax,%edi
   140001b37:	lea    0xd79c2(%rip),%rdx        # 1400d9500 <.rdata+0x500>
   140001b3e:	mov    $0x1f,%r8d
   140001b44:	jmp    140001b5f <tx_fn_m0_bench_vector_scale_0+0x32f>
   140001b46:	lea    0x78(%rsp),%r8
   140001b4b:	call   140055000 <txrt_add_i64>
   140001b50:	mov    %eax,%edi
   140001b52:	lea    0xd7b37(%rip),%rdx        # 1400d9690 <.rdata+0x690>
   140001b59:	mov    $0x2a,%r8d
   140001b5f:	mov    $0xd,%r9d
   140001b65:	mov    %rsi,%rcx
   140001b68:	call   14004f780 <txrt_stack_error_location>
   140001b6d:	mov    %edi,%ecx
   140001b6f:	call   140053f00 <txrt_require_success>
   140001b74:	lea    0xd7805(%rip),%rdx        # 1400d9380 <.rdata+0x380>
   140001b7b:	jmp    140001b84 <tx_fn_m0_bench_vector_scale_0+0x354>
   140001b7d:	lea    0xd783c(%rip),%rdx        # 1400d93c0 <.rdata+0x3c0>
   140001b84:	mov    $0x17,%r8d
   140001b8a:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001b8f:	lea    0xd786a(%rip),%rdx        # 1400d9400 <.rdata+0x400>
   140001b96:	mov    $0x18,%r8d
   140001b9c:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001b9e:	lea    0xd789b(%rip),%rdx        # 1400d9440 <.rdata+0x440>
   140001ba5:	mov    $0x18,%r8d
   140001bab:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001bad:	lea    0xd78cc(%rip),%rdx        # 1400d9480 <.rdata+0x480>
   140001bb4:	mov    $0x1a,%r8d
   140001bba:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001bbc:	lea    0xd78fd(%rip),%rdx        # 1400d94c0 <.rdata+0x4c0>
   140001bc3:	mov    $0x1a,%r8d
   140001bc9:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001bcb:	lea    0xd797e(%rip),%rdx        # 1400d9550 <.rdata+0x550>
   140001bd2:	mov    $0x22,%r8d
   140001bd8:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001bda:	lea    0xd79ef(%rip),%rdx        # 1400d95d0 <.rdata+0x5d0>
   140001be1:	mov    $0x22,%r8d
   140001be7:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001be9:	lea    0xd7a20(%rip),%rdx        # 1400d9610 <.rdata+0x610>
   140001bf0:	mov    $0x25,%r8d
   140001bf6:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001bf8:	lea    0xd7a51(%rip),%rdx        # 1400d9650 <.rdata+0x650>
   140001bff:	mov    $0x25,%r8d
   140001c05:	jmp    140001c1d <tx_fn_m0_bench_vector_scale_0+0x3ed>
   140001c07:	lea    0xd7ae2(%rip),%rdx        # 1400d96f0 <.rdata+0x6f0>
   140001c0e:	jmp    140001c17 <tx_fn_m0_bench_vector_scale_0+0x3e7>
   140001c10:	lea    0xd7b59(%rip),%rdx        # 1400d9770 <.rdata+0x770>
   140001c17:	mov    $0x2d,%r8d
   140001c1d:	mov    $0x5,%r9d
   140001c23:	mov    %rsi,%rcx
   140001c26:	mov    %eax,%esi
   140001c28:	call   14004f780 <txrt_stack_error_location>
   140001c2d:	mov    %esi,%ecx
   140001c2f:	call   140053f00 <txrt_require_success>
   140001c34:	int3
   140001c35:	data16 cs nopw 0x0(%rax,%rax,1)
