"""Dev tool: list south_park_td.exe code addresses found on a crashing thread's stack.

usage: python dump_stack.py <dump.dmp> <exe-with-pdb> [thread-id]
Without a thread id it uses the exception thread. Prints candidate return
addresses (heuristic stack scan) symbolized with llvm-symbolizer.
"""
import subprocess, sys, struct
from minidump.minidumpfile import MinidumpFile

dmp, exe = sys.argv[1], sys.argv[2]
md = MinidumpFile.parse(dmp)
reader = md.get_reader()
mod = next(m for m in md.modules.modules if m.name.lower().endswith('south_park_td.exe'))
lo, hi = mod.baseaddress, mod.baseaddress + mod.size
tid = int(sys.argv[3], 0) if len(sys.argv) > 3 else md.exception.exception_records[0].ThreadId
exc = md.exception.exception_records[0]
print('exception thread', exc.ThreadId, 'code', exc.ExceptionRecord.ExceptionCode, 'at', hex(exc.ExceptionRecord.ExceptionAddress))
th = next(t for t in md.threads.threads if t.ThreadId == tid)
ctx = th.ContextObject
rsp = ctx.Rsp
print('thread', tid, 'rip', hex(ctx.Rip), 'rsp', hex(rsp))
data = b''
for off in range(0, 0x40000, 0x1000):
    try:
        data += reader.read(rsp + off, 0x1000)
    except Exception:
        break
addrs = [ctx.Rip] if lo <= ctx.Rip < hi else []
for i in range(0, len(data) - 8, 8):
    v = struct.unpack_from('<Q', data, i)[0]
    if lo <= v < hi:
        addrs.append(v)
sym = r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\Llvm\x64\bin\llvm-symbolizer.exe"
inp = '\n'.join(hex(a - lo) for a in addrs[:120])
out = subprocess.run([sym, '--obj', exe, '--relative-address', '--no-inlines', '--functions=short'], input=inp,
                     capture_output=True, text=True).stdout.split('\n\n')
seen = []
for a, o in zip(addrs, out):
    name = o.strip().split('\n')[0] if o.strip() else '?'
    if not seen or seen[-1] != name:
        seen.append(name)
        print(hex(a - lo), name, (o.strip().split('\n') + [''])[1].replace('/', '\\').split('\\')[-1])
