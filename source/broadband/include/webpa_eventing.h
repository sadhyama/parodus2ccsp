/**
 * @file webpa_eventing.h
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2025  Comcast
 */
#ifndef WEBPA_EVENTING_H
#define WEBPA_EVENTING_H

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>

#define NOTIFY_PARAM_FILE "/nvram/webpa_dynamic_params_db"

#define DYNAMIC_PARAM 0
#define STATIC_PARAM 1
#define OFF 0
#define ON 1

typedef struct g_NotifyParam
{
    char *paramName;
    bool paramType;
    bool paramSubscriptionStatus;
    struct g_NotifyParam *next;
} g_NotifyParam;

typedef enum {
    NOTIFY_SUBSCRIPTION_SUCCESS                    =  200,
    NOTIFY_SUBSCRIPTION_FAILURE                    =  500,
    NOTIFY_SUBSCRIPTION_MULTI_STATUS               =  207,
    NOTIFY_SUBSCRIPTION_ERR_INVALID_INPUT          =  400,
    NOTIFY_SUBSCRIPTION_ERR_BOOTUP_IN_PROGRESS     =  503
} NOTIFY_SUBSCRIPTION_STATUS_CODE; 

void readDynamicParamsFromDBFile(int *notifyListSize);
int writeDynamicParamToDBFile(const char *param);
g_NotifyParam* searchParaminGlobalList(const char *paramName);
void addParamToGlobalList(const char* paramName, bool paramType, bool paramSubscriptionStatus);
char* CreateJsonFromGlobalNotifyList();
void setBootupNotifyInitDone(bool value);
bool getBootupNotifyInitDone();
g_NotifyParam* getGlobalNotifyHead();
bool getParamStatus(g_NotifyParam *param);
void updateParamStatus(g_NotifyParam *param, bool status);

#endif // WEBPA_EVENTING_H