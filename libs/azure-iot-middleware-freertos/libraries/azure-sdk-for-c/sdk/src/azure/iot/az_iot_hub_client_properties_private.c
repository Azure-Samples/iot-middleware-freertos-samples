// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_iot_hub_client_properties_private.h"

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/iot/az_iot_hub_client.h>
#include <azure/iot/az_iot_hub_client_properties.h>

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

// verify_valid_json_position makes sure that the az_json_reader
// is in a good state.  Applications modify the az_json_reader as they
// traverse properties and a poorly written application could leave
// it in an invalid state.
static az_result verify_valid_json_position(
    az_json_reader* jr,
    az_iot_hub_client_properties_message_type message_type,
    az_span component_name)
{
  // Not on a property name or end of object
  if (jr->current_depth != 0
      && (jr->token.kind != AZ_JSON_TOKEN_PROPERTY_NAME
          && jr->token.kind != AZ_JSON_TOKEN_END_OBJECT))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  // Component property - In user property value object
  if ((message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
       && jr->current_depth > 2)
      || (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
          && jr->current_depth > 3))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  // Non-component property - In user property value object
  if ((az_span_size(component_name) == 0)
      && ((message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
           && jr->current_depth > 1)
          || (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
              && jr->current_depth > 2)))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  return AZ_OK;
}

// process_first_move_if_needed performs initial setup when beginning to parse
// the JSON document.  It sets the next read token to the appropriate
// location based on whether we have a full twin or a patch and what property_type
// the application requested.
// Returns AZ_OK if this is NOT the first read of the document.
static az_result process_first_move_if_needed(
    az_json_reader* jr,
    az_iot_hub_client_properties_message_type message_type,
    az_iot_hub_client_property_type property_type)

{
  if (jr->current_depth == 0)
  {
    _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

    if (jr->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }

    _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

    if (message_type == AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE)
    {
      const az_span property_to_query
          = (property_type == AZ_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE)
          ? iot_hub_properties_reported
          : iot_hub_properties_desired;
      _az_RETURN_IF_FAILED(json_child_token_move(jr, property_to_query));
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
    }
    return AZ_OK;
  }
  else
  {
    // Not the first move so continue
    return AZ_OK;
  }
}

// The underlying twin has various metadata embedded in the JSON.
// This metadata should not be passed back to the caller
// but should instead be silently ignored/skipped.
static az_result skip_metadata_if_needed(
    az_json_reader* jr,
    _az_iot_hub_client_properties_message_type message_type)
{
  while (true)
  {
    // Within the "root" or "component name" section
    if ((message_type == _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
         && jr->current_depth == 1)
        || (message_type == _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
            && jr->current_depth == 2))
    {
      if ((az_json_token_is_text_equal(&jr->token, iot_hub_properties_desired_version)))
      {
        // Skip version property name and property value
        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

        continue;
      }
      else
      {
        return AZ_OK;
      }
    }
    // Within the property value section
    else if (
        (message_type == _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
         && jr->current_depth == 2)
        || (message_type == _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
            && jr->current_depth == 3))
    {
      if (az_json_token_is_text_equal(&jr->token, component_properties_label_name))
      {
        // Skip label property name and property value
        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

        continue;
      }
      else
      {
        return AZ_OK;
      }
    }
    else
    {
      return AZ_OK;
    }
  }
}

// Check if the component name is in the model.  While this is sometimes
// indicated in the twin metadata (via "__t":"c" as a child), this metadata will NOT
// be specified during a TWIN PATCH operation.  Hence we cannot rely on it
// being present.  We instead use the application provided component_name list.
static bool is_component_in_model(
    az_span* component_names,
    int32_t component_names_length,
    az_json_token const* component_name,
    az_span* out_component_name)
{
  int32_t index = 0;

  while (index < component_names_length)
  {
    if (az_json_token_is_text_equal(component_name, component_names[index]))
    {
      *out_component_name = component_names[index];
      return true;
    }

    index++;
  }

  return false;
}

AZ_NODISCARD az_result _az_iot_hub_client_properties_get_next_component_property(
    az_span* component_names,
    int32_t component_names_length,
    az_json_reader* ref_json_reader,
    _az_iot_hub_client_properties_message_type message_type,
    _az_iot_hub_client_property_type property_type,
    az_span* out_component_name)
{
  _az_RETURN_IF_FAILED(
      verify_valid_json_position(ref_json_reader, message_type, *out_component_name));
  _az_RETURN_IF_FAILED(process_first_move_if_needed(ref_json_reader, message_type, property_type));

  while (true)
  {
    _az_RETURN_IF_FAILED(skip_metadata_if_needed(ref_json_reader, message_type));

    if (ref_json_reader->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      // We've read all the children of the current object we're traversing.
      if ((message_type == _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
           && ref_json_reader->current_depth == 0)
          || (message_type == _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
              && ref_json_reader->current_depth == 1))
      {
        // We've read the last child the root of the JSON tree we're traversing.  We're done.
        return AZ_ERROR_IOT_END_OF_PROPERTIES;
      }

      // There are additional tokens to read.  Continue.
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      continue;
    }

    break;
  }

  if ((message_type == _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
       && ref_json_reader->current_depth == 1)
      || (message_type == _AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
          && ref_json_reader->current_depth == 2))
  {
    // Retrieve the next property/component pair.
    if (is_component_in_model(
            component_names, component_names_length, &ref_json_reader->token, out_component_name))
    {
      // Properties that are children of components are simply modeled as JSON children
      // in the underlying twin.  Traverse into the object.
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      if (ref_json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      _az_RETURN_IF_FAILED(skip_metadata_if_needed(ref_json_reader, message_type));
    }
    else
    {
      // The current property is not the child of a component.
      *out_component_name = AZ_SPAN_EMPTY;
    }
  }

  return AZ_OK;
}
