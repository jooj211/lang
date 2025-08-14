#!/usr/bin/env python3
import sys, re, pathlib

USAGE = '''Usage:
  python3 jvm_stackcheck.py <out.j> printAutomata
  python3 jvm_stackcheck.py <out.j> <methodName> [descriptor]

Examples:
  python3 jvm_stackcheck.py out.j printAutomata '(Ljava/util/HashMap;)V'
'''

def extract_method(txt, name, desc=None):
    if desc is None:
        pat = rf'\.method\s+.*\s{name}\('
    else:
        desc_esc = re.escape(desc)
        pat = rf'\.method\s+.*\s{name}\({desc_esc}'
    m = re.search(pat, txt)
    if not m:
        return None
    start = m.start()
    end = txt.find(".end method", start)
    if end == -1:
        end = start + 4000
    return txt[start:end].splitlines()

def parse_sig(sig):
    # returns list of arg kinds ('I' or 'A') and ret kind
    args, ret = sig
    kinds = []
    j = 0
    while j < len(args):
        c = args[j]
        if c == 'L':
            k = args.find(';', j)
            kinds.append('A'); j = k+1
        elif c == '[':
            # array -> ref
            while j < len(args) and args[j] == '[':
                j += 1
            if j < len(args) and args[j] == 'L':
                j = args.find(';', j)+1
            else:
                j += 1
            kinds.append('A')
        elif c in 'ZBCSI':
            kinds.append('I'); j += 1
        elif c in 'FJD':  # treat as ref-width for simplicity
            kinds.append('A'); j += 1
        else:
            j += 1
    retk = 'V' if ret == 'V' else ('I' if ret in 'ZBCSI' else 'A')
    return kinds, retk

def check_stack(lines):
    problems = []
    stack = []
    for i, raw in enumerate(lines, 1):
        line = raw.strip()
        if not line or line.startswith(('.limit', '.var', '.signature', '.end', '.line', ';', '.method')):
            continue
        op = line.split()[0]

        def need(n, kinds):
            if len(stack) < n: return False
            return all(stack[-idx] == k for idx,k in enumerate(kinds,1))

        try:
            if op in ('return',):
                stack = []
            elif op in ('ireturn',):
                if not need(1, ('I',)):
                    problems.append((i, raw, "ireturn needs int on stack"))
                else: stack.pop()
            elif op in ('istore','istore_0','istore_1','istore_2','istore_3'):
                if not need(1, ('I',)): problems.append((i, raw, "istore needs int")); 
                else: stack.pop()
            elif op in ('astore','astore_0','astore_1','astore_2','astore_3'):
                if not need(1, ('A',)): problems.append((i, raw, "astore needs ref")); 
                else: stack.pop()
            elif op in ('iconst_m1','iconst_0','iconst_1','iconst_2','iconst_3','iconst_4','iconst_5','bipush','sipush','iload','iload_0','iload_1','iload_2','iload_3'):
                stack.append('I')
            elif op in ('aload','aload_0','aload_1','aload_2','aload_3','aconst_null'):
                stack.append('A')
            elif op == 'ldc':
                # naive: strings -> A, numbers -> I
                if '"' in raw: stack.append('A')
                else: stack.append('I')
            elif op == 'getstatic':
                stack.append('A')
            elif op == 'getfield':
                # needs objectref, pushes value; approximate as A
                if not need(1, ('A',)): problems.append((i, raw, "getfield needs objectref"))
                else: stack.pop(); stack.append('A')
            elif op == 'putfield':
                if not need(2, ('A','A')): problems.append((i, raw, "putfield needs value + objectref"))
                else: stack.pop(); stack.pop()
            elif op == 'iadd' or op == 'isub':
                if not need(2, ('I','I')): problems.append((i, raw, f"{op} needs 2 ints"))
                else: stack.pop(); stack.pop(); stack.append('I')
            elif op.startswith('if_icmp'):
                if not need(2, ('I','I')): problems.append((i, raw, f"{op} needs 2 ints"))
                else: stack.pop(); stack.pop()
            elif op.startswith('if_acmp'):
                if not need(2, ('A','A')): problems.append((i, raw, f"{op} needs 2 refs"))
                else: stack.pop(); stack.pop()
            elif op in ('ifne','ifeq','iflt','ifge','ifgt','ifle'):
                if not need(1, ('I',)): problems.append((i, raw, f"{op} needs int"))
                else: stack.pop()
            elif op in ('invokestatic','invokevirtual'):
                sigm = re.search(r'\((.*?)\)(.)', raw)
                if not sigm:
                    continue
                args, ret = parse_sig((sigm.group(1), sigm.group(2)))
                # pop args
                ok = True
                for t in reversed(args):
                    if not need(1, (t,)): problems.append((i, raw, f"{op} arg expects {t}")); ok=False; break
                    stack.pop()
                if not ok: continue
                # pop object for invokevirtual
                if op == 'invokevirtual':
                    if not need(1, ('A',)): problems.append((i, raw, "invokevirtual needs objectref")); 
                    else: stack.pop()
                # push ret
                if ret == 'I': stack.append('I')
                elif ret == 'A': stack.append('A')
            elif op == 'iaload':
                if not need(2, ('I','A')): problems.append((i, raw, "iaload needs arrayref+int (top:int)"))
                else: stack.pop(); stack.pop(); stack.append('I')
            elif op == 'aaload':
                if not need(2, ('I','A')): problems.append((i, raw, "aaload needs arrayref+int (top:int)"))
                else: stack.pop(); stack.pop(); stack.append('A')
            elif op == 'checkcast':
                if not need(1, ('A',)): problems.append((i, raw, "checkcast needs ref"))
                else: pass
            elif op == 'dup':
                if not stack: problems.append((i, raw, "dup needs value"))
                else: stack.append(stack[-1])
            elif op == 'pop':
                if not stack: problems.append((i, raw, "pop needs value"))
                else: stack.pop()
            elif op == 'iastore':
                if not need(3, ('I','I','A')): problems.append((i, raw, "iastore needs arrayref,int,int (top:int)"))
                else: stack.pop(); stack.pop(); stack.pop()
            elif op == 'aastore':
                if not need(3, ('A','I','A')): problems.append((i, raw, "aastore needs arrayref,int,ref (top:ref)"))
                else: stack.pop(); stack.pop(); stack.pop()
            # labels/goto/etc. ignored for this simple pass
        except Exception as e:
            problems.append((i, raw, f"exception in checker: {e}"))
    return problems

def main():
    if len(sys.argv) < 3:
        print(USAGE); sys.exit(1)
    path = pathlib.Path(sys.argv[1])
    name = sys.argv[2]
    desc = sys.argv[3] if len(sys.argv) > 3 else '(Ljava/util/HashMap;)V' if name=='printAutomata' else None
    txt = path.read_text(errors="ignore")
    lines = extract_method(txt, name, desc)
    if not lines:
        print("Method not found."); sys.exit(2)
    for i, line in enumerate(lines, 1):
        print(f"{i:04d}: {line}")
    probs = check_stack(lines)
    print("\n=== Problems ===")
    if not probs:
        print("(none)")
    else:
        for ln, ins, msg in probs:
            print(f"{ln:04d}: {msg} :: {ins}")

if __name__ == '__main__':
    main()
