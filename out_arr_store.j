.class public Program
.super java/lang/Object

.method public <init>()V
  aload_0
  invokespecial java/lang/Object/<init>()V
  return
.end method

.method public static main()V
  .limit stack 16
  .limit locals 64
  aconst_null
  astore 0
  new java/util/HashMap
  dup
  invokespecial java/util/HashMap/<init>()V
  astore 0
  aload 0
  ldc "x"
  bipush 10
  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;
  invokevirtual java/util/HashMap/put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;
  pop
  aload 0
  ldc "y"
  bipush 32
  invokestatic java/lang/Integer/valueOf(I)Ljava/lang/Integer;
  invokevirtual java/util/HashMap/put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;
  pop
  getstatic java/lang/System/out Ljava/io/PrintStream;
  aload 0
  ldc "x"
  invokevirtual java/util/HashMap/get(Ljava/lang/Object;)Ljava/lang/Object;
  checkcast java/lang/Integer
  invokevirtual java/lang/Integer/intValue()I
  invokestatic java/lang/String/valueOf(I)Ljava/lang/String;
  invokevirtual java/io/PrintStream/print(Ljava/lang/String;)V
  getstatic java/lang/System/out Ljava/io/PrintStream;
  aload 0
  ldc "y"
  invokevirtual java/util/HashMap/get(Ljava/lang/Object;)Ljava/lang/Object;
  checkcast java/lang/Integer
  invokevirtual java/lang/Integer/intValue()I
  invokestatic java/lang/String/valueOf(I)Ljava/lang/String;
  invokevirtual java/io/PrintStream/print(Ljava/lang/String;)V
  return
.end method

.method public static main([Ljava/lang/String;)V
  .limit stack 4
  .limit locals 2
  invokestatic Program/main()V
  return
.end method

