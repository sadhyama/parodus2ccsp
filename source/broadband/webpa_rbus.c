#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include "webpa_rbus.h"

static rbusHandle_t rbus_handle;
static bool isRbus = false;

#define MAX_BUFFER_LEN        512
#define MAX_PARAM_KEY_LEN     64

bool isRbusEnabled()
{
        if(RBUS_ENABLED == rbus_checkStatus())
        {
                isRbus = true;
        }
        else
        {
                isRbus = false;
        }
        WalInfo("Webpa RBUS mode active status = %s\n", isRbus ? "true":"false");
        return isRbus;
}

bool isRbusInitialized()
{
    return rbus_handle != NULL ? true : false;
}

WDMP_STATUS webpaRbusInit(const char *pComponentName)
{
        int ret = RBUS_ERROR_SUCCESS;

        WalInfo("rbus_open for component %s\n", pComponentName);
        ret = rbus_open(&rbus_handle, pComponentName);
        if(ret != RBUS_ERROR_SUCCESS)
        {
                WalError("webpaRbusInit failed with error code %d\n", ret);
                return WDMP_FAILURE;
        }
        WalInfo("webpaRbusInit is success. ret is %d\n", ret);
        return WDMP_SUCCESS;
}

void webpaRbus_Uninit()
{
    rbus_close(rbus_handle);
}

rbusError_t setTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        if(isRbusInitialized)
        {
                if(traceContext[0] != NULL && traceContext[1] != NULL) {
                       if(strlen(traceContext[0]) > 0 && strlen(traceContext[1]) > 0) {
			    WalInfo("Invoked setTraceContext function with value traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);    
                            ret = rbusHandle_SetTraceContextFromString(rbus_handle, traceContext[0], traceContext[1]);
                            if(ret == RBUS_ERROR_SUCCESS) {
                                  WalPrint("SetTraceContext request success\n");
                            }
                             else {
                                   WalError("SetTraceContext request failed with error code - %d\n", ret);
                             }
                        }
                        else {
                              WalError("Header is empty\n");
                        }
                  }
                  else {
                        WalError("Header is NULL\n");
                  }
        }
        else {
                WalError("Rbus not initialzed in setTraceContext function\n");
        }	
        return ret;
}

rbusError_t getTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        char traceParent[MAX_BUFFER_LEN] = {'\0'};
        char traceState[MAX_BUFFER_LEN] = {'\0'};
	if(isRbusInitialized)
        {
	      ret =  rbusHandle_GetTraceContextAsString(rbus_handle, traceParent, sizeof(traceParent), traceState, sizeof(traceState));
	      if( ret == RBUS_ERROR_SUCCESS) {
		      if(strlen(traceParent) > 0 && strlen(traceState) > 0) {
			      WalPrint("GetTraceContext request success\n");
		              traceContext[0] = strdup(traceParent);
	                      traceContext[1] = strdup(traceState);
			      WalInfo("traceContext value, traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);
	               }
		       else {
			       WalPrint("traceParent & traceState are empty\n");
		       }	       
	      }
	      else {
		      WalError("GetTraceContext request failed with error code - %d\n", ret);
	      }	      
	}
        else { 
              WalError("Rbus not initialzed in getTraceContext function\n");
	}
        return ret;
}

rbusError_t clearTraceContext()
{
	rbusError_t ret = RBUS_ERROR_BUS_ERROR;
	if(isRbusInitialized)
	{
		ret = rbusHandle_ClearTraceContext(rbus_handle);
		if(ret == RBUS_ERROR_SUCCESS) {
			WalInfo("ClearTraceContext request success\n");
		}
		else {
			WalError("ClearTraceContext request failed with error code - %d\n", ret);
		}
	}
	else {
		WalError("Rbus not initialized in clearTraceContext funcion\n");
        }
}

/**
 * Register data elements for data model and methods implementation using rbus.
 */
void regWebpaDataModel()
{
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    if(!rbus_handle)
    {
        WalError("regWebpaDataModel failed in getting rbus handle\n");
        return;
    }
    
    rbusDataElement_t dataElements[2] = {
        {WEBPA_NOTIFY_SUBSCRIPTION_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NotifySubscriptionListGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {WEBPA_NOTIFY_SUBSCRIPTION_METHOD, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, NotifySubscriptionListMethodHandler}}
    };

	rc = rbus_regDataElements(rbus_handle, 2, dataElements);

    if(rc == RBUS_ERROR_SUCCESS)
    {
		WalInfo("Registered data elements: %s, %s with rbus\n", WEBPA_NOTIFY_SUBSCRIPTION_PARAM, WEBPA_NOTIFY_SUBSCRIPTION_METHOD);
    }
    else
	{
		WalError("Failed in registering data elements: %s, %s with rbus\n", WEBPA_NOTIFY_SUBSCRIPTION_PARAM, WEBPA_NOTIFY_SUBSCRIPTION_METHOD);
	}
	return;
}

rbusError_t NotifySubscriptionListGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    WalPrint("NotifySubscriptionListGetHandler is called\n");
    const char* paramName = NULL;

    paramName = rbusProperty_GetName(property);
    if(strncmp(paramName, WEBPA_NOTIFY_SUBSCRIPTION_PARAM, strlen(WEBPA_NOTIFY_SUBSCRIPTION_PARAM)) != 0)
    {
        WalError("Unexpected parameter: %s\n", paramName);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    char* buffer = CreateJsonFromGlobalNotifyList();

    if(buffer == NULL)
    {
        WalError("NotifySubscriptionListGetHandler: Failed to get notify param list.\n");
        return RBUS_ERROR_BUS_ERROR;
    }

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, buffer);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    WAL_FREE(buffer);
    return RBUS_ERROR_SUCCESS;
}

static void setRbusResponse(
    rbusObject_t outParams,
    const char* msgStr,
    NOTIFY_SUBSCRIPTION_STATUS_CODE status,
    cJSON* successArr,
    cJSON* failureArr)
{
    rbusValue_t message = NULL, statusCode = NULL;
    rbusValue_Init(&message);
    rbusValue_Init(&statusCode);

    cJSON* respObj = cJSON_CreateObject();
    if (respObj)
    {
        cJSON_AddStringToObject(respObj, "message", msgStr ? msgStr : "");
        if (successArr)
            cJSON_AddItemToObject(respObj, "success", successArr);
        if (failureArr)
            cJSON_AddItemToObject(respObj, "failure", failureArr);
        cJSON_AddNumberToObject(respObj, "statusCode", status);

        char* respStr = cJSON_PrintUnformatted(respObj);
        if (respStr)
        {
            rbusValue_SetString(message, respStr);
            rbusObject_SetValue(outParams, "message", message);
            WAL_FREE(respStr);
        }
        cJSON_Delete(respObj);
    }

    rbusValue_SetInt32(statusCode, status);
    rbusObject_SetValue(outParams, "statusCode", statusCode);

    rbusValue_Release(message);
    rbusValue_Release(statusCode);
}

static int validate_notify_params(rbusObject_t inParams, int paramCount, char *err_msg, size_t len)
{
    WalPrint("------------ validate_notify_params ----------\n");
    if (paramCount == 0)
    {
        snprintf(err_msg, len, "No parameters provided");
        return -1;
    }

    for (int i = 0; i < paramCount; i++)
    {
        char keyName[MAX_PARAM_KEY_LEN];
        snprintf(keyName, sizeof(keyName), "param%d", i);
        rbusValue_t paramVal = rbusObject_GetValue(inParams, keyName);
        if (!paramVal || rbusValue_GetType(paramVal) != RBUS_OBJECT)
        {
            WalError("Invalid method request format\n");
            snprintf(err_msg, len, "Invalid method request format");
            return -1;
        }

        rbusObject_t subObj = rbusValue_GetObject(paramVal);
        if (!subObj)
        {
            WalError("Invalid method request format\n");
            snprintf(err_msg, len, "Invalid method request format");
            return -1;
        }

        const char* name = NULL;
        const char* notifType = NULL;

        // Validate parameter name
        rbusValue_t valName = rbusObject_GetValue(subObj, "name");
        if (!valName)
        {
            WalError("Missing name field in method request\n");
            snprintf(err_msg, len, "Missing name field in method request");
            return -1;
        }
        else if (rbusValue_GetType(valName) != RBUS_STRING)
        {
            WalError("name is not a string in method request\n");
            snprintf(err_msg, len, "name is not a string in method request");
            return -1;
        }
        name = rbusValue_GetString(valName, NULL);

        if (!name || !*name)
        {
            WalError("Parameter name is Empty/NULL\n");
            snprintf(err_msg, len, "Parameter name is Empty/NULL");
            return -1;
        }

        // Validate parameter type
        rbusValue_t valType = rbusObject_GetValue(subObj, "notificationType");
        if (!valType)
        {
            WalError("Missing notificationType field in method request\n");
            snprintf(err_msg, len, "Missing notificationType field in method request");
            return -1;
        }
        else if (rbusValue_GetType(valType) != RBUS_STRING)
        {
            WalError("notificationType is not a string in method request\n");
            snprintf(err_msg, len, "notificationType is not a string in method request");
            return -1;
        }
        notifType = rbusValue_GetString(valType, NULL);

        if(!notifType || !*notifType)
        {
            WalError("NotifyType is Empty/NULL\n");
            snprintf(err_msg, len, "NotifyType is Empty/NULL");
            return -1;
        }
    }
    return 0;
}

rbusError_t NotifySubscriptionListMethodHandler(
    rbusHandle_t handle,
    const char* methodName,
    rbusObject_t inParams,
    rbusObject_t outParams,
    rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;

    if(!methodName || strncmp(methodName, WEBPA_NOTIFY_SUBSCRIPTION_METHOD, strlen(WEBPA_NOTIFY_SUBSCRIPTION_METHOD)) != 0)
    {
        WalError("Unexpected method: %s\n", methodName);
        setRbusResponse(outParams, "Unexpected method", NOTIFY_SUBSCRIPTION_FAILURE, NULL, NULL);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST; 
    }

    WalInfo("Received method request for dynamic notification subscription %s\n", methodName);

    if(!getBootupNotifyInitDone())
    {
        WalInfo("Notification setup during Bootup is in Progress, rejecting method request\n");
        setRbusResponse(outParams, "Notification setup during Bootup is in Progress, rejecting method request", NOTIFY_SUBSCRIPTION_BOOTUP_IN_PROGRESS, NULL, NULL);
        return RBUS_ERROR_BUS_ERROR;
    }

    int failureCount = 0, successCount = 0, invalidCount = 0;
    cJSON *successArr = cJSON_CreateArray();
    cJSON *failureArr = cJSON_CreateArray();
    char err_msg[MAX_BUFFER_LEN] = {0};
    char resp_str[MAX_BUFFER_LEN] = {0};

    // Extract inParams
    rbusProperty_t prop = rbusObject_GetProperties(inParams);
    int paramCount = prop ? rbusProperty_Count(prop) : 0;

    WalInfo("Number of parameters received for subscription %d\n", paramCount);

    int ret = validate_notify_params(inParams, paramCount, err_msg,  sizeof(err_msg));

    if(ret == 0)
    {
        WalInfo("Method request parameter validation is successful\n");
        for (int i = 0; i < paramCount; i++)
        {
            char keyName[MAX_BUFFER_LEN] = {0};
            snprintf(keyName, sizeof(keyName), "param%d", i);
            rbusValue_t paramVal = rbusObject_GetValue(inParams, keyName);
            rbusObject_t subObj = rbusValue_GetObject(paramVal);

            const char* name = NULL;
            const char* notifType = NULL;

            name = rbusValue_GetString(rbusObject_GetValue(subObj, "name"), NULL);
            notifType = rbusValue_GetString(rbusObject_GetValue(subObj, "notificationType"), NULL);

            WalInfo("Processing parameter %s for subscription\n", name ? name : "");

            if (notifType && strcmp(notifType, "ValueChange") == 0)
            {
                g_NotifyParam *node = searchParaminGlobalList(name);
                if(!node || getParamStatus(node) == OFF)
                {
                    WDMP_STATUS wret = WDMP_FAILURE;
                    char *wmsg = NULL;
                    wmsg = (char *) malloc(sizeof(char) * MAX_BUFFER_LEN);
                    if(!wmsg)
                    {
                        WalError("malloc failed for wmsg\n");
                        return RBUS_ERROR_BUS_ERROR;
                    }
                    param_t att = {0};
                    att.name = strdup(name);
                    att.value = strdup("1");
                    att.type = WDMP_INT;

                    // Turn ON notification via setAttributes
                    setAttributes(&att, 1, NULL, &wret);
                    mapWdmpStatusToStatusMessage(wret, wmsg);

                    if (wret == WDMP_SUCCESS)
                    {
                        WalInfo("Successfully subscribed notification for parameter %s\n", att.name);
                        cJSON_AddItemToArray(successArr, cJSON_CreateString(name));
                        successCount++;

                        if (!node)
                        {
                            WalInfo("Adding parameter %s to global notify list\n", name);
                            addParamToGlobalList(att.name, DYNAMIC_PARAM, ON);
                            if (writeDynamicParamToDBFile(name))
                                WalPrint("Added parameter %s to dynamic notify DB file\n", att.name);
                            else
                                WalError("Write to dynamic notify DB file failed for %s\n", name);
                        }
                        else
                        {
                            WalInfo("Parameter %s found in global notify list. Updating status to ON.\n", name);
                            updateParamStatus(node, ON);
                        }
                    }
                    else
                    {
                        WalError("Failed to subscribe notification for parameter %s. %s\n", att.name, wmsg ? wmsg : "unknown");
                        snprintf(resp_str, sizeof(resp_str), "Failed to subscribe notification. %s", wmsg ? wmsg : "unknown");
                        cJSON *failObj = cJSON_CreateObject();
                        cJSON_AddStringToObject(failObj, "name", name);
                        cJSON_AddStringToObject(failObj, "reason", resp_str);
                        cJSON_AddItemToArray(failureArr, failObj);
                        failureCount++;
                    }
                    WAL_FREE(att.name);
                    WAL_FREE(att.value);
                    WAL_FREE(wmsg);
                }
                else if(getParamStatus(node) == ON)
                {
                    // Already subscribed for notification. Sending success
                    WalInfo("Subscription already exists for parameter %s\n", name);
                    cJSON_AddItemToArray(successArr, cJSON_CreateString(name));
                    successCount++;
                }
            }
            else
            {
                WalError("Notification type is not supported: %s\n", notifType);
                cJSON *failObj = cJSON_CreateObject();
                cJSON_AddStringToObject(failObj, "parameter", name);
                cJSON_AddStringToObject(failObj, "reason", "Notification type is not supported");
                cJSON_AddItemToArray(failureArr, failObj);
                failureCount++;
                invalidCount++;
            }
        }
    }
    else
    {
        WalError("Parameter validation for method request failed. %s\n", err_msg);
        cJSON_Delete(successArr);
        cJSON_Delete(failureArr);
        setRbusResponse(outParams, err_msg, NOTIFY_SUBSCRIPTION_INVALID_INPUT, NULL, NULL);
        return RBUS_ERROR_INVALID_INPUT;
    }

    // Create JSON response for success, failure, and partial success parameters
    const int total = successCount + failureCount;
    const int isAllSuccess = (failureCount == 0 && total > 0);
    const int isAllFailure = (successCount == 0 && total > 0);
    const int isPartial    = (!isAllSuccess && !isAllFailure);

    const char* msgStr = NULL;
    NOTIFY_SUBSCRIPTION_STATUS_CODE notifyStatus = NOTIFY_SUBSCRIPTION_FAILURE;

    if (isAllSuccess)
    {
        msgStr = strdup("Subscriptions Success");
        notifyStatus = NOTIFY_SUBSCRIPTION_SUCCESS;
    }
    else if (isAllFailure)
    {
        if (invalidCount == failureCount)
        {
            msgStr = strdup("Notification type is not supported");
            notifyStatus = NOTIFY_SUBSCRIPTION_INVALID_INPUT;
        }
        else
        {
            if (strlen(resp_str) > 0)
            {
                msgStr = strdup(resp_str);
            }
            else
            {
                msgStr = strdup("Subscriptions failed");
            }
            notifyStatus = NOTIFY_SUBSCRIPTION_FAILURE;
        }
    }
    else
    {
        msgStr = strdup("Partial success");
        notifyStatus = NOTIFY_SUBSCRIPTION_MULTI_STATUS;
    }

    if (isPartial)
    {
        setRbusResponse(outParams, msgStr, notifyStatus, successArr, failureArr);
    }
    else
    {
        setRbusResponse(outParams, msgStr, notifyStatus, NULL, NULL);
        cJSON_Delete(successArr);
        cJSON_Delete(failureArr);
    }

    rbusValue_t messageVal = rbusObject_GetValue(outParams, "message");
    const char* respMsgStr = messageVal ? rbusValue_GetString(messageVal, NULL) : "";

    if(msgStr) WAL_FREE(msgStr);
    WalInfo("Subscription method request completed: %s\n", respMsgStr);
    return RBUS_ERROR_SUCCESS;
}

