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
#include "mused_recorder.h"
#include "mused_recorder_msg.h"
#include "mmsvc_core.h"
#include "mmsvc_core_ipc.h"
#include "mm_types.h"
#include "mmsvc_recorder.h"
#include <dlog.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MMSVC_RECORDER"

void _mmsvc_recorder_disp_recording_limit_reached_cb(recorder_recording_limit_type_e type, void *user_data)
{
	Client client = (Client)user_data;

	LOGW("Enter");
	int cb_type = (int)type;
	mmsvc_recorder_msg_event1(MMSVC_RECORDER_CB_EVENT,
								MMSVC_RECORDER_EVENT_TYPE_RECORDING_LIMITED,
								client,
								INT, cb_type);
	return;
}

void _mmsvc_recorder_disp_recording_status_cb(unsigned long long elapsed_time, unsigned long long file_size, void *user_data)
{
	Client client = (Client)user_data;

	LOGW("Enter");
	double cb_elapsed_time = elapsed_time;
	double cb_file_size = file_size;
	mmsvc_recorder_msg_event2(MMSVC_RECORDER_CB_EVENT,
								MMSVC_RECORDER_EVENT_TYPE_RECORDING_STATUS,
								client,
								DOUBLE, cb_elapsed_time,
								DOUBLE, cb_file_size);
	return;
}

void _mmsvc_recorder_disp_state_changed_cb(recorder_state_e previous , recorder_state_e current , bool by_policy, void *user_data)
{
	Client client = (Client)user_data;

	LOGW("Enter");
	int cb_previous = (int)previous;
	int cb_current = (int)current;
	int cb_by_policy = (int)by_policy;
	mmsvc_recorder_msg_event3(MMSVC_RECORDER_CB_EVENT,
								MMSVC_RECORDER_EVENT_TYPE_STATE_CHANGE,
								client,
								INT, cb_previous,
								INT, cb_current,
								INT, cb_by_policy);
	return;
}

void _mmsvc_recorder_disp_interrupted_cb(recorder_policy_e policy, recorder_state_e previous, recorder_state_e current, void *user_data)
{
	Client client = (Client)user_data;

	LOGW("Enter");
	int cb_policy = (int)policy;
	int cb_previous = (int)previous;
	int cb_current = (int)current;
	mmsvc_recorder_msg_event3(MMSVC_RECORDER_CB_EVENT,
								MMSVC_RECORDER_EVENT_TYPE_INTERRUPTED,
								client,
								INT, cb_policy,
								INT, cb_previous,
								INT, cb_current);
	return;
}

void _mmsvc_recorder_disp_error_cb(recorder_error_e error, recorder_state_e current_state, void *user_data)
{
	Client client = (Client)user_data;
	int cb_error = (int)error;
	int cb_current_state = (int)current_state;
	LOGW("Enter");

	mmsvc_recorder_msg_event2(MMSVC_RECORDER_CB_EVENT,
							    MMSVC_RECORDER_EVENT_TYPE_INTERRUPTED,
							    client,
							    INT, cb_error,
							    INT, cb_current_state);
	return;
}

void _mmsvc_recorder_disp_audio_stream_cb(void* stream, int size, audio_sample_type_e format, int channel, unsigned int timestamp, void *user_data)
{
	Client client = (Client)user_data;
	int cb_size = size;
	int cb_format = (int)format;
	int cb_channel = channel;
	int cb_timestamp = timestamp;
	mmsvc_recorder_transport_info_s transport_info;
	int tKey = 0;
	LOGW("Enter");

	transport_info.data_size = size;

	if (mmsvc_recorder_ipc_init_tbm(&transport_info) == FALSE) {
		LOGE("TBM Init failed");
		return;
	}
	LOGW("bohandle_ptr : 0x%x, export_bo : %d, tSize : %d", transport_info.bo_handle.ptr, transport_info.bo, transport_info.data_size);

	memcpy(transport_info.bo_handle.ptr, stream, size);
	tKey = mmsvc_recorder_ipc_export_tbm_bo(transport_info);
	tbm_bo_unmap(transport_info.bo);

	if(tKey == 0) {
		LOGE("Create key_info ERROR!!");
		return;
	}

	mmsvc_recorder_msg_event5(MMSVC_RECORDER_CB_EVENT,
							    MMSVC_RECORDER_EVENT_TYPE_AUDIO_STREAM,
							    client,
							    INT, cb_size,
							    INT, cb_format,
							    INT, cb_channel,
							    INT, cb_timestamp,
							    INT, tKey);
	return;
}

void _mmsvc_recorder_disp_foreach_supported_video_resolution_cb(int width, int height, void *user_data)
{
	Client client = (Client)user_data;
	LOGW("Enter");

	mmsvc_recorder_msg_event2(MMSVC_RECORDER_CB_EVENT,
							    MMSVC_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_RESOLUTION,
							    client,
							    INT, width,
							    INT, height);
	return;
}

void _mmsvc_recorder_disp_foreach_supported_file_format_cb(recorder_file_format_e format, void *user_data)
{
	Client client = (Client)user_data;
	int cb_format = (int)format;
	LOGW("Enter");

	mmsvc_recorder_msg_event1(MMSVC_RECORDER_CB_EVENT,
							    MMSVC_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_FILE_FORMAT,
							    client,
							    INT, cb_format);
	return;
}

void _mmsvc_recorder_disp_foreach_supported_audio_encoder_cb(recorder_audio_codec_e codec, void *user_data)
{
	Client client = (Client)user_data;
	int cb_codec = (int)codec;
	LOGW("Enter");

	mmsvc_recorder_msg_event1(MMSVC_RECORDER_CB_EVENT,
							    MMSVC_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_AUDIO_ENCODER,
							    client,
							    INT, cb_codec);
	return;
}

void _mmsvc_recorder_disp_foreach_supported_video_encoder_cb(recorder_video_codec_e codec, void *user_data)
{
	Client client = (Client)user_data;
	int cb_codec = (int)codec;
	LOGW("Enter");

	mmsvc_recorder_msg_event1(MMSVC_RECORDER_CB_EVENT,
							    MMSVC_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_ENCODER,
							    client,
							    INT, cb_codec);
	return;
}

int recorder_dispatcher_create(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_CREATE;
	recorder_h recorder;
	int camera_handle;
	int recorder_type;
	int handle;
	LOGW("Enter");
	mmsvc_recorder_msg_get(recorder_type, mmsvc_core_client_get_msg(client));
	if (recorder_type == MMSVC_RECORDER_TYPE_VIDEO) {
		mmsvc_recorder_msg_get(camera_handle, mmsvc_core_client_get_msg(client));
		LOGW("video type, camera handle : 0x%x", camera_handle);
		ret = mmsvc_recorder_create_videorecorder((camera_h)camera_handle, &recorder);
	} else if (recorder_type == MMSVC_RECORDER_TYPE_AUDIO) {
		LOGW("audio type");
		ret = mmsvc_recorder_create_audiorecorder(&recorder);
	}
	handle = (int)recorder;
	LOGW("recorder handle : 0x%x, api : %d, client", handle, api, client);
	mmsvc_recorder_msg_return1(api, ret, client, INT, handle);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_destroy(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_DESTROY;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_destroy((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_state(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_GET_STATE;
	int handle;
	recorder_state_e state;
	int get_state;

	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_get_state((recorder_h)handle, &state);
	get_state = (int)state;
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_state);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_prepare(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_PREPARE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_prepare((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unprepare(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_UNPREPARE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_unprepare((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_start(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_START;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_start((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_pause(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_PAUSE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_pause((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_commit(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_COMMIT;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_commit((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_cancel(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_CANCEL;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_cancel((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_video_resolution(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int width;
	int height;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_VIDEO_RESOLUTION;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(width, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(height, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_video_resolution((recorder_h)handle, width, height);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_video_resolution(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int get_width;
	int get_height;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_GET_VIDEO_RESOLUTION;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_get_video_resolution((recorder_h)handle, &get_width, &get_height);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return2(api,
								ret,
								client,
								INT, get_width,
								INT, get_height);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_video_resolution(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_FOREACH_SUPPORTED_VIDEO_RESOLUTION;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_foreach_supported_video_resolution((recorder_h)handle,
							(recorder_supported_video_resolution_cb)_mmsvc_recorder_disp_foreach_supported_video_resolution_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_audio_level(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	double get_level;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_GET_AUDIO_LEVEL;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_get_audio_level((recorder_h)handle, &get_level);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api,
								ret,
								client,
								DOUBLE, get_level);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_filename(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	char filename[MMSVC_RECORDER_MSG_MAX_LENGTH] = {0,};
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_FILENAME;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get_string(filename, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_filename((recorder_h)handle, filename);
	LOGW("handle : 0x%x, filename : %s", handle, filename);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_filename(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	char *get_filename;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_GET_FILENAME;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_get_filename((recorder_h)handle, &get_filename);
	LOGW("handle : 0x%x, filename : %s", handle, get_filename);
	mmsvc_recorder_msg_return1(api, ret, client, STRING, get_filename);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_file_format(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int set_format;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_FILE_FORMAT;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(set_format, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_file_format((recorder_h)handle, (recorder_file_format_e)set_format);
	LOGW("handle : 0x%x, set_format : %d", handle, set_format);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_file_format(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	recorder_file_format_e format;
	int get_format;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_GET_FILE_FORMAT;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_get_file_format((recorder_h)handle, &format);
	get_format = (int)format;
	LOGW("handle : 0x%x, get_format : %d", handle, get_format);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_format);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_state_changed_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_STATE_CHANGED_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_state_changed_cb((recorder_h)handle,
							(recorder_state_changed_cb)_mmsvc_recorder_disp_state_changed_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_state_changed_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_UNSET_STATE_CHANGED_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_unset_state_changed_cb((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_interrupted_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_INTERRUPTED_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_interrupted_cb((recorder_h)handle,
							(recorder_interrupted_cb)_mmsvc_recorder_disp_interrupted_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_interrupted_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_UNSET_INTERRUPTED_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_unset_interrupted_cb((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_audio_stream_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_AUDIO_STREAM_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_audio_stream_cb((recorder_h)handle,
							(recorder_audio_stream_cb)_mmsvc_recorder_disp_audio_stream_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_audio_stream_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_UNSET_AUDIO_STREAM_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_unset_audio_stream_cb((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_error_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_ERROR_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_error_cb((recorder_h)handle,
							(recorder_error_cb)_mmsvc_recorder_disp_error_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_error_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_UNSET_ERROR_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_unset_error_cb((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_recording_status_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_RECORDING_STATUS_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_recording_status_cb((recorder_h)handle,
							(recorder_recording_status_cb)_mmsvc_recorder_disp_recording_status_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_recording_status_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_UNSET_RECORDING_STATUS_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_unset_recording_status_cb((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_recording_limit_reached_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_RECORDING_LIMIT_REACHED_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_recording_limit_reached_cb((recorder_h)handle,
							(recorder_recording_limit_reached_cb)_mmsvc_recorder_disp_recording_limit_reached_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_unset_recording_limit_reached_cb(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_UNSET_RECORDING_LIMIT_REACHED_CB;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_unset_recording_limit_reached_cb((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_file_format(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_FOREACH_SUPPORTED_FILE_FORMAT;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_foreach_supported_file_format((recorder_h)handle,
							(recorder_supported_file_format_cb)_mmsvc_recorder_disp_foreach_supported_file_format_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_size_limit(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int kbyte;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_SIZE_LIMIT;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(kbyte, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_size_limit((recorder_h)handle, kbyte);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_time_limit(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int second;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_TIME_LIMIT;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(second, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_time_limit((recorder_h)handle, second);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_device(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int set_device;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_AUDIO_DEVICE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(set_device, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_audio_device((recorder_h)handle, (recorder_audio_device_e)set_device);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_audio_encoder(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int set_codec;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_AUDIO_ENCODER;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(set_codec, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_audio_encoder((recorder_h)handle, (recorder_audio_codec_e)set_codec);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_audio_encoder(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	recorder_audio_codec_e codec;
	int get_codec;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_GET_AUDIO_ENCODER;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_get_audio_encoder((recorder_h)handle, &codec);
	get_codec = (int)codec;
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_codec);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_set_video_encoder(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int set_codec;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_SET_VIDEO_ENCODER;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(set_codec, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_set_video_encoder((recorder_h)handle, (recorder_video_codec_e)set_codec);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_get_video_encoder(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	recorder_video_codec_e codec;
	int get_codec;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_GET_VIDEO_ENCODER;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_get_video_encoder((recorder_h)handle, &codec);
	get_codec = (int)codec;
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_codec);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_samplerate(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int samplerate;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_AUDIO_SAMPLERATE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(samplerate, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_audio_samplerate((recorder_h)handle, samplerate);
	LOGW("handle : 0x%x samplerate : %d", handle, samplerate);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_encoder_bitrate(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int bitrate;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_AUDIO_ENCODER_BITRATE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(bitrate, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_audio_encoder_bitrate((recorder_h)handle, bitrate);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_video_encoder_bitrate(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int bitrate;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_VIDEO_ENCODER_BITRATE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(bitrate, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_video_encoder_bitrate((recorder_h)handle, bitrate);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_size_limit(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int get_kbyte;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_SIZE_LIMIT;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_size_limit((recorder_h)handle, &get_kbyte);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_kbyte);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_time_limit(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int get_second;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_TIME_LIMIT;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_time_limit((recorder_h)handle, &get_second);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_second);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_device(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	recorder_audio_device_e device;
	int get_device;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_AUDIO_DEVICE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_audio_device((recorder_h)handle, &device);
	get_device = (int)device;
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_device);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_samplerate(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int get_samplerate;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_AUDIO_SAMPLERATE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_audio_samplerate((recorder_h)handle, &get_samplerate);
	LOGW("handle : 0x%x, get_samplerate : %d", handle, get_samplerate);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_samplerate);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_encoder_bitrate(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int get_bitrate;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_AUDIO_ENCODER_BITRATE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_audio_encoder_bitrate((recorder_h)handle, &get_bitrate);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_bitrate);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_video_encoder_bitrate(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int get_bitrate;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_VIDEO_ENCODER_BITRATE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_video_encoder_bitrate((recorder_h)handle, &get_bitrate);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_bitrate);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_audio_encoder(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_FOREACH_SUPPORTED_AUDIO_ENCODER;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_foreach_supported_audio_encoder((recorder_h)handle,
							(recorder_supported_audio_encoder_cb)_mmsvc_recorder_disp_foreach_supported_audio_encoder_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_foreach_supported_video_encoder(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_FOREACH_SUPPORTED_VIDEO_ENCODER;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_foreach_supported_video_encoder((recorder_h)handle,
							(recorder_supported_video_encoder_cb)_mmsvc_recorder_disp_foreach_supported_video_encoder_cb,
							(void *)client);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_mute(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int set_enable;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_MUTE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(set_enable, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_mute((recorder_h)handle, (bool)set_enable);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_is_muted(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_IS_MUTED;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_is_muted((recorder_h)handle);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_recording_motion_rate(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	double rate;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_RECORDING_MOTION_RATE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(rate, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_recording_motion_rate((recorder_h)handle, rate);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_recording_motion_rate(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	double get_rate;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_RECORDING_MOTION_RATE;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_recording_motion_rate((recorder_h)handle, &get_rate);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, DOUBLE, get_rate);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_audio_channel(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int channel_count;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_AUDIO_CHANNEL;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(channel_count, mmsvc_core_client_get_msg(client));
	LOGW("channel_count : %d", channel_count);
	ret = mmsvc_recorder_attr_set_audio_channel((recorder_h)handle, channel_count);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_audio_channel(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int get_channel_count;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_AUDIO_CHANNEL;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_audio_channel((recorder_h)handle, &get_channel_count);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_channel_count);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_set_orientation_tag(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	int set_orientation;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_SET_ORIENTATION_TAG;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	mmsvc_recorder_msg_get(set_orientation, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_set_orientation_tag((recorder_h)handle, (recorder_rotation_e)set_orientation);
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return(api, ret, client);

	return MMSVC_RECORDER_ERROR_NONE;
}

int recorder_dispatcher_attr_get_orientation_tag(Client client)
{
	int ret = RECORDER_ERROR_NONE;
	int handle;
	recorder_rotation_e orientation;
	int get_orientation;
	mmsvc_recorder_api_e api = MMSVC_RECORDER_API_ATTR_GET_ORIENTATION_TAG;
	mmsvc_recorder_msg_get(handle, mmsvc_core_client_get_msg(client));
	ret = mmsvc_recorder_attr_get_orientation_tag((recorder_h)handle, &orientation);
	get_orientation = (int)orientation;
	LOGW("handle : 0x%x", handle);
	mmsvc_recorder_msg_return1(api, ret, client, INT, get_orientation);

	return MMSVC_RECORDER_ERROR_NONE;
}

int (*dispatcher[MMSVC_RECORDER_API_MAX]) (Client client) = {
	recorder_dispatcher_create, /* MMSVC_RECORDER_API_CREATE, */
	recorder_dispatcher_destroy, /* MMSVC_RECORDER_API_DESTROY, */
	recorder_dispatcher_get_state, /* MMSVC_RECORDER_API_GET_STATE, */
	recorder_dispatcher_prepare, /* MMSVC_RECORDER_API_PREPARE, */
	recorder_dispatcher_unprepare, /* MMSVC_RECORDER_API_UNPREPARE, */
	recorder_dispatcher_start, /* MMSVC_RECORDER_API_START, */
	recorder_dispatcher_pause, /* MMSVC_RECORDER_API_PAUSE, */
	recorder_dispatcher_commit, /* MMSVC_RECORDER_API_COMMIT, */
	recorder_dispatcher_cancel, /* MMSVC_RECORDER_API_CANCEL, */
	recorder_dispatcher_set_video_resolution, /* MMSVC_RECORDER_API_SET_VIDEO_RESOLUTION, */
	recorder_dispatcher_get_video_resolution, /* MMSVC_RECORDER_API_GET_VIDEO_RESOLUTION, */
	recorder_dispatcher_foreach_supported_video_resolution, /* MMSVC_RECORDER_API_FOREACH_SUPPORTED_VIDEO_RESOLUTION, */
	recorder_dispatcher_get_audio_level, /* MMSVC_RECORDER_API_GET_AUDIO_LEVEL, */
	recorder_dispatcher_set_filename, /* MMSVC_RECORDER_API_SET_FILENAME, */
	recorder_dispatcher_get_filename, /* MMSVC_RECORDER_API_GET_FILENAME, */
	recorder_dispatcher_set_file_format, /* MMSVC_RECORDER_API_SET_FILE_FORMAT, */
	recorder_dispatcher_get_file_format, /* MMSVC_RECORDER_API_GET_FILE_FORMAT, */
	recorder_dispatcher_set_state_changed_cb, /* MMSVC_RECORDER_API_SET_STATE_CHANGED_CB, */
	recorder_dispatcher_unset_state_changed_cb, /* MMSVC_RECORDER_API_UNSET_STATE_CHANGED_CB, */
	recorder_dispatcher_set_interrupted_cb, /* MMSVC_RECORDER_API_SET_INTERRUPTED_CB, */
	recorder_dispatcher_unset_interrupted_cb, /* MMSVC_RECORDER_API_UNSET_INTERRUPTED_CB, */
	recorder_dispatcher_set_audio_stream_cb, /* MMSVC_RECORDER_API_SET_AUDIO_STREAM_CB, */
	recorder_dispatcher_unset_audio_stream_cb, /* MMSVC_RECORDER_API_UNSET_AUDIO_STREAM_CB, */
	recorder_dispatcher_set_error_cb, /* MMSVC_RECORDER_API_SET_ERROR_CB, */
	recorder_dispatcher_unset_error_cb, /* MMSVC_RECORDER_API_UNSET_ERROR_CB, */
	recorder_dispatcher_set_recording_status_cb, /* MMSVC_RECORDER_API_SET_RECORDING_STATUS_CB, */
	recorder_dispatcher_unset_recording_status_cb, /* MMSVC_RECORDER_API_UNSET_RECORDING_STATUS_CB, */
	recorder_dispatcher_set_recording_limit_reached_cb, /* MMSVC_RECORDER_API_SET_RECORDING_LIMIT_RECHEAD_CB, */
	recorder_dispatcher_unset_recording_limit_reached_cb, /* MMSVC_RECORDER_API_UNSET_RECORDING_LIMIT_RECHEAD_CB, */
	recorder_dispatcher_foreach_supported_file_format, /* MMSVC_RECORDER_API_FOREACH_SUPPORTED_FILE_FORMAT, */
	recorder_dispatcher_attr_set_size_limit, /* MMSVC_RECORDER_API_ATTR_SET_SIZE_LIMIT, */
	recorder_dispatcher_attr_set_time_limit, /* MMSVC_RECORDER_API_ATTR_SET_TIME_LIMIT, */
	recorder_dispatcher_attr_set_audio_device, /* MMSVC_RECORDER_API_ATTR_SET_AUDIO_DEVICE, */
	recorder_dispatcher_set_audio_encoder, /* MMSVC_RECORDER_API_SET_AUDIO_ENCODER, */
	recorder_dispatcher_get_audio_encoder, /* MMSVC_RECORDER_API_GET_AUDIO_ENCODER, */
	recorder_dispatcher_set_video_encoder, /* MMSVC_RECORDER_API_SET_VIDEO_ENCODER, */
	recorder_dispatcher_get_video_encoder, /* MMSVC_RECORDER_API_GET_VIDEO_ENCODER, */
	recorder_dispatcher_attr_set_audio_samplerate, /* MMSVC_RECORDER_API_ATTR_SET_AUDIO_SAMPLERATE, */
	recorder_dispatcher_attr_set_audio_encoder_bitrate, /* MMSVC_RECORDER_API_ATTR_SET_AUDIO_ENCODER_BITRATE, */
	recorder_dispatcher_attr_set_video_encoder_bitrate, /* MMSVC_RECORDER_API_ATTR_SET_VIDEO_ENCODER_BITRATE, */
	recorder_dispatcher_attr_get_size_limit, /* MMSVC_RECORDER_API_ATTR_GET_SIZE_LIMIT, */
	recorder_dispatcher_attr_get_time_limit, /* MMSVC_RECORDER_API_ATTR_GET_TIME_LIMIT, */
	recorder_dispatcher_attr_get_audio_device, /* MMSVC_RECORDER_API_ATTR_GET_AUDIO_DEVICE, */
	recorder_dispatcher_attr_get_audio_samplerate, /* MMSVC_RECORDER_API_ATTR_GET_AUDIO_SAMPLERATE, */
	recorder_dispatcher_attr_get_audio_encoder_bitrate, /* MMSVC_RECORDER_API_ATTR_GET_AUDIO_ENCODER_BITRATE, */
	recorder_dispatcher_attr_get_video_encoder_bitrate, /* MMSVC_RECORDER_API_ATTR_GET_VIDEO_ENCODER_BITRATE, */
	recorder_dispatcher_foreach_supported_audio_encoder, /* MMSVC_RECORDER_API_FOREACH_SUPPORTED_AUDIO_ENCODER, */
	recorder_dispatcher_foreach_supported_video_encoder, /* MMSVC_RECORDER_API_FOREACH_SUPPORTED_VIDEO_ENCODER, */
	recorder_dispatcher_attr_set_mute, /* MMSVC_RECORDER_API_ATTR_SET_MUTE, */
	recorder_dispatcher_attr_is_muted, /* MMSVC_RECORDER_API_ATTR_IS_MUTED, */
	recorder_dispatcher_attr_set_recording_motion_rate, /* MMSVC_RECORDER_API_ATTR_SET_RECORDING_MOTION_RATE, */
	recorder_dispatcher_attr_get_recording_motion_rate, /* MMSVC_RECORDER_API_ATTR_GET_RECORDING_MOTION_RATE, */
	recorder_dispatcher_attr_set_audio_channel, /* MMSVC_RECORDER_API_ATTR_SET_AUDIO_CHANNEL, */
	recorder_dispatcher_attr_get_audio_channel, /* MMSVC_RECORDER_API_ATTR_GET_AUDIO_CHANNEL, */
	recorder_dispatcher_attr_set_orientation_tag, /* MMSVC_RECORDER_API_ATTR_SET_ORIENTATION_TAG, */
	recorder_dispatcher_attr_get_orientation_tag, /* MMSVC_RECORDER_API_ATTR_GET_ORIENTATION_TAG, */
};
