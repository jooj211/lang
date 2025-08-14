.class public Program
.super java/lang/Object

.method public <init>()V
  aload_0
  invokespecial java/lang/Object/<init>()V
  return
.end method

.method public static printBoard([[Ljava/lang/Object;II)V
  .limit stack 64
  .limit locals 64
  iconst_0
  istore 3
  iconst_0
  istore 4
L1:
  iload 4
  iload 1
  if_icmpge L2
  iconst_0
  istore 5
  iconst_0
  istore 6
L3:
  iload 6
  iload 2
  if_icmpge L4
  aload 0
  iload 3
  aaload
  iload 5
  aaload
  invokestatic java/lang/String/valueOf(I)Ljava/lang/String;
  getstatic java/lang/System/out Ljava/io/PrintStream;
  swap
  invokevirtual java/io/PrintStream/print(Ljava/lang/String;)V
  iload 5
  iconst_1
  iadd
  istore 5
  iinc 6 1
  goto L3
L4:
  bipush 10
  invokestatic java/lang/String/valueOf(C)Ljava/lang/String;
  getstatic java/lang/System/out Ljava/io/PrintStream;
  swap
  invokevirtual java/io/PrintStream/print(Ljava/lang/String;)V
  iload 3
  iconst_1
  iadd
  istore 3
  iinc 4 1
  goto L1
L2:
  return
.end method

.method public static startBoard(CII)[[Ljava/lang/Object;
  .limit stack 64
  .limit locals 64
  iload 1
  anewarray java/lang/Object
  astore 3
  iconst_0
  istore 4
  iconst_0
  istore 5
L5:
  iload 5
  iload 1
  if_icmpge L6
  iconst_0
  istore 6
  aload 3
  iload 4
  iload 2
  anewarray java/lang/Object
  iastore
  iconst_0
  istore 7
L7:
  iload 7
  iload 2
  if_icmpge L8
  aload 3
  iload 4
  iaload
  iload 6
  iload 0
  iastore
  iload 6
  iconst_1
  iadd
  istore 6
  iinc 7 1
  goto L7
L8:
  iload 4
  iconst_1
  iadd
  istore 4
  iinc 5 1
  goto L5
L6:
  aload 3
  areturn
  aconst_null
  areturn
.end method

.method public static set([[Ljava/lang/Object;II)V
  .limit stack 64
  .limit locals 64
  aload 0
  iload 1
  aaload
  iload 2
  bipush 65
  iastore
  return
.end method

.method public static main()V
  .limit stack 64
  .limit locals 64
  bipush 42
  iconst_3
  iconst_4
  invokestatic Program/startBoard(CII)[[Ljava/lang/Object;
  astore 0
  aload 0
  iconst_3
  iconst_4
  invokestatic Program/printBoard([[Ljava/lang/Object;II)V
  aload 0
  iconst_1
  iconst_2
  invokestatic Program/set([[Ljava/lang/Object;II)V
  aload 0
  iconst_3
  iconst_4
  invokestatic Program/printBoard([[Ljava/lang/Object;II)V
  return
.end method

.method public static main([Ljava/lang/String;)V
  .limit stack 4
  .limit locals 2
  invokestatic Program/main()V
  return
.end method

