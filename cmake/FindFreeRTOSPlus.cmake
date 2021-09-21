# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if(NOT FREERTOS_PATH)
    set(FREERTOS_PATH /opt/FreeRTOS CACHE PATH "Path to FreeRTOS")
    message(STATUS "No FREERTOS_PATH specified using default: ${FREERTOS_PATH}")
endif()

set(FreeRTOSPlus_PATH ${FREERTOS_PATH}/FreeRTOS-Plus CACHE PATH "Path to FreeRTOSPlus")

if(NOT (TARGET FreeRTOSPlus::Utilities::backoff_algorithm))
    add_library(FreeRTOSPlus::Utilities::backoff_algorithm INTERFACE IMPORTED)
    target_sources(FreeRTOSPlus::Utilities::backoff_algorithm INTERFACE
        "${FreeRTOSPlus_PATH}/Source/Utilities/backoff_algorithm/source/backoff_algorithm.c")
    target_include_directories(FreeRTOSPlus::Utilities::backoff_algorithm INTERFACE
        "${FreeRTOSPlus_PATH}/Source/Utilities/backoff_algorithm/source/include")
endif()

if(NOT (TARGET FreeRTOSPlus::Utilities::logging))
    add_library(FreeRTOSPlus::Utilities::logging INTERFACE IMPORTED)
    target_include_directories(FreeRTOSPlus::Utilities::logging INTERFACE
        "${FreeRTOSPlus_PATH}/Source/Utilities/logging")
endif()

if(NOT (TARGET FreeRTOSPlus::ThirdParty::mbedtls))
    add_library(FreeRTOSPlus::ThirdParty::mbedtls INTERFACE IMPORTED)
    target_sources(FreeRTOSPlus::ThirdParty::mbedtls INTERFACE
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/aes.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/aesni.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/arc4.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/aria.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/asn1parse.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/asn1write.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/base64.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/bignum.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/blowfish.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/camellia.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ccm.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/certs.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/chacha20.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/chachapoly.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/cipher.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/cipher_wrap.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/cmac.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ctr_drbg.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/debug.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/des.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/dhm.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ecdh.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ecdsa.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ecjpake.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ecp.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ecp_curves.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/entropy.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/entropy_poll.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/error.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/gcm.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/havege.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/hkdf.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/hmac_drbg.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/md.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/md2.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/md4.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/md5.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/memory_buffer_alloc.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/net_sockets.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/nist_kw.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/oid.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/padlock.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/pem.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/pk.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/pk_wrap.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/pkcs11.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/pkcs12.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/pkcs5.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/pkparse.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/pkwrite.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/platform.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/platform_util.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/poly1305.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/psa_crypto.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/psa_crypto_se.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/psa_crypto_slot_management.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/psa_crypto_storage.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/psa_its_file.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ripemd160.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/rsa.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/rsa_internal.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/sha1.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/sha256.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/sha512.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_cache.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_ciphersuites.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_cli.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_cookie.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_msg.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_srv.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_ticket.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_tls.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/ssl_tls13_keys.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/threading.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/timing.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/version.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/version_features.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/x509.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/x509_create.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/x509_crl.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/x509_crt.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/x509_csr.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/x509write_crt.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/x509write_csr.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/xtea.c
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/library/error.c)
    target_include_directories(FreeRTOSPlus::ThirdParty::mbedtls INTERFACE
        ${FreeRTOSPlus_PATH}/ThirdParty/mbedtls/include)
    target_include_directories(FreeRTOSPlus::ThirdParty::mbedtls INTERFACE
        ${FreeRTOSPlus_PATH}/Source/Utilities/mbedtls_freertos)
    target_compile_definitions(FreeRTOSPlus::ThirdParty::mbedtls INTERFACE
        MBEDTLS_CONFIG_FILE="mbedtls_config.h")
endif()

if(NOT (TARGET FreeRTOSPlus::TCPIP))
    add_library(FreeRTOSPlus::TCPIP INTERFACE IMPORTED)
    target_sources(FreeRTOSPlus::TCPIP INTERFACE
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_DNS.c
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_DHCP.c
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_ARP.c
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_WIN.c
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_Stream_Buffer.c
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_IP.c
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_IP.c
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_UDP_IP.c
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_Sockets.c)
    target_include_directories(FreeRTOSPlus::TCPIP INTERFACE
        ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/include/)
endif()

if(NOT (TARGET FreeRTOSPlus::TCPIP::PORT))
    add_library(FreeRTOSPlus::TCPIP::PORT INTERFACE IMPORTED)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeRTOSPlus
    REQUIRED_VARS FreeRTOSPlus_PATH
    FOUND_VAR FreeRTOSPlus_FOUND
    HANDLE_COMPONENTS
)
