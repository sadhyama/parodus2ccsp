/*
 * Copyright 2021 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "webcfg_util.h"
#include "webpa_adapter.h"
#include <rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>
#include <rbus-core/rbus_core.h>
#include <rbus-core/rbus_session_mgr.h>
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SERIAL_NUMBER				"Device.DeviceInfo.SerialNumber"
#define FIRMWARE_VERSION			"Device.DeviceInfo.X_CISCO_COM_FirmwareName"
#define DEVICE_BOOT_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_BootTime"
#define MODEL_NAME				"Device.DeviceInfo.ModelName"
#define PRODUCT_CLASS				"Device.DeviceInfo.ProductClass"
#define CONN_CLIENT_PARAM			"Device.NotifyComponent.X_RDKCENTRAL-COM_Connected-Client"
#define LAST_REBOOT_REASON			"Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason"
#define PARTNER_ID				"Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId"
#define ACCOUNT_ID				"Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"
#define FIRMW_START_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_MaintenanceWindow.FirmwareUpgradeStartTime"
#define FIRMW_END_TIME			"Device.DeviceInfo.X_RDKCENTRAL-COM_MaintenanceWindow.FirmwareUpgradeEndTime" 

#if defined(_COSA_BCM_MIPS_)
#define DEVICE_MAC                   "Device.DPoE.Mac_address"
#elif defined(PLATFORM_RASPBERRYPI)
#define DEVICE_MAC                   "Device.Ethernet.Interface.5.MACAddress"
#elif defined(RDKB_EMU)
#define DEVICE_MAC                   "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC"
#else
#define DEVICE_MAC                   "Device.X_CISCO_COM_CableModem.MACAddress"
#endif

#define WEBCFG_URL_PARAM "Device.X_RDK_WebConfig.URL"
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static char *PSMPrefix  = "eRT.com.cisco.spvtg.ccsp.webpa.";
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void __attribute__((weak)) getValues_rbus(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, int *retStatus);
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void getValues_rbus(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, int *retStatus)
{
	UNUSED(paramName);
	UNUSED(paramCount);
	UNUSED(index);
	UNUSED(timeSpan);
	UNUSED(paramArr);
	UNUSED(retValCount);
	UNUSED(retStatus);
	return;
}

char* get_deviceMAC(void)
{
	char *deviceMAC = NULL;
	deviceMAC = getParameterValue(DEVICE_MAC);
	return deviceMAC;
}

char * getSerialNumber()
{
	char *serialNum = NULL;
	serialNum = getParameterValue(SERIAL_NUMBER);
	return serialNum;
}

char * getDeviceBootTime()
{
	char *bootTime = NULL;
	bootTime = getParameterValue(DEVICE_BOOT_TIME);
	return bootTime;
}

char * getProductClass()
{
	char *productClass = NULL;
	productClass = getParameterValue(PRODUCT_CLASS);
	return productClass;
}

char * getModelName()
{
	char *modelName = NULL;
	modelName = getParameterValue(MODEL_NAME);
	return modelName;
}

char * getFirmwareVersion()
{
	char *firmware = NULL;
	firmware = getParameterValue(FIRMWARE_VERSION);
	return firmware;
}

char * getConnClientParamName()
{
	return CONN_CLIENT_PARAM;
}

char * getRebootReason()
{
	char *reboot_reason = NULL;
	reboot_reason = getParameterValue(LAST_REBOOT_REASON);
	return reboot_reason;
}

char * getPartnerID()
{
	char *partnerId = NULL;
	partnerId = getParameterValue(PARTNER_ID);
	return partnerId;
}

char * getAccountID()
{
	char *accountId = NULL;
	accountId = getParameterValue(ACCOUNT_ID);
	return accountId;
}

char * getFirmwareUpgradeStartTime()
{
	char *FirmwareUpgradeStartTime = NULL;
	FirmwareUpgradeStartTime = getParameterValue(FIRMW_START_TIME);
	return FirmwareUpgradeStartTime;
}

char * getFirmwareUpgradeEndTime()
{
	char *FirmwareUpgradeEndTime = NULL;
	FirmwareUpgradeEndTime = getParameterValue(FIRMW_END_TIME);
	return FirmwareUpgradeEndTime;
}

int Get_Webconfig_URL( char *pString)
{
	pString = NULL;
	int retPsmGet = 0;
	if(isRbusEnabled())
	{
		retPsmGet = rbus_GetValueFromDB( WEBCFG_URL_PARAM, &pString);
		WalInfo("Get_Webconfig_URL. retPsmGet %d pString %s\n", pString);
	}
    	return 0;
}

int Set_Webconfig_URL( char *pString)
{
    int retPsmGet = 0;
    if(isRbusEnabled())
    {
    	retPsmSet = rbus_StoreValueIntoDB( WEBCFG_URL_PARAM, pString );
	WalInfo("Set_Webconfig_URL. retPsmGet %d pString %s\n", pString);
    }
    return 0;
}

char * getParameterValue()
{
	if(isRbusEnabled())
	{
		int paramCount=0;
		WDMP_STATUS ret = WDMP_FAILURE;
		int count=0;
		const char *getParamList[1];
		getParamList[0] = paramName;

		char *paramValue = (char *) malloc(sizeof(char)*64);
		paramCount = sizeof(getParamList)/sizeof(getParamList[0]);
		param_t **parametervalArr = (param_t **) malloc(sizeof(param_t *) * paramCount);

		WalInfo("B4 getValues_rbus\n");
		WalInfo("paramName : %s paramCount %d\n",paramName[0], paramCount);
		getValues_rbus(getParamList, paramCount, 0, NULL, &parametervalArr, &count, &ret);
		WalInfo("count is %d ret %d\n", count, ret);

		if (ret == WDMP_SUCCESS )
		{
			strncpy(paramValue, parametervalArr[0]->value,64);
			WalInfo("B4 parametervalArr name free\n");
			WAL_FREE(parametervalArr[0]->name);
			WAL_FREE(parametervalArr[0]->value);
			WAL_FREE(parametervalArr[0]);
			WalInfo("After parametervalArr[0] free\n");
		}
		else
		{
			WalError("Failed to GetValue for %s\n", getParamList[0]);
			WAL_FREE(paramValue);
		}
		WalInfo("B4 parametervalArr free\n");
		WAL_FREE(parametervalArr);
		WalInfo("getParameterValue : paramValue is %s\n", paramValue);
		return paramValue;
	}
	WalError("getParameterValue : returns NULL\n");
	return NULL;
}

/**
 * To persist TR181 parameter values in PSM DB.
 */
int rbus_StoreValueIntoDB(char *paramName, char *value)
{
	char recordName[ 256] = {'\0'};
	char psmName[256] = {'\0'};
	rbusParamVal_t val[1];
	bool commit = 1;
	rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;
	rbus_error_t err = RTMESSAGE_BUS_SUCCESS;

	sprintf(recordName, "%s%s",PSMPrefix, paramName);
	WebcfgInfo("rbus_StoreValueIntoDB recordName is %s\n", recordName);

	val[0].paramName  = recordName;
	val[0].paramValue = value;
	val[0].type = 0;

	sprintf(psmName, "%s%s", "eRT.", DEST_COMP_ID_PSM);
	WebcfgInfo("rbus_StoreValueIntoDB psmName is %s\n", psmName);

	rbusMessage request, response;

	rbusMessage_Init(&request);
	rbusMessage_SetInt32(request, 0); //sessionId
	rbusMessage_SetString(request, "webconfig"); //component name that invokes the set
	rbusMessage_SetInt32(request, (int32_t)1); //size of params

	rbusMessage_SetString(request, val[0].paramName); //param details
	rbusMessage_SetInt32(request, val[0].type);
	rbusMessage_SetString(request, val[0].paramValue);
	rbusMessage_SetString(request, commit ? "1" : "0");


	if((err = rbus_invokeRemoteMethod(psmName, METHOD_SETPARAMETERVALUES, request, 6000, &response)) != RTMESSAGE_BUS_SUCCESS)
        {
            WebcfgError("rbus_invokeRemoteMethod failed with err %d", err);
            errorcode = RBUS_ERROR_BUS_ERROR;
        }
        else
        {
            int ret = -1;
            char const* pErrorReason = NULL;
            rbusMessage_GetInt32(response, &ret);

            WebcfgInfo("Response from the remote method is [%d]!", ret);
            errorcode = (rbusError_t) ret;

            if((errorcode == RBUS_ERROR_SUCCESS) || (errorcode == 100)) //legacy error codes returned from component PSM.
            {
                errorcode = RBUS_ERROR_SUCCESS;
                WebcfgInfo("Successfully Set the Value");
            }
            else
            {
                rbusMessage_GetString(response, &pErrorReason);
                WebcfgError("Failed to Set the Value for %s", pErrorReason);
            }

            rbusMessage_Release(response);
        }

	return errorcode;
}

/**
 * To fetch TR181 parameter values from PSM DB.
 */
int rbus_GetValueFromDB( char* paramName, char** paramValue)
{
	char recordName[ 256] = {'\0'};
	char psmName[256] = {'\0'};
	char * parameterNames[1] = {NULL};
	int32_t type = 0;
	rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;
	rbus_error_t err = RTMESSAGE_BUS_SUCCESS;
	*paramValue = NULL;

	sprintf(recordName, "%s%s",PSMPrefix, paramName);
	WebcfgInfo("rbus_GetValueFromDB recordName is %s\n", recordName);

	parameterNames[0] = (char*)recordName;

	sprintf(psmName, "%s%s", "eRT.", DEST_COMP_ID_PSM);
	WebcfgInfo("rbus_GetValueFromDB psmName is %s\n", psmName);

	rbusMessage request, response;

	rbusMessage_Init(&request);
	rbusMessage_SetString(request, psmName); //component name that invokes the get
	rbusMessage_SetInt32(request, (int32_t)1); //size of params

	rbusMessage_SetString(request, parameterNames[0]); //param details

	if((err = rbus_invokeRemoteMethod(psmName, METHOD_GETPARAMETERVALUES, request, 6000, &response)) != RTMESSAGE_BUS_SUCCESS)
        {
            WebcfgError("rbus_invokeRemoteMethod GET failed with err %d\n", err);
            errorcode = RBUS_ERROR_BUS_ERROR;
        }
        else
        {
            int valSize=0;
	    int ret = -1;
	    rbusMessage_GetInt32(response, &ret);
	    WebcfgInfo("Response from the remote method is [%d]!\n",ret);
            errorcode = (rbusError_t) ret;

	    if((errorcode == RBUS_ERROR_SUCCESS) || (errorcode == 100)) //legacy error code from component.
            {
                WebcfgInfo("Successfully Get the Value\n");
		errorcode = RBUS_ERROR_SUCCESS;
		rbusMessage_GetInt32(response, &valSize);
		if(1/*valSize*/)
		{
			char const *buff = NULL;

			//Param Name
			rbusMessage_GetString(response, &buff);
			WebcfgInfo("Requested param buff [%s]\n", buff);
			if(buff && (strcmp(recordName, buff) == 0))
			{
				rbusMessage_GetInt32(response, &type);
				WebcfgInfo("Requested param type [%d]\n", type);
				buff = NULL;
				rbusMessage_GetString(response, &buff);
				*paramValue = strdup(buff); //free buff
				WebcfgInfo("Requested param DB value [%s]\n", *paramValue);
			}
			else
			{
			    WebcfgError("Requested param: [%s], Received Param: [%s]\n", recordName, buff);
			    errorcode = RBUS_ERROR_BUS_ERROR;
			}
		}
            }
            else
            {
		WebcfgError("Response from remote Get method failed!!\n");
		errorcode = RBUS_ERROR_BUS_ERROR;
            }

            rbusMessage_Release(response);
        }
	WebcfgInfo("rbus_GetValueFromDB End\n");
	return errorcode;
}

