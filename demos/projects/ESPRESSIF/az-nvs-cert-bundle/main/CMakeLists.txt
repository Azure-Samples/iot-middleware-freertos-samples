# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

idf_component_register(SRCS "az_nvs_cert_bundle_main.c"
                    INCLUDE_DIRS "."
                    REQUIRES recovery-nvs)
