/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
enum tcb_type {
	TCB_CLIENT_TYPE_APP	= 0x00,
	TCB_CLIENT_TYPE_SERVICE	= 0x01,
	TCB_CLIENT_TYPE_UNKNOWN = 0xff,
};

struct tcb;
struct service_context;
struct service_event_item;

extern int tcb_fd(struct tcb *tcb);
extern struct service_context *tcb_svc_ctx(struct tcb *tcb);
extern int tcb_client_type(struct tcb *tcb);
extern int tcb_client_type_set(struct tcb *tcb, enum tcb_type type);

extern struct service_context *service_common_create(const char *addr, int (*service_thread_main)(struct tcb *tcb, struct packet *packet, void *data), void *data);
extern int service_common_destroy(struct service_context *svc_ctx);

extern int service_common_multicast_packet(struct tcb *tcb, struct packet *packet, int type);
extern int service_common_unicast_packet(struct tcb *tcb, struct packet *packet);

extern struct service_event_item *service_common_add_timer(struct service_context *svc_ctx, double timer, int (*timer_cb)(struct service_context *svc_cx, void *data), void *data);
extern int service_common_del_timer(struct service_context *svc_ctx, struct service_event_item *item);

#define CRITICAL_SECTION_BEGIN(handle) \
do { \
	int ret; \
	ret = pthread_mutex_lock(handle); \
	if (ret != 0) \
		fprintf(stderr, "Failed to lock: %s\n", strerror(ret)); \
} while (0)

#define CRITICAL_SECTION_END(handle) \
do { \
	int ret; \
	ret = pthread_mutex_unlock(handle); \
	if (ret != 0) \
		fprintf(stderr, "Failed to unlock: %s\n", strerror(ret)); \
} while (0)

#define CANCEL_SECTION_BEGIN() do { \
	int ret; \
	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); \
	if (ret != 0) \
		fprintf(stderr, "Unable to set cancelate state: %s\n", strerror(ret)); \
} while (0)

#define CANCEL_SECTION_END() do { \
	int ret; \
	ret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); \
	if (ret != 0) \
		fprintf(stderr, "Unable to set cancelate state: %s\n", strerror(ret)); \
} while (0)

#define CLOSE_PIPE(p)	do { \
	int status; \
	status = close(p[PIPE_READ]); \
	if (status < 0) \
		fprintf(stderr, "close: %s\n", strerror(errno)); \
	status = close(p[PIPE_WRITE]); \
	if (status < 0) \
		fprintf(stderr, "close: %s\n", strerror(errno)); \
} while (0)

#define PIPE_READ 0
#define PIPE_WRITE 1
#define PIPE_MAX 2

/* End of a file */
