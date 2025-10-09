#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include "webpa_rbus.h"

static rbusHandle_t rbus_handle;
static bool isRbus = false;

rbusDataElement_t dataElements[1] = {
    {WEBPA_NOTIFY_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NotifyParamGetHandler, NULL, NULL, NULL, NULL, NULL}}
    // {WEBPA_SUBSCRIBE_LIST, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, NotifyParamMethodHandler}}
};

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

	rc = rbus_regDataElements(rbus_handle, 1, dataElements);

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

	rc = rbus_unregDataElements(rbus_handle, 1, dataElements);
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

    char* buffer = NULL;
    CreateJsonFromGlobalNotifyList(&buffer);

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
