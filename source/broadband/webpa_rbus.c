#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include <cJSON.h>
#include "webpa_rbus.h"

static rbusHandle_t rbus_handle;
static bool isRbus = false;



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
        char traceParent[512] = {'\0'};
        char traceState[512] = {'\0'};
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
        {WEBPA_NOTIFY_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NotifySubscriptionListGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {WEBPA_NOTIFY_SUBSCRIPTION, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, NotifySubscriptionListMethodHandler}}
    };

	rc = rbus_regDataElements(rbus_handle, 2, dataElements);

    if(rc == RBUS_ERROR_SUCCESS)
    {
		WalInfo("Registered data elements: %s, %s with rbus\n", WEBPA_NOTIFY_PARAM, WEBPA_NOTIFY_SUBSCRIPTION);
    }
    else
	{
		WalError("Failed in registering data elements: %s, %s with rbus\n", WEBPA_NOTIFY_PARAM, WEBPA_NOTIFY_SUBSCRIPTION);
	}
	return;
}

rbusError_t NotifySubscriptionListGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    WalInfo("NotifySubscriptionListGetHandler is called\n");
    const char* paramName = NULL;

    paramName = rbusProperty_GetName(property);
    if(strncmp(paramName, WEBPA_NOTIFY_PARAM, strlen(WEBPA_NOTIFY_PARAM)) != 0)
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
    NOTIFY_EVENT_STATUS_CODE status,
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
            free(respStr);
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
    WalInfo("------------ validate_notify_params ----------\n");
    if (paramCount == 0)
    {
        snprintf(err_msg, len, "No parameters provided");
        return -1;
    }

    for (int i = 0; i < paramCount; i++)
    {
        char keyName[64];
        snprintf(keyName, sizeof(keyName), "param%d", i);
        rbusValue_t paramVal = rbusObject_GetValue(inParams, keyName);
        if (!paramVal || rbusValue_GetType(paramVal) != RBUS_OBJECT)
        {
            snprintf(err_msg, len, "Invalid or Missing Structure");
            return -1;
        }

        rbusObject_t subObj = rbusValue_GetObject(paramVal);
        if (!subObj)
        {
            snprintf(err_msg, len, "Invalid or Missing SubObject structure");
            return -1;
        }

        const char* name = NULL;
        const char* notifType = NULL;

        rbusValue_t val = rbusObject_GetValue(subObj, "name");
        if (val && rbusValue_GetType(val) == RBUS_STRING)
            name = rbusValue_GetString(val, NULL);

        val = rbusObject_GetValue(subObj, "notificationType");
        if (val && rbusValue_GetType(val) == RBUS_STRING)
            notifType = rbusValue_GetString(val, NULL);

        if (!name || !*name || !notifType || !*notifType)
        {
            snprintf(err_msg, len, "Parameter name or type is Empty/NULL");
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
    (void)methodName;
    (void)asyncHandle;

    /* Reject subscription while device bootup */
    if(!getBootupNotifyInitDone())
    {
        WalInfo("Notification setup during Bootup is in Progress\n");
        setRbusResponse(outParams, "Notification setup during Bootup is in Progress", NOTIFY_EVENT_ERR_BOOTUP_IN_PROGRESS, NULL, NULL);
        return RBUS_ERROR_BUS_ERROR;
    }

    int failureCount = 0, successCount = 0, invalidCount = 0;
    cJSON *successArr = cJSON_CreateArray();
    cJSON *failureArr = cJSON_CreateArray();

    /* Extract inParams */
    rbusProperty_t prop = rbusObject_GetProperties(inParams);
    int paramCount = prop ? rbusProperty_Count(prop) : 0;

    WalInfo("No. of parameters received to subscribe: %d\n", paramCount);

    char err_msg[256] = {0};
    int ret = validate_notify_params(inParams, paramCount, err_msg,  sizeof(err_msg));

    if(ret == 0)
    {
        WalInfo("All parameters are valid\n");
        for (int i = 0; i < paramCount; i++)
        {
            char keyName[64];
            snprintf(keyName, sizeof(keyName), "param%d", i);
            rbusValue_t paramVal = rbusObject_GetValue(inParams, keyName);
            rbusObject_t subObj = rbusValue_GetObject(paramVal);

            const char* name = NULL;
            const char* notifType = NULL;

            name = rbusValue_GetString(rbusObject_GetValue(subObj, "name"), NULL);
            notifType = rbusValue_GetString(rbusObject_GetValue(subObj, "notificationType"), NULL);

            if (notifType && strcmp(notifType, "ValueChange") == 0)
            {
                g_NotifyParam *node = searchParaminGlobalList(name);
                if(!node || node->paramSubscriptionStatus == OFF)
                {
                    WDMP_STATUS wret = WDMP_FAILURE;
                    param_t att = {0};
                    att.name = strdup(name);
                    att.value = strdup("1");
                    att.type = WDMP_INT;

                    /* Turn ON notification via setAttributes */
                    setAttributes(&att, 1, NULL, &wret);

                    if (wret == WDMP_SUCCESS)
                    {
                        if (!node)
                        {
                            WalInfo("Adding parameter: %s to global list\n", name);
                            addParamToGlobalList(att.name, DYNAMIC_PARAM, ON);
                            if (writeDynamicParamToDBFile(name))
                                WalInfo("Added parameter: %s to Dynamic Notify DB File\n", att.name);
                            else
                                WalError("Write to DB file failed for %s\n", name);
                        }
                        else
                        {
                            WalInfo("parameter: %s is found. Setting status to ON\n", name);
                            pthread_mutex_lock(&g_NotifyParamMut);
                            node->paramSubscriptionStatus = ON;
                            pthread_mutex_unlock(&g_NotifyParamMut);
                        }

                        WalInfo("Successfully set notification ON for parameter : %s ret: %d\n", att.name, (int)wret);
                        cJSON_AddItemToArray(successArr, cJSON_CreateString(name));
                        successCount++;
                    }
                    else
                    {
                        WalError("Failed to turn notification ON for parameter : %s ret: %d\n", att.name, (int)wret);
                        cJSON *failObj = cJSON_CreateObject();
                        cJSON_AddStringToObject(failObj, "parameter", name);
                        cJSON_AddStringToObject(failObj, "reason", "setAttributes failed for parameter");
                        cJSON_AddItemToArray(failureArr, failObj);
                        failureCount++;
                    }
                    WAL_FREE(att.name);
                    WAL_FREE(att.value);
                }
                else if(node->paramSubscriptionStatus == ON)
                {
                    /* Already subscribed: treat as success for simplicity */
                    WalInfo("Parameter: %s is subscribed already\n", name);
                    cJSON_AddItemToArray(successArr, cJSON_CreateString(name));
                    successCount++;
                    continue;
                }
            }
            else
            {
                WalError("Unsupported notification type: %s\n", notifType);
                cJSON *failObj = cJSON_CreateObject();
                cJSON_AddStringToObject(failObj, "parameter", name);
                cJSON_AddStringToObject(failObj, "reason", "Unsupported notification type");
                cJSON_AddItemToArray(failureArr, failObj);
                failureCount++;
                invalidCount++;
                continue;
            }
        }
    }
    else
    {
        WalError("Parameter validation failed. err: %s\n", err_msg);
        cJSON_Delete(successArr);
        cJSON_Delete(failureArr);
        setRbusResponse(outParams, err_msg, NOTIFY_EVENT_ERR_INVALID_INPUT, NULL, NULL);
        return RBUS_ERROR_INVALID_INPUT;
    }

    /* message and structure */
    const int total = successCount + failureCount;
    const int isAllSuccess = (failureCount == 0 && total > 0);
    const int isAllFailure = (successCount == 0 && total > 0);
    const int isPartial    = (!isAllSuccess && !isAllFailure);

    const char* msgStr;
    NOTIFY_EVENT_STATUS_CODE notifyStatus = NOTIFY_EVENT_FAILURE; // Default to server failure

    if (isAllSuccess)
    {
        msgStr = "Subscriptions Success";
        notifyStatus = NOTIFY_EVENT_SUCCESS; 
    }
    else if (isAllFailure)
    {
        if (invalidCount == failureCount)
        {
            msgStr = "Unsupported notification type";
            notifyStatus = NOTIFY_EVENT_ERR_INVALID_INPUT;
        }
        else
        {
            msgStr = "Subscriptions failed";
            notifyStatus = NOTIFY_EVENT_FAILURE;
        }
    }
    else
    {
        msgStr = "Partial success";
        notifyStatus = NOTIFY_EVENT_MULTI_STATUS;
    }

    /* Build JSON response */
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
    WalInfo("NotifySubscriptionListMethodHandler completed: %s (Status: %d)\n", respMsgStr, notifyStatus);
    return RBUS_ERROR_SUCCESS;
}
