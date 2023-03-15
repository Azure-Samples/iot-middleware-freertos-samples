// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 */
 
#ifndef __SBL_OTA_FLAG_H__
#define __SBL_OTA_FLAG_H__

#define REMAP_FLAG_ADDRESS 0xFFFE0UL
#define IMAGE_VERSION_OFFSET 0x14

#define BOOT_FLAG_SET       1

struct image_version {
    uint8_t iv_major;
    uint8_t iv_minor;
    uint16_t iv_revision;
    uint32_t iv_build_num;
};

/** Image header.  All fields are in little endian byte order. */
struct image_header {
    uint32_t ih_magic;
    uint32_t ih_load_addr;
    uint16_t ih_hdr_size;           /* Size of image header (bytes). */
    uint16_t ih_protect_tlv_size;   /* Size of protected TLV area (bytes). */
    uint32_t ih_img_size;           /* Does not include header. */
    uint32_t ih_flags;              /* IMAGE_F_[...]. */
    struct image_version ih_ver;
    uint32_t _pad1;
};

#ifdef SOC_REMAP_ENABLE
struct remap_trailer {
    uint8_t image_position;
    uint8_t pad1[7];
    uint8_t image_ok;
    uint8_t pad2[7];
    uint8_t magic[16];
};
#define IMAGE_TRAILER_SIZE     sizeof(struct remap_trailer)
#else
struct swap_trailer {
    uint8_t copy_done;
    uint8_t pad1[7];
    uint8_t image_ok;
    uint8_t pad2[7];
    uint8_t magic[16];
};
#define IMAGE_TRAILER_SIZE     sizeof(struct swap_trailer)
#endif

/* write the image trailer at the end of the flash partition */
void enable_image(void);

void write_image_ok(void);

#endif
