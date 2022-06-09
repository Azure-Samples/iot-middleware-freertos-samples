// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/az_iot_hub_client_properties.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include "az_iot_hub_client_properties_private.h"

static const az_span properties_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span properties_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span properties_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span properties_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");

AZ_NODISCARD az_result az_iot_hub_client_properties_get_reported_publish_topic(
    az_iot_hub_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return az_iot_hub_client_twin_patch_get_publish_topic(
      client, request_id, mqtt_topic, mqtt_topic_size, out_mqtt_topic_length);
}

AZ_NODISCARD az_result az_iot_hub_client_properties_document_get_publish_topic(
    az_iot_hub_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return az_iot_hub_client_twin_document_get_publish_topic(
      client, request_id, mqtt_topic, mqtt_topic_size, out_mqtt_topic_length);
}

AZ_NODISCARD az_result az_iot_hub_client_properties_parse_received_topic(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_properties_message* out_message)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_message);

  az_iot_hub_client_twin_response hub_twin_response;
  _az_RETURN_IF_FAILED(
      az_iot_hub_client_twin_parse_received_topic(client, received_topic, &hub_twin_response));

  out_message->request_id = hub_twin_response.request_id;
  out_message->message_type
      = (az_iot_hub_client_properties_message_type)hub_twin_response.response_type;
  out_message->status = hub_twin_response.status;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_properties_writer_begin_component(
    az_iot_hub_client const* client,
    az_json_writer* ref_json_writer,
    az_span component_name)
{
  _az_PRECONDITION_NOT_NULL(ref_json_writer);
  _az_PRECONDITION_VALID_SPAN(component_name, 1, false);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(ref_json_writer, component_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(ref_json_writer, component_properties_label_name));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_string(ref_json_writer, component_properties_label_value));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_properties_writer_end_component(
    az_iot_hub_client const* client,
    az_json_writer* ref_json_writer)
{
  _az_PRECONDITION_NOT_NULL(ref_json_writer);

  (void)client;

  return az_json_writer_append_end_object(ref_json_writer);
}

AZ_NODISCARD az_result az_iot_hub_client_properties_writer_begin_response_status(
    az_iot_hub_client const* client,
    az_json_writer* ref_json_writer,
    az_span property_name,
    int32_t status_code,
    int32_t version,
    az_span description)
{
  _az_PRECONDITION_NOT_NULL(ref_json_writer);
  _az_PRECONDITION_VALID_SPAN(property_name, 1, false);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(ref_json_writer, property_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(ref_json_writer, properties_ack_code_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(ref_json_writer, status_code));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(ref_json_writer, properties_ack_version_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(ref_json_writer, version));

  if (az_span_size(description) != 0)
  {
    _az_RETURN_IF_FAILED(
        az_json_writer_append_property_name(ref_json_writer, properties_ack_description_name));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(ref_json_writer, description));
  }

  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(ref_json_writer, properties_response_value_name));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_properties_writer_end_response_status(
    az_iot_hub_client const* client,
    az_json_writer* ref_json_writer)
{
  _az_PRECONDITION_NOT_NULL(ref_json_writer);

  (void)client;

  return az_json_writer_append_end_object(ref_json_writer);
}

// Move reader to the value of property name
static az_result json_child_token_move(az_json_reader* ref_jr, az_span property_name)
{
  do
  {
    if ((ref_jr->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
        && az_json_token_is_text_equal(&(ref_jr->token), property_name))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_jr));

      return AZ_OK;
    }
    else if (ref_jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(ref_jr)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (ref_jr->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  } while (az_result_succeeded(az_json_reader_next_token(ref_jr)));

  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result az_iot_hub_client_properties_get_properties_version(
    az_iot_hub_client const* client,
    az_json_reader* ref_json_reader,
    az_iot_hub_client_properties_message_type message_type,
    int32_t* out_version)
{
  _az_PRECONDITION_NOT_NULL(ref_json_reader);
  _az_PRECONDITION(
      (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED)
      || (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE));
  _az_PRECONDITION_NOT_NULL(out_version);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

  if (ref_json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

  if (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE)
  {
    _az_RETURN_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_properties_desired));
    _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
  }

  _az_RETURN_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_properties_desired_version));
  _az_RETURN_IF_FAILED(az_json_token_get_int32(&ref_json_reader->token, out_version));

  return AZ_OK;
}

/*
Assuming a JSON of either the below types

AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED:

{
  //ROOT COMPONENT or COMPONENT NAME section
  "component_one": {
    //PROPERTY VALUE section
    "prop_one": 1,
    "prop_two": "string"
  },
  "component_two": {
    "prop_three": 45,
    "prop_four": "string"
  },
  "not_component": 42,
  "$version": 5
}

AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE:

{
  "desired": {
    //ROOT COMPONENT or COMPONENT NAME section
    "component_one": {
        //PROPERTY VALUE section
        "prop_one": 1,
        "prop_two": "string"
    },
    "component_two": {
        "prop_three": 45,
        "prop_four": "string"
    },
    "not_component": 42,
    "$version": 5
  },
  "reported": {
      "manufacturer": "Sample-Manufacturer",
      "model": "pnp-sample-Model-123",
      "swVersion": "1.0.0.0",
      "osName": "Contoso"
  }
}

*/
AZ_NODISCARD az_result az_iot_hub_client_properties_get_next_component_property(
    az_iot_hub_client const* client,
    az_json_reader* ref_json_reader,
    az_iot_hub_client_properties_message_type message_type,
    az_iot_hub_client_property_type property_type,
    az_span* out_component_name)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_reader);
  _az_PRECONDITION_NOT_NULL(out_component_name);
  _az_PRECONDITION(
      (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED)
      || (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE));
  _az_PRECONDITION(
      (property_type == AZ_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE)
      || (property_type == AZ_IOT_HUB_CLIENT_PROPERTY_WRITABLE));
  _az_PRECONDITION(
      (property_type == AZ_IOT_HUB_CLIENT_PROPERTY_WRITABLE)
      || ((property_type == AZ_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE)
          && (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE)));

  return _az_iot_hub_client_properties_get_next_component_property(
      client->_internal.options.component_names,
      client->_internal.options.component_names_length,
      ref_json_reader,
      message_type,
      property_type,
      out_component_name);
}
