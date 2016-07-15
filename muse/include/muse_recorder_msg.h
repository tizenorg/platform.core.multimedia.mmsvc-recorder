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

/**
 * @file muse_recorder_msg.h
 * @brief This file contains the muse_camera message APIs, related structures, defines and macros.
 */

/**
 * @brief Definition for the handle parameter.
 */
#define PARAM_HANDLE        "handle"

/**
 * @brief Definition for the ret parameter.
 */
#define PARAM_RET           "ret"

/**
 * @brief Definition for the event parameter.
 */
#define PARAM_EVENT         "event"

/**
 * @brief Definition for the api class parameter.
 */
#define PARAM_API_CLASS     "api_class"

/**
 * @brief Definition for the event class parameter.
 */
#define PARAM_EVENT_CLASS   "event_class"

/**
 * @brief Definition for the error parameter.
 */
#define PARAM_ERROR         "error"

/**
 * @brief Definition for the tbm key parameter.
 */
#define PARAM_TBM_KEY       "t_key"

/**
 * @brief Definition for the display mode parameter.
 */
#define PARAM_DISPLAY_MODE  "display_mode"

/**
 * @brief Definition for the device type parameter.
 */
#define PARAM_DEVICE_TYPE   "device_type"

/**
 * @brief Definition for the recorder type parameter audio/video
 */
 #define PARAM_RECORDER_TYPE "recorder_type"

/**
 * @brief Definition for the camera handle ipc parameter
 */
#define PARAM_CAMERA_HANDLE "camera_handle"

/**
 * @brief Definition for the INTEGER type.
 */
typedef int32_t INT;

/**
 * @brief Definition for the 64 bit INTEGER type.
 */
typedef int64_t INT64;

/**
 * @brief Definition for the 64 bit integer pointer type.
 */
typedef intptr_t POINTER;

/**
 * @brief Definition for the 64 bit DOUBLE type.
 */
typedef double DOUBLE;

/**
 * @brief Definition for the 64 bit STRING type.
 */
typedef const char* STRING;

/**
 * @brief Query the specific value from the input message via ipc.
 * @param[in] param The key to query, the variable name should be matched to the message's one.
 * @param[out] buf The string of message buffer.
 */
#define muse_recorder_msg_get(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, NULL, MUSE_TYPE_ANY)

/**
 * @brief Query the specific value from the input message via ipc.
 * @param[in] param The key to query, the variable name should be matched to the message's one.
 * @param[out] buf The string of message buffer.
 */
#define muse_recorder_msg_get_double(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, NULL, MUSE_TYPE_DOUBLE)

/**
 * @brief Query the specific string type value from the input message via ipc.
 * @param[in] param The key to query, the variable name should be matched to the message's one.
 * @param[out] buf The string of message buffer.
 */
#define muse_recorder_msg_get_string(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, param, NULL, MUSE_TYPE_STRING)

/**
 * @brief Query the specific array type value from the input message via ipc.
 * @param[in] param The key to query, the variable name should be matched to the message's one.
 * @param[out] buf The string of message buffer.
 */
#define muse_recorder_msg_get_array(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, param, NULL, MUSE_TYPE_ARRAY)

/**
 * @brief Query the specific pointer type value from the input message via ipc.
 * @param[in] param The key to query, the variable name should be matched to the message's one.
 * @param[out] buf The string of message buffer.
 */
#define muse_recorder_msg_get_pointer(param, buf) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, NULL, MUSE_TYPE_POINTER)


/**
 * @brief Query the specific value with error return from the input message via ipc.
 * @param[in] param The key to query, the variable name should be matched to the message's one.
 * @param[out] buf The string of message buffer.
 * @param[out] e The error return from the core api.
 */
#define muse_recorder_msg_get_error_e(param, buf, e) \
	muse_core_msg_json_deserialize(#param, buf, NULL, &param, &e, MUSE_TYPE_ANY)

/**
 * @brief Send the message from proxy to module via ipc.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] fd The socket fd that connected to the module via ipc.
 * @param[in] cb_info The callback information, waiting for the ack from the module.
 * @param[out] ret The delivered return value from the module to proxy side.
 */
#define muse_recorder_msg_send(api, fd, cb_info, ret) \
	do {	\
		char *__sndMsg__; \
		int __len__; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, 0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} else \
			ret = client_wait_for_cb_return(api, cb_info, RECORDER_CALLBACK_TIME_OUT); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)


/**
 * @brief Send the message from proxy to module via ipc, adding 1 more parameter.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] fd The socket fd that connected to the module via ipc.
 * @param[in] cb_info The callback information, waiting for the ack from the module.
 * @param[out] ret The delivered return value from the module to proxy side.
 * @param[in] type The data type of the parameter.
 * @param[in] param A single parameter to be included in the message.
 */
#define muse_recorder_msg_send1(api, fd, cb_info, ret, type, param) \
	do {	\
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
			ret = client_wait_for_cb_return(api, cb_info, RECORDER_CALLBACK_TIME_OUT); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Send the message from proxy to module via ipc, adding 1 more parameter.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] fd The socket fd that connected to the module via ipc.
 * @param[in] cb_info The callback information, waiting for the ack from the module.
 * @param[in] type The data type of the parameter.
 * @param[in] param A single parameter to be included in the message.
 */
#define muse_recorder_msg_send1_no_return(api, fd, cb_info, type, param) \
	do {	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_##type, #param, __value__, \
			0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Send the message from proxy to module via ipc, adding 2 more parameters.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] fd The socket fd that connected to the module via ipc.
 * @param[in] cb_info The callback information, waiting for the ack from the module.
 * @param[out] ret The delivered return value from the module to proxy side.
 * @param[in] type1 The data type of the parameter.
 * @param[in] param1 The first parameter to be included in the message.
 * @param[in] type2 The data type of the parameter.
 * @param[in] param2 The 2nd parameter to be included in the message.
 */
#define muse_recorder_msg_send2(api, fd, cb_info, ret, type1, param1, type2, param2) \
	do {	\
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
			ret = client_wait_for_cb_return(api, cb_info, RECORDER_CALLBACK_TIME_OUT); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)


/**
 * @brief Send the message from proxy to module via ipc, adding an array data.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] fd The socket fd that connected to the module via ipc.
 * @param[in] cb_info The callback information, waiting for the ack from the module.
 * @param[out] ret The delivered return value from the module to proxy side.
 * @param[in] param The array data parameter to be included in the message.
 * @param[in] length The length of the array.
 * @param[in] datum_size The size of the array.
 */
#define muse_recorder_msg_send_array(api, fd, cb_info, ret, param, length, datum_size) \
	do {	\
		char *__sndMsg__; \
		int __len__; \
		int *__value__ = (int *)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, #length, length, \
			MUSE_TYPE_ARRAY, #param, \
			datum_size == sizeof(int) ? length :  \
			length / sizeof(int) + (length % sizeof(int) ? 1 : 0), \
			__value__, \
			0); \
		__len__ = muse_core_ipc_send_msg(fd, __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} else \
			ret = client_wait_for_cb_return(api, cb_info, RECORDER_CALLBACK_TIME_OUT); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Returning the ack message from the server to client side.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] class The enumeration of the api class.
 * @param[out] ret The delivered return value from the module to proxy side.
 * @param[in] module The module info for the ipc transportation.
 */
#define muse_recorder_msg_return(api, class, ret, module) \
	do {	\
		char *__sndMsg__; \
		int __len__; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_API_CLASS, class, \
			MUSE_TYPE_INT, PARAM_RET, ret, \
			0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Returning the ack message from the server to client side.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] class The enumeration of the api class.
 * @param[out] ret The delivered return value from the module to proxy side.
 * @param[in] module The module info for the ipc transportation.
 * @param[in] type The data type of the parameter.
 * @param[in] param A parameter to be included in the message.
 */
#define muse_recorder_msg_return1(api, class, ret, module, type, param) \
	do {	\
		char *__sndMsg__; \
		int __len__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_API_CLASS, class, \
			MUSE_TYPE_INT, PARAM_RET, ret, \
			MUSE_TYPE_##type, #param, __value__, \
			0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Returning the ack message from the server to client side, adding 2 parameters.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] class The enumeration of the api class.
 * @param[out] ret The delivered return value from the module to proxy side.
 * @param[in] module The module info for the ipc transportation.
 * @param[in] type1 The data type of the parameter.
 * @param[in] param1 The 1st parameter to be included in the message.
 * @param[in] type2 The data type of the parameter.
 * @param[in] param2 The 2nd parameter to be included in the message.
 */
#define muse_recorder_msg_return2(api, class, ret, module, type1, param1, type2, param2) \
	do {	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_API_CLASS, class, \
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
	} while (0)

/**
 * @brief Returning the ack message from the server to client side, adding 3 parameters.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] class The enumeration of the api class.
 * @param[out] ret The delivered return value from the module to proxy side.
 * @param[in] module The module info for the ipc transportation.
 * @param[in] type1 The data type of the parameter.
 * @param[in] param1 The 1st parameter to be included in the message.
 * @param[in] type2 The data type of the parameter.
 * @param[in] param2 The 2nd parameter to be included in the message.
 * @param[in] type3 The data type of the parameter.
 * @param[in] param3 The 3rd parameter to be included in the message.
 */
#define muse_recorder_msg_return3(api, class, ret, module, type1, param1, type2, param2, type3, param3) \
	do {	\
		char *__sndMsg__; \
		int __len__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_API_CLASS, class, \
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
	} while (0)

/**
 * @brief Returning the ack message from the server to client side, adding array parameter.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] class The enumeration of the api class.
 * @param[out] ret The delivered return value from the module to proxy side.
 * @param[in] module The module info for the ipc transportation.
 * @param[in] param The array data parameter to be included in the message.
 * @param[in] length The length of the array.
 * @param[in] datum_size The size of the array.
 */
#define muse_recorder_msg_return_array(api, class, ret, module, param, length, datum_size) \
	do {	\
		char *__sndMsg__; \
		int __len__; \
		int *__value__ = (int *)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_API_CLASS, class, \
			MUSE_TYPE_INT, PARAM_RET, ret, \
			MUSE_TYPE_INT, #length, length, \
			MUSE_TYPE_ARRAY, #param, \
			datum_size == sizeof(int) ? length :  \
			length / sizeof(int) + (length % sizeof(int) ? 1 : 0), \
			__value__, \
			0); \
		__len__ = muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		if (__len__ <= 0) { \
			LOGE("sending message failed"); \
			ret = RECORDER_ERROR_INVALID_OPERATION; \
		} \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Returning the event ack message from the server to client side.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] event The enumeration of the event.
 * @param[in] class The enumeration of the event class.
 * @param[in] module The module info for the ipc transportation.
 */
#define muse_recorder_msg_event(api, event, class, module) \
	do {	\
		char *__sndMsg__; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_EVENT, event, \
			MUSE_TYPE_INT, PARAM_EVENT_CLASS, class, \
			0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Returning the event ack message from the server to client side, adding a parameter.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] event The enumeration of the event.
 * @param[in] class The enumeration of the event class.
 * @param[in] module The module info for the ipc transportation.
 * @param[in] type The data type of the parameter.
 * @param[in] param A parameter to be included in the message.
 */
#define muse_recorder_msg_event1(api, event, class, module, type, param) \
	do {	\
		char *__sndMsg__; \
		type __value__ = (type)param; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_EVENT, event, \
			MUSE_TYPE_INT, PARAM_EVENT_CLASS, class, \
			MUSE_TYPE_##type, #param, __value__, \
			0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Returning the event ack message from the server to client side, adding 2 parameters.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] event The enumeration of the event.
 * @param[in] class The enumeration of the event class.
 * @param[in] module The module info for the ipc transportation.
 * @param[in] param1 The 1st parameter to be included in the message.
 * @param[in] type2 The data type of the parameter.
 * @param[in] param2 The 2nd parameter to be included in the message.
 */
#define muse_recorder_msg_event2(api, event, class, module, type1, param1, type2, param2) \
	do {	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_EVENT, event, \
			MUSE_TYPE_INT, PARAM_EVENT_CLASS, class, \
			MUSE_TYPE_##type1, #param1, __value1__, \
			MUSE_TYPE_##type2, #param2, __value2__, \
			0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Returning the event ack message from the server to client side, adding 3 parameters.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] event The enumeration of the event.
 * @param[in] class The enumeration of the event class.
 * @param[in] module The module info for the ipc transportation.
 * @param[in] param1 The 1st parameter to be included in the message.
 * @param[in] type2 The data type of the parameter.
 * @param[in] param2 The 2nd parameter to be included in the message.
 * @param[in] type3 The data type of the parameter.
 * @param[in] param3 The 3rd parameter to be included in the message.
 */
#define muse_recorder_msg_event3(api, event, class, module, type1, param1, type2, param2, type3, param3) \
	do {	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_EVENT, event, \
			MUSE_TYPE_INT, PARAM_EVENT_CLASS, class, \
			MUSE_TYPE_##type1, #param1, __value1__, \
			MUSE_TYPE_##type2, #param2, __value2__, \
			MUSE_TYPE_##type3, #param3, __value3__, \
			0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

/**
 * @brief Returning the event ack message from the server to client side, adding 5 parameters.
 * @param[in] api The enumeration of the corresponding api.
 * @param[in] event The enumeration of the event.
 * @param[in] class The enumeration of the event class.
 * @param[in] module The module info for the ipc transportation.
 * @param[in] param1 The 1st parameter to be included in the message.
 * @param[in] type2 The data type of the parameter.
 * @param[in] param2 The 2nd parameter to be included in the message.
 * @param[in] type3 The data type of the parameter.
 * @param[in] param3 The 3rd parameter to be included in the message.
 * @param[in] type4 The data type of the parameter.
 * @param[in] param4 The 4th parameter to be included in the message.
 * @param[in] type5 The data type of the parameter.
 * @param[in] param5 The 5th parameter to be included in the message.
 */
#define muse_recorder_msg_event5(api, event, class, module, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5) \
	do {	\
		char *__sndMsg__; \
		type1 __value1__ = (type1)param1; \
		type2 __value2__ = (type2)param2; \
		type3 __value3__ = (type3)param3; \
		type4 __value4__ = (type4)param4; \
		type5 __value5__ = (type5)param5; \
		__sndMsg__ = muse_core_msg_json_factory_new(api, \
			MUSE_TYPE_INT, PARAM_EVENT, event, \
			MUSE_TYPE_INT, PARAM_EVENT_CLASS, class, \
			MUSE_TYPE_##type1, #param1, __value1__, \
			MUSE_TYPE_##type2, #param2, __value2__, \
			MUSE_TYPE_##type3, #param3, __value3__, \
			MUSE_TYPE_##type4, #param4, __value4__, \
			MUSE_TYPE_##type5, #param5, __value5__, \
			0); \
		muse_core_ipc_send_msg(muse_core_client_get_msg_fd(module), __sndMsg__); \
		muse_core_msg_json_factory_free(__sndMsg__); \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /*__RECORDER_MSG_PRIVATE_H__*/
