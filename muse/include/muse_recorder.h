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

#ifndef __MUSE_RECORDER_H__
#define __MUSE_RECORDER_H__

#ifdef _cplusplus
extern "C" {
#endif

/**
  * @file muse_recorder.h
  * @brief This file contains the muse recorder API for framework, related structures and enumerations.
  */

#include "tbm_bufmgr.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Enumeration for the muse recorder apis.
 */
typedef enum {
	MUSE_RECORDER_API_CREATE, //0
	MUSE_RECORDER_API_DESTROY,
	MUSE_RECORDER_API_GET_STATE,
	MUSE_RECORDER_API_PREPARE,
	MUSE_RECORDER_API_UNPREPARE,
	MUSE_RECORDER_API_START, //5
	MUSE_RECORDER_API_PAUSE,
	MUSE_RECORDER_API_COMMIT,
	MUSE_RECORDER_API_CANCEL,
	MUSE_RECORDER_API_SET_VIDEO_RESOLUTION,
	MUSE_RECORDER_API_GET_VIDEO_RESOLUTION, //10
	MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_RESOLUTION,
	MUSE_RECORDER_API_GET_AUDIO_LEVEL,
	MUSE_RECORDER_API_SET_FILENAME,
	MUSE_RECORDER_API_GET_FILENAME,
	MUSE_RECORDER_API_SET_FILE_FORMAT, //15
	MUSE_RECORDER_API_GET_FILE_FORMAT,
	MUSE_RECORDER_API_SET_STATE_CHANGED_CB,
	MUSE_RECORDER_API_UNSET_STATE_CHANGED_CB,
	MUSE_RECORDER_API_SET_INTERRUPTED_CB,
	MUSE_RECORDER_API_UNSET_INTERRUPTED_CB, //20
	MUSE_RECORDER_API_SET_AUDIO_STREAM_CB,
	MUSE_RECORDER_API_UNSET_AUDIO_STREAM_CB,
	MUSE_RECORDER_API_SET_ERROR_CB,
	MUSE_RECORDER_API_UNSET_ERROR_CB,
	MUSE_RECORDER_API_SET_RECORDING_STATUS_CB, //25
	MUSE_RECORDER_API_UNSET_RECORDING_STATUS_CB,
	MUSE_RECORDER_API_SET_RECORDING_LIMIT_REACHED_CB,
	MUSE_RECORDER_API_UNSET_RECORDING_LIMIT_REACHED_CB,
	MUSE_RECORDER_API_FOREACH_SUPPORTED_FILE_FORMAT,
	MUSE_RECORDER_API_ATTR_SET_SIZE_LIMIT, //30
	MUSE_RECORDER_API_ATTR_SET_TIME_LIMIT,
	MUSE_RECORDER_API_ATTR_SET_AUDIO_DEVICE,
	MUSE_RECORDER_API_SET_AUDIO_ENCODER,
	MUSE_RECORDER_API_GET_AUDIO_ENCODER,
	MUSE_RECORDER_API_SET_VIDEO_ENCODER, //35
	MUSE_RECORDER_API_GET_VIDEO_ENCODER,
	MUSE_RECORDER_API_ATTR_SET_AUDIO_SAMPLERATE,
	MUSE_RECORDER_API_ATTR_SET_AUDIO_ENCODER_BITRATE,
	MUSE_RECORDER_API_ATTR_SET_VIDEO_ENCODER_BITRATE,
	MUSE_RECORDER_API_ATTR_GET_SIZE_LIMIT, //40
	MUSE_RECORDER_API_ATTR_GET_TIME_LIMIT,
	MUSE_RECORDER_API_ATTR_GET_AUDIO_DEVICE,
	MUSE_RECORDER_API_ATTR_GET_AUDIO_SAMPLERATE,
	MUSE_RECORDER_API_ATTR_GET_AUDIO_ENCODER_BITRATE,
	MUSE_RECORDER_API_ATTR_GET_VIDEO_ENCODER_BITRATE, //45
	MUSE_RECORDER_API_FOREACH_SUPPORTED_AUDIO_ENCODER,
	MUSE_RECORDER_API_FOREACH_SUPPORTED_VIDEO_ENCODER,
	MUSE_RECORDER_API_ATTR_SET_MUTE,
	MUSE_RECORDER_API_ATTR_IS_MUTED,
	MUSE_RECORDER_API_ATTR_SET_RECORDING_MOTION_RATE, //50
	MUSE_RECORDER_API_ATTR_GET_RECORDING_MOTION_RATE,
	MUSE_RECORDER_API_ATTR_SET_AUDIO_CHANNEL,
	MUSE_RECORDER_API_ATTR_GET_AUDIO_CHANNEL,
	MUSE_RECORDER_API_ATTR_SET_ORIENTATION_TAG,
	MUSE_RECORDER_API_ATTR_GET_ORIENTATION_TAG, //55
	MUSE_RECORDER_API_ATTR_SET_ROOT_DIRECTORY,
	MUSE_RECORDER_API_MAX
} muse_recorder_api_e;

/**
 * @brief Enumeration for the muse recorder events.
 */
typedef enum {
	MUSE_RECORDER_EVENT_TYPE_STATE_CHANGE,
	MUSE_RECORDER_EVENT_TYPE_RECORDING_LIMITED,
	MUSE_RECORDER_EVENT_TYPE_RECORDING_STATUS,
	MUSE_RECORDER_EVENT_TYPE_INTERRUPTED,
	MUSE_RECORDER_EVENT_TYPE_AUDIO_STREAM,
	MUSE_RECORDER_EVENT_TYPE_ERROR,
	MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_AUDIO_ENCODER,
	MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_FILE_FORMAT,
	MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_ENCODER,
	MUSE_RECORDER_EVENT_TYPE_FOREACH_SUPPORTED_VIDEO_RESOLUTION,
	MUSE_RECORDER_EVENT_TYPE_NUM
}muse_recorder_event_e;

/**
 * @brief The structure type for muse camera errors.
 */
typedef enum {
	MUSE_RECORDER_ERROR_INVALID = -1,
	MUSE_RECORDER_ERROR_NONE = 1,
} muse_recorder_error_e;

/**
 * @brief The structure type for muse recorder type.
 */
typedef enum {
	MUSE_RECORDER_TYPE_AUDIO = 0,
	MUSE_RECORDER_TYPE_VIDEO
}muse_recorder_type_e;

/**
 * @brief The structure type for muse recorder source type.
 */
typedef enum {
	MUSE_RECORDER_SOURCE_TYPE_UNKNOWN,
	MUSE_RECORDER_SOURCE_TYPE_CAMERA,
}muse_recorder_source_type_e;

/**
 * @brief The structure type for data transport for the muse recorder.
 */
typedef struct {
	int data_size;
	int tbm_key;
	tbm_bo bo;
	tbm_bo_handle bo_handle;
	tbm_bufmgr bufmgr;
} muse_recorder_transport_info_s;

/**
 * @brief The structure type for the userdata, registering into the daemon core.
 */
typedef struct {
	tbm_bufmgr bufmgr;
} muse_recorder_info_s;

/**
 * @brief Definition for the callback event id.
 */
#define MUSE_RECORDER_CB_EVENT	MUSE_RECORDER_API_MAX + 1

/**
 * @brief Definition for the max message length.
 */
#define MUSE_RECORDER_MSG_MAX_LENGTH		256

/**
 * @brief Definition for the wait time of the ipc callback.
 */
#define CALLBACK_TIME_OUT 3

/*
 * @brief Makes the tbm buffer object, and set to the muse recorder structure.
 * @param[out] transport_info The allocated structure, tbm bo will be set in here.
 * @return TRUE on success, otherwise a FALSE value
 */
bool muse_recorder_ipc_make_tbm(muse_recorder_transport_info_s *transport_info);

/**
 * @brief Exports the tbm buffer object, another process can import this bo.
 * @param[in] transport_info Using transport_info.bo to export.
 * @return TBM gem name on success, otherwise a negative error value
 */
int muse_recorder_ipc_export_tbm(muse_recorder_transport_info_s transport_info);

/**
 * @brief Initialize the tbm buffer manager, mainly at the client side.
 * @param[out] transport_info The allocated structure, tbm bufmgr will be set in here.
 * @return TRUE on success, otherwise a FALSE value
 */
bool muse_recorder_ipc_init_tbm(muse_recorder_transport_info_s *transport_info);

/**
 * @brief Imports the tbm buffer object.
 * @param[out] transport_info Set the transport_info.bo.
 * @return TRUE on success, otherwise a FALSE value
 */
int muse_recorder_ipc_import_tbm(muse_recorder_transport_info_s *transport_info);

/**
 * @brief Unreference the tbm buffer object.
 * @param[in] transport_info Using the transport_info.bo.
 * @return TRUE on success, otherwise a FALSE value
 */
void muse_recorder_ipc_unref_tbm(muse_recorder_transport_info_s *transport_info);

#ifdef __cplusplus
}
#endif
#endif				/* __MM_MEDIA_PLAYER2_H__ */
