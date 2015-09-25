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

#ifndef __MMSVC_RECORDER_H__
#define __MMSVC_RECORDER_H__

#ifdef _cplusplus
extern "C" {
#endif

#include "tbm_bufmgr.h"
#include <stdbool.h>
#include <stdio.h>

typedef enum {
	MMSVC_RECORDER_API_CREATE, //0
	MMSVC_RECORDER_API_DESTROY,
	MMSVC_RECORDER_API_GET_STATE,
	MMSVC_RECORDER_API_PREPARE,
	MMSVC_RECORDER_API_UNPREPARE,
	MMSVC_RECORDER_API_START, //5
	MMSVC_RECORDER_API_PAUSE,
	MMSVC_RECORDER_API_COMMIT,
	MMSVC_RECORDER_API_CANCEL,
	MMSVC_RECORDER_API_SET_VIDEO_RESOLUTION,
	MMSVC_RECORDER_API_GET_VIDEO_RESOLUTION, //10
	MMSVC_RECORDER_API_FOREACH_SUPPORTED_VIDEO_RESOLUTION,
	MMSVC_RECORDER_API_GET_AUDIO_LEVEL,
	MMSVC_RECORDER_API_SET_FILENAME,
	MMSVC_RECORDER_API_GET_FILENAME,
	MMSVC_RECORDER_API_SET_FILE_FORMAT, //15
	MMSVC_RECORDER_API_GET_FILE_FORMAT,
	MMSVC_RECORDER_API_SET_STATE_CHANGED_CB,
	MMSVC_RECORDER_API_UNSET_STATE_CHANGED_CB,
	MMSVC_RECORDER_API_SET_INTERRUPTED_CB,
	MMSVC_RECORDER_API_UNSET_INTERRUPTED_CB, //20
	MMSVC_RECORDER_API_SET_AUDIO_STREAM_CB,
	MMSVC_RECORDER_API_UNSET_AUDIO_STREAM_CB,
	MMSVC_RECORDER_API_SET_ERROR_CB,
	MMSVC_RECORDER_API_UNSET_ERROR_CB,
	MMSVC_RECORDER_API_SET_RECORDING_STATUS_CB, //25
	MMSVC_RECORDER_API_UNSET_RECORDING_STATUS_CB,
	MMSVC_RECORDER_API_SET_RECORDING_LIMIT_REACHED_CB,
	MMSVC_RECORDER_API_UNSET_RECORDING_LIMIT_REACHED_CB,
	MMSVC_RECORDER_API_FOREACH_SUPPORTED_FILE_FORMAT,
	MMSVC_RECORDER_API_ATTR_SET_SIZE_LIMIT, //30
	MMSVC_RECORDER_API_ATTR_SET_TIME_LIMIT,
	MMSVC_RECORDER_API_ATTR_SET_AUDIO_DEVICE,
	MMSVC_RECORDER_API_SET_AUDIO_ENCODER,
	MMSVC_RECORDER_API_GET_AUDIO_ENCODER,
	MMSVC_RECORDER_API_SET_VIDEO_ENCODER, //35
	MMSVC_RECORDER_API_GET_VIDEO_ENCODER,
	MMSVC_RECORDER_API_ATTR_SET_AUDIO_SAMPLERATE,
	MMSVC_RECORDER_API_ATTR_SET_AUDIO_ENCODER_BITRATE,
	MMSVC_RECORDER_API_ATTR_SET_VIDEO_ENCODER_BITRATE,
	MMSVC_RECORDER_API_ATTR_GET_SIZE_LIMIT, //40
	MMSVC_RECORDER_API_ATTR_GET_TIME_LIMIT,
	MMSVC_RECORDER_API_ATTR_GET_AUDIO_DEVICE,
	MMSVC_RECORDER_API_ATTR_GET_AUDIO_SAMPLERATE,
	MMSVC_RECORDER_API_ATTR_GET_AUDIO_ENCODER_BITRATE,
	MMSVC_RECORDER_API_ATTR_GET_VIDEO_ENCODER_BITRATE, //45
	MMSVC_RECORDER_API_FOREACH_SUPPORTED_AUDIO_ENCODER,
	MMSVC_RECORDER_API_FOREACH_SUPPORTED_VIDEO_ENCODER,
	MMSVC_RECORDER_API_ATTR_SET_MUTE,
	MMSVC_RECORDER_API_ATTR_IS_MUTED,
	MMSVC_RECORDER_API_ATTR_SET_RECORDING_MOTION_RATE, //50
	MMSVC_RECORDER_API_ATTR_GET_RECORDING_MOTION_RATE,
	MMSVC_RECORDER_API_ATTR_SET_AUDIO_CHANNEL,
	MMSVC_RECORDER_API_ATTR_GET_AUDIO_CHANNEL,
	MMSVC_RECORDER_API_ATTR_SET_ORIENTATION_TAG,
	MMSVC_RECORDER_API_ATTR_GET_ORIENTATION_TAG, //55
	MMSVC_RECORDER_API_MAX //56
} mmsvc_recorder_api_e;

typedef enum {
	MMSVC_RECORDER_EVENT_TYPE_STATE_CHANGE,
	MMSVC_RECORDER_EVENT_TYPE_RECORDING_LIMITED,
	MMSVC_RECORDER_EVENT_TYPE_RECORDING_STATUS,
	MMSVC_RECORDER_EVENT_TYPE_INTERRUPTED,
	MMSVC_RECORDER_EVENT_TYPE_AUDIO_STREAM,
	MMSVC_RECORDER_EVENT_TYPE_ERROR,
	MMSVC_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_AUDIO_ENCODER,
	MMSVC_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_FILE_FORMAT,
	MMSVC_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_ENCODER,
	MMSVC_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_RESOLUTION,
	MMSVC_RECORDER_EVENT_TYPE_NUM
}mmsvc_recorder_event_e;

typedef enum {
	MMSVC_RECORDER_ERROR_INVALID = -1,
	MMSVC_RECORDER_ERROR_NONE = 1,
} mmsvc_recorder_error_e;

typedef enum {
	MMSVC_RECORDER_TYPE_AUDIO = 0,
	MMSVC_RECORDER_TYPE_VIDEO
}mmsvc_recorder_type_e;

typedef enum {
	MMSVC_RECORDER_SOURCE_TYPE_UNKNOWN,
	MMSVC_RECORDER_SOURCE_TYPE_CAMERA,
}mmsvc_recorder_source_type_e;

/**
 * @brief The structure type for data transport
 */
typedef struct {
	int data_size;
	int tbm_key;
	tbm_bo bo;
	tbm_bo_handle bo_handle;
	tbm_bufmgr bufmgr;
} mmsvc_recorder_transport_info_s;

typedef struct {
	tbm_bufmgr bufmgr;
} mmsvc_recorder_info_s;

#define MMSVC_RECORDER_CB_EVENT	MMSVC_RECORDER_API_MAX + 1
#define MMSVC_MSG_MAX_LENGTH		256
#define MMSVC_PARSE_STRING_SIZE	200

bool mmsvc_recorder_ipc_make_tbm(mmsvc_recorder_transport_info_s *transport_info);
int mmsvc_recorder_ipc_export_tbm(mmsvc_recorder_transport_info_s transport_info);
bool mmsvc_recorder_ipc_init_tbm(mmsvc_recorder_transport_info_s *transport_info);
int mmsvc_recorder_ipc_import_tbm(mmsvc_recorder_transport_info_s *transport_info);
void mmsvc_recorder_ipc_unref_tbm(mmsvc_recorder_transport_info_s *transport_info);

#ifdef __cplusplus
}
#endif
#endif				/* __MM_MEDIA_PLAYER2_H__ */
