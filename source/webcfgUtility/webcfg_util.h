/*
 * Copyright 2020 Comcast Cable Communications Management, LLC
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
#ifndef __WEBCFGUTIL_H__
#define __WEBCFGUTIL_H__

#include <stdint.h>
#include <wdmp-c.h>
#include <rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>
#include <rbus-core/rbus_core.h>
#include <rbus-core/rbus_session_mgr.h>
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DEST_COMP_ID_PSM   "com.cisco.spvtg.ccsp.psm"
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

char * getParameterValue();
void getValues_rbus(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, int *retStatus);


#endif
