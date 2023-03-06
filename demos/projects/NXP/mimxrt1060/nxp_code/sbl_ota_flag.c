// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 */
#include "fsl_debug_console.h"
#include "flash_info.h"
#include "sbl_ota_flag.h"
#include "flexspi_flash.h"

#ifndef SFW_STANDALONE_XIP
/* image header, size 1KB */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__GNUC__)
    __attribute__((section(".image_header")))
#elif defined(__ICCARM__)
#pragma location=".image_header"
#endif

const char __image_header[1024] = {0x3D, 0xB8, 0xF3, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
#endif

const uint32_t boot_img_magic[] = {
    0xf395c277,
    0x7fefd260,
    0x0f505235,
    0x8079b62c,
};

#ifdef SOC_REMAP_ENABLE
struct remap_trailer s_remap_trailer;
/* write the remap image trailer */
void enable_image(void)
{
    uint32_t off;
    status_t status;
    uint32_t primask;

    memset((void *)&s_remap_trailer, 0xff, IMAGE_TRAILER_SIZE);
    memcpy((void *)s_remap_trailer.magic, boot_img_magic, sizeof(boot_img_magic));

    off = REMAP_FLAG_ADDRESS + 16;
    
    PRINTF("write magic number offset = 0x%x\r\n", off);

    primask = DisableGlobalIRQ();
    status = sfw_flash_write(off, (void *)&s_remap_trailer.magic, IMAGE_TRAILER_SIZE - 16);
    if (status) 
    {
        PRINTF("enable_image: failed to write remap flag\r\n");
        return;
    }
    EnableGlobalIRQ(primask);
}

void write_image_ok(void)
{
    uint32_t off;
    status_t status;
    uint32_t primask;
    
    sfw_flash_read(REMAP_FLAG_ADDRESS, &s_remap_trailer, 32);
    
    primask = DisableGlobalIRQ();
    status = sfw_flash_erase(FLASH_AREA_IMAGE_1_OFFSET - SECTOR_SIZE, SECTOR_SIZE);
    
    EnableGlobalIRQ(primask);
    
    memset((void *)s_remap_trailer.magic, 0xff, IMAGE_TRAILER_SIZE - 16);

    s_remap_trailer.image_ok = 0xFF;
    s_remap_trailer.pad1[3] = 0x0;
    
    off = REMAP_FLAG_ADDRESS;

    PRINTF("Write OK flag: off = 0x%x\r\n", off);
    
    primask = DisableGlobalIRQ();
    status = sfw_flash_write(off, (void *)&s_remap_trailer, IMAGE_TRAILER_SIZE);
    if (status) 
    {
        return;
    }
    EnableGlobalIRQ(primask);
}
#else
struct swap_trailer s_swap_trailer;
/* write the image trailer at the end of the flash partition */
void enable_image(void)
{
    uint32_t off;
    status_t status;
    uint32_t primask;
#ifdef SOC_LPC55S69_SERIES
    /* The flash of LPC55xx have the limit of offset when do write operation*/
    uint8_t write_buff[512];
    memset(write_buff, 0xff, 512);
#endif    
    
    memset((void *)&s_swap_trailer, 0xff, IMAGE_TRAILER_SIZE);
    memcpy((void *)s_swap_trailer.magic, boot_img_magic, sizeof(boot_img_magic));

#ifdef SOC_LPC55S69_SERIES
    memcpy(&write_buff[512 - IMAGE_TRAILER_SIZE], (void *)&s_swap_trailer, IMAGE_TRAILER_SIZE);
    off = FLASH_AREA_IMAGE_2_OFFSET + FLASH_AREA_IMAGE_2_SIZE - 512;
#else
    off = FLASH_AREA_IMAGE_2_OFFSET + FLASH_AREA_IMAGE_2_SIZE - IMAGE_TRAILER_SIZE;
#endif

    PRINTF("write magic number offset = 0x%x\r\n", off);

    primask = DisableGlobalIRQ();
#ifdef SOC_LPC55S69_SERIES
    status = sfw_flash_write(off, write_buff, 512);
#else
    status = sfw_flash_write(off, (void *)&s_swap_trailer, IMAGE_TRAILER_SIZE);
#endif
    if (status) 
    {
        PRINTF("enable_image: failed to write trailer2\r\n");
        return;
    }
    EnableGlobalIRQ(primask);
}
void write_image_ok(void)
{
    uint32_t off;
    status_t status;
    uint32_t primask;
#ifdef SOC_LPC55S69_SERIES
    /* The flash of LPC55xx have the limit of offset when do write operation*/
    static uint8_t write_buff[512];
    memset(write_buff, 0xff, 512);
#endif    
    
    memset((void *)&s_swap_trailer, 0xff, IMAGE_TRAILER_SIZE);
    memcpy((void *)s_swap_trailer.magic, boot_img_magic, sizeof(boot_img_magic));
    
    s_swap_trailer.image_ok= BOOT_FLAG_SET;

#ifdef SOC_LPC55S69_SERIES
    memcpy(&write_buff[512 - IMAGE_TRAILER_SIZE], (void *)&s_swap_trailer, IMAGE_TRAILER_SIZE);
    off = FLASH_AREA_IMAGE_1_OFFSET + FLASH_AREA_IMAGE_1_SIZE - 512;
#else
    off = FLASH_AREA_IMAGE_1_OFFSET + FLASH_AREA_IMAGE_1_SIZE - IMAGE_TRAILER_SIZE;
#endif

    PRINTF("Write OK flag: off = 0x%x\r\n", off);
    
    primask = DisableGlobalIRQ();
#ifdef SOC_LPC55S69_SERIES
    status = sfw_flash_write(off, write_buff, 512);
#else
    status = sfw_flash_write(off, (void *)&s_swap_trailer, IMAGE_TRAILER_SIZE);
#endif
    if (status) 
    {
        return;
    }
    EnableGlobalIRQ(primask);
}
#endif

/**
 * Compare image version numbers not including the build number
 *
 * @param ver1           Pointer to the first image version to compare.
 * @param ver2           Pointer to the second image version to compare.
 *
 * @retval -1           If ver1 is strictly less than ver2.
 * @retval 0            If the image version numbers are equal,
 *                      (not including the build number).
 * @retval 1            If ver1 is strictly greater than ver2.
 */
int8_t image_version_cmp(const struct image_version *ver1,
                 const struct image_version *ver2)
{
    if (ver1->iv_major > ver2->iv_major) {
        return 1;
    }
    if (ver1->iv_major < ver2->iv_major) {
        return -1;
    }
    /* The major version numbers are equal, continue comparison. */
    if (ver1->iv_minor > ver2->iv_minor) {
        return 1;
    }
    if (ver1->iv_minor < ver2->iv_minor) {
        return -1;
    }
    /* The minor version numbers are equal, continue comparison. */
    if (ver1->iv_revision > ver2->iv_revision) {
        return 1;
    }
    if (ver1->iv_revision < ver2->iv_revision) {
        return -1;
    }

    return 0;
}
