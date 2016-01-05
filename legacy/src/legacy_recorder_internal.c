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

#include <stdio.h>
#include <stdlib.h>
#include <mm_camcorder.h>
#include <legacy_recorder.h>
#include <legacy_recorder_internal.h>
#include <legacy_recorder_private.h>
#include <dlog.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LEGACY_RECORDER"

int legacy_recorder_set_client_pid(recorder_h recorder, int pid)
{
	int ret;
	recorder_s *handle = (recorder_s *)recorder;

	if (handle == NULL) {
		LOGE("NULL handle");
		return RECORDER_ERROR_INVALID_PARAMETER;
	}

	LOGD("pid %d", pid);

	ret = mm_camcorder_set_attributes(handle->mm_handle, NULL,
	                                  MMCAM_PID_FOR_SOUND_FOCUS, pid,
	                                  NULL);

	return __convert_recorder_error_code(__func__, ret);
}
