/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file sfw.h
 * @brief Contains required defines to properly use sfw (Secure Firmware) code from NXP
 *
 * @note This file must be named sfw.h, as it's referenced from sfw files.
 */

#ifndef __SFW_H__
#define __SFW_H__

/* MCU-SFW RT1060 Configuration */
#define SOC_IMXRTYYYY_SERIES

/* MCU SFW Flash Map */

#define BOOT_FLASH_BASE        0x60000000
#define BOOT_FLASH_ACT_APP     0x60100000
#define BOOT_FLASH_CAND_APP    0x60200000

/* MCU SFW Component */

/* Flash IAP */

#define COMPONENT_FLASHIAP
#define COMPONENT_FLASHIAP_ROM

/* Flash device parameters */

#define COMPONENT_FLASHIAP_SIZE    8388608

/* Platform Drivers Config */

#define BOARD_FLASH_SUPPORT
#define ISSI_IS25WPxxxA
#define SOC_MIMXRT1062DVL6A

/* Onboard Peripheral Drivers */

#define COMPONENT_PHY

#endif /* ifndef __SFW_H__ */
