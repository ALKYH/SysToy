#include "kernel.h"

struct elf32_header {
    u8 ident[16];
    u16 type;
    u16 machine;
    u32 version;
    u32 entry;
    u32 phoff;
    u32 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
} __attribute__((packed));

struct elf32_program_header {
    u32 type;
    u32 offset;
    u32 vaddr;
    u32 paddr;
    u32 filesz;
    u32 memsz;
    u32 flags;
    u32 align;
} __attribute__((packed));

struct fat32_boot_sector {
    u8 jump[3];
    u8 oem_name[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sector_count;
    u8 fat_count;
    u16 root_entry_count;
    u16 total_sectors_16;
    u8 media;
    u16 fat_size_16;
    u16 sectors_per_track;
    u16 head_count;
    u32 hidden_sectors;
    u32 total_sectors_32;
    u32 fat_size_32;
    u16 ext_flags;
    u16 fs_version;
    u32 root_cluster;
} __attribute__((packed));

struct fat32_dir_entry {
    u8 name[11];
    u8 attr;
    u8 nt_reserved;
    u8 create_tenth;
    u16 create_time;
    u16 create_date;
    u16 access_date;
    u16 cluster_hi;
    u16 write_time;
    u16 write_date;
    u16 cluster_lo;
    u32 size;
} __attribute__((packed));

static void zero_fill(u8* dst, u32 size) {
    u32 i;
    for (i = 0; i < size; ++i) {
        dst[i] = 0;
    }
}

static void copy_bytes(u8* dst, const u8* src, u32 size) {
    u32 i;
    for (i = 0; i < size; ++i) {
        dst[i] = src[i];
    }
}

static int names_equal_11(const u8* lhs, const char* rhs) {
    u32 i;
    for (i = 0; i < 11; ++i) {
        if (lhs[i] != (u8)rhs[i]) {
            return 0;
        }
    }
    return 1;
}

static void copy_name_11_to_text(const u8* raw, char* out) {
    u32 src = 0;
    u32 dst = 0;
    u32 end = 8;

    while (end > 0 && raw[end - 1] == ' ') {
        end -= 1;
    }

    for (src = 0; src < end; ++src) {
        out[dst++] = (char)raw[src];
    }

    if (raw[8] != ' ' || raw[9] != ' ' || raw[10] != ' ') {
        out[dst++] = '.';
        for (src = 8; src < 11; ++src) {
            if (raw[src] == ' ') {
                break;
            }
            out[dst++] = (char)raw[src];
        }
    }

    out[dst] = '\0';
}

int fat32_load_root_file(const u8* image, const char* name_11, struct fat32_file_view* file) {
    const struct fat32_boot_sector* boot = (const struct fat32_boot_sector*)image;
    u32 bytes_per_sector = boot->bytes_per_sector;
    u32 fat_sectors = boot->fat_size_32;
    u32 data_start_sector = boot->reserved_sector_count + boot->fat_count * fat_sectors;
    u32 root_dir_offset = data_start_sector * bytes_per_sector;
    u32 idx;

    if (boot->bytes_per_sector == 0 || boot->sectors_per_cluster == 0) {
        return -1;
    }

    for (idx = 0; idx < 16; ++idx) {
        const struct fat32_dir_entry* entry =
            (const struct fat32_dir_entry*)(image + root_dir_offset + idx * sizeof(struct fat32_dir_entry));
        u32 cluster;
        u32 cluster_offset;

        if (entry->name[0] == 0x00 || entry->name[0] == 0xE5) {
            continue;
        }

        if (!names_equal_11(entry->name, name_11)) {
            continue;
        }

        cluster = ((u32)entry->cluster_hi << 16) | entry->cluster_lo;
        if (cluster < boot->root_cluster) {
            return -2;
        }

        cluster_offset = root_dir_offset + (cluster - boot->root_cluster) * bytes_per_sector;
        file->data = image + cluster_offset;
        file->size = entry->size;
        return 0;
    }

    return -3;
}

int fat32_list_root_files(const u8* image, struct fat32_dir_listing* listing) {
    const struct fat32_boot_sector* boot = (const struct fat32_boot_sector*)image;
    u32 bytes_per_sector = boot->bytes_per_sector;
    u32 fat_sectors = boot->fat_size_32;
    u32 data_start_sector = boot->reserved_sector_count + boot->fat_count * fat_sectors;
    u32 root_dir_offset = data_start_sector * bytes_per_sector;
    u32 idx;

    if (boot->bytes_per_sector == 0 || boot->sectors_per_cluster == 0) {
        return -1;
    }

    listing->count = 0;
    for (idx = 0; idx < 16; ++idx) {
        const struct fat32_dir_entry* entry =
            (const struct fat32_dir_entry*)(image + root_dir_offset + idx * sizeof(struct fat32_dir_entry));

        if (entry->name[0] == 0x00 || entry->name[0] == 0xE5) {
            continue;
        }

        if (listing->count >= 16) {
            break;
        }

        copy_name_11_to_text(entry->name, listing->names[listing->count]);
        listing->count += 1;
    }

    return 0;
}

int elf_load_image(const u8* elf_data, u32 elf_size, struct loaded_program* program) {
    const struct elf32_header* header = (const struct elf32_header*)elf_data;
    const struct elf32_program_header* ph;
    u32 idx;
    u32 image_base = 0xFFFFFFFF;
    u32 image_end = 0;
    u8* image;

    if (elf_size < sizeof(*header)) {
        return -1;
    }

    if (header->ident[0] != 0x7F || header->ident[1] != 'E' || header->ident[2] != 'L' || header->ident[3] != 'F') {
        return -2;
    }

    if (header->machine != 243 || header->phentsize != sizeof(struct elf32_program_header)) {
        console_write("[elf] machine=");
        console_write_hex(header->machine);
        console_write(" phentsize=");
        console_write_hex(header->phentsize);
        console_write("\n");
        return -3;
    }

    ph = (const struct elf32_program_header*)(elf_data + header->phoff);
    for (idx = 0; idx < header->phnum; ++idx) {
        if (ph[idx].type != 1) {
            continue;
        }

        if (ph[idx].offset + ph[idx].filesz > elf_size) {
            return -4;
        }

        if (ph[idx].vaddr < image_base) {
            image_base = ph[idx].vaddr;
        }

        if (ph[idx].vaddr + ph[idx].memsz > image_end) {
            image_end = ph[idx].vaddr + ph[idx].memsz;
        }
    }

    if (image_base == 0xFFFFFFFF || image_end <= image_base) {
        return -5;
    }

    image = (u8*)memory_alloc(image_end - image_base);
    if (!image) {
        return -6;
    }

    zero_fill(image, image_end - image_base);

    for (idx = 0; idx < header->phnum; ++idx) {
        if (ph[idx].type != 1) {
            continue;
        }

        copy_bytes(
            image + (ph[idx].vaddr - image_base),
            elf_data + ph[idx].offset,
            ph[idx].filesz
        );
    }

    program->image = image;
    program->entry_point = (u32)(image + (header->entry - image_base));
    program->size = image_end - image_base;
    return 0;
}
