target triple = "x86_64-w64-windows-gnu"

declare void @txrt_require_success(i32)

define i32 @main() "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87"
{
entry:
    call void @txrt_require_success(i32 0)
    ret i32 0
}
