#!/usr/bin/env python3
"""Exercise the original ARM glibc loader in trace mode, without running Wampy."""
from pathlib import Path
import json,re,subprocess
root=Path.cwd();b=root/'.arm-build';env=json.loads((b/'environment.json').read_text());lib=b/'sysroot/lib'
for p in list(lib.iterdir()):
    if not p.is_file() or p.is_symlink():continue
    data=subprocess.run([env['prefix']+'readelf','-d',str(p)],stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,text=True).stdout
    match=re.search(r'\(SONAME\).*?\[(.*?)\]',data)
    if match and not (lib/match[1]).exists():(lib/match[1]).symlink_to(p.name)
result=subprocess.run(['qemu-arm','-cpu','cortex-a9','-L',str(b/'sysroot'),'-E','LD_LIBRARY_PATH='+str(lib),'-E','LD_TRACE_LOADED_OBJECTS=1','-E','LD_WARN=1','-E','LD_BIND_NOW=1',str(b/'output/wampy')],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=30)
(b/'output/loader-check.txt').write_text(result.stdout)
print(result.stdout)
if result.returncode or 'not found' in result.stdout or 'undefined symbol' in result.stdout or 'libc.so.6 =>' not in result.stdout:raise SystemExit('ARM runtime loader validation failed')
print('Original ARM glibc loader resolved application dependencies without executing main')
