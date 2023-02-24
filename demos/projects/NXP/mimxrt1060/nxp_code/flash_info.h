// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 */
#ifndef __FLASH_INFO_H__
#define __FLASH_INFO_H__

#include "flexspi_flash.h"

#define FLASH_AREA_IMAGE_SECTOR_SIZE	SECTOR_SIZE
#define FLASH_DEVICE_BASE_ADDR			FlexSPI_AMBA_BASE
/* BOOT_MAX_IMG_SECTORS must be bigger than the image slot sector number */
#define FLASH_AREA_IMAGE_1_OFFSET		0x100000	
#define FLASH_AREA_IMAGE_1_SIZE			0x100000
#define FLASH_AREA_IMAGE_2_OFFSET		(FLASH_AREA_IMAGE_1_OFFSET + FLASH_AREA_IMAGE_1_SIZE)
#define FLASH_AREA_IMAGE_2_SIZE			FLASH_AREA_IMAGE_1_SIZE	

#endif
