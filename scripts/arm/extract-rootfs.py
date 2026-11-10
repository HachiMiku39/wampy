#!/usr/bin/env python3
"""Extract the Android boot image's MTK/gzip initramfs as a CPIO, read-only."""
from pathlib import Path
import struct, sys, zlib
blob=Path(sys.argv[1]).read_bytes()
start=blob.find(b'ANDROID!',0,1024)
if start<0: raise ValueError('No Android boot header: '+blob[:64].hex())
kernel=struct.unpack_from('<I',blob,start+8)[0]
size=struct.unpack_from('<I',blob,start+16)[0]
page=struct.unpack_from('<I',blob,start+36)[0]
if page not in (512,1024,2048,4096,8192,16384): raise ValueError('Invalid page size')
offset=start+page+((kernel+page-1)//page)*page
ramdisk=blob[offset:offset+size]
gzip_offset=ramdisk.find(b'\x1f\x8b',0,1024)
if gzip_offset<0: raise ValueError('No gzip initramfs')
cpio=zlib.decompress(ramdisk[gzip_offset:],31)
if not cpio.startswith((b'070701',b'070702')): raise ValueError('Invalid CPIO archive')
Path(sys.argv[2]).write_bytes(cpio)
print('Extracted initramfs CPIO:',len(cpio),'bytes')
