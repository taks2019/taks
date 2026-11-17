#!/usr/bin/env python3
"""Convert verified TAKS GLES3 frame images from raw PPM to portable PNG using stdlib."""
import pathlib
import struct
import zlib

def chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I",len(data))+tag+data+struct.pack(">I",zlib.crc32(tag+data)&0xffffffff)

def convert(ppm: pathlib.Path):
    data=ppm.read_bytes()
    pos=0
    tokens=[]
    while len(tokens)<4:
        i=data.find(b"\n",pos)
        if i<0:raise ValueError(f"Bad PPM: {ppm}")
        line=data[pos:i].strip()
        pos=i+1
        if not line or line.startswith(b"#"):continue
        tokens.extend(line.split())
    if len(tokens)!=4 or tokens[0]!=b"P6" or tokens[3]!=b"255":
        raise ValueError(f"Unexpected PPM format: {ppm}")
    w,h=map(int,tokens[1:3])
    rgb=data[pos:]
    if len(rgb)!=w*h*3:raise ValueError(f"Wrong PPM size: {ppm}")
    raw=bytearray()
    for row in range(h):
        raw.append(0)
        raw.extend(rgb[row*w*3:(row+1)*w*3])
    png=b"\x89PNG\r\n\x1a\n"
    png+=chunk(b"IHDR",struct.pack(">IIBBBBB",w,h,8,2,0,0,0))
    png+=chunk(b"IDAT",zlib.compress(raw,6))
    png+=chunk(b"IEND",b"")
    output=ppm.with_suffix(".png")
    output.write_bytes(png)
    print(f"Validated raster image: {output} ({w} x {h})")

frames=sorted(pathlib.Path("build-previews").glob("*.ppm"))
if len(frames)!=6:raise RuntimeError(f"Expected 6 GPU frames with real photogrammetry rocks, found {len(frames)}")
for path in frames:convert(path)
