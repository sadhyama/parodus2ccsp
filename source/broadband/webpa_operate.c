/**
 * @file webpa_operate.c
 *
 * @description OPERATE dispatch module for parodus2ccsp.
 *
 *  When processRequest() receives a SET with dataType == WDMP_APPLICATION (13)
 *  and name == "RDK.Operate", it delegates to processOperateRequest() here.
 *
 *  This module:
 *    1. Base64-decodes the payload using trower-base64.
 *    2. Parses the JSON to extract "method" and "params".
 *    3. Builds an rbusObject_t inParams from the params map.
 *    4. Calls rbusMethod_Invoke() synchronously.
 *    5. Serialises outParams as a flat JSON object and base64-encodes it.
 *    6. Returns the encoded result via responseMessage.
 *
 * Copyright (c) 2024 Comcast
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <cJSON.h>
#include <trower-base64/base64.h>
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>

#include <wdmp-c.h>
#include "webpa_internal.h"
#include "webpa_rbus.h"
#include "webpa_operate.h"

/*----------------------------------------------------------------------------*/
/*                             Internal helpers                               */
/*----------------------------------------------------------------------------*/

/**
 * @brief Map a WDMP DATA_TYPE integer to the corresponding rbusValueType_t.
 *        Returns -1 for unsupported types.
 */
static int wdmp_to_rbus_type(int wdmp_type, rbusValueType_t *out_type)
{
    switch (wdmp_type)
    {
        case WDMP_STRING:  *out_type = RBUS_STRING;  return 0;
        case WDMP_INT:     *out_type = RBUS_INT32;   return 0;
        case WDMP_UINT:    *out_type = RBUS_UINT32;  return 0;
        case WDMP_BOOLEAN: *out_type = RBUS_BOOLEAN; return 0;
        case WDMP_LONG:    *out_type = RBUS_INT64;   return 0;
        case WDMP_ULONG:   *out_type = RBUS_UINT64;  return 0;
        case WDMP_FLOAT:   *out_type = RBUS_SINGLE;  return 0;
        case WDMP_DOUBLE:  *out_type = RBUS_DOUBLE;  return 0;
        default:           return -1;
    }
}

/**
 * @brief Set a typed value on an rbusValue_t from a cJSON string representation.
 */
static int set_rbus_value_from_json(rbusValue_t val, rbusValueType_t rbus_type,
                                     const char *str_value)
{
    char *endptr = NULL;
    switch (rbus_type)
    {
        case RBUS_STRING:
            rbusValue_SetString(val, str_value);
            break;
        case RBUS_INT32:
            rbusValue_SetInt32(val, (int32_t)strtol(str_value, &endptr, 10));
            break;
        case RBUS_UINT32:
            rbusValue_SetUInt32(val, (uint32_t)strtoul(str_value, &endptr, 10));
            break;
        case RBUS_BOOLEAN:
        {
            bool b = (strcmp(str_value, "true") == 0 || strcmp(str_value, "1") == 0);
            rbusValue_SetBoolean(val, b);
            break;
        }
        case RBUS_INT64:
            rbusValue_SetInt64(val, (int64_t)strtoll(str_value, &endptr, 10));
            break;
        case RBUS_UINT64:
            rbusValue_SetUInt64(val, (uint64_t)strtoull(str_value, &endptr, 10));
            break;
        case RBUS_SINGLE:
            rbusValue_SetSingle(val, (float)strtof(str_value, &endptr));
            break;
        case RBUS_DOUBLE:
            rbusValue_SetDouble(val, strtod(str_value, &endptr));
            break;
        default:
            return -1;
    }
    return 0;
}

/**
 * @brief Map rbusError_t to the appropriate WDMP_STATUS.
 */
static WDMP_STATUS rbus_error_to_wdmp(rbusError_t rbus_err)
{
    switch (rbus_err)
    {
        case RBUS_ERROR_SUCCESS:        return WDMP_SUCCESS;
        case RBUS_ERROR_TIMEOUT:        return WDMP_ERR_TIMEOUT;
        case RBUS_ERROR_INVALID_METHOD: return WDMP_ERR_METHOD_NOT_SUPPORTED;
        default:                        return WDMP_FAILURE;
    }
}

/**
 * @brief Serialise a flat rbusObject_t as a JSON object string.
 *        Each top-level property's value is stored as its string representation.
 *        Caller must cJSON_free / free the returned string.
 *        Returns NULL on failure.
 */
static char *serialise_out_params(rbusObject_t outParams)
{
    cJSON *root = cJSON_CreateObject();
    if (!root)
        return NULL;

    if (outParams != NULL)
    {
        rbusProperty_t prop = rbusObject_GetProperties(outParams);
        while (prop != NULL)
        {
            const char *name = rbusProperty_GetName(prop);
            rbusValue_t  val  = rbusProperty_GetValue(prop);
            if (name && val)
            {
                char buf[512];
                char *str = rbusValue_ToString(val, buf, sizeof(buf));
                cJSON_AddStringToObject(root, name, str ? str : "");
            }
            prop = rbusProperty_GetNext(prop);
        }
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

/*----------------------------------------------------------------------------*/
/*                             Public function                                */
/*----------------------------------------------------------------------------*/

WDMP_STATUS processOperateRequest(const param_t *param, char **responseMessage)
{
    WDMP_STATUS ret        = WDMP_FAILURE;
    uint8_t    *decoded    = NULL;
    cJSON      *root       = NULL;
    rbusObject_t inParams  = NULL;
    rbusObject_t outParams = NULL;
    char       *method     = NULL;
    char       *json_str   = NULL;
    uint8_t    *encoded    = NULL;

    if (responseMessage)
        *responseMessage = NULL;

    if (!param || !param->value || !responseMessage)
    {
        WalError("processOperateRequest: NULL input\n");
        return WDMP_ERR_INVALID_PARAMETER_VALUE;
    }

    /* ------------------------------------------------------------------ */
    /* 1. Base64-decode the payload                                        */
    /* ------------------------------------------------------------------ */
    size_t encoded_len = strlen(param->value);
    size_t decoded_size = b64_get_decoded_buffer_size(encoded_len);
    decoded = (uint8_t *)malloc(decoded_size + 1);
    if (!decoded)
    {
        WalError("processOperateRequest: malloc failed for decoded buffer\n");
        return WDMP_FAILURE;
    }

    size_t decoded_len = b64_decode((const uint8_t *)param->value, encoded_len, decoded);
    if (decoded_len == 0)
    {
        WalError("processOperateRequest: base64 decode failed\n");
        free(decoded);
        return WDMP_ERR_INVALID_PARAMETER_VALUE;
    }
    decoded[decoded_len] = '\0';

    /* ------------------------------------------------------------------ */
    /* 2. Parse JSON and extract "method" and "params"                     */
    /* ------------------------------------------------------------------ */
    root = cJSON_ParseWithLength((const char *)decoded, decoded_len);
    free(decoded);
    decoded = NULL;

    if (!root)
    {
        WalError("processOperateRequest: JSON parse failed\n");
        return WDMP_ERR_INVALID_PARAMETER_VALUE;
    }

    cJSON *method_item = cJSON_GetObjectItem(root, "method");
    if (!method_item || !cJSON_IsString(method_item) ||
        !method_item->valuestring || method_item->valuestring[0] == '\0')
    {
        WalError("processOperateRequest: missing or invalid 'method' field\n");
        cJSON_Delete(root);
        return WDMP_ERR_INVALID_PARAMETER_VALUE;
    }
    method = method_item->valuestring;

    /* ------------------------------------------------------------------ */
    /* 3. Build rbusObject inParams from the optional "params" object      */
    /* ------------------------------------------------------------------ */
    rbusObject_Init(&inParams, NULL);

    cJSON *params_obj = cJSON_GetObjectItem(root, "params");
    if (params_obj && cJSON_IsObject(params_obj))
    {
        cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, params_obj)
        {
            const char *key = entry->string;
            cJSON *value_item    = cJSON_GetObjectItem(entry, "value");
            cJSON *datatype_item = cJSON_GetObjectItem(entry, "dataType");

            if (!key || !value_item || !datatype_item ||
                !cJSON_IsNumber(datatype_item))
            {
                WalError("processOperateRequest: malformed param entry for key '%s'\n",
                         key ? key : "<null>");
                rbusObject_Release(inParams);
                cJSON_Delete(root);
                return WDMP_ERR_INVALID_PARAMETER_VALUE;
            }

            int wdmp_type = datatype_item->valueint;
            rbusValueType_t rbus_type;
            if (wdmp_to_rbus_type(wdmp_type, &rbus_type) != 0)
            {
                WalError("processOperateRequest: unsupported dataType %d for param '%s'\n",
                         wdmp_type, key);
                rbusObject_Release(inParams);
                cJSON_Delete(root);
                return WDMP_ERR_UNSUPPORTED_DATATYPE;
            }

            /* Convert the value to string for uniform handling */
            char *val_str = NULL;
            if (cJSON_IsString(value_item))
            {
                val_str = value_item->valuestring;
            }
            else
            {
                /* For non-string JSON types, print to a temporary buffer */
                val_str = cJSON_PrintUnformatted(value_item);
            }

            rbusValue_t rval;
            rbusValue_Init(&rval);
            if (set_rbus_value_from_json(rval, rbus_type,
                                          val_str ? val_str : "") != 0)
            {
                WalError("processOperateRequest: failed to set rbus value for param '%s'\n", key);
                if (!cJSON_IsString(value_item) && val_str)
                    free(val_str);
                rbusValue_Release(rval);
                rbusObject_Release(inParams);
                cJSON_Delete(root);
                return WDMP_ERR_INVALID_PARAMETER_VALUE;
            }

            if (!cJSON_IsString(value_item) && val_str)
                free(val_str);

            rbusObject_SetValue(inParams, key, rval);
            rbusValue_Release(rval);
        }
    }

    /* ------------------------------------------------------------------ */
    /* 4. Invoke the RBUS method synchronously                             */
    /* ------------------------------------------------------------------ */
    rbusHandle_t handle = getRbusHandle();
    rbusError_t  rbus_err = rbusMethod_Invoke(handle, method, inParams, &outParams);

    if (rbus_err == RBUS_ERROR_TIMEOUT)
    {
        WalError("processOperateRequest: rbusMethod_Invoke timed out for method '%s'\n", method);
    }
    else if (rbus_err != RBUS_ERROR_SUCCESS)
    {
        WalError("processOperateRequest: rbusMethod_Invoke failed with error %d for method '%s'\n",
                 rbus_err, method);
    }

    ret = rbus_error_to_wdmp(rbus_err);

    rbusObject_Release(inParams);
    inParams = NULL;

    if (ret != WDMP_SUCCESS)
    {
        if (outParams)
            rbusObject_Release(outParams);
        cJSON_Delete(root);
        return ret;
    }

    /* ------------------------------------------------------------------ */
    /* 5. Serialise outParams to JSON then base64-encode                   */
    /* ------------------------------------------------------------------ */
    json_str = serialise_out_params(outParams);
    if (outParams)
    {
        rbusObject_Release(outParams);
        outParams = NULL;
    }
    cJSON_Delete(root);
    root = NULL;

    if (!json_str)
    {
        WalError("processOperateRequest: failed to serialise outParams\n");
        return WDMP_FAILURE;
    }

    size_t json_len   = strlen(json_str);
    size_t enc_size   = b64_get_encoded_buffer_size(json_len);
    encoded = (uint8_t *)malloc(enc_size + 1);
    if (!encoded)
    {
        WalError("processOperateRequest: malloc failed for encoded buffer\n");
        free(json_str);
        return WDMP_FAILURE;
    }

    b64_encode((const uint8_t *)json_str, json_len, encoded);
    free(json_str);
    json_str = NULL;

    encoded[enc_size] = '\0';
    *responseMessage = (char *)encoded;

    WalInfo("processOperateRequest: method '%s' invoked successfully\n", method);
    return WDMP_SUCCESS;
}
