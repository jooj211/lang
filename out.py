import sys

# input token buffer for typed reads
_tok = []
def _next():
    global _tok
    while not _tok:
        line = sys.stdin.readline()
        if not line: return ''
        _tok = line.strip().split()
    return _tok.pop(0)

def _read_int(): return int(_next())
def _read_float(): return float(_next())
def _read_bool(): return _next().lower() == 'true'
def _read_char():
    s = _next()
    return s[0] if s else '\x00'

def _print(v):
    sys.stdout.write('true' if isinstance(v, bool) and v else ('false' if isinstance(v, bool) else str(v)))

def _new_int_array(n): return [0 for _ in range(n)]
def _new_obj_array(n, _factory=None):
    return [(_factory() if _factory else None) for _ in range(n)]
def _index_funret(v, i):
    try:
        return v[i]
    except (TypeError, IndexError):
        return v

def fibonacci(n):
    if ((n < 1)):
        return n
    if ((n == 1)):
        return n
    return (fibonacci((n - 1)) + fibonacci((n - 2)))

def main():
    v = fibonacci(15)
    _print(v)
    _print('\n')


if __name__ == '__main__':
    try:
        main()
    except NameError:
        pass
