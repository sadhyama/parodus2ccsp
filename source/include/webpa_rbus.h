#ifndef _WEBPA_RBUS_H_
#define _WEBPA_RBUS_H_

#include <stdio.h>
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>

#include "webpa_adapter.h"
#include "webpa_notification.h"
#include <wdmp-c.h>
#include <cimplog.h>

#include <dslh_definitions_database.h>

#define MAX_PARAM_LEN 256
#define WEBPA_NOTIFY_PARAM "Device.DeviceInfo.Webpa.NotifySubscriptionList"
#define WEBPA_NOTIFY_SUBSCRIPTION "Device.Webpa.Subscription.NotifyEvent()"

bool isRbusEnabled();
bool isRbusInitialized();
WDMP_STATUS webpaRbusInit(const char *pComponentName);
void webpaRbus_Uninit();
rbusError_t setTraceContext(char* traceContext[]);
rbusError_t getTraceContext(char* traceContext[]);
rbusError_t clearTraceContext();
void regWebpaDataModel();
rbusError_t NotifySubscriptionListGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
rbusError_t NotifySubscriptionListMethodHandler(rbusHandle_t handle, const char* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);
static void setRbusResponse(rbusObject_t outParams, const char* msgStr, NOTIFY_EVENT_STATUS_CODE status, cJSON* successArr, cJSON* failureArr);
static int validate_notify_params(rbusObject_t inParams, int paramCount, char *err_msg, size_t len);
#endif
