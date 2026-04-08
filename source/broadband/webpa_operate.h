/**
 * @file webpa_operate.h
 *
 * @description Declares the OPERATE dispatch helper for parodus2ccsp.
 *              Invoked from processRequest() when a SET request carries
 *              a parameter with dataType == WDMP_APPLICATION (13).
 *
 * Copyright (c) 2024 Comcast
 */
#ifndef _WEBPA_OPERATE_H_
#define _WEBPA_OPERATE_H_

#include <wdmp-c.h>

/**
 * @brief processOperateRequest - Handle an OPERATE (RBUS method invocation) request.
 *
 * Decodes the base64 payload in param->value, extracts the RBUS method name and
 * optional inParams, calls rbusMethod_Invoke synchronously, serialises the outParams
 * as JSON, base64-encodes the result, and stores it in responseMessage.
 *
 * @param[in]  param           The single RDK.Operate parameter from the SET request.
 * @param[out] responseMessage Caller-owned buffer pointer; set to a newly malloc'd,
 *                             null-terminated base64 string on success.  The caller
 *                             must free this pointer.  Set to NULL on error.
 * @return WDMP_STATUS indicating success or the specific failure reason.
 */
WDMP_STATUS processOperateRequest(const param_t *param, char **responseMessage);

#endif /* _WEBPA_OPERATE_H_ */
