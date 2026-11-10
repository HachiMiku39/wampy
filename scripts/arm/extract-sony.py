"""Extract Sony Packman UM03/PFW records with CRC checks; never execute them."""
from pathlib import Path, PureWindowsPath
import hashlib
import json
import struct
import sys
import zlib

def extract(source, output):
    source, output = Path(source), Path(output)
    data = source.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    n = struct.unpack_from('<H', data, pe + 6)[0]
    opt = struct.unpack_from('<H', data, pe + 20)[0]
    ends = []
    for i in range(n):
        q = pe + 24 + opt + 40 * i
        size, pos = struct.unpack_from('<II', data, q + 16)
        ends.append(pos + size)
    off = max(ends)
    output.mkdir(parents=True, exist_ok=True)
    files = []
    if data[off:off + 4] != b'UM\x03\x0a':
        # Legacy Packman has a file directory followed by stored payloads.
        # Its CAB bytes are complemented (MSCF -> b2 ac bc b9).
        version, directory_length, count = struct.unpack_from('<III', data, off + 24)
        if version != 1 or not 0 < directory_length < 512 or not 0 < count < 512:
            raise ValueError(f'Unknown Sony container at {off}: {data[off:off+64].hex()}')
        position = off + 36 + directory_length * 2
        entries = []
        for _ in range(count):
            length, size = struct.unpack_from('<II', data, position)
            position += 8
            if not 0 < length < 1024: raise ValueError('Invalid Packman filename')
            name = data[position:position + 2 * length].decode('utf-16le')
            position += 2 * length + 8  # file timestamp
            win = PureWindowsPath(name)
            if win.drive or win.root or '..' in win.parts: raise ValueError('Unsafe Packman path')
            entries.append((win, size))
        invert = bytes.maketrans(bytes(range(256)), bytes(255 - i for i in range(256)))
        for win, size in entries:
            if position + size > len(data): raise ValueError('Packman payload outside source')
            content = data[position:position + size]
            if content.startswith(b'\xb2\xac\xbc\xb9'):
                content = content.translate(invert)
                if not content.startswith(b'MSCF'): raise ValueError('Invalid complemented CAB')
            target = output.joinpath(*win.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
            files.append({'name': str(win), 'size': size, 'sha256': hashlib.sha256(content).hexdigest()})
            position += size
        manifest = {'source_sha256': hashlib.sha256(data).hexdigest(), 'files': files,
                    'end_offset': position, 'remaining_bytes': len(data) - position}
        (output / 'extraction_manifest.json').write_text(json.dumps(manifest, indent=2))
        return manifest
    while data[off:off + 4] == b'UM\x03\x0a':
        method = struct.unpack_from('<I', data, off + 16)[0]
        compressed_crc, crc = struct.unpack_from('<II', data, off + 28)
        compressed, uncompressed = struct.unpack_from('<QQ', data, off + 36)
        namelen = struct.unpack_from('<I', data, off + 56)[0]
        if not 0 < namelen < 1024 or uncompressed > 1024 ** 3:
            raise ValueError('Unexpected PFW record')
        name = data[off + 64:off + 64 + 2 * namelen].decode('utf-16le')
        win = PureWindowsPath(name)
        if win.drive or win.root or '..' in win.parts:
            raise ValueError('Unsafe output path')
        start = off + 64 + 2 * namelen
        if start + compressed > len(data):
            raise ValueError('Payload outside source')
        raw = data[start:start + compressed]
        if zlib.crc32(raw) != compressed_crc:
            raise ValueError(f'Compressed CRC mismatch for {name}')
        if method == 9:
            content = zlib.decompress(raw, -15)
        elif method == 0:
            content = raw
        else:
            raise ValueError(f'Unimplemented compression method {method}')
        if len(content) != uncompressed:
            raise ValueError('Uncompressed size mismatch')
        actual = zlib.crc32(content)
        if actual != crc:
            raise ValueError(f'CRC mismatch for {name}')
        target = output.joinpath(*win.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
        files.append({'name': name, 'record_offset': off, 'compressed': compressed,
                      'size': len(content), 'crc32': f'{actual:08x}',
                      'sha256': hashlib.sha256(content).hexdigest()})
        off = start + compressed
    manifest = {'source': str(source), 'source_sha256': hashlib.sha256(data).hexdigest(),
                'files': files, 'end_offset': off, 'remaining_bytes': len(data) - off}
    (output / 'extraction_manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2))
    return manifest

if __name__ == '__main__':
    m = extract(sys.argv[1], sys.argv[2])
    print(json.dumps({'files': len(m['files']), 'end_offset': m['end_offset'],
                      'remaining_bytes': m['remaining_bytes']}))
    for f in m["files"]: print(f["size"], f["name"])
