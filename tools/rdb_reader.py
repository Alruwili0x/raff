"""Bounded MessagePack reader for the official RetroArch RARCHDB format."""
import struct

class Reader:
    def __init__(self, data, position=0):
        self.data, self.position = data, position

    def take(self, size):
        if size < 0 or self.position + size > len(self.data):
            raise ValueError('Truncated MessagePack')
        result = self.data[self.position:self.position + size]
        self.position += size
        return result

    def integer(self, size, signed=False):
        return int.from_bytes(self.take(size), 'big', signed=signed)

    def value(self, depth=0):
        if depth > 32:
            raise ValueError('MessagePack nesting limit')
        code = self.integer(1)
        if code <= 0x7f: return code
        if code >= 0xe0: return code - 256
        if 0xa0 <= code <= 0xbf: return self.take(code & 31).decode('utf-8')
        if 0x80 <= code <= 0x8f: return self.mapping(code & 15, depth)
        if 0x90 <= code <= 0x9f: return self.array(code & 15, depth)
        if code == 0xc0: return None
        if code in (0xc2, 0xc3): return code == 0xc3
        if code in (0xc4, 0xc5, 0xc6): return self.take(self.integer(1 << (code - 0xc4)))
        if code in (0xcc, 0xcd, 0xce, 0xcf): return self.integer(1 << (code - 0xcc))
        if code in (0xd0, 0xd1, 0xd2, 0xd3): return self.integer(1 << (code - 0xd0), True)
        if code in (0xd9, 0xda, 0xdb): return self.take(self.integer(1 << (code - 0xd9))).decode('utf-8')
        if code in (0xdc, 0xdd): return self.array(self.integer(2 << (code - 0xdc)), depth)
        if code in (0xde, 0xdf): return self.mapping(self.integer(2 << (code - 0xde)), depth)
        if code in (0xca, 0xcb): return struct.unpack('>f' if code == 0xca else '>d', self.take(4 if code == 0xca else 8))[0]
        raise ValueError(f'Unsupported MessagePack code {code:x}')

    def mapping(self, count, depth):
        if count > 100000: raise ValueError('Map limit')
        result = {}
        for _ in range(count):
            key = self.value(depth + 1)
            result[key] = self.value(depth + 1)
        return result

    def array(self, count, depth):
        if count > 100000: raise ValueError('Array limit')
        return [self.value(depth + 1) for _ in range(count)]

def records(path):
    data = path.read_bytes()
    if data[:8] != b'RARCHDB\0': raise ValueError('Invalid RDB header')
    metadata_offset = int.from_bytes(data[8:16], 'big')
    if not 16 < metadata_offset < len(data): raise ValueError('Invalid RDB metadata offset')
    metadata = Reader(data, metadata_offset).value()
    reader, count = Reader(data, 16), 0
    while reader.position < metadata_offset:
        row = reader.value()
        if row is None: break
        if not isinstance(row, dict): raise ValueError('Invalid RDB row')
        count += 1
        yield row
    if count != metadata.get('count'): raise ValueError(f'RDB count mismatch {count} != {metadata}')

if __name__ == '__main__':
    import pathlib, sys
    for p in map(pathlib.Path, sys.argv[1:]):
        rows = list(records(p))
        print(p.name, len(rows), rows[:1])
