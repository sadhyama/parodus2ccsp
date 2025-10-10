#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include "webpa_rbus.h"

static rbusHandle_t rbus_handle;
static bool isRbus = false;

rbusDataElement_t dataElements[] = {
    {WEBPA_NOTIFY_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NotifyParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
    {WEBPA_NOTIFY_SUBSCRIPTION, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, NotifyParamMethodHandler}}
};
#define dataElementsCount sizeof(dataElements)/sizeof(dataElements[0])

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
int regWebPaDataModel()
{
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    if(!rbus_handle)
    {
        WalError("regWebPaDataModel failed in getting bus handles\n");
        return rc;
    }

	rc = rbus_regDataElements(rbus_handle, dataElementsCount, dataElements);

    if(rc == RBUS_ERROR_SUCCESS)
    {
		WalInfo("Registered data element %s with rbus \n ", WEBPA_NOTIFY_PARAM);
    }
    else
	{
		WalError("Failed in registering data element %s \n", WEBPA_NOTIFY_PARAM);
	}
	return rc;
}

/**
 * Un-Register data elements for dataModel implementation using rbus.
 */
int UnregWebPaDataModel()
{
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    if(!rbus_handle)
    {
        WalError("regWebPaDataModel failed in getting bus handles\n");
        return rc;
    }

	rc = rbus_unregDataElements(rbus_handle, dataElementsCount, dataElements);
    if(rc == RBUS_ERROR_SUCCESS)
    {
		WalInfo("Registered data element %s with rbus \n ", WEBPA_NOTIFY_PARAM);
    }
    else
	{
		WalError("Failed in registering data element %s \n", WEBPA_NOTIFY_PARAM);
	}
	return rc;
}

rbusError_t NotifyParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    WalInfo("NotifyParamGetHandler is called\n");

    const char* paramName = rbusProperty_GetName(property);
    if(strncmp(paramName, WEBPA_NOTIFY_PARAM, strlen(WEBPA_NOTIFY_PARAM)) != 0)
    {
        WalError("Unexpected parameter = %s\n", paramName);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    char* buffer = CreateJsonFromGlobalNotifyList();

    if(buffer == NULL)
    {
        WalError("NotifyParamGetHandler: Failed to generate JSON from notify param list.\n");
        return RBUS_ERROR_BUS_ERROR;
    }

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, buffer);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    free(buffer);
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

    WalInfo("NotifyParamMethodHandler invoked\n");

    rbusValue_t message = NULL;
    rbusValue_t statusCode = NULL;
    WDMP_STATUS wret = WDMP_FAILURE;
    int prop_count  = 0, failureCount = 0, successCount = 0;

    rbusValue_Init(&message);
    rbusValue_Init(&statusCode);

/* Extract inParams */
    rbusProperty_t props = rbusObject_GetProperties(inParams);
    prop_count = props ? rbusProperty_Count(props) : 0;

    if (prop_count == 0)
    {
        WalError("No parameters provided\n");
        rbusValue_SetString(message, "No parameters provided");
        rbusValue_SetInt32(statusCode, 400);
        goto set_response;
    }

    size_t allocSize = (prop_count ? prop_count : 1) * 128;
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

        if (!name || !*name) continue;
        if (notifType == NULL || strcmp(notifType, "ValueChange") != 0) continue;

        paramStatus found = searchParaminGlobalList(name);
        if(found == PARAM_NOT_FOUND || found == PARAM_FOUND_OFF)
        {
            att.name = strdup(name);
            att.value = strdup("1");
            att.type = WDMP_INT;
            setAttributes(&att, 1, NULL, &wret);
            if (wret == WDMP_SUCCESS)
            {
                addParamtoGlobalList(att.name, DYNAMIC_PARAM, ON);
                if (!writeDynamicParamToDBFile(name))
                {
                    WalError("Write to DB file failed for '%s'\n", name);
                }
                if (successBuf[0] != '\0') {
                    strncat(successBuf, ", ", allocSize - strlen(successBuf) - 1);
                }
                strncat(successBuf, name, allocSize - strlen(successBuf) - 1);
                WalInfo("Successfully set notification ON for parameter : %s ret: %d\n", att.name, (int)wret);
                successCount++;
            }
            else
            {
                if (failedBuf[0] != '\0') {
                    strncat(failedBuf, ", ", allocSize - strlen(failedBuf) - 1);
                }
                strncat(failedBuf, name, allocSize - strlen(failedBuf) - 1);
                WalError("Failed to turn notification ON for parameter : %s ret: %d\n", att.name, (int)wret);
                failureCount++;
            }
            WAL_FREE(att.name);
            WAL_FREE(att.value);
        }
        else if(found == PARAM_FOUND_ON)
        {
            WalInfo("Parameter '%s' already exists in globallist. \n", name);
            continue;
        }
    }

    size_t buffSize = snprintf(NULL, 0, "Success: %s Failed: %s", successBuf[0] ? successBuf : "None", failedBuf[0]  ? failedBuf  : "None") + 1;
    char *buffer = malloc(buffSize);
    if (buffer) {
        snprintf(buffer, buffSize, "Success: %s Failed: %s", successBuf[0] ? successBuf : "None", failedBuf[0]  ? failedBuf  : "None");
        rbusValue_SetString(message, buffer);
    } else {
        rbusValue_SetString(message, "UNKNOWN");
    }

    int http_resp_code;
    if (failureCount == 0) http_resp_code = 200;
    else if (successCount == 0) http_resp_code = 500;
    else http_resp_code = 207;
    rbusValue_SetInt32(statusCode, http_resp_code);

set_response:
    if (message) rbusObject_SetValue(outParams, "message", message);
    if (statusCode) rbusObject_SetValue(outParams, "statusCode", statusCode);
    const char* finalMsg = rbusValue_GetString(message, NULL);
    WalInfo("NotifyParamMethodHandler completed: %s\n", finalMsg ? finalMsg : "SUBSCRIPTION STATUS UNKNOWN");
    if (message) rbusValue_Release(message);
    if (statusCode) rbusValue_Release(statusCode);
    if (successBuf) free(successBuf);
    if(failedBuf) free(failedBuf);
    if (buffer) free(buffer);
    return RBUS_ERROR_SUCCESS;
}


