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

#ifndef __RECORDER_MSG_PRIVATE_H__
#define __RECORDER_MSG_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "muse_core_msg_json.h"

#define PARAM_HANDLE			"handle"
#define PARAM_RET				"ret"
#define PARAM_EVENT			"event"
#define PARAM_ERROR			"error"
#define PARAM_TBM_KEY			"t_key"
#define PARAM_DISPLAY_MODE	"display_mode"
#define PARAM_DEVICE_TYPE		"device_type"
#define PARAM_RECORDER_TYPE	"recorder_type"
#define PARAM_CAMERA_HANDLE	"camera_handle"

#define CALLBACK_TIME_OUT 3

typedef int32_t INT;
typedef int64_t INT64;
typedef intptr_t POINTER;
typedef double DOUBLE;
typedef const char* STRING;

#define muse_recorder_msg_get(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, NULL, MUSE_TYPE_ANY)

#define muse_recorder_msg_get_string(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, param, NULL, MUSE_TYPE_STRING)

#define muse_recorder_msg_get_array(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, param, NULL, MUSE_TYPE_ARRAY)

#define muse_recorder_msg_get_pointer(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, NULL, MUSE_TYPE_POINTER)

#define muse_recorder_msg_get_error_e(param, buf, e) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, &e, MUSE_TYPE_ANY)

#define muse_recorder_msg_send(api, fd, cb_info, ret) \
	do{	\
		char *__sndMsg__; \
		callback_cb_info_s *cb_info_s; \
		int __len__; \
		cb_info_s = (callback_cb_info_s *)cb_info; \
		cb_info_s->activating[api] = 0; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, 0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} else \
			ret = client_wait_for_cb_return(api, cb_info, CALLBACK_TIME_OUT); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_send1(api, fd, cb_info, ret, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type, #param, __value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} else \
			ret = client_wait_for_cb_return(api, cb_info, CALLBACK_TIME_OUT); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_send2(api, fd, cb_info, ret, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} else \
			ret = client_wait_for_cb_return(api, cb_info, CALLBACK_TIME_OUT); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_send_array(api, fd, cb_info, ret, param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int *__value__ = (int *)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, #length, length, \
				MUSE_TYPE_ARRAY, #param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} else \
			ret = client_wait_for_cb_return(api, cb_info, CALLBACK_TIME_OUT); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_send1_async(api, fd, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type, #param, __value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			return RECORDER_ERROR_INVALID_OPERATION; \
		} \
	}while(0)

#define muse_recorder_msg_send2_async(api, fd, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			return RECORDER_ERROR_INVALID_OPERATION; \
		} \
	}while(0)

#define muse_recorder_msg_return(api, ret, module) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_RET, ret, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_return1(api, ret, module, type, param) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_RET, ret, \
				MUSE_TYPE_##type, #param, __value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_return2(api, ret, module, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_RET, ret, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_return3(api, ret, module, type1, param1, type2, param2, type3, param3) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_RET, ret, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_return_array(api, ret, module, param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		int __len__; \
		int *__value__ = (int *)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_RET, ret, \
				MUSE_TYPE_INT, #length, length, \
				MUSE_TYPE_ARRAY, #param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__value__, \
				0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_event(api, event, fd) \
	do{	\
		char *__sndMsg__; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_EVENT, event, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_event1(api, event, module, type, param) \
	do{	\
		char *__sndMsg__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_EVENT, event, \
				MUSE_TYPE_##type, #param, __value__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_event2(api, event, module, type1, param1, type2, param2) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_event3(api, event, module, type1, param1, type2, param2, type3, param3) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_event4(api, event, module, type1, param1, type2, param2, type3, param3, type4, param4) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_event5(api, event, module, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
				MUSE_TYPE_##type5, #param5, __value5__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_event2_array(api, event, module, type1, param1, type2, param2, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_INT, #length, length, \
				MUSE_TYPE_ARRAY, #arr_param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__arr_value__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#define muse_recorder_msg_event6_array(api, event, client, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5, type6, param6, arr_param, length, datum_size) \
	do{	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		type6 __value6__ = (type6)param6; \
		int *__arr_value__ = (int *)arr_param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
				MUSE_TYPE_INT, PARAM_EVENT, event, \
				MUSE_TYPE_##type1, #param1, __value1__, \
				MUSE_TYPE_##type2, #param2, __value2__, \
				MUSE_TYPE_##type3, #param3, __value3__, \
				MUSE_TYPE_##type4, #param4, __value4__, \
				MUSE_TYPE_##type5, #param5, __value5__, \
				MUSE_TYPE_##type6, #param6, __value6__, \
				MUSE_TYPE_INT, #length, length, \
				MUSE_TYPE_ARRAY, #arr_param, \
					datum_size == sizeof(int)? length :  \
					length / sizeof(int) + (length % sizeof(int)?1:0), \
					__arr_value__, \
				0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	}while(0)

#ifdef __cplusplus
}
#endif

#endif /*__RECORDER_MSG_PRIVATE_H__*/
