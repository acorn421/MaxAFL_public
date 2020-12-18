; ModuleID = '../test-instr.c'
source_filename = "../test-instr.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [6 x i8] c"Hum?\0A\00", align 1
@.str.1 = private unnamed_addr constant [26 x i8] c"Looks like a zero to me!\0A\00", align 1
@.str.2 = private unnamed_addr constant [31 x i8] c"A non-zero value? How quaint!\0A\00", align 1
@__afl_area_ptr = external global i8*
@__afl_prev_loc = external thread_local global i32
@__maxafl_visit_integer_func = external global i32 (i32, i32, i32, i32, i32, i64, i64, i64, i64)*
@__maxafl_visit_float_func = external global i32 (i32, i32, i32, i32, i32, double, double)*
@__maxafl_visit_br_func = external global i32 (i32, i32)*
@__maxafl_visit_etc_func = external global i32 (i32, i32, i32)*
@__maxafl_enter_internal_call_func = external global i32 (i32)*
@__maxafl_exit_internal_call_func = external global i32 (i32)*

; Function Attrs: noreturn nounwind uwtable
define dso_local i32 @main(i32, i8** nocapture readnone) local_unnamed_addr #0 !dbg !7 {
  %3 = load i32, i32* @__afl_prev_loc, !nosanitize !2
  %4 = load i8*, i8** @__afl_area_ptr, !nosanitize !2
  %5 = xor i32 %3, 63051
  %6 = getelementptr i8, i8* %4, i32 %5
  %7 = load i8, i8* %6, !nosanitize !2
  %8 = add i8 %7, 1
  store i8 %8, i8* %6, !nosanitize !2
  store i32 31525, i32* @__afl_prev_loc, !nosanitize !2
  %9 = alloca [8 x i8], align 1
  call void @llvm.dbg.value(metadata i32 %0, metadata !15, metadata !DIExpression()), !dbg !21
  call void @llvm.dbg.value(metadata i8** %1, metadata !16, metadata !DIExpression()), !dbg !22
  %10 = getelementptr inbounds [8 x i8], [8 x i8]* %9, i64 0, i64 0, !dbg !23
  call void @llvm.lifetime.start.p0i8(i64 8, i8* nonnull %10) #6, !dbg !23
  call void @llvm.dbg.declare(metadata [8 x i8]* %9, metadata !17, metadata !DIExpression()), !dbg !24
  %11 = call i64 @read(i32 0, i8* nonnull %10, i64 8) #6, !dbg !25
  %12 = load i32 (i32, i32, i32, i32, i32, i64, i64, i64, i64)*, i32 (i32, i32, i32, i32, i32, i64, i64, i64, i64)** @__maxafl_visit_integer_func, !dbg !27
  %13 = call i32 %12(i32 0, i32 0, i32 40, i32 11, i32 32, i64 %11, i64 1, i64 %11, i64 1), !dbg !27, !nosanitize !2
  %14 = icmp slt i64 %11, 1, !dbg !27, !max_cmpid !28
  %15 = load i32 (i32, i32)*, i32 (i32, i32)** @__maxafl_visit_br_func, !dbg !29
  %16 = call i32 %15(i32 0, i32 0), !dbg !29, !nosanitize !2
  br i1 %14, label %17, label %25, !dbg !29

; <label>:17:                                     ; preds = %2
  %18 = load i32, i32* @__afl_prev_loc, !dbg !30, !nosanitize !2
  %19 = load i8*, i8** @__afl_area_ptr, !dbg !30, !nosanitize !2
  %20 = xor i32 %18, 13656, !dbg !30
  %21 = getelementptr i8, i8* %19, i32 %20, !dbg !30
  %22 = load i8, i8* %21, !dbg !30, !nosanitize !2
  %23 = add i8 %22, 1, !dbg !30
  store i8 %23, i8* %21, !dbg !30, !nosanitize !2
  store i32 6828, i32* @__afl_prev_loc, !dbg !30, !nosanitize !2
  %24 = tail call i32 (i32, i8*, ...) @__printf_chk(i32 1, i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str, i64 0, i64 0)) #6, !dbg !30
  tail call void @exit(i32 1) #7, !dbg !32
  unreachable, !dbg !32

; <label>:25:                                     ; preds = %2
  %26 = load i32, i32* @__afl_prev_loc, !dbg !33, !nosanitize !2
  %27 = load i8*, i8** @__afl_area_ptr, !dbg !33, !nosanitize !2
  %28 = xor i32 %26, 3212, !dbg !33
  %29 = getelementptr i8, i8* %27, i32 %28, !dbg !33
  %30 = load i8, i8* %29, !dbg !33, !nosanitize !2
  %31 = add i8 %30, 1, !dbg !33
  store i8 %31, i8* %29, !dbg !33, !nosanitize !2
  store i32 1606, i32* @__afl_prev_loc, !dbg !33, !nosanitize !2
  %32 = load i8, i8* %10, align 1, !dbg !33, !tbaa !35
  %33 = sext i8 %32 to i64, !dbg !38
  %34 = zext i8 %32 to i64, !dbg !38
  %35 = load i32 (i32, i32, i32, i32, i32, i64, i64, i64, i64)*, i32 (i32, i32, i32, i32, i32, i64, i64, i64, i64)** @__maxafl_visit_integer_func, !dbg !38
  %36 = call i32 %35(i32 0, i32 1, i32 32, i32 11, i32 32, i64 %33, i64 48, i64 %34, i64 48), !dbg !38, !nosanitize !2
  %37 = icmp eq i8 %32, 48, !dbg !38, !max_cmpid !39
  %38 = load i32 (i32, i32)*, i32 (i32, i32)** @__maxafl_visit_br_func, !dbg !40
  %39 = call i32 %38(i32 0, i32 1), !dbg !40, !nosanitize !2
  br i1 %37, label %40, label %48, !dbg !40

; <label>:40:                                     ; preds = %25
  %41 = load i32, i32* @__afl_prev_loc, !dbg !41, !nosanitize !2
  %42 = load i8*, i8** @__afl_area_ptr, !dbg !41, !nosanitize !2
  %43 = xor i32 %41, 53351, !dbg !41
  %44 = getelementptr i8, i8* %42, i32 %43, !dbg !41
  %45 = load i8, i8* %44, !dbg !41, !nosanitize !2
  %46 = add i8 %45, 1, !dbg !41
  store i8 %46, i8* %44, !dbg !41, !nosanitize !2
  store i32 26675, i32* @__afl_prev_loc, !dbg !41, !nosanitize !2
  %47 = tail call i32 (i32, i8*, ...) @__printf_chk(i32 1, i8* getelementptr inbounds ([26 x i8], [26 x i8]* @.str.1, i64 0, i64 0)) #6, !dbg !41
  br label %56, !dbg !41

; <label>:48:                                     ; preds = %25
  %49 = load i32, i32* @__afl_prev_loc, !dbg !42, !nosanitize !2
  %50 = load i8*, i8** @__afl_area_ptr, !dbg !42, !nosanitize !2
  %51 = xor i32 %49, 26665, !dbg !42
  %52 = getelementptr i8, i8* %50, i32 %51, !dbg !42
  %53 = load i8, i8* %52, !dbg !42, !nosanitize !2
  %54 = add i8 %53, 1, !dbg !42
  store i8 %54, i8* %52, !dbg !42, !nosanitize !2
  store i32 13332, i32* @__afl_prev_loc, !dbg !42, !nosanitize !2
  %55 = tail call i32 (i32, i8*, ...) @__printf_chk(i32 1, i8* getelementptr inbounds ([31 x i8], [31 x i8]* @.str.2, i64 0, i64 0)) #6, !dbg !42
  br label %56

; <label>:56:                                     ; preds = %48, %40
  %57 = load i32, i32* @__afl_prev_loc, !dbg !43, !nosanitize !2
  %58 = load i8*, i8** @__afl_area_ptr, !dbg !43, !nosanitize !2
  %59 = xor i32 %57, 2844, !dbg !43
  %60 = getelementptr i8, i8* %58, i32 %59, !dbg !43
  %61 = load i8, i8* %60, !dbg !43, !nosanitize !2
  %62 = add i8 %61, 1, !dbg !43
  store i8 %62, i8* %60, !dbg !43, !nosanitize !2
  store i32 1422, i32* @__afl_prev_loc, !dbg !43, !nosanitize !2
  tail call void @exit(i32 0) #7, !dbg !43
  unreachable, !dbg !43
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #2

declare dso_local i64 @read(i32, i8* nocapture, i64) local_unnamed_addr #3

declare dso_local i32 @__printf_chk(i32, i8*, ...) local_unnamed_addr #3

; Function Attrs: noreturn nounwind
declare dso_local void @exit(i32) local_unnamed_addr #4

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone
declare i32 @__maxafl_visit_cmp_integer(i32, i32, i32, i32, i32, i64, i64, i64, i64) #5

; Function Attrs: nounwind readnone
declare i32 @__maxafl_visit_cmp_float(i32, i32, i32, i32, i32, double, double) #5

; Function Attrs: nounwind readnone
declare i32 @__maxafl_visit_br(i32, i32) #5

; Function Attrs: nounwind readnone
declare i32 @__maxafl_visit_etc(i32, i32, i32) #5

; Function Attrs: nounwind readnone
declare i32 @__maxafl_enter_internal_call(i32) #5

; Function Attrs: nounwind readnone
declare i32 @__maxafl_exit_internal_call(i32) #5

attributes #0 = { noreturn nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { argmemonly nounwind }
attributes #3 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { noreturn nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nounwind readnone }
attributes #6 = { nounwind }
attributes #7 = { noreturn nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 7.0.1 (tags/RELEASE_701/final)", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "../test-instr.c", directory: "/home/user/work/maxAFL/afl/llvm_mode")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"clang version 7.0.1 (tags/RELEASE_701/final)"}
!7 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 29, type: !8, isLocal: false, isDefinition: true, scopeLine: 29, flags: DIFlagPrototyped, isOptimized: true, unit: !0, retainedNodes: !14)
!8 = !DISubroutineType(types: !9)
!9 = !{!10, !10, !11}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!12 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !13, size: 64)
!13 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!14 = !{!15, !16, !17}
!15 = !DILocalVariable(name: "argc", arg: 1, scope: !7, file: !1, line: 29, type: !10)
!16 = !DILocalVariable(name: "argv", arg: 2, scope: !7, file: !1, line: 29, type: !11)
!17 = !DILocalVariable(name: "buf", scope: !7, file: !1, line: 31, type: !18)
!18 = !DICompositeType(tag: DW_TAG_array_type, baseType: !13, size: 64, elements: !19)
!19 = !{!20}
!20 = !DISubrange(count: 8)
!21 = !DILocation(line: 29, column: 14, scope: !7)
!22 = !DILocation(line: 29, column: 27, scope: !7)
!23 = !DILocation(line: 31, column: 3, scope: !7)
!24 = !DILocation(line: 31, column: 8, scope: !7)
!25 = !DILocation(line: 33, column: 7, scope: !26)
!26 = distinct !DILexicalBlock(scope: !7, file: !1, line: 33, column: 7)
!27 = !DILocation(line: 33, column: 23, scope: !26)
!28 = !{i32 0}
!29 = !DILocation(line: 33, column: 7, scope: !7)
!30 = !DILocation(line: 34, column: 5, scope: !31)
!31 = distinct !DILexicalBlock(scope: !26, file: !1, line: 33, column: 28)
!32 = !DILocation(line: 35, column: 5, scope: !31)
!33 = !DILocation(line: 38, column: 7, scope: !34)
!34 = distinct !DILexicalBlock(scope: !7, file: !1, line: 38, column: 7)
!35 = !{!36, !36, i64 0}
!36 = !{!"omnipotent char", !37, i64 0}
!37 = !{!"Simple C/C++ TBAA"}
!38 = !DILocation(line: 38, column: 14, scope: !34)
!39 = !{i32 1}
!40 = !DILocation(line: 38, column: 7, scope: !7)
!41 = !DILocation(line: 39, column: 5, scope: !34)
!42 = !DILocation(line: 41, column: 5, scope: !34)
!43 = !DILocation(line: 43, column: 3, scope: !7)
