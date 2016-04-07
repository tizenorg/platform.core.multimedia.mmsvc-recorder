/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include "muse_recorder.h"
#include "muse_recorder_msg.h"
#include <muse_core.h>
#include <muse_core_ipc.h>
#include <muse_core_security.h>
#include <muse_camera_internal.h>
#include <mm_types.h>
#include <dlog.h>
#include "legacy_recorder_internal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MUSED_RECORDER"

#define RECORDER_PRIVILEGE_NAME "http://tizen.org/privilege/recorder"


/**
 * @brief The structure type for the exported bo data.
 */
typedef struct {
	tbm_bo bo;
	int key;
} muse_recorder_export_data;

/**
 * @brief The structure type for the muse recorder.
 */
typedef struct {
	recorder_h recorder_handle;
	tbm_bufmgr bufmgr;
	GList *data_list;
	GMutex list_lock;
} muse_recorder_handle_s;


void _recorder_disp_recording_limit_reached_cb(recorder_recording_limit_type_e type, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event1(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_RECORDING_LIMITED,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_MAIN,
	                         module,
	                         INT, type);

	return;
}

void _recorder_disp_recording_status_cb(unsigned long long elapsed_time, unsigned long long file_size, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;
	int64_t cb_elapsed_time = (int64_t)elapsed_time;
	int64_t cb_file_size = (int64_t)file_size;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event2(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_RECORDING_STATUS,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_MAIN,
	                         module,
	                         INT64, cb_elapsed_time,
	                         INT64, cb_file_size);

	return;
}

void _recorder_disp_state_changed_cb(recorder_state_e previous, recorder_state_e current, bool by_policy, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event3(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_STATE_CHANGE,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_MAIN,
	                         module,
	                         INT, previous,
	                         INT, current,
	                         INT, by_policy);

	return;
}

void _recorder_disp_interrupted_cb(recorder_policy_e policy, recorder_state_e previous, recorder_state_e current, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event3(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_INTERRUPTED,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_MAIN,
	                         module,
	                         INT, policy,
	                         INT, previous,
	                         INT, current);

	return;
}

void _recorder_disp_error_cb(recorder_error_e error, recorder_state_e current_state, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event2(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_ERROR,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_MAIN,
	                         module,
	                         INT, error,
	                         INT, current_state);

	return;
}

void _recorder_disp_audio_stream_cb(void* stream, int size, audio_sample_type_e format, int channel, unsigned int timestamp, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;
	muse_recorder_handle_s *muse_recorder = NULL;
	muse_recorder_export_data *export_data = NULL;
	int tbm_key = 0;
	tbm_bo bo = NULL;
	tbm_bo_handle bo_handle = {.ptr = NULL};

	if (module == NULL || stream == NULL) {
		LOGE("NULL data %p, %p", module, stream);
		return;
	}

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		return;
	}

	export_data = g_new0(muse_recorder_export_data, 1);
	if (export_data == NULL) {
		LOGE("alloc export_data failed");
		return;
	}

	LOGD("Enter");

	/* make tbm bo */
	bo = tbm_bo_alloc(muse_recorder->bufmgr, size, TBM_BO_DEFAULT);
	if (bo == NULL) {
		LOGE("bo alloc failed : bufmgr %p, size %d", muse_recorder->bufmgr, size);
		g_free(export_data);
		export_data = NULL;
		return;
	}

	bo_handle = tbm_bo_map(bo, TBM_DEVICE_CPU, TBM_OPTION_READ | TBM_OPTION_WRITE);
	if (bo_handle.ptr == NULL) {
		LOGE("bo map Error!");
		tbm_bo_unref(bo);
		g_free(export_data);
		export_data = NULL;
		return;
	}

	memcpy(bo_handle.ptr, stream, size);

	tbm_bo_unmap(bo);

	tbm_key = tbm_bo_export(bo);
	if(tbm_key == 0) {
		LOGE("Create key_info ERROR!!");
		tbm_bo_unref(bo);
		bo = NULL;
		g_free(export_data);
		export_data = NULL;
		return;
	}

	/* set bo info */
	export_data->key = tbm_key;
	export_data->bo = bo;

	/* add bo info to list */
	g_mutex_lock(&muse_recorder->list_lock);
	muse_recorder->data_list = g_list_append(muse_recorder->data_list, (gpointer)export_data);
	g_mutex_unlock(&muse_recorder->list_lock);

	/* send message */
	muse_recorder_msg_event5(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_AUDIO_STREAM,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_SUB,
	                         module,
	                         INT, size,
	                         INT, format,
	                         INT, channel,
	                         INT, timestamp,
	                         INT, tbm_key);

	return;
}

void _recorder_disp_foreach_supported_video_resolution_cb(int width, int height, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event2(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_RESOLUTION,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_SUB,
	                         module,
	                         INT, width,
	                         INT, height);

	return;
}

void _recorder_disp_foreach_supported_file_format_cb(recorder_file_format_e format, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event1(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_FILE_FORMAT,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_SUB,
	                         module,
	                         INT, format);

	return;
}

void _recorder_disp_foreach_supported_audio_encoder_cb(recorder_audio_codec_e codec, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event1(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_AUDIO_ENCODER,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_SUB,
	                         module,
	                         INT, codec);

	return;
}

void _recorder_disp_foreach_supported_video_encoder_cb(recorder_video_codec_e codec, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	if (module == NULL) {
		LOGE("NULL module");
		return;
	}

	muse_recorder_msg_event1(MUSE_RECORDER_CB_EVENT,
	                         MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_ENCODER,
	                         MUSE_RECORDER_EVENT_CLASS_THREAD_SUB,
	                         module,
	                         INT, codec);

	return;
}

static int _recorder_remove_export_data(muse_module_h module, int key, int remove_all)
{
	muse_recorder_handle_s *muse_recorder = NULL;
	GList *tmp_list = NULL;
	muse_recorder_export_data *export_data = NULL;

	if (module == NULL || (key <= 0 && remove_all == FALSE)) {
		LOGE("invalid parameter %p, %d", module, key);
		return FALSE;
	}

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		return FALSE;
	}

	g_mutex_lock(&muse_recorder->list_lock);

	tmp_list = muse_recorder->data_list;

	while (tmp_list) {
		export_data = (muse_recorder_export_data *)tmp_list->data;
		if (export_data) {
			if (export_data->key == key || remove_all) {
				/*LOGD("key %d matched, remove it (remove_all %d)", key, remove_all);*/

				if (export_data->bo) {
					tbm_bo_unref(export_data->bo);
					export_data->bo = NULL;
				} else {
					LOGW("bo for key %d is NULL", key);
				}
				export_data->key = 0;

				muse_recorder->data_list = g_list_remove(muse_recorder->data_list, export_data);

				g_free(export_data);
				export_data = NULL;

				if (remove_all == FALSE) {
					/*LOGD("key %d, remove done");*/
					g_mutex_unlock(&muse_recorder->list_lock);
					return TRUE;
				} else {
					LOGD("check next data");
				}
			}
		} else {
			LOGW("NULL data");
		}

		tmp_list = tmp_list->next;
	}

	g_mutex_unlock(&muse_recorder->list_lock);

	if (remove_all) {
		LOGD("remove all done");
	} else {
		LOGE("should not be reached here - key %d", key);
	}

	return FALSE;
}

int recorder_dispatcher_create(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int recorder_type = MUSE_RECORDER_TYPE_AUDIO;
	int client_fd = -1;
	int pid = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_CREATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	intptr_t camera_handle = 0;
	muse_recorder_handle_s *muse_recorder = NULL;
	muse_camera_handle_s *muse_camera = NULL;
	intptr_t handle = 0;

	muse_recorder_msg_get(recorder_type, muse_core_client_get_msg(module));

	LOGD("Enter - type %d", recorder_type);

	/* privilege check */
	client_fd = muse_core_client_get_msg_fd(module);
	if (!muse_core_security_check_cynara(client_fd, RECORDER_PRIVILEGE_NAME)) {
		LOGE("security check failed");
		ret = RECORDER_ERROR_PERMISSION_DENIED;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	/* init handle */
	muse_recorder = (muse_recorder_handle_s *)malloc(sizeof(muse_recorder_handle_s));
	if (muse_recorder == NULL) {
		ret = RECORDER_ERROR_OUT_OF_MEMORY;
		LOGE("handle alloc failed 0x%x", ret);
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	memset(muse_recorder, 0x0, sizeof(muse_recorder_handle_s));

	g_mutex_init(&muse_recorder->list_lock);

	if (muse_core_ipc_get_bufmgr(&muse_recorder->bufmgr) != MM_ERROR_NONE) {
		LOGE("muse_core_ipc_get_bufmgr failed");

		g_mutex_clear(&muse_recorder->list_lock);
		free(muse_recorder);
		muse_recorder = NULL;

		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);

		return MUSE_RECORDER_ERROR_NONE;
	}

	if (recorder_type == MUSE_RECORDER_TYPE_VIDEO) {
		muse_recorder_msg_get_pointer(camera_handle, muse_core_client_get_msg(module));
		if (camera_handle == 0) {
			LOGE("NULL handle");

			g_mutex_clear(&muse_recorder->list_lock);
			free(muse_recorder);
			muse_recorder = NULL;

			ret = RECORDER_ERROR_INVALID_PARAMETER;
			muse_recorder_msg_return(api, class, ret, module);
			return MUSE_RECORDER_ERROR_NONE;
		}

		muse_camera = (muse_camera_handle_s *)camera_handle;

		LOGD("video type, camera handle : %p", muse_camera->camera_handle);

		ret = legacy_recorder_create_videorecorder(muse_camera->camera_handle, &muse_recorder->recorder_handle);
	} else if (recorder_type == MUSE_RECORDER_TYPE_AUDIO) {
		muse_recorder_msg_get(pid, muse_core_client_get_msg(module));

		LOGD("audio type - pid %d", pid);
		ret = legacy_recorder_create_audiorecorder(&muse_recorder->recorder_handle);
		if (ret == RECORDER_ERROR_NONE) {
			ret = legacy_recorder_set_client_pid(muse_recorder->recorder_handle, pid);
			if (ret != RECORDER_ERROR_NONE) {
				LOGE("legacy_recorder_set_client_pid failed 0x%x", ret);
				legacy_recorder_destroy(muse_recorder->recorder_handle);
				muse_recorder->recorder_handle = NULL;
			}
		}
	}

	if (ret == RECORDER_ERROR_NONE) {
		LOGD("recorder handle : %p, module : %p", muse_recorder, module);

		handle = (intptr_t)muse_recorder->recorder_handle;
		muse_core_ipc_set_handle(module, (intptr_t)muse_recorder);
		muse_recorder_msg_return1(api, class, ret, module, POINTER, handle);
	} else {
		g_mutex_clear(&muse_recorder->list_lock);
		free(muse_recorder);
		muse_recorder = NULL;

		LOGE("error 0x%d", ret);
		muse_recorder_msg_return(api, class, ret, module);
	}

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_destroy(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_handle_s *muse_recorder = NULL;
	muse_recorder_api_e api = MUSE_RECORDER_API_DESTROY;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_destroy(muse_recorder->recorder_handle);
	if (ret == RECORDER_ERROR_NONE) {
		_recorder_remove_export_data(module, 0, TRUE);

		g_mutex_clear(&muse_recorder->list_lock);

		muse_recorder->bufmgr = NULL;

		free(muse_recorder);
		muse_recorder = NULL;
	} else {
		LOGE("recorder destroy failed 0x%x", ret);
	}

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_state(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_STATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	recorder_state_e get_state = RECORDER_STATE_NONE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_get_state(muse_recorder->recorder_handle, &get_state);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_state);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_prepare(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_PREPARE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_prepare(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unprepare(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNPREPARE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_unprepare(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_start(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_START;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_start(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_pause(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_PAUSE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_pause(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_commit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_COMMIT;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_commit(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_cancel(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_CANCEL;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_cancel(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_video_resolution(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int width = 0;
	int height = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_VIDEO_RESOLUTION;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(width, muse_core_client_get_msg(module));
	muse_recorder_msg_get(height, muse_core_client_get_msg(module));

	ret = legacy_recorder_set_video_resolution(muse_recorder->recorder_handle, width, height);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_video_resolution(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int get_width = 0;
	int get_height = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_VIDEO_RESOLUTION;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_get_video_resolution(muse_recorder->recorder_handle, &get_width, &get_height);

	muse_recorder_msg_return2(api, class, ret, module,
	                          INT, get_width,
	                          INT, get_height);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_video_resolution(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_RESOLUTION;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_THREAD_SUB;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_foreach_supported_video_resolution(muse_recorder->recorder_handle,
	                                                         (recorder_supported_video_resolution_cb)_recorder_disp_foreach_supported_video_resolution_cb,
	                                                         (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_audio_level(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	double get_level = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_AUDIO_LEVEL;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_get_audio_level(muse_recorder->recorder_handle, &get_level);

	muse_recorder_msg_return1(api, class, ret, module, DOUBLE, get_level);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_filename(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	char filename[MUSE_RECORDER_MSG_MAX_LENGTH] = {0,};
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_FILENAME;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get_string(filename, muse_core_client_get_msg(module));

	ret = legacy_recorder_set_filename(muse_recorder->recorder_handle, filename);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_filename(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	char *get_filename = NULL;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_FILENAME;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_get_filename(muse_recorder->recorder_handle, &get_filename);
	if (ret == RECORDER_ERROR_NONE && get_filename) {
		muse_recorder_msg_return1(api, class, ret, module, STRING, get_filename);
	} else {
		muse_recorder_msg_return(api, class, ret, module);
	}

	if (get_filename) {
		free(get_filename);
		get_filename = NULL;
	}

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_file_format(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_file_format_e set_format = RECORDER_FILE_FORMAT_3GP;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_FILE_FORMAT;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(set_format, muse_core_client_get_msg(module));

	ret = legacy_recorder_set_file_format(muse_recorder->recorder_handle, set_format);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_file_format(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_file_format_e get_format = RECORDER_FILE_FORMAT_3GP;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_FILE_FORMAT;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_get_file_format(muse_recorder->recorder_handle, &get_format);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_format);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_state_changed_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_STATE_CHANGED_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_set_state_changed_cb(muse_recorder->recorder_handle,
	                                           (recorder_state_changed_cb)_recorder_disp_state_changed_cb,
	                                           (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_state_changed_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_STATE_CHANGED_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_unset_state_changed_cb(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_interrupted_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_INTERRUPTED_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_set_interrupted_cb(muse_recorder->recorder_handle,
	                                         (recorder_interrupted_cb)_recorder_disp_interrupted_cb,
	                                         (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_interrupted_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_INTERRUPTED_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_unset_interrupted_cb(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_audio_stream_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_AUDIO_STREAM_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_set_audio_stream_cb(muse_recorder->recorder_handle,
	                                          (recorder_audio_stream_cb)_recorder_disp_audio_stream_cb,
	                                          (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_audio_stream_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_AUDIO_STREAM_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_unset_audio_stream_cb(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_error_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_ERROR_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_set_error_cb(muse_recorder->recorder_handle,
	                                   (recorder_error_cb)_recorder_disp_error_cb,
	                                   (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_error_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_ERROR_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_unset_error_cb(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_recording_status_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_RECORDING_STATUS_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_set_recording_status_cb(muse_recorder->recorder_handle,
	                                              (recorder_recording_status_cb)_recorder_disp_recording_status_cb,
	                                              (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_recording_status_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_RECORDING_STATUS_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_unset_recording_status_cb(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_recording_limit_reached_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_RECORDING_LIMIT_REACHED_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_set_recording_limit_reached_cb(muse_recorder->recorder_handle,
	                                                     (recorder_recording_limit_reached_cb)_recorder_disp_recording_limit_reached_cb,
	                                                     (void *)module);
	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_recording_limit_reached_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_RECORDING_LIMIT_REACHED_CB;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_unset_recording_limit_reached_cb(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_file_format(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_FOREACH_SUPPORTED_FILE_FORMAT;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_THREAD_SUB;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_foreach_supported_file_format(muse_recorder->recorder_handle,
	                                                    (recorder_supported_file_format_cb)_recorder_disp_foreach_supported_file_format_cb,
	                                                    (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_size_limit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int kbyte = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_SIZE_LIMIT;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(kbyte, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_size_limit(muse_recorder->recorder_handle, kbyte);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_time_limit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int second = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_TIME_LIMIT;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(second, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_time_limit(muse_recorder->recorder_handle, second);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_device(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_audio_device_e set_device = RECORDER_AUDIO_DEVICE_MIC;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_AUDIO_DEVICE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(set_device, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_audio_device(muse_recorder->recorder_handle, set_device);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_audio_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_audio_codec_e set_codec = RECORDER_AUDIO_CODEC_AMR;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_AUDIO_ENCODER;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(set_codec, muse_core_client_get_msg(module));

	ret = legacy_recorder_set_audio_encoder(muse_recorder->recorder_handle, set_codec);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_audio_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_audio_codec_e get_codec = RECORDER_AUDIO_CODEC_AMR;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_AUDIO_ENCODER;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_get_audio_encoder(muse_recorder->recorder_handle, &get_codec);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_codec);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_video_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_video_codec_e set_codec = RECORDER_VIDEO_CODEC_MPEG4;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_VIDEO_ENCODER;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(set_codec, muse_core_client_get_msg(module));

	ret = legacy_recorder_set_video_encoder(muse_recorder->recorder_handle, set_codec);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_video_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_video_codec_e get_codec = RECORDER_VIDEO_CODEC_MPEG4;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_VIDEO_ENCODER;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_get_video_encoder(muse_recorder->recorder_handle, &get_codec);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_codec);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_samplerate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int samplerate = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_AUDIO_SAMPLERATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(samplerate, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_audio_samplerate(muse_recorder->recorder_handle, samplerate);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_encoder_bitrate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int bitrate = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_AUDIO_ENCODER_BITRATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(bitrate, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_audio_encoder_bitrate(muse_recorder->recorder_handle, bitrate);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_video_encoder_bitrate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int bitrate = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_VIDEO_ENCODER_BITRATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(bitrate, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_video_encoder_bitrate(muse_recorder->recorder_handle, bitrate);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_size_limit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int get_kbyte = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_SIZE_LIMIT;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_size_limit(muse_recorder->recorder_handle, &get_kbyte);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_kbyte);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_time_limit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int get_second = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_TIME_LIMIT;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_time_limit(muse_recorder->recorder_handle, &get_second);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_second);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_device(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_audio_device_e get_device = RECORDER_AUDIO_DEVICE_MIC;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_AUDIO_DEVICE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_audio_device(muse_recorder->recorder_handle, &get_device);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_device);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_samplerate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int get_samplerate = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_AUDIO_SAMPLERATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_audio_samplerate(muse_recorder->recorder_handle, &get_samplerate);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_samplerate);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_encoder_bitrate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int get_bitrate = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_AUDIO_ENCODER_BITRATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_audio_encoder_bitrate(muse_recorder->recorder_handle, &get_bitrate);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_bitrate);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_video_encoder_bitrate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int get_bitrate = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_VIDEO_ENCODER_BITRATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_video_encoder_bitrate(muse_recorder->recorder_handle, &get_bitrate);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_bitrate);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_audio_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_FOREACH_SUPPORTED_AUDIO_ENCODER;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_THREAD_SUB;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_foreach_supported_audio_encoder(muse_recorder->recorder_handle,
	                                                      (recorder_supported_audio_encoder_cb)_recorder_disp_foreach_supported_audio_encoder_cb,
	                                                      (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_video_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_ENCODER;
		muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_THREAD_SUB;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_foreach_supported_video_encoder(muse_recorder->recorder_handle,
	                                                      (recorder_supported_video_encoder_cb)_recorder_disp_foreach_supported_video_encoder_cb,
	                                                      (void *)module);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_mute(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int set_enable = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_MUTE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(set_enable, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_mute(muse_recorder->recorder_handle, (bool)set_enable);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_is_muted(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_IS_MUTED;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_is_muted(muse_recorder->recorder_handle);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_recording_motion_rate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	double rate = 1.0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_RECORDING_MOTION_RATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get_double(rate, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_recording_motion_rate(muse_recorder->recorder_handle, rate);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_recording_motion_rate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	double get_rate = 1.0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_RECORDING_MOTION_RATE;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_recording_motion_rate(muse_recorder->recorder_handle, &get_rate);

	LOGD("get rate %lf", get_rate);

	muse_recorder_msg_return1(api, class, ret, module, DOUBLE, get_rate);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_channel(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int channel_count = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_AUDIO_CHANNEL;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(channel_count, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_audio_channel(muse_recorder->recorder_handle, channel_count);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_channel(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	int get_channel_count = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_AUDIO_CHANNEL;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_audio_channel(muse_recorder->recorder_handle, &get_channel_count);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_channel_count);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_orientation_tag(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_rotation_e set_orientation = RECORDER_ROTATION_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_ORIENTATION_TAG;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(set_orientation, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_orientation_tag(muse_recorder->recorder_handle, set_orientation);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_orientation_tag(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	recorder_rotation_e get_orientation = RECORDER_ROTATION_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_ORIENTATION_TAG;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	ret = legacy_recorder_attr_get_orientation_tag(muse_recorder->recorder_handle, &get_orientation);

	muse_recorder_msg_return1(api, class, ret, module, INT, get_orientation);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_root_directory(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	char root_directory[MUSE_RECORDER_MSG_MAX_LENGTH] = {0,};
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_ROOT_DIRECTORY;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get_string(root_directory, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_root_directory(muse_recorder->recorder_handle, root_directory);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_return_buffer(muse_module_h module)
{
	int tbm_key = 0;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get(tbm_key, muse_core_client_get_msg(module));

	/*LOGD("handle : %p, key : %d", muse_recorder, tbm_key);*/

	if (!_recorder_remove_export_data(module, tbm_key, FALSE)) {
		LOGE("remove export data failed : key %d", tbm_key);
	}

	return MUSE_RECORDER_ERROR_NONE;
}


int recorder_dispatcher_set_sound_stream_info(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	char stream_type[MUSE_RECORDER_MSG_MAX_LENGTH] = {0,};
	int stream_index = 0;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_SOUND_STREAM_INFO;
	muse_recorder_api_class_e class = MUSE_RECORDER_API_CLASS_IMMEDIATE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		ret = RECORDER_ERROR_INVALID_OPERATION;
		muse_recorder_msg_return(api, class, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	muse_recorder_msg_get_string(stream_type, muse_core_client_get_msg(module));
	muse_recorder_msg_get(stream_index, muse_core_client_get_msg(module));

	ret = legacy_recorder_set_sound_stream_info(muse_recorder->recorder_handle, stream_type, stream_index);

	muse_recorder_msg_return(api, class, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}


int (*dispatcher[MUSE_RECORDER_API_MAX]) (muse_module_h module) = {
	recorder_dispatcher_create, /* MUSE_RECORDER_API_CREATE */
	recorder_dispatcher_destroy, /* MUSE_RECORDER_API_DESTROY */
	recorder_dispatcher_get_state, /* MUSE_RECORDER_API_GET_STATE */
	recorder_dispatcher_prepare, /* MUSE_RECORDER_API_PREPARE */
	recorder_dispatcher_unprepare, /* MUSE_RECORDER_API_UNPREPARE */
	recorder_dispatcher_start, /* MUSE_RECORDER_API_START */
	recorder_dispatcher_pause, /* MUSE_RECORDER_API_PAUSE */
	recorder_dispatcher_commit, /* MUSE_RECORDER_API_COMMIT */
	recorder_dispatcher_cancel, /* MUSE_RECORDER_API_CANCEL */
	recorder_dispatcher_set_video_resolution, /* MUSE_RECORDER_API_SET_VIDEO_RESOLUTION */
	recorder_dispatcher_get_video_resolution, /* MUSE_RECORDER_API_GET_VIDEO_RESOLUTION */
	recorder_dispatcher_foreach_supported_video_resolution, /* MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_RESOLUTION */
	recorder_dispatcher_get_audio_level, /* MUSE_RECORDER_API_GET_AUDIO_LEVEL */
	recorder_dispatcher_set_filename, /* MUSE_RECORDER_API_SET_FILENAME */
	recorder_dispatcher_get_filename, /* MUSE_RECORDER_API_GET_FILENAME */
	recorder_dispatcher_set_file_format, /* MUSE_RECORDER_API_SET_FILE_FORMAT */
	recorder_dispatcher_get_file_format, /* MUSE_RECORDER_API_GET_FILE_FORMAT */
	recorder_dispatcher_set_state_changed_cb, /* MUSE_RECORDER_API_SET_STATE_CHANGED_CB */
	recorder_dispatcher_unset_state_changed_cb, /* MUSE_RECORDER_API_UNSET_STATE_CHANGED_CB */
	recorder_dispatcher_set_interrupted_cb, /* MUSE_RECORDER_API_SET_INTERRUPTED_CB */
	recorder_dispatcher_unset_interrupted_cb, /* MUSE_RECORDER_API_UNSET_INTERRUPTED_CB */
	recorder_dispatcher_set_audio_stream_cb, /* MUSE_RECORDER_API_SET_AUDIO_STREAM_CB */
	recorder_dispatcher_unset_audio_stream_cb, /* MUSE_RECORDER_API_UNSET_AUDIO_STREAM_CB */
	recorder_dispatcher_set_error_cb, /* MUSE_RECORDER_API_SET_ERROR_CB */
	recorder_dispatcher_unset_error_cb, /* MUSE_RECORDER_API_UNSET_ERROR_CB */
	recorder_dispatcher_set_recording_status_cb, /* MUSE_RECORDER_API_SET_RECORDING_STATUS_CB */
	recorder_dispatcher_unset_recording_status_cb, /* MUSE_RECORDER_API_UNSET_RECORDING_STATUS_CB */
	recorder_dispatcher_set_recording_limit_reached_cb, /* MUSE_RECORDER_API_SET_RECORDING_LIMIT_RECHEAD_CB */
	recorder_dispatcher_unset_recording_limit_reached_cb, /* MUSE_RECORDER_API_UNSET_RECORDING_LIMIT_RECHEAD_CB */
	recorder_dispatcher_foreach_supported_file_format, /* MUSE_RECORDER_API_FOREACH_SUPPORTED_FILE_FORMAT */
	recorder_dispatcher_attr_set_size_limit, /* MUSE_RECORDER_API_ATTR_SET_SIZE_LIMIT */
	recorder_dispatcher_attr_set_time_limit, /* MUSE_RECORDER_API_ATTR_SET_TIME_LIMIT */
	recorder_dispatcher_attr_set_audio_device, /* MUSE_RECORDER_API_ATTR_SET_AUDIO_DEVICE */
	recorder_dispatcher_set_audio_encoder, /* MUSE_RECORDER_API_SET_AUDIO_ENCODER */
	recorder_dispatcher_get_audio_encoder, /* MUSE_RECORDER_API_GET_AUDIO_ENCODER */
	recorder_dispatcher_set_video_encoder, /* MUSE_RECORDER_API_SET_VIDEO_ENCODER */
	recorder_dispatcher_get_video_encoder, /* MUSE_RECORDER_API_GET_VIDEO_ENCODER */
	recorder_dispatcher_attr_set_audio_samplerate, /* MUSE_RECORDER_API_ATTR_SET_AUDIO_SAMPLERATE */
	recorder_dispatcher_attr_set_audio_encoder_bitrate, /* MUSE_RECORDER_API_ATTR_SET_AUDIO_ENCODER_BITRATE */
	recorder_dispatcher_attr_set_video_encoder_bitrate, /* MUSE_RECORDER_API_ATTR_SET_VIDEO_ENCODER_BITRATE */
	recorder_dispatcher_attr_get_size_limit, /* MUSE_RECORDER_API_ATTR_GET_SIZE_LIMIT */
	recorder_dispatcher_attr_get_time_limit, /* MUSE_RECORDER_API_ATTR_GET_TIME_LIMIT */
	recorder_dispatcher_attr_get_audio_device, /* MUSE_RECORDER_API_ATTR_GET_AUDIO_DEVICE */
	recorder_dispatcher_attr_get_audio_samplerate, /* MUSE_RECORDER_API_ATTR_GET_AUDIO_SAMPLERATE */
	recorder_dispatcher_attr_get_audio_encoder_bitrate, /* MUSE_RECORDER_API_ATTR_GET_AUDIO_ENCODER_BITRATE */
	recorder_dispatcher_attr_get_video_encoder_bitrate, /* MUSE_RECORDER_API_ATTR_GET_VIDEO_ENCODER_BITRATE */
	recorder_dispatcher_foreach_supported_audio_encoder, /* MUSE_RECORDER_API_FOREACH_SUPPORTED_AUDIO_ENCODER */
	recorder_dispatcher_foreach_supported_video_encoder, /* MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_ENCODER */
	recorder_dispatcher_attr_set_mute, /* MUSE_RECORDER_API_ATTR_SET_MUTE */
	recorder_dispatcher_attr_is_muted, /* MUSE_RECORDER_API_ATTR_IS_MUTED */
	recorder_dispatcher_attr_set_recording_motion_rate, /* MUSE_RECORDER_API_ATTR_SET_RECORDING_MOTION_RATE */
	recorder_dispatcher_attr_get_recording_motion_rate, /* MUSE_RECORDER_API_ATTR_GET_RECORDING_MOTION_RATE */
	recorder_dispatcher_attr_set_audio_channel, /* MUSE_RECORDER_API_ATTR_SET_AUDIO_CHANNEL */
	recorder_dispatcher_attr_get_audio_channel, /* MUSE_RECORDER_API_ATTR_GET_AUDIO_CHANNEL */
	recorder_dispatcher_attr_set_orientation_tag, /* MUSE_RECORDER_API_ATTR_SET_ORIENTATION_TAG */
	recorder_dispatcher_attr_get_orientation_tag, /* MUSE_RECORDER_API_ATTR_GET_ORIENTATION_TAG */
	recorder_dispatcher_attr_set_root_directory, /* MUSE_RECORDER_API_ATTR_SET_ROOT_DIRECTORY */
	recorder_dispatcher_return_buffer, /* MUSE_RECORDER_API_RETURN_BUFFER */
	recorder_dispatcher_set_sound_stream_info /* MUSE_RECORDER_API_SET_SOUND_STREAM_INFO */
};


/******************/
/* cmd dispatcher */
/******************/
static int recorder_cmd_dispatcher_shutdown(muse_module_h module)
{
	recorder_state_e state = RECORDER_STATE_NONE;
	muse_recorder_handle_s *muse_recorder = NULL;

	muse_recorder = (muse_recorder_handle_s *)muse_core_ipc_get_handle(module);
	if (muse_recorder == NULL) {
		LOGE("NULL handle");
		return MUSE_RECORDER_ERROR_NONE;
	}

	legacy_recorder_get_state(muse_recorder->recorder_handle, &state);

	LOGW("current state : %d", state);

	switch (state) {
	case RECORDER_STATE_PAUSED:
	case RECORDER_STATE_RECORDING:
		legacy_recorder_commit(muse_recorder->recorder_handle);
	case RECORDER_STATE_READY:
		legacy_recorder_unprepare(muse_recorder->recorder_handle);
	case RECORDER_STATE_CREATED:
		if (legacy_recorder_destroy(muse_recorder->recorder_handle) == RECORDER_ERROR_NONE) {
			_recorder_remove_export_data(module, 0, TRUE);

			g_mutex_clear(&muse_recorder->list_lock);

			muse_recorder->bufmgr = NULL;

			free(muse_recorder);
			muse_recorder = NULL;
		}
	default:
		break;
	}

	LOGW("done");

	return MUSE_RECORDER_ERROR_NONE;
}

int (*cmd_dispatcher[MUSE_MODULE_EVENT_MAX])(muse_module_h module) = {
	NULL, /* MUSE_MODULE_EVENT_INITIALIZE */
	recorder_cmd_dispatcher_shutdown, /* MUSE_MODULE_EVENT_SHUTDOWN */
	NULL, /* MUSE_MODULE_EVENT_DEBUG_INFO_DUMP */
};
