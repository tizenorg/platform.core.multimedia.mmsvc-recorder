/*
* Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef __TIZEN_MULTIMEDIA_LEGACY_RECORDER_INTERNAL_H__
#define	__TIZEN_MULTIMEDIA_LEGACY_RECORDER_INTERNAL_H__
#include <legacy_recorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @file legacy_recorder_internal.h
  * @brief This file contains the legacy recorder internal API for framework, related structures and enumerations.
  */

/**
 * @brief Set pid of client for sound focus API.
 * @ingroup CAPI_MEDIA_RECORDER_MUSED_MODULE
 * @param[in] recorder The handle to the recorder
 * @param[in] pid The pid of client
 *
 * @return @c 0 on success, otherwise a negative error value
 * @retval #RECORDER_ERROR_NONE Successful
 * @retval #RECORDER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #RECORDER_ERROR_INVALID_OPERATION Invalid operation
 * @retval #RECORDER_ERROR_INVALID_STATE Invalid state
 * @pre	The recorder state must be #RECORDER_STATE_CREATED.
 */
int legacy_recorder_set_client_pid(recorder_h recorder, int pid);


#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_MULTIMEDIA_LEGACY_RECORDER_INTERNAL_H__ */

