from __future__ import annotations

import struct
import sys
from pathlib import Path


BYTES_PER_SECTOR = 512
SECTORS_PER_CLUSTER = 1
RESERVED_SECTORS = 32
FAT_COUNT = 2
SECTORS_PER_FAT = 128
TOTAL_SECTORS = 2048
ROOT_CLUSTER = 2
MEDIA = 0xF8
ROOT_DIR_ENTRIES = [
    ("TASKA   ELF", Path("build/user_a.elf")),
    ("TASKB   ELF", Path("build/user_b.elf")),
]


def build_boot_sector() -> bytes:
    sector = bytearray(BYTES_PER_SECTOR)
    sector[0:3] = b"\xEB\x58\x90"
    sector[3:11] = b"SYSTOY32"
    struct.pack_into("<H", sector, 11, BYTES_PER_SECTOR)
    struct.pack_into("<B", sector, 13, SECTORS_PER_CLUSTER)
    struct.pack_into("<H", sector, 14, RESERVED_SECTORS)
    struct.pack_into("<B", sector, 16, FAT_COUNT)
    struct.pack_into("<H", sector, 17, 0)
    struct.pack_into("<H", sector, 19, 0)
    struct.pack_into("<B", sector, 21, MEDIA)
    struct.pack_into("<H", sector, 22, 0)
    struct.pack_into("<H", sector, 24, 63)
    struct.pack_into("<H", sector, 26, 255)
    struct.pack_into("<I", sector, 28, 0)
    struct.pack_into("<I", sector, 32, TOTAL_SECTORS)
    struct.pack_into("<I", sector, 36, SECTORS_PER_FAT)
    struct.pack_into("<H", sector, 40, 0)
    struct.pack_into("<H", sector, 42, 0)
    struct.pack_into("<I", sector, 44, ROOT_CLUSTER)
    struct.pack_into("<H", sector, 48, 1)
    struct.pack_into("<H", sector, 50, 6)
    sector[64] = 0x80
    sector[66] = 0x29
    struct.pack_into("<I", sector, 67, 0x12345678)
    sector[71:82] = b"SYSTOY FAT "
    sector[82:90] = b"FAT32   "
    sector[510:512] = b"\x55\xAA"
    return bytes(sector)


def build_fat_sector() -> bytes:
    fat = bytearray(SECTORS_PER_FAT * BYTES_PER_SECTOR)
    struct.pack_into("<I", fat, 0, 0x0FFFFFF8)
    struct.pack_into("<I", fat, 4, 0x0FFFFFFF)
    struct.pack_into("<I", fat, 8, 0x0FFFFFFF)
    struct.pack_into("<I", fat, 12, 0x0FFFFFFF)
    struct.pack_into("<I", fat, 16, 0x0FFFFFFF)
    return bytes(fat)


def build_dir_entry(name_11: str, cluster: int, size: int) -> bytes:
    entry = bytearray(32)
    entry[0:11] = name_11.encode("ascii")
    entry[11] = 0x20
    struct.pack_into("<H", entry, 20, (cluster >> 16) & 0xFFFF)
    struct.pack_into("<H", entry, 26, cluster & 0xFFFF)
    struct.pack_into("<I", entry, 28, size)
    return bytes(entry)


def build_image() -> bytes:
    image = bytearray(TOTAL_SECTORS * BYTES_PER_SECTOR)
    boot = build_boot_sector()
    fat = build_fat_sector()

    image[0:BYTES_PER_SECTOR] = boot

    fat1_offset = RESERVED_SECTORS * BYTES_PER_SECTOR
    fat2_offset = fat1_offset + len(fat)
    image[fat1_offset:fat1_offset + len(fat)] = fat
    image[fat2_offset:fat2_offset + len(fat)] = fat

    data_start_sector = RESERVED_SECTORS + FAT_COUNT * SECTORS_PER_FAT
    root_dir_offset = data_start_sector * BYTES_PER_SECTOR
    root_dir = bytearray(BYTES_PER_SECTOR)

    for idx, (name_11, file_path) in enumerate(ROOT_DIR_ENTRIES):
        file_bytes = file_path.read_bytes()
        cluster = 3 + idx
        cluster_offset = root_dir_offset + (cluster - ROOT_CLUSTER) * BYTES_PER_SECTOR
        image[cluster_offset:cluster_offset + len(file_bytes)] = file_bytes
        entry = build_dir_entry(name_11, cluster, len(file_bytes))
        root_dir[idx * 32:(idx + 1) * 32] = entry

    image[root_dir_offset:root_dir_offset + BYTES_PER_SECTOR] = root_dir
    return bytes(image)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: make_fat32_image.py <output-image>", file=sys.stderr)
        return 1

    output = Path(sys.argv[1])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(build_image())
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
