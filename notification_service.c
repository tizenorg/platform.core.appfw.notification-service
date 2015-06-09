/*
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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
 *
*/
#include <stdio.h>
#include <Eina.h>
#include <packet.h>
#include <notification_ipc.h>
#include <notification_noti.h>
#include <notification_error.h>

#include "service_common.h"

#ifndef NOTIFICATION_ADDR
#define NOTIFICATION_ADDR "/tmp/.notification.service"
#endif

#ifndef NOTIFICATION_DEL_PACKET_UNIT
#define NOTIFICATION_DEL_PACKET_UNIT 10
#endif

static struct info {
    Eina_List *context_list;
    struct service_context *svc_ctx;
} s_info = {
	.context_list = NULL, /*!< \WARN: This is only used for SERVICE THREAD */
	.svc_ctx = NULL, /*!< \WARN: This is only used for MAIN THREAD */
};

struct context {
	struct tcb *tcb;
	double seq;
};

struct noti_service {
	const char *cmd;
	void (*handler)(struct tcb *tcb, struct packet *packet, void *data);
};

/*!
 * FUNCTIONS to handle notifcation
 */
static inline int get_priv_id(int num_deleted, int *list_deleted, int index) {
	if (index < num_deleted) {
		return *(list_deleted + index);
	} else {
		return -1;
	}
}

static inline char *get_string(char *string)
{
	if (string == NULL) {
		return NULL;
	}
	if (string[0] == '\0') {
		return NULL;
	}

	return string;
}

static inline struct packet *create_packet_from_deleted_list(int op_num, int *list, int start_index) {
	return packet_create(
		"del_noti_multiple",
		"iiiiiiiiiii",
		((op_num - start_index) > NOTIFICATION_DEL_PACKET_UNIT) ? NOTIFICATION_DEL_PACKET_UNIT : op_num - start_index,
		get_priv_id(op_num, list, start_index),
		get_priv_id(op_num, list, start_index + 1),
		get_priv_id(op_num, list, start_index + 2),
		get_priv_id(op_num, list, start_index + 3),
		get_priv_id(op_num, list, start_index + 4),
		get_priv_id(op_num, list, start_index + 5),
		get_priv_id(op_num, list, start_index + 6),
		get_priv_id(op_num, list, start_index + 7),
		get_priv_id(op_num, list, start_index + 8),
		get_priv_id(op_num, list, start_index + 9)
		);
}

/*!
 * SERVICE HANDLER
 */
static void _handler_insert(struct tcb *tcb, struct packet *packet, void *data)
{
	int ret = 0;
	int priv_id = 0;
	struct packet *packet_reply = NULL;
	struct packet *packet_service = NULL;
	notification_h noti = NULL;

	noti = notification_create(NOTIFICATION_TYPE_NOTI);
	if (noti != NULL) {
		if (notification_ipc_make_noti_from_packet(noti, packet) == NOTIFICATION_ERROR_NONE) {
			ret = notification_noti_insert(noti);
			if (ret != NOTIFICATION_ERROR_NONE) {
				notification_free(noti);
				return ;
			}

			notification_get_id(noti, NULL, &priv_id);
			packet_reply = packet_create_reply(packet, "ii", ret, priv_id);
			if (packet_reply) {
				service_common_unicast_packet(tcb, packet_reply);
				packet_destroy(packet_reply);
			}

			packet_service = notification_ipc_make_packet_from_noti(noti, "add_noti", 2);
			if (packet_service != NULL) {
				service_common_multicast_packet(tcb, packet_service, TCB_CLIENT_TYPE_SERVICE);
				packet_destroy(packet_service);
			}
		}
		notification_free(noti);
	}
}

static void _handler_update(struct tcb *tcb, struct packet *packet, void *data)
{
	int ret = 0;
	int priv_id = 0;
	struct packet *packet_reply = NULL;
	struct packet *packet_service = NULL;
	notification_h noti = NULL;

	noti = notification_create(NOTIFICATION_TYPE_NOTI);
	if (noti != NULL) {
		if (notification_ipc_make_noti_from_packet(noti, packet) == NOTIFICATION_ERROR_NONE) {
			ret = notification_noti_update(noti);
			if (ret != NOTIFICATION_ERROR_NONE) {
				notification_free(noti);
				return ;
			}

			notification_get_id(noti, NULL, &priv_id);
			packet_reply = packet_create_reply(packet, "ii", ret, priv_id);
			if (packet_reply) {
				service_common_unicast_packet(tcb, packet_reply);
				packet_destroy(packet_reply);
			}

			packet_service = notification_ipc_make_packet_from_noti(noti, "update_noti", 2);
			if (packet_service != NULL) {
				service_common_multicast_packet(tcb, packet_service, TCB_CLIENT_TYPE_SERVICE);
				packet_destroy(packet_service);
			}
		}
		notification_free(noti);
	}
}

static void _handler_refresh(struct tcb *tcb, struct packet *packet, void *data)
{
	int ret = NOTIFICATION_ERROR_NONE;
	struct packet *packet_reply = NULL;

	packet_reply = packet_create_reply(packet, "i", ret);
	if (packet_reply) {
		service_common_unicast_packet(tcb, packet_reply);
		packet_destroy(packet_reply);
	}

	service_common_multicast_packet(tcb, packet, TCB_CLIENT_TYPE_SERVICE);
}

static void _handler_delete_single(struct tcb *tcb, struct packet *packet, void *data)
{
	int ret = 0;
	int priv_id = 0;
	struct packet *packet_reply = NULL;
	struct packet *packet_service = NULL;
	char *pkgname = NULL;

	if (packet_get(packet, "si", &pkgname, &priv_id) == 2) {
		pkgname = get_string(pkgname);

		ret = notification_noti_delete_by_priv_id(pkgname, priv_id);

		packet_reply = packet_create_reply(packet, "ii", ret, priv_id);
		if (packet_reply) {
			service_common_unicast_packet(tcb, packet_reply);
			packet_destroy(packet_reply);
		}

		packet_service = packet_create("del_noti_single", "ii", 1, priv_id);
		if (packet_service != NULL) {
			service_common_multicast_packet(tcb, packet_service, TCB_CLIENT_TYPE_SERVICE);
			packet_destroy(packet_service);
		}
	}
}

static void _handler_delete_multiple(struct tcb *tcb, struct packet *packet, void *data)
{
	int ret = 0;
	struct packet *packet_reply = NULL;
	struct packet *packet_service = NULL;
	char *pkgname = NULL;
	notification_type_e type = 0;
	int num_deleted = 0;
	int *list_deleted = NULL;

	if (packet_get(packet, "si", &pkgname, &type) == 2) {
		pkgname = get_string(pkgname);

		ret = notification_noti_delete_all(type, pkgname, &num_deleted, &list_deleted);

		packet_reply = packet_create_reply(packet, "ii", ret, num_deleted);
		if (packet_reply) {
			service_common_unicast_packet(tcb, packet_reply);
			packet_destroy(packet_reply);
		}

		if (num_deleted > 0) {
			if (num_deleted <= NOTIFICATION_DEL_PACKET_UNIT) {
				packet_service = create_packet_from_deleted_list(num_deleted, list_deleted, 0);

				if (packet_service) {
					service_common_multicast_packet(tcb, packet_service, TCB_CLIENT_TYPE_SERVICE);
					packet_destroy(packet_service);
				}
			} else {
				int set = 0;
				int set_total = num_deleted / NOTIFICATION_DEL_PACKET_UNIT;

				for (set = 0; set <= set_total; set++) {
					packet_service = create_packet_from_deleted_list(num_deleted,
							list_deleted, set * NOTIFICATION_DEL_PACKET_UNIT);

					if (packet_service) {
						service_common_multicast_packet(tcb, packet_service, TCB_CLIENT_TYPE_SERVICE);
						packet_destroy(packet_service);
					}
				}
			}
		}

		if (list_deleted != NULL) {
			free(list_deleted);
			list_deleted = NULL;
		}
	}
}

static void _handler_service_register(struct tcb *tcb, struct packet *packet, void *data)
{
	struct packet *packet_reply;
	int ret;

	ret = tcb_client_type_set(tcb, TCB_CLIENT_TYPE_SERVICE);

	packet_reply = packet_create_reply(packet, "i", ret);
	if (packet_reply) {
		service_common_unicast_packet(tcb, packet_reply);
		packet_destroy(packet_reply);
	}
}

/*!
 * SERVICE THREAD
 */
static int service_thread_main(struct tcb *tcb, struct packet *packet, void *data)
{
	int i = 0;
	const char *command;
	static struct noti_service service_req_table[] = {
		{
			.cmd = "add_noti",
			.handler = _handler_insert,
		},
		{
			.cmd = "update_noti",
			.handler = _handler_update,
		},
		{
			.cmd = "refresh_noti",
			.handler = _handler_refresh,
		},
		{
			.cmd = "del_noti_single",
			.handler = _handler_delete_single,
		},
		{
			.cmd = "del_noti_multiple",
			.handler = _handler_delete_multiple,
		},
		{
			.cmd = "service_register",
			.handler = _handler_service_register,
		},
		{
			.cmd = NULL,
			.handler = NULL,
		},
	};

	if (!packet) {
		return 0;
	}

	command = packet_command(packet);
	if (!command) {
		return -EINVAL;
	}

	switch (packet_type(packet)) {
	case PACKET_REQ:
		/* Need to send reply packet */
		for (i = 0; service_req_table[i].cmd; i++) {
			if (strcmp(service_req_table[i].cmd, command))
				continue;

			service_req_table[i].handler(tcb, packet, data);
			break;
		}

		break;
	case PACKET_REQ_NOACK:
		break;
	case PACKET_ACK:
		break;
	default:
		return -EINVAL;
	}

	/*!
	 * return value has no meanning,
	 * it will be printed by dlogutil.
	 */
	return 0;
}


/*!
 * MAIN THREAD
 * Do not try to do anyother operation in these functions
 */
int notification_service_init(void)
{
	if (s_info.svc_ctx) {
		return -1;
	}

	s_info.svc_ctx = service_common_create(NOTIFICATION_ADDR, service_thread_main, NULL);
	if (!s_info.svc_ctx) {
		return -1;
	}

	return 0;
}

int notification_service_fini(void)
{
	if (!s_info.svc_ctx)
		return -1;

	service_common_destroy(s_info.svc_ctx);
	return 0;
}

/* End of a file */
