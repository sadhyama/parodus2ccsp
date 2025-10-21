#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
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
        {WEBPA_NOTIFY_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NotifyParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {WEBPA_NOTIFY_SUBSCRIPTION, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, NotifyParamMethodHandler}}
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

rbusError_t NotifyParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    WalInfo("NotifyParamGetHandler is called\n");
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
        WalError("NotifyParamGetHandler: Failed to get notify param list.\n");
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

rbusError_t NotifyParamMethodHandler(
    rbusHandle_t handle,
    const char* methodName,
    rbusObject_t inParams,
    rbusObject_t outParams,
    rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)methodName;
    (void)asyncHandle;

    WalPrint("NotifyParamMethodHandler invoked\n");

    rbusValue_t message, statusCode;
    message = statusCode = NULL;
    WDMP_STATUS wret = WDMP_FAILURE;
    NOTIFY_SUBSCRIPTION_STATUS_CODE notifyStatus = NOTIFY_SUBSCRIPTION_FAILURE;
    int prop_count  = 0, failureCount = 0, successCount = 0, alreadySubscribedCount = 0;

    rbusValue_Init(&message);
    rbusValue_Init(&statusCode);

    if(getBootupNotifyInProgress())
    {
        notifyStatus = NOTIFY_SUBSCRIPTION_BOOTUP_IN_PROGRESS;
        WalInfo("Notification setup during bootup is in Progress\n");
        rbusValue_SetString(message, "Notification setup during Bbotup is in Progress.");
        rbusValue_SetInt32(statusCode, notifyStatus);
        rbusObject_SetValue(outParams, "message", message);
        rbusObject_SetValue(outParams, "statusCode", statusCode);
        rbusValue_Release(message); rbusValue_Release(statusCode);
        return RBUS_ERROR_BUS_ERROR;
    }
    WalInfo("Bootup is completed. Proceeding with dynamic subscribptions.\n");

/* Extract inParams */
    rbusProperty_t props = rbusObject_GetProperties(inParams);
    prop_count = props ? rbusProperty_Count(props) : 0;

    if (prop_count == 0)
    {
        notifyStatus = NOTIFY_SUBSCRIPTION_INVALID_INPUT;
        WalError("No parameters provided\n");
        rbusValue_SetString(message, "No parameters provided");
        rbusValue_SetInt32(statusCode, notifyStatus);
        rbusObject_SetValue(outParams, "message", message);
        rbusObject_SetValue(outParams, "statusCode", statusCode);
        rbusValue_Release(message); rbusValue_Release(statusCode);
        return RBUS_ERROR_INVALID_INPUT;
    }

    WalInfo("No. of parameters received to be subscribed: %d\n", prop_count);

    size_t allocSize = prop_count * 128;
    char *successBuf = calloc(1, allocSize);
    char *failedBuf  = calloc(1, allocSize);
    if (!successBuf || !failedBuf) {
        WalError("malloc failed for buffers\n");
        return RBUS_ERROR_BUS_ERROR;
    }

    for (int i = 0; i < prop_count; i++)
    {
        char keyName[64];
        param_t att;
        memset(&att, 0, sizeof(att));
        notifyStatus = NOTIFY_SUBSCRIPTION_FAILURE ;
        snprintf(keyName, sizeof(keyName), "param%d", i);
        rbusValue_t paramVal = rbusObject_GetValue(inParams, keyName);
        if (!paramVal || rbusValue_GetType(paramVal) != RBUS_OBJECT)
        {
            WalError("Missing/Invalid object structure for %s\n", keyName);
            continue;
        }
        rbusObject_t subObj = rbusValue_GetObject(paramVal);
        if (!subObj)
        {
            WalError("Invalid object structure for %s\n", keyName);
            continue;
        }

        rbusValue_t val = NULL;
        const char* name = NULL;
        const char* notifType = NULL;

        val = rbusObject_GetValue(subObj, "name");
        if (val && rbusValue_GetType(val) == RBUS_STRING)
            name = rbusValue_GetString(val, NULL);

        val = rbusObject_GetValue(subObj, "notificationType");
        if (val && rbusValue_GetType(val) == RBUS_STRING)
            notifType = rbusValue_GetString(val, NULL);

        WalInfo("%s: name=%s, notificationType=%s\n", keyName, name ? name : "NULL", notifType ? notifType : "NULL");
        if (!name || !*name || !notifType || strcmp(notifType, "ValueChange") != 0 || strncmp(name, "Device.", 7) == 0)
        {
            WalError("Invalid or unsupported subscription entry: name='%s', notificationType='%s'\n", name ? name : "NULL", notifType ? notifType : "NULL");
            if (failedBuf[0] != '\0') {
                strncat(failedBuf, ", ", allocSize - strlen(failedBuf) - 1);
            }
            strncat(failedBuf, (name && *name) ? name : "NULL", allocSize - strlen(failedBuf) - 1);
            failureCount++;
            continue;
        }

        g_NotifyParam *node = searchParaminGlobalList(name);
        if(!node || node->paramSubscriptionStatus == OFF)
        {
            att.name = strdup(name);
            att.value = strdup("1");
            att.type = WDMP_INT;
            setAttributes(&att, 1, NULL, &wret);
            if (wret == WDMP_SUCCESS)
            {
                if (!node)
                {
                    WalInfo("parameter: %s is not found in the globallist. Adding.\n", name);
                    addParamToGlobalList(att.name, DYNAMIC_PARAM, ON);
                }
                else
                {
                    WalInfo("parameter: %s is found in the globallist. Turning on.\n", name);
                    pthread_mutex_lock(&g_NotifyParamMut);
                    node->paramSubscriptionStatus = ON;
                    pthread_mutex_unlock(&g_NotifyParamMut);
                }

                WalInfo("Successfully set notification ON for parameter : %s ret: %d\n", att.name, (int)wret);

                if (successBuf[0] != '\0') {
                    strncat(successBuf, ", ", allocSize - strlen(successBuf) - 1);
                }
                strncat(successBuf, (name && *name) ? name : "NULL", allocSize - strlen(successBuf) - 1);
                successCount++;

                if (!writeDynamicParamToDBFile(name))
                {
                    WalError("Write to DB file failed for '%s'\n", name);
                }

            }
            else
            {
                WalError("Failed to turn notification ON for parameter : %s ret: %d\n", att.name, (int)wret);
                if (failedBuf[0] != '\0') {
                    strncat(failedBuf, ", ", allocSize - strlen(failedBuf) - 1);
                }
                strncat(failedBuf, (name && *name) ? name : "NULL", allocSize - strlen(failedBuf) - 1);
                failureCount++;
            }
            WAL_FREE(att.name);
            WAL_FREE(att.value);
        }
        else if(node->paramSubscriptionStatus == ON)
        {
            WalInfo("Parameter '%s' already exists in globallist. \n", name);
            if (failedBuf[0] != '\0') {
                strncat(failedBuf, ", ", allocSize - strlen(failedBuf) - 1);
            }
            strncat(failedBuf, (name && *name) ? name : "NULL", allocSize - strlen(failedBuf) - 1);
            failureCount++;
            alreadySubscribedCount++;
            continue;
        }
    }

    if (failureCount == 0)
    {
        rbusValue_SetString(message, "Subscriptions processed successfully");
        notifyStatus = NOTIFY_SUBSCRIPTION_SUCCESS;
    }
    else if (successCount == 0)
    {
        if(alreadySubscribedCount > 0 && failureCount == alreadySubscribedCount)
        {
            rbusValue_SetString(message, "Subscriptions already exists");
            notifyStatus = NOTIFY_SUBSCRIPTION_ALREADY_EXISTS;
        }
        else
        {
            rbusValue_SetString(message, "Subscriptions failed");
            notifyStatus = NOTIFY_SUBSCRIPTION_FAILURE;
        }
    }
    else
    {
        size_t buffSize = snprintf(NULL, 0, "Success: %s, Failed: %s", successBuf[0] ? successBuf : "None", failedBuf[0] ? failedBuf : "None") + 1;
        char *buffer = malloc(buffSize);
        if(buffer)
        {
            snprintf(buffer, buffSize, "Success: %s, Failed: %s", successBuf[0] ? successBuf : "None", failedBuf[0] ? failedBuf : "None") + 1;
            rbusValue_SetString(message, buffer);
            free(buffer);
        } else {
            rbusValue_SetString(message, "Unknown");
        }
        notifyStatus = NOTIFY_SUBSCRIPTION_MULTI_STATUS;
    }
    rbusValue_SetInt32(statusCode, notifyStatus);
    rbusObject_SetValue(outParams, "message", message);
    rbusObject_SetValue(outParams, "statusCode", statusCode);

    WalInfo("Subscriptions completed: %s (status: %d)\n", rbusValue_GetString(message, NULL), rbusValue_GetInt32(statusCode));
    
    rbusValue_Release(message); rbusValue_Release(statusCode);
    if (successBuf) free(successBuf);
    if(failedBuf) free(failedBuf);

    if (notifyStatus == NOTIFY_SUBSCRIPTION_SUCCESS || notifyStatus == NOTIFY_SUBSCRIPTION_MULTI_STATUS) {
        return RBUS_ERROR_SUCCESS;
    }
    else {
        return RBUS_ERROR_BUS_ERROR;
    }
}
