from __future__ import annotations

import json
import struct
import sys
from pathlib import Path


def read_u8(data: bytes, offset: int) -> int:
    return data[offset]


def read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def parse_dir_entries(image: bytes, bytes_per_sector: int, data_start_sector: int) -> list[str]:
    root_dir_offset = data_start_sector * bytes_per_sector
    entries = []

    for idx in range(0, 16):
        base = root_dir_offset + idx * 32
        first = image[base]
        if first in (0x00, 0xE5):
            continue

        raw_name = image[base:base + 11].decode("ascii", errors="replace")
        name = raw_name[:8].rstrip()
        ext = raw_name[8:11].rstrip()
        entries.append(f"{name}.{ext}" if ext else name)

    return entries


def parse_boot_sector(data: bytes) -> dict[str, int | str]:
    bytes_per_sector = read_u16(data, 11)
    sectors_per_cluster = read_u8(data, 13)
    reserved_sector_count = read_u16(data, 14)
    fat_count = read_u8(data, 16)
    total_sectors = read_u32(data, 32)
    sectors_per_fat = read_u32(data, 36)
    root_cluster = read_u32(data, 44)

    fat_start_sector = reserved_sector_count
    data_start_sector = reserved_sector_count + fat_count * sectors_per_fat

    return {
        "oem_name": data[3:11].decode("ascii", errors="replace").rstrip(),
        "bytes_per_sector": bytes_per_sector,
        "sectors_per_cluster": sectors_per_cluster,
        "reserved_sector_count": reserved_sector_count,
        "fat_count": fat_count,
        "total_sectors": total_sectors,
        "sectors_per_fat": sectors_per_fat,
        "root_cluster": root_cluster,
        "fat_start_sector": fat_start_sector,
        "data_start_sector": data_start_sector,
    }


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: show_fat32_layout.py <image>", file=sys.stderr)
        return 1

    image = Path(sys.argv[1]).read_bytes()
    info = parse_boot_sector(image[:512])
    info["entries"] = parse_dir_entries(
        image,
        int(info["bytes_per_sector"]),
        int(info["data_start_sector"]),
    )
    print(json.dumps(info, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
