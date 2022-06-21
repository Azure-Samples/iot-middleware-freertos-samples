// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_adu.h"
#include <az_test_log.h>
#include <az_test_precondition.h>
#include <az_test_span.h>
#include <azure/core/az_log.h>
#include <azure/core/az_precondition.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/az_iot_adu.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#define TEST_SPAN_BUFFER_SIZE 512
#define TEST_ADU_DEVICE_MANUFACTURER "Contoso"
#define TEST_ADU_DEVICE_MODEL "FooBar"
#define TEST_AZ_IOT_ADU_CLIENT_AGENT_VERSION AZ_IOT_ADU_CLIENT_AGENT_VERSION
#define TEST_ADU_DEVICE_VERSION "1.0"

static uint8_t expected_agent_state_payload[]
    = "{\"deviceUpdate\":{\"__t\":\"c\",\"agent\":{\"deviceProperties\":{\"manufacturer\":"
      "\"Contoso\",\"model\":\"FooBar\",\"interfaceId\":\"dtmi:azure:iot:deviceUpdate;1\","
      "\"aduVer\":\"DU;agent/"
      "0.8.0-rc1-public-preview\"},\"compatPropertyNames\":\"manufacturer,model\",\"state\":0,"
      "\"installedUpdateId\":\"{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"FooBar\\\","
      "\\\"version\\\":\\\"1.0\\\"}\"}}}";

az_iot_adu_device_information adu_device_information
    = { .manufacturer = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_MANUFACTURER),
        .model = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_MODEL),
        .adu_version = AZ_SPAN_LITERAL_FROM_STR(TEST_AZ_IOT_ADU_CLIENT_AGENT_VERSION),
        .do_version = AZ_SPAN_LITERAL_EMPTY,
        .update_id = { .provider = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_MANUFACTURER),
                       .name = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_MODEL),
                       .version = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_VERSION) } };

#ifndef AZ_NO_PRECONDITION_CHECKING
ENABLE_PRECONDITION_CHECK_TESTS()

// PRECONDITION TESTS

#endif

static void test_az_iot_adu_client_get_agent_state_payload_succeed(void** state)
{
  (void)state;

  az_json_writer jw;
  uint8_t payload_buffer[TEST_SPAN_BUFFER_SIZE];

  assert_int_equal(
      az_json_writer_init(&jw, az_span_create(payload_buffer, sizeof(payload_buffer)), NULL),
      AZ_OK);

  assert_int_equal(
      az_iot_adu_client_get_agent_state_payload(
          &adu_device_information, AZ_IOT_ADU_CLIENT_AGENT_STATE_IDLE, NULL, NULL, &jw),
      AZ_OK);

  printf("%.*s\n", (int)sizeof(expected_agent_state_payload), payload_buffer);

  assert_memory_equal(
      payload_buffer, expected_agent_state_payload, sizeof(expected_agent_state_payload));
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_adu()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  SETUP_PRECONDITION_CHECK_TESTS();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
  // Precondition Tests
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_adu_client_get_agent_state_payload_succeed),
  };
  return cmocka_run_group_tests_name("az_iot_adu", tests, NULL, NULL);
}
