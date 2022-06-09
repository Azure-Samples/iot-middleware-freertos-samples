// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_CLIENT_PROPERTIES_PRIVATE_H
#define _az_IOT_HUB_CLIENT_PROPERTIES_PRIVATE_H

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/iot/az_iot_hub_client.h>

#include <azure/core/_az_cfg_prefix.h>

static const az_span iot_hub_properties_reported = AZ_SPAN_LITERAL_FROM_STR("reported");
static const az_span iot_hub_properties_desired = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span iot_hub_properties_desired_version = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span component_properties_label_name = AZ_SPAN_LITERAL_FROM_STR("__t");
static const az_span component_properties_label_value = AZ_SPAN_LITERAL_FROM_STR("c");

typedef enum
{
  _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
  = 1, /**< A response from a properties "GET" request. */
  _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
  = 2, /**< A message with a payload containing updated writable properties for the device to
          process. */
  _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_ACKNOWLEDGEMENT
  = 3, /**< A response acknowledging the service has received properties that the device sent. */
  _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_ERROR
  = 4, /**< An error has occurred from the service processing properties. */
} _az_iot_hub_client_properties_message_type;

typedef enum
{
  /** @brief Property was originally reported from the device. */
  _AZ_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE,
  /** @brief Property was received from the service. */
  _AZ_IOT_HUB_CLIENT_PROPERTY_WRITABLE
} _az_iot_hub_client_property_type;

/**
 * @brief Iteratively read the IoT Plug and Play component properties.
 *
 * Note that between calls, the #az_span pointed to by \p out_component_name shall not be modified,
 * only checked and compared. Internally, the #az_span is only changed if the component name changes
 * in the JSON document and is not necessarily set every invocation of the function.
 *
 * On success, the `ref_json_reader` will be set on a valid property name. After checking the
 * property name, the reader can be advanced to the property value by calling
 * az_json_reader_next_token(). Note that on the subsequent call to this API, it is expected that
 * the json reader will be placed AFTER the read property name and value. That means that after
 * reading the property value (including single values or complex objects), the user must call
 * az_json_reader_next_token().
 *
 * Below is a code snippet which you can use as a starting point:
 *
 * @code
 *
 * while (az_result_succeeded(az_iot_hub_client_properties_get_next_component_property(
 *       &hub_client, &jr, message_type, AZ_IOT_HUB_CLIENT_PROPERTY_WRITABLE, &component_name)))
 * {
 *   // Check if property is of interest (substitute user_property for your own property name)
 *   if (az_json_token_is_text_equal(&jr.token, user_property))
 *   {
 *     az_json_reader_next_token(&jr);
 *
 *     // Get the property value here
 *     // Example: az_json_token_get_int32(&jr.token, &user_int);
 *
 *     // Skip to next property value
 *     az_json_reader_next_token(&jr);
 *   }
 *   else
 *   {
 *     // The JSON reader must be advanced regardless of whether the property
 *     // is of interest or not.
 *     az_json_reader_next_token(&jr);
 *
 *     // Skip children in case the property value is an object
 *     az_json_reader_skip_children(&jr);
 *     az_json_reader_next_token(&jr);
 *   }
 * }
 *
 * @endcode
 *
 * @warning If you need to retrieve more than one \p property_type, you should first complete the
 * scan of all components for the first property type (until the API returns
 * #AZ_ERROR_IOT_END_OF_PROPERTIES). Then you must call az_json_reader_init() again after this call
 * and before the next call to az_iot_hub_client_properties_get_next_component_property with the
 * different \p property_type.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in,out] ref_json_reader The #az_json_reader to parse through. The ownership of iterating
 * through this json reader is shared between the user and this API.
 * @param[in] message_type The #az_iot_hub_client_properties_message_type representing the message
 * type associated with the payload.
 * @param[in] property_type The #az_iot_hub_client_property_type to scan for.
 * @param[out] out_component_name The #az_span* representing the value of the component.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_reader must not be `NULL`.
 * @pre \p out_component_name must not be `NULL`. It must point to an #az_span instance.
 * @pre \p message_type must be `AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED` or
 * `AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE`.
 * @pre \p property_type must be `AZ_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE` or
 * `AZ_IOT_HUB_CLIENT_PROPERTY_WRITABLE`.
 * @pre \p If `AZ_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE` is specified in \p property_type,
 * then \p message_type must be `AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK If the function returned a valid #az_json_reader pointing to the property name and
 * the #az_span with a component name.
 * @retval #AZ_ERROR_JSON_INVALID_STATE If the json reader is passed in at an unexpected location.
 * @retval #AZ_ERROR_IOT_END_OF_PROPERTIES If there are no more properties left for the component.
 */
AZ_NODISCARD az_result _az_iot_hub_client_properties_get_next_component_property(
    az_span* component_names,
    int32_t component_names_length,
    az_json_reader* ref_json_reader,
    _az_iot_hub_client_properties_message_type message_type,
    _az_iot_hub_client_property_type property_type,
    az_span* out_component_name);

#include <azure/core/_az_cfg_suffix.h>

#endif //_az_IOT_HUB_CLIENT_PROPERTIES_PRIVATE_H
