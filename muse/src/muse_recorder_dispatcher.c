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
#include "legacy_recorder_internal.h"
#include <muse_core.h>
#include <muse_core_ipc.h>
#include <muse_core_security.h>
#include <muse_camera.h>
#include <mm_types.h>
#include <dlog.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MMSVC_RECORDER"

#define RECORDER_PRIVILEGE_NAME "http://tizen.org/privilege/recorder"


void _recorder_disp_recording_limit_reached_cb(recorder_recording_limit_type_e type, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	LOGD("Enter");
	int cb_type = (int)type;
	muse_recorder_msg_event1(MUSE_RECORDER_CB_EVENT,
								MUSE_RECORDER_EVENT_TYPE_RECORDING_LIMITED,
								module,
								INT, cb_type);
	return;
}

void _recorder_disp_recording_status_cb(unsigned long long elapsed_time, unsigned long long file_size, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	LOGD("Enter");
	double cb_elapsed_time = elapsed_time;
	double cb_file_size = file_size;
	muse_recorder_msg_event2(MUSE_RECORDER_CB_EVENT,
								MUSE_RECORDER_EVENT_TYPE_RECORDING_STATUS,
								module,
								DOUBLE, cb_elapsed_time,
								DOUBLE, cb_file_size);
	return;
}

void _recorder_disp_state_changed_cb(recorder_state_e previous , recorder_state_e current , bool by_policy, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	LOGD("Enter");
	int cb_previous = (int)previous;
	int cb_current = (int)current;
	int cb_by_policy = (int)by_policy;
	muse_recorder_msg_event3(MUSE_RECORDER_CB_EVENT,
								MUSE_RECORDER_EVENT_TYPE_STATE_CHANGE,
								module,
								INT, cb_previous,
								INT, cb_current,
								INT, cb_by_policy);
	return;
}

void _recorder_disp_interrupted_cb(recorder_policy_e policy, recorder_state_e previous, recorder_state_e current, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;

	LOGD("Enter");
	int cb_policy = (int)policy;
	int cb_previous = (int)previous;
	int cb_current = (int)current;
	muse_recorder_msg_event3(MUSE_RECORDER_CB_EVENT,
								MUSE_RECORDER_EVENT_TYPE_INTERRUPTED,
								module,
								INT, cb_policy,
								INT, cb_previous,
								INT, cb_current);
	return;
}

void _recorder_disp_error_cb(recorder_error_e error, recorder_state_e current_state, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;
	int cb_error = (int)error;
	int cb_current_state = (int)current_state;
	LOGD("Enter");

	muse_recorder_msg_event2(MUSE_RECORDER_CB_EVENT,
							    MUSE_RECORDER_EVENT_TYPE_INTERRUPTED,
							    module,
							    INT, cb_error,
							    INT, cb_current_state);
	return;
}

void _recorder_disp_audio_stream_cb(void* stream, int size, audio_sample_type_e format, int channel, unsigned int timestamp, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;
	int cb_size = size;
	int cb_format = (int)format;
	int cb_channel = channel;
	int cb_timestamp = timestamp;
	muse_recorder_transport_info_s transport_info;
	muse_recorder_info_s *recorder_data;
	int tKey = 0;
	LOGD("Enter");

	/* Initial TBM setting */
	transport_info.data_size = size;
	recorder_data = (muse_recorder_info_s *)muse_core_client_get_cust_data(module);
	transport_info.bufmgr = recorder_data->bufmgr;

	if (muse_recorder_ipc_make_tbm(&transport_info) == FALSE) {
		LOGE("TBM Init failed");
		return;
	}
	LOGD("bohandle_ptr : 0x%x, export_bo : %d, tSize : %d", transport_info.bo_handle.ptr, transport_info.bo, transport_info.data_size);

	memcpy(transport_info.bo_handle.ptr, stream, size);
	tKey = muse_recorder_ipc_export_tbm(transport_info);

	if(tKey == 0) {
		LOGE("Create key_info ERROR!!");
		muse_recorder_ipc_unref_tbm(&transport_info);
		return;
	}

	muse_recorder_msg_event5(MUSE_RECORDER_CB_EVENT,
							    MUSE_RECORDER_EVENT_TYPE_AUDIO_STREAM,
							    module,
							    INT, cb_size,
							    INT, cb_format,
							    INT, cb_channel,
							    INT, cb_timestamp,
							    INT, tKey);

	muse_recorder_ipc_unref_tbm(&transport_info);
	return;
}

void _recorder_disp_foreach_supported_video_resolution_cb(int width, int height, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;
	LOGD("Enter");

	muse_recorder_msg_event2(MUSE_RECORDER_CB_EVENT,
							    MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_RESOLUTION,
							    module,
							    INT, width,
							    INT, height);
	return;
}

void _recorder_disp_foreach_supported_file_format_cb(recorder_file_format_e format, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;
	int cb_format = (int)format;
	LOGD("Enter");

	muse_recorder_msg_event1(MUSE_RECORDER_CB_EVENT,
							    MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_FILE_FORMAT,
							    module,
							    INT, cb_format);
	return;
}

void _recorder_disp_foreach_supported_audio_encoder_cb(recorder_audio_codec_e codec, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;
	int cb_codec = (int)codec;
	LOGD("Enter");

	muse_recorder_msg_event1(MUSE_RECORDER_CB_EVENT,
							    MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_AUDIO_ENCODER,
							    module,
							    INT, cb_codec);
	return;
}

void _recorder_disp_foreach_supported_video_encoder_cb(recorder_video_codec_e codec, void *user_data)
{
	muse_module_h module = (muse_module_h)user_data;
	int cb_codec = (int)codec;
	LOGD("Enter");

	muse_recorder_msg_event1(MUSE_RECORDER_CB_EVENT,
							    MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_ENCODER,
							    module,
							    INT, cb_codec);
	return;
}

int recorder_dispatcher_create(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_CREATE;
	recorder_h recorder = NULL;;
	intptr_t camera_handle;
	muse_camera_handle_s *muse_camera = NULL;
	muse_recorder_info_s *recorder_data;
	tbm_bufmgr bufmgr;
	int recorder_type;
	int client_fd = -1;
	int pid = 0;
	intptr_t handle;

	LOGD("Enter");

	muse_recorder_msg_get(recorder_type, muse_core_client_get_msg(module));

	/* privilege check */
	client_fd = muse_core_client_get_msg_fd(module);
	if (!muse_core_security_check_cynara(client_fd, RECORDER_PRIVILEGE_NAME)) {
		LOGE("security check failed");
		ret = RECORDER_ERROR_PERMISSION_DENIED;
		muse_recorder_msg_return(api, ret, module);
		return MUSE_RECORDER_ERROR_NONE;
	}

	if (recorder_type == MUSE_RECORDER_TYPE_VIDEO) {
		muse_recorder_msg_get_pointer(camera_handle, muse_core_client_get_msg(module));
		if (camera_handle == 0) {
			LOGE("NULL handle");
			ret = RECORDER_ERROR_INVALID_PARAMETER;
			muse_recorder_msg_return(api, ret, module);
			return MUSE_RECORDER_ERROR_NONE;
		}

		muse_camera = (muse_recorder_info_s *)camera_handle;

		LOGD("video type, camera handle : %p", muse_camera->camera_handle);

		ret = legacy_recorder_create_videorecorder(muse_camera->camera_handle, &recorder);
	} else if (recorder_type == MUSE_RECORDER_TYPE_AUDIO) {
		muse_recorder_msg_get(pid, muse_core_client_get_msg(module));

		LOGD("audio type - pid %d", pid);
		ret = legacy_recorder_create_audiorecorder(&recorder);
		if (ret == RECORDER_ERROR_NONE) {
			ret = legacy_recorder_set_client_pid(recorder, pid);
			if (ret != RECORDER_ERROR_NONE) {
				LOGE("legacy_recorder_set_client_pid failed 0x%x", ret);
				legacy_recorder_destroy(recorder);
				recorder = NULL;
			}
		}
	}

	if (ret == RECORDER_ERROR_NONE) {
		handle = (intptr_t)recorder;
		LOGD("recorder handle : 0x%x, api : %d, module", handle, api, module);
		muse_core_ipc_set_handle(module, handle);

		recorder_data = (muse_recorder_info_s *)g_new(muse_recorder_info_s, sizeof(muse_recorder_info_s));
		muse_core_ipc_get_bufmgr(&bufmgr);
		LOGD("bufmgr: 0x%x", bufmgr);
		if (bufmgr != NULL) {
			recorder_data->bufmgr = bufmgr;
			muse_core_client_set_cust_data(module, (void *)recorder_data);
		} else {
			LOGE("TBM bufmgr is NULL => check the legacy_core.");
		}
		muse_recorder_msg_return1(api, ret, module, POINTER, handle);
	} else {
		LOGE("error 0x%d", ret);
		muse_recorder_msg_return(api, ret, module);
	}

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_destroy(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_DESTROY;
	muse_recorder_info_s *recorder_data;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_destroy((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	recorder_data = (muse_recorder_info_s *)muse_core_client_get_cust_data(module);
	if (recorder_data != NULL) {
		g_free(recorder_data);
		recorder_data = NULL;
	}

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_state(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_STATE;
	intptr_t handle;
	recorder_state_e state;
	int get_state;

	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_get_state((recorder_h)handle, &state);
	get_state = (int)state;
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_state);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_prepare(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_PREPARE;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_prepare((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unprepare(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNPREPARE;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_unprepare((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_start(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_START;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_start((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_pause(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_PAUSE;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_pause((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_commit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_COMMIT;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_commit((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_cancel(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_CANCEL;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_cancel((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_video_resolution(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int width;
	int height;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_VIDEO_RESOLUTION;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(width, muse_core_client_get_msg(module));
	muse_recorder_msg_get(height, muse_core_client_get_msg(module));
	ret = legacy_recorder_set_video_resolution((recorder_h)handle, width, height);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_video_resolution(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int get_width;
	int get_height;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_VIDEO_RESOLUTION;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_get_video_resolution((recorder_h)handle, &get_width, &get_height);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return2(api,
								ret,
								module,
								INT, get_width,
								INT, get_height);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_video_resolution(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_RESOLUTION;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_foreach_supported_video_resolution((recorder_h)handle,
							(recorder_supported_video_resolution_cb)_recorder_disp_foreach_supported_video_resolution_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_audio_level(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	double get_level;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_AUDIO_LEVEL;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_get_audio_level((recorder_h)handle, &get_level);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api,
								ret,
								module,
								DOUBLE, get_level);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_filename(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	char filename[MUSE_RECORDER_MSG_MAX_LENGTH] = {0,};
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_FILENAME;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get_string(filename, muse_core_client_get_msg(module));
	ret = legacy_recorder_set_filename((recorder_h)handle, filename);
	LOGD("handle : 0x%x, filename : %s", handle, filename);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_filename(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	char *get_filename;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_FILENAME;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_get_filename((recorder_h)handle, &get_filename);
	LOGD("handle : 0x%x, filename : %s", handle, get_filename);
	muse_recorder_msg_return1(api, ret, module, STRING, get_filename);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_file_format(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int set_format;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_FILE_FORMAT;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(set_format, muse_core_client_get_msg(module));
	ret = legacy_recorder_set_file_format((recorder_h)handle, (recorder_file_format_e)set_format);
	LOGD("handle : 0x%x, set_format : %d", handle, set_format);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_file_format(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	recorder_file_format_e format;
	int get_format;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_FILE_FORMAT;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_get_file_format((recorder_h)handle, &format);
	get_format = (int)format;
	LOGD("handle : 0x%x, get_format : %d", handle, get_format);
	muse_recorder_msg_return1(api, ret, module, INT, get_format);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_state_changed_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_STATE_CHANGED_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_set_state_changed_cb((recorder_h)handle,
							(recorder_state_changed_cb)_recorder_disp_state_changed_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_state_changed_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_STATE_CHANGED_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_unset_state_changed_cb((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_interrupted_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_INTERRUPTED_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_set_interrupted_cb((recorder_h)handle,
							(recorder_interrupted_cb)_recorder_disp_interrupted_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_interrupted_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_INTERRUPTED_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_unset_interrupted_cb((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_audio_stream_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_AUDIO_STREAM_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_set_audio_stream_cb((recorder_h)handle,
							(recorder_audio_stream_cb)_recorder_disp_audio_stream_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_audio_stream_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_AUDIO_STREAM_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_unset_audio_stream_cb((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_error_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_ERROR_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_set_error_cb((recorder_h)handle,
							(recorder_error_cb)_recorder_disp_error_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_error_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_ERROR_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_unset_error_cb((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_recording_status_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_RECORDING_STATUS_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_set_recording_status_cb((recorder_h)handle,
							(recorder_recording_status_cb)_recorder_disp_recording_status_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_recording_status_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_RECORDING_STATUS_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_unset_recording_status_cb((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_recording_limit_reached_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_RECORDING_LIMIT_REACHED_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_set_recording_limit_reached_cb((recorder_h)handle,
							(recorder_recording_limit_reached_cb)_recorder_disp_recording_limit_reached_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_recording_limit_reached_cb(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_UNSET_RECORDING_LIMIT_REACHED_CB;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_unset_recording_limit_reached_cb((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_file_format(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_FOREACH_SUPPORTED_FILE_FORMAT;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_foreach_supported_file_format((recorder_h)handle,
							(recorder_supported_file_format_cb)_recorder_disp_foreach_supported_file_format_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_size_limit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int kbyte;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_SIZE_LIMIT;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(kbyte, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_size_limit((recorder_h)handle, kbyte);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_time_limit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int second;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_TIME_LIMIT;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(second, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_time_limit((recorder_h)handle, second);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_device(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int set_device;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_AUDIO_DEVICE;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(set_device, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_audio_device((recorder_h)handle, (recorder_audio_device_e)set_device);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_audio_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int set_codec;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_AUDIO_ENCODER;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(set_codec, muse_core_client_get_msg(module));
	ret = legacy_recorder_set_audio_encoder((recorder_h)handle, (recorder_audio_codec_e)set_codec);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_audio_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	recorder_audio_codec_e codec;
	int get_codec;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_AUDIO_ENCODER;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_get_audio_encoder((recorder_h)handle, &codec);
	get_codec = (int)codec;
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_codec);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_video_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int set_codec;
	muse_recorder_api_e api = MUSE_RECORDER_API_SET_VIDEO_ENCODER;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(set_codec, muse_core_client_get_msg(module));
	ret = legacy_recorder_set_video_encoder((recorder_h)handle, (recorder_video_codec_e)set_codec);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_video_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	recorder_video_codec_e codec;
	int get_codec;
	muse_recorder_api_e api = MUSE_RECORDER_API_GET_VIDEO_ENCODER;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_get_video_encoder((recorder_h)handle, &codec);
	get_codec = (int)codec;
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_codec);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_samplerate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int samplerate;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_AUDIO_SAMPLERATE;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(samplerate, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_audio_samplerate((recorder_h)handle, samplerate);
	LOGD("handle : 0x%x samplerate : %d", handle, samplerate);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_encoder_bitrate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int bitrate;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_AUDIO_ENCODER_BITRATE;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(bitrate, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_audio_encoder_bitrate((recorder_h)handle, bitrate);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_video_encoder_bitrate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int bitrate;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_VIDEO_ENCODER_BITRATE;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(bitrate, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_video_encoder_bitrate((recorder_h)handle, bitrate);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_size_limit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int get_kbyte;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_SIZE_LIMIT;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_size_limit((recorder_h)handle, &get_kbyte);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_kbyte);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_time_limit(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int get_second;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_TIME_LIMIT;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_time_limit((recorder_h)handle, &get_second);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_second);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_device(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	recorder_audio_device_e device;
	int get_device;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_AUDIO_DEVICE;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_audio_device((recorder_h)handle, &device);
	get_device = (int)device;
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_device);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_samplerate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int get_samplerate;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_AUDIO_SAMPLERATE;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_audio_samplerate((recorder_h)handle, &get_samplerate);
	LOGD("handle : 0x%x, get_samplerate : %d", handle, get_samplerate);
	muse_recorder_msg_return1(api, ret, module, INT, get_samplerate);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_encoder_bitrate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int get_bitrate;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_AUDIO_ENCODER_BITRATE;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_audio_encoder_bitrate((recorder_h)handle, &get_bitrate);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_bitrate);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_video_encoder_bitrate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int get_bitrate;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_VIDEO_ENCODER_BITRATE;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_video_encoder_bitrate((recorder_h)handle, &get_bitrate);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_bitrate);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_audio_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_FOREACH_SUPPORTED_AUDIO_ENCODER;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_foreach_supported_audio_encoder((recorder_h)handle,
							(recorder_supported_audio_encoder_cb)_recorder_disp_foreach_supported_audio_encoder_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_video_encoder(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_ENCODER;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_foreach_supported_video_encoder((recorder_h)handle,
							(recorder_supported_video_encoder_cb)_recorder_disp_foreach_supported_video_encoder_cb,
							(void *)module);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_mute(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int set_enable;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_MUTE;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(set_enable, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_mute((recorder_h)handle, (bool)set_enable);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_is_muted(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_IS_MUTED;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_is_muted((recorder_h)handle);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_recording_motion_rate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	double rate;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_RECORDING_MOTION_RATE;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(rate, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_recording_motion_rate((recorder_h)handle, rate);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_recording_motion_rate(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	double get_rate;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_RECORDING_MOTION_RATE;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_recording_motion_rate((recorder_h)handle, &get_rate);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, DOUBLE, get_rate);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_channel(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int channel_count;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_AUDIO_CHANNEL;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(channel_count, muse_core_client_get_msg(module));
	LOGD("channel_count : %d", channel_count);
	ret = legacy_recorder_attr_set_audio_channel((recorder_h)handle, channel_count);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_channel(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int get_channel_count;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_AUDIO_CHANNEL;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_audio_channel((recorder_h)handle, &get_channel_count);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_channel_count);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_orientation_tag(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	int set_orientation;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_ORIENTATION_TAG;
	handle = muse_core_ipc_get_handle(module);
	muse_recorder_msg_get(set_orientation, muse_core_client_get_msg(module));
	ret = legacy_recorder_attr_set_orientation_tag((recorder_h)handle, (recorder_rotation_e)set_orientation);
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_orientation_tag(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	recorder_rotation_e orientation;
	int get_orientation;
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_GET_ORIENTATION_TAG;
	handle = muse_core_ipc_get_handle(module);
	ret = legacy_recorder_attr_get_orientation_tag((recorder_h)handle, &orientation);
	get_orientation = (int)orientation;
	LOGD("handle : 0x%x", handle);
	muse_recorder_msg_return1(api, ret, module, INT, get_orientation);

	return MUSE_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_root_directory(muse_module_h module)
{
	int ret = RECORDER_ERROR_NONE;
	intptr_t handle;
	char root_directory[MUSE_RECORDER_MSG_MAX_LENGTH] = {0,};
	muse_recorder_api_e api = MUSE_RECORDER_API_ATTR_SET_ROOT_DIRECTORY;

	handle = muse_core_ipc_get_handle(module);

	muse_recorder_msg_get_string(root_directory, muse_core_client_get_msg(module));

	ret = legacy_recorder_attr_set_root_directory((recorder_h)handle, root_directory);

	LOGD("handle : 0x%x, root_directory : %s", handle, root_directory);

	muse_recorder_msg_return(api, ret, module);

	return MUSE_RECORDER_ERROR_NONE;
}

int (*dispatcher[MUSE_RECORDER_API_MAX]) (muse_module_h module) = {
	recorder_dispatcher_create, /* MUSE_RECORDER_API_CREATE, */
	recorder_dispatcher_destroy, /* MUSE_RECORDER_API_DESTROY, */
	recorder_dispatcher_get_state, /* MUSE_RECORDER_API_GET_STATE, */
	recorder_dispatcher_prepare, /* MUSE_RECORDER_API_PREPARE, */
	recorder_dispatcher_unprepare, /* MUSE_RECORDER_API_UNPREPARE, */
	recorder_dispatcher_start, /* MUSE_RECORDER_API_START, */
	recorder_dispatcher_pause, /* MUSE_RECORDER_API_PAUSE, */
	recorder_dispatcher_commit, /* MUSE_RECORDER_API_COMMIT, */
	recorder_dispatcher_cancel, /* MUSE_RECORDER_API_CANCEL, */
	recorder_dispatcher_set_video_resolution, /* MUSE_RECORDER_API_SET_VIDEO_RESOLUTION, */
	recorder_dispatcher_get_video_resolution, /* MUSE_RECORDER_API_GET_VIDEO_RESOLUTION, */
	recorder_dispatcher_foreach_supported_video_resolution, /* MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_RESOLUTION, */
	recorder_dispatcher_get_audio_level, /* MUSE_RECORDER_API_GET_AUDIO_LEVEL, */
	recorder_dispatcher_set_filename, /* MUSE_RECORDER_API_SET_FILENAME, */
	recorder_dispatcher_get_filename, /* MUSE_RECORDER_API_GET_FILENAME, */
	recorder_dispatcher_set_file_format, /* MUSE_RECORDER_API_SET_FILE_FORMAT, */
	recorder_dispatcher_get_file_format, /* MUSE_RECORDER_API_GET_FILE_FORMAT, */
	recorder_dispatcher_set_state_changed_cb, /* MUSE_RECORDER_API_SET_STATE_CHANGED_CB, */
	recorder_dispatcher_unset_state_changed_cb, /* MUSE_RECORDER_API_UNSET_STATE_CHANGED_CB, */
	recorder_dispatcher_set_interrupted_cb, /* MUSE_RECORDER_API_SET_INTERRUPTED_CB, */
	recorder_dispatcher_unset_interrupted_cb, /* MUSE_RECORDER_API_UNSET_INTERRUPTED_CB, */
	recorder_dispatcher_set_audio_stream_cb, /* MUSE_RECORDER_API_SET_AUDIO_STREAM_CB, */
	recorder_dispatcher_unset_audio_stream_cb, /* MUSE_RECORDER_API_UNSET_AUDIO_STREAM_CB, */
	recorder_dispatcher_set_error_cb, /* MUSE_RECORDER_API_SET_ERROR_CB, */
	recorder_dispatcher_unset_error_cb, /* MUSE_RECORDER_API_UNSET_ERROR_CB, */
	recorder_dispatcher_set_recording_status_cb, /* MUSE_RECORDER_API_SET_RECORDING_STATUS_CB, */
	recorder_dispatcher_unset_recording_status_cb, /* MUSE_RECORDER_API_UNSET_RECORDING_STATUS_CB, */
	recorder_dispatcher_set_recording_limit_reached_cb, /* MUSE_RECORDER_API_SET_RECORDING_LIMIT_RECHEAD_CB, */
	recorder_dispatcher_unset_recording_limit_reached_cb, /* MUSE_RECORDER_API_UNSET_RECORDING_LIMIT_RECHEAD_CB, */
	recorder_dispatcher_foreach_supported_file_format, /* MUSE_RECORDER_API_FOREACH_SUPPORTED_FILE_FORMAT, */
	recorder_dispatcher_attr_set_size_limit, /* MUSE_RECORDER_API_ATTR_SET_SIZE_LIMIT, */
	recorder_dispatcher_attr_set_time_limit, /* MUSE_RECORDER_API_ATTR_SET_TIME_LIMIT, */
	recorder_dispatcher_attr_set_audio_device, /* MUSE_RECORDER_API_ATTR_SET_AUDIO_DEVICE, */
	recorder_dispatcher_set_audio_encoder, /* MUSE_RECORDER_API_SET_AUDIO_ENCODER, */
	recorder_dispatcher_get_audio_encoder, /* MUSE_RECORDER_API_GET_AUDIO_ENCODER, */
	recorder_dispatcher_set_video_encoder, /* MUSE_RECORDER_API_SET_VIDEO_ENCODER, */
	recorder_dispatcher_get_video_encoder, /* MUSE_RECORDER_API_GET_VIDEO_ENCODER, */
	recorder_dispatcher_attr_set_audio_samplerate, /* MUSE_RECORDER_API_ATTR_SET_AUDIO_SAMPLERATE, */
	recorder_dispatcher_attr_set_audio_encoder_bitrate, /* MUSE_RECORDER_API_ATTR_SET_AUDIO_ENCODER_BITRATE, */
	recorder_dispatcher_attr_set_video_encoder_bitrate, /* MUSE_RECORDER_API_ATTR_SET_VIDEO_ENCODER_BITRATE, */
	recorder_dispatcher_attr_get_size_limit, /* MUSE_RECORDER_API_ATTR_GET_SIZE_LIMIT, */
	recorder_dispatcher_attr_get_time_limit, /* MUSE_RECORDER_API_ATTR_GET_TIME_LIMIT, */
	recorder_dispatcher_attr_get_audio_device, /* MUSE_RECORDER_API_ATTR_GET_AUDIO_DEVICE, */
	recorder_dispatcher_attr_get_audio_samplerate, /* MUSE_RECORDER_API_ATTR_GET_AUDIO_SAMPLERATE, */
	recorder_dispatcher_attr_get_audio_encoder_bitrate, /* MUSE_RECORDER_API_ATTR_GET_AUDIO_ENCODER_BITRATE, */
	recorder_dispatcher_attr_get_video_encoder_bitrate, /* MUSE_RECORDER_API_ATTR_GET_VIDEO_ENCODER_BITRATE, */
	recorder_dispatcher_foreach_supported_audio_encoder, /* MUSE_RECORDER_API_FOREACH_SUPPORTED_AUDIO_ENCODER, */
	recorder_dispatcher_foreach_supported_video_encoder, /* MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_ENCODER, */
	recorder_dispatcher_attr_set_mute, /* MUSE_RECORDER_API_ATTR_SET_MUTE, */
	recorder_dispatcher_attr_is_muted, /* MUSE_RECORDER_API_ATTR_IS_MUTED, */
	recorder_dispatcher_attr_set_recording_motion_rate, /* MUSE_RECORDER_API_ATTR_SET_RECORDING_MOTION_RATE, */
	recorder_dispatcher_attr_get_recording_motion_rate, /* MUSE_RECORDER_API_ATTR_GET_RECORDING_MOTION_RATE, */
	recorder_dispatcher_attr_set_audio_channel, /* MUSE_RECORDER_API_ATTR_SET_AUDIO_CHANNEL, */
	recorder_dispatcher_attr_get_audio_channel, /* MUSE_RECORDER_API_ATTR_GET_AUDIO_CHANNEL, */
	recorder_dispatcher_attr_set_orientation_tag, /* MUSE_RECORDER_API_ATTR_SET_ORIENTATION_TAG, */
	recorder_dispatcher_attr_get_orientation_tag, /* MUSE_RECORDER_API_ATTR_GET_ORIENTATION_TAG, */
	recorder_dispatcher_attr_set_root_directory, /* MUSE_RECORDER_API_ATTR_SET_ROOT_DIRECTORY, */
};
