/*
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
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
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <secure_socket.h>
#include <packet.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <Eina.h>

#include <cynara-client.h>
#include <cynara-creds-socket.h>
#include <cynara-session.h>

#include "service_common.h"

#define EVT_CH		'e'
#define EVT_END_CH	'x'

int errno;

struct service_event_item {
	enum {
		SERVICE_EVENT_TIMER,
	} type;

	union {
		struct {
			int fd;
		} timer;
	} info;

	int (*event_cb)(struct service_context *svc_cx, void *data);
	void *cbdata;
};

/*!
 * \note
 * Server information and global (only in this file-scope) variables are defined
 */
struct service_context {
	pthread_t server_thid; /*!< Server thread Id */
	int fd; /*!< Server socket handle */

	cynara *p_cynara; /*!< Cynara handle */

	Eina_List *tcb_list; /*!< TCB list, list of every thread for client connections */

	Eina_List *packet_list;
	pthread_mutex_t packet_list_lock;
	int evt_pipe[PIPE_MAX];
	int tcb_pipe[PIPE_MAX];

	int (*service_thread_main)(struct tcb *tcb, struct packet *packet, void *data);
	void *service_thread_data;

	Eina_List *event_list;
};

struct packet_info {
	struct tcb *tcb;
	struct packet *packet;
};

/*!
 * \note
 * Thread Control Block
 * - The main server will create a thread for every client connections.
 *   When a new client is comming to us, this TCB block will be allocated and initialized.
 */
struct tcb { /* Thread controll block */
	struct service_context *svc_ctx;
	pthread_t thid; /*!< Thread Id */
	int fd; /*!< Connection handle */
	char *client; /*!< Client socket credential */
	char *user; /*!< User socket credential */
	char *session; /*!< Session context for cynara */
	enum tcb_type type;
};

/*!
 * Do services for clients
 * Routing packets to destination processes.
 * CLIENT THREAD
 */
static void *client_packet_pump_main(void *data)
{
	struct tcb *tcb = data;
	struct service_context *svc_ctx = tcb->svc_ctx;
	struct packet *packet;
	fd_set set;
	char *ptr;
	int size;
	int packet_offset;
	int recv_offset;
	int pid;
	long ret;
	char evt_ch = EVT_CH;
	enum {
		RECV_INIT,
		RECV_HEADER,
		RECV_PAYLOAD,
		RECV_DONE,
	} recv_state;
	struct packet_info *packet_info;
	Eina_List *l;

	ret = 0;
	recv_state = RECV_INIT;

	/*
	 * Fill connection credentials
	 */
	if (!fill_creds(tcb)) {
		ret = -EPERM;
	}
	/*!
	 * \note
	 * To escape from the switch statement, we use this ret value
	 */
	while (ret == 0) {
		FD_ZERO(&set);
		FD_SET(tcb->fd, &set);
		ret = select(tcb->fd + 1, &set, NULL, NULL, NULL);
		if (ret < 0) {
			ret = -errno;
			if (errno == EINTR) {
				ret = 0;
				continue;
			}
			free(ptr);
			ptr = NULL;
			break;
		} else if (ret == 0) {
			ret = -ETIMEDOUT;
			free(ptr);
			ptr = NULL;
			break;
		}

		if (!FD_ISSET(tcb->fd, &set)) {
			ret = -EINVAL;
			free(ptr);
			ptr = NULL;
			break;
		}
		
		/*!
		 * \TODO
		 * Service!!! Receive packet & route packet
		 */
		switch (recv_state) {
		case RECV_INIT:
			size = packet_header_size();
			packet_offset = 0;
			recv_offset = 0;
			packet = NULL;
			ptr = malloc(size);
			if (!ptr) {
				ret = -ENOMEM;
				break;
			}
			recv_state = RECV_HEADER;
			/* Go through, don't break from here */
		case RECV_HEADER:
			ret = secure_socket_recv(tcb->fd, ptr, size - recv_offset, &pid);
			if (ret <= 0) {
				if (ret == 0)
					ret = -ECANCELED;
				free(ptr);
				ptr = NULL;
				break;
			}

			recv_offset += ret;
			ret = 0;

			if (recv_offset == size) {
				packet = packet_build(packet, packet_offset, ptr, size);
				free(ptr);
				ptr = NULL;
				if (!packet) {
					ret = -EFAULT;
					break;
				}

				packet_offset += recv_offset;

				size = packet_payload_size(packet);
				if (size <= 0) {
					recv_state = RECV_DONE;
					recv_offset = 0;
					break;
				}

				recv_state = RECV_PAYLOAD;
				recv_offset = 0;

				ptr = malloc(size);
				if (!ptr) {
					ret = -ENOMEM;
				}
			}
			break;
		case RECV_PAYLOAD:
			ret = secure_socket_recv(tcb->fd, ptr, size - recv_offset, &pid);
			if (ret <= 0) {
				if (ret == 0)
					ret = -ECANCELED;
				free(ptr);
				ptr = NULL;
				break;
			}

			recv_offset += ret;
			ret = 0;

			if (recv_offset == size) {
				packet = packet_build(packet, packet_offset, ptr, size);
				free(ptr);
				ptr = NULL;
				if (!packet) {
					ret = -EFAULT;
					break;
				}

				packet_offset += recv_offset;

				recv_state = RECV_DONE;
				recv_offset = 0;
			}
			break;
		case RECV_DONE:
		default:
			/* Dead code */
			break;
		}

		if (recv_state == RECV_DONE) {
			/*!
			 * Push this packet to the packet list with TCB
			 * Then the service main function will get this.
			 */
			packet_info = malloc(sizeof(*packet_info));
			if (!packet_info) {
				ret = -errno;
				packet_destroy(packet);
				break;
			}

			packet_info->packet = packet;
			packet_info->tcb = tcb;

			CRITICAL_SECTION_BEGIN(&svc_ctx->packet_list_lock);
			svc_ctx->packet_list = eina_list_append(svc_ctx->packet_list, packet_info);
			CRITICAL_SECTION_END(&svc_ctx->packet_list_lock);

			if (write(svc_ctx->evt_pipe[PIPE_WRITE], &evt_ch, sizeof(evt_ch)) != sizeof(evt_ch)) {
				ret = -errno;
				CRITICAL_SECTION_BEGIN(&svc_ctx->packet_list_lock);
				svc_ctx->packet_list = eina_list_remove(svc_ctx->packet_list, packet_info);
				CRITICAL_SECTION_END(&svc_ctx->packet_list_lock);

				packet_destroy(packet);
				free(packet_info);
				break;
			} else {
				recv_state = RECV_INIT;
			}
		}
	}

	CRITICAL_SECTION_BEGIN(&svc_ctx->packet_list_lock);
	EINA_LIST_FOREACH(svc_ctx->packet_list, l, packet_info) {
		if (packet_info->tcb == tcb) {
			packet_info->tcb = NULL;
		}
	}
	CRITICAL_SECTION_END(&svc_ctx->packet_list_lock);

	/*!
	 * \note
	 * Emit a signal to collect this TCB from the SERVER THREAD.
	 */
	write(svc_ctx->tcb_pipe[PIPE_WRITE], &tcb, sizeof(tcb)) != sizeof(tcb);

	if (ptr)
		free(ptr);

	return (void *)ret;
}

/*!
 * \note
 * SERVER THREAD
 */
static inline struct tcb *tcb_create(struct service_context *svc_ctx, int fd)
{
	struct tcb *tcb;
	int status;

	tcb = malloc(sizeof(*tcb));
	if (!tcb) {
		return NULL;
	}

	tcb->fd = fd;
	tcb->svc_ctx = svc_ctx;
	tcb->type = TCB_CLIENT_TYPE_APP;

	status = pthread_create(&tcb->thid, NULL, client_packet_pump_main, tcb);
	if (status != 0) {
		free(tcb);
		return NULL;
	}

	svc_ctx->tcb_list = eina_list_append(svc_ctx->tcb_list, tcb);
	return tcb;
}

/*!
 * \note
 * SERVER THREAD
 */
static inline void tcb_teminate_all(struct service_context *svc_ctx)
{
	struct tcb *tcb;
	void *ret;
	int status;

	/*!
	 * We don't need to make critical section on here.
	 * If we call this after terminate the server thread first.
	 * Then there is no other thread to access tcb_list.
	 */
	EINA_LIST_FREE(svc_ctx->tcb_list, tcb) {
		/*!
		 * ASSERT(tcb->fd >= 0);
		 */
		secure_socket_destroy_handle(tcb->fd);

		status = pthread_join(tcb->thid, &ret);

		free(tcb);
	}
}

/*!
 * \note
 * SERVER THREAD
 */
static inline void tcb_destroy(struct service_context *svc_ctx, struct tcb *tcb)
{
	void *ret;
	int status;

	svc_ctx->tcb_list = eina_list_remove(svc_ctx->tcb_list, tcb);
	/*!
	 * ASSERT(tcb->fd >= 0);
	 * Close the connection, and then collecting the return value of thread
	 */
	secure_socket_destroy_handle(tcb->fd);
	free(tcb->user);
	free(tcb->client);
	free(tcb->session);

	status = pthread_join(tcb->thid, &ret);

	free(tcb);
}

/*!
 * \note
 * SERVER THREAD
 */
static inline int find_max_fd(struct service_context *svc_ctx)
{
	int fd;
	Eina_List *l;
	struct service_event_item *item;

	fd = svc_ctx->fd > svc_ctx->tcb_pipe[PIPE_READ] ? svc_ctx->fd : svc_ctx->tcb_pipe[PIPE_READ];
	fd = fd > svc_ctx->evt_pipe[PIPE_READ] ? fd : svc_ctx->evt_pipe[PIPE_READ];

	EINA_LIST_FOREACH(svc_ctx->event_list, l, item) {
		if (item->type == SERVICE_EVENT_TIMER && fd < item->info.timer.fd)
			fd = item->info.timer.fd;
	}

	fd += 1;
	return fd;
}

/*!
 * \note
 * SERVER THREAD
 */
static inline void update_fdset(struct service_context *svc_ctx, fd_set *set)
{
	Eina_List *l;
	struct service_event_item *item;

	FD_ZERO(set);
	FD_SET(svc_ctx->fd, set);
	FD_SET(svc_ctx->tcb_pipe[PIPE_READ], set);
	FD_SET(svc_ctx->evt_pipe[PIPE_READ], set);

	EINA_LIST_FOREACH(svc_ctx->event_list, l, item) {
		if (item->type == SERVICE_EVENT_TIMER)
			FD_SET(item->info.timer.fd, set);
	}
}

/*!
 * \note
 * SERVER THREAD
 */
static inline void processing_timer_event(struct service_context *svc_ctx, fd_set *set)
{
	uint64_t expired_count;
	Eina_List *l;
	Eina_List *n;
	struct service_event_item *item;

	EINA_LIST_FOREACH_SAFE(svc_ctx->event_list, l, n, item) {
		switch (item->type) {
		case SERVICE_EVENT_TIMER:
			if (!FD_ISSET(item->info.timer.fd, set))
				break;

			if (read(item->info.timer.fd, &expired_count, sizeof(expired_count)) == sizeof(expired_count)) {
				if (item->event_cb(svc_ctx, item->cbdata) >= 0)
					break;
			}

			if (!eina_list_data_find(svc_ctx->event_list, item))
				break;

			svc_ctx->event_list = eina_list_remove(svc_ctx->event_list, item);
			close(item->info.timer.fd);

			free(item);
			break;
		default:
			break;
		}
	}
}

/*!
 * Accept new client connections
 * And create a new thread for service.
 *
 * Create Client threads & Destroying them
 * SERVER THREAD
 */
static void *server_main(void *data)
{
	struct service_context *svc_ctx = data;
	fd_set set;
	long ret;
	int client_fd;
	struct tcb *tcb;
	int fd;
	char evt_ch;
	struct packet_info *packet_info;

	while (1) {
		fd = find_max_fd(svc_ctx);
		update_fdset(svc_ctx, &set);

		ret = select(fd, &set, NULL, NULL, NULL);
		if (ret < 0) {
			ret = -errno;
			if (errno == EINTR) {
				continue;
			}
			break;
		} else if (ret == 0) {
			ret = -ETIMEDOUT;
			break;
		}

		if (FD_ISSET(svc_ctx->fd, &set)) {
			client_fd = secure_socket_get_connection_handle(svc_ctx->fd);
			if (client_fd < 0) {
				ret = -EFAULT;
				break;
			}

			tcb = tcb_create(svc_ctx, client_fd);
			if (!tcb)
				secure_socket_destroy_handle(client_fd);
		} 

		if (FD_ISSET(svc_ctx->tcb_pipe[PIPE_READ], &set)) {
			if (read(svc_ctx->tcb_pipe[PIPE_READ], &tcb, sizeof(tcb)) != sizeof(tcb)) {
				ret = -EFAULT;
				break;
			}

			/*!
			 * \note
			 * Invoke the service thread main, to notify the termination of a TCB
			 */
			ret = svc_ctx->service_thread_main(tcb, NULL, svc_ctx->service_thread_data);

			/*!
			 * at this time, the client thread can access this tcb.
			 * how can I protect this TCB from deletion without disturbing the server thread?
			 */
			tcb_destroy(svc_ctx, tcb);
		} 

		if (FD_ISSET(svc_ctx->evt_pipe[PIPE_READ], &set)) {
			if (read(svc_ctx->evt_pipe[PIPE_READ], &evt_ch, sizeof(evt_ch)) != sizeof(evt_ch)) {
				ret = -EFAULT;
				break;
			}

			CRITICAL_SECTION_BEGIN(&svc_ctx->packet_list_lock);
			packet_info = eina_list_nth(svc_ctx->packet_list, 0);
			svc_ctx->packet_list = eina_list_remove(svc_ctx->packet_list, packet_info);
			CRITICAL_SECTION_END(&svc_ctx->packet_list_lock);

			/*!
			 * \CRITICAL
			 * What happens if the client thread is terminated, so the packet_info->tcb is deleted
			 * while processing svc_ctx->service_thread_main?
			 */
			ret = svc_ctx->service_thread_main(packet_info->tcb, packet_info->packet, svc_ctx->service_thread_data);

			packet_destroy(packet_info->packet);
			free(packet_info);
		}

		processing_timer_event(svc_ctx, &set);
		/* If there is no such triggered FD? */
	}

	/*!
	 * Consuming all pended packets before terminates server thread.
	 *
	 * If the server thread is terminated, we should flush all pended packets.
	 * And we should services them.
	 * While processing this routine, the mutex is locked.
	 * So every other client thread will be slowed down, sequently, every clients can meet problems.
	 * But in case of termination of server thread, there could be systemetic problem.
	 * This only should be happenes while terminating the master daemon process.
	 */
	CRITICAL_SECTION_BEGIN(&svc_ctx->packet_list_lock);
	EINA_LIST_FREE(svc_ctx->packet_list, packet_info) {
		ret = read(svc_ctx->evt_pipe[PIPE_READ], &evt_ch, sizeof(evt_ch));
		ret = svc_ctx->service_thread_main(packet_info->tcb, packet_info->packet, svc_ctx->service_thread_data);
		packet_destroy(packet_info->packet);
		free(packet_info);
	}
	CRITICAL_SECTION_END(&svc_ctx->packet_list_lock);

	tcb_teminate_all(svc_ctx);
	return (void *)ret;
}

/*!
 * \NOTE
 * MAIN THREAD
 */
struct service_context *service_common_create(const char *addr, int (*service_thread_main)(struct tcb *tcb, struct packet *packet, void *data), void *data)
{
	int status, ret;
	struct service_context *svc_ctx = NULL;

	if (!service_thread_main || !addr) {
		return NULL;
	}

	svc_ctx = calloc(1, sizeof(*svc_ctx));
	if (!svc_ctx) {
		return NULL;
	}

	svc_ctx->fd = secure_socket_create_server(addr);
	if (svc_ctx->fd < 0) {
		free(svc_ctx);
		return NULL;
	}

	svc_ctx->service_thread_main = service_thread_main;
	svc_ctx->service_thread_data = data;

	fcntl(svc_ctx->fd, F_SETFD, FD_CLOEXEC);

	fcntl(svc_ctx->fd, F_SETFL, O_NONBLOCK);

	if (pipe2(svc_ctx->evt_pipe, O_NONBLOCK | O_CLOEXEC) < 0) {
		secure_socket_destroy_handle(svc_ctx->fd);
		free(svc_ctx);
		return NULL;
	}

	if (pipe2(svc_ctx->tcb_pipe, O_NONBLOCK | O_CLOEXEC) < 0) {
		CLOSE_PIPE(svc_ctx->evt_pipe);
		secure_socket_destroy_handle(svc_ctx->fd);
		free(svc_ctx);
		return NULL;
	}

	status = pthread_mutex_init(&svc_ctx->packet_list_lock, NULL);
	if (status != 0) {
		CLOSE_PIPE(svc_ctx->evt_pipe);
		CLOSE_PIPE(svc_ctx->tcb_pipe);
		secure_socket_destroy_handle(svc_ctx->fd);
		free(svc_ctx);
		return NULL;
	}

	ret = cynara_initialize(&svc_ctx->p_cynara, NULL);
	if (ret != CYNARA_API_SUCCESS) {
		print_cynara_error("Cynara initialize failed", ret);
		status = pthread_mutex_destroy(&svc_ctx->packet_list_lock);
		CLOSE_PIPE(svc_ctx->evt_pipe);
		CLOSE_PIPE(svc_ctx->tcb_pipe);
		secure_socket_destroy_handle(svc_ctx->fd);
		free(svc_ctx);
		return NULL;
	}

	status = pthread_create(&svc_ctx->server_thid, NULL, server_main, svc_ctx);
	if (status != 0) {
		status = pthread_mutex_destroy(&svc_ctx->packet_list_lock);
		CLOSE_PIPE(svc_ctx->evt_pipe);
		CLOSE_PIPE(svc_ctx->tcb_pipe);
		secure_socket_destroy_handle(svc_ctx->fd);
		cynara_finish(svc_ctx->p_cynara);
		free(svc_ctx);
		return NULL;
	}

	return svc_ctx;
}

/*!
 * \note
 * MAIN THREAD
 */
int service_common_destroy(struct service_context *svc_ctx)
{
	int status;
	void *ret;

	if (!svc_ctx)
		return -EINVAL;

	/*!
	 * \note
	 * Terminate server thread
	 */
	secure_socket_destroy_handle(svc_ctx->fd);

	status = pthread_join(svc_ctx->server_thid, &ret);
	status = pthread_mutex_destroy(&svc_ctx->packet_list_lock);

	(void)cynara_finish(svc_ctx->p_cynara);

	CLOSE_PIPE(svc_ctx->evt_pipe);
	CLOSE_PIPE(svc_ctx->tcb_pipe);
	free(svc_ctx);
	return 0;
}

/*!
 * \note
 * SERVER THREAD
 */
int tcb_fd(struct tcb *tcb)
{
	if (!tcb)
		return -EINVAL;

	return tcb->fd;
}

/*!
 * \note
 * SERVER THREAD
 */
int tcb_client_type(struct tcb *tcb)
{
	if (!tcb)
		return -EINVAL;

	return tcb->type;
}

/*!
 * \note
 * SERVER THREAD
 */
int tcb_client_type_set(struct tcb *tcb, enum tcb_type type)
{
	if (!tcb)
		return -EINVAL;

	tcb->type = type;
	return 0;
}

/*!
 * \note
 * SERVER THREAD
 */
struct service_context *tcb_svc_ctx(struct tcb *tcb)
{
	if (!tcb)
		return NULL;

	return tcb->svc_ctx;
}

/*!
 * \note
 * SERVER THREAD
 */
int service_common_unicast_packet(struct tcb *tcb, struct packet *packet)
{
	if (!tcb || !packet)
		return -EINVAL;

	return secure_socket_send(tcb->fd, (void *)packet_data(packet), packet_size(packet));
}

/*!
 * \note
 * SERVER THREAD
 */
int service_common_multicast_packet(struct tcb *tcb, struct packet *packet, int type)
{
	Eina_List *l;
	struct tcb *target;
	struct service_context *svc_ctx;
	int ret;

	if (!tcb || !packet)
		return -EINVAL;

	svc_ctx = tcb->svc_ctx;

	EINA_LIST_FOREACH(svc_ctx->tcb_list, l, target) {
		if (target == tcb || target->type != type) {
			continue;
		}

		ret = secure_socket_send(target->fd, (void *)packet_data(packet), packet_size(packet));
	}
	return 0;
}

/*!
 * \note
 * SERVER THREAD
 */
struct service_event_item *service_common_add_timer(struct service_context *svc_ctx, double timer, int (*timer_cb)(struct service_context *svc_cx, void *data), void *data)
{
	struct service_event_item *item;
	struct itimerspec spec;

	item = calloc(1, sizeof(*item));
	if (!item) {
		return NULL;
	}

	item->type = SERVICE_EVENT_TIMER;
	item->info.timer.fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (item->info.timer.fd < 0) {
		free(item);
		return NULL;
	}

	spec.it_interval.tv_sec = (time_t)timer;
	spec.it_interval.tv_nsec = (timer - spec.it_interval.tv_sec) * 1000000000;
	spec.it_value.tv_sec = 0;
	spec.it_value.tv_nsec = 0;

	if (timerfd_settime(item->info.timer.fd, 0, &spec, NULL) < 0) {
		close(item->info.timer.fd);
		free(item);
		return NULL;
	}

	item->event_cb = timer_cb;
	item->cbdata = data;

	svc_ctx->event_list = eina_list_append(svc_ctx->event_list, item);
	return item;
}

/*!
 * \note
 * SERVER THREAD
 */
int service_common_del_timer(struct service_context *svc_ctx, struct service_event_item *item)
{
	if (!eina_list_data_find(svc_ctx->event_list, item)) {
		return -EINVAL;
	}

	svc_ctx->event_list = eina_list_remove(svc_ctx->event_list, item);

	close(item->info.timer.fd);
	free(item);
	return 0;
}


void print_cynara_error(int ret, char *msg) {
	char buff[255] = {0};
	if (cynara_strerror(ret, buff, sizeof(buff)) != CYNARA_API_SUCCESS) {
		fprintf(stderr, "Cynara strerror failed\n");
	}
	fprintf(stderr, "%s (%d) : %s\n", msg, ret, buff);
}

int fill_creds(struct tcb *tcb) {
	int ret;
	char *client = NULL;
	char *user = NULL;
	char *session = NULL;
	pid_t pid;

	ret = cynara_creds_socket_get_client(tcb->fd, CLIENT_METHOD_DEFAULT, &client);
	if (ret != CYNARA_API_SUCCESS) {
		print_cynara_error(ret, "Cynara creds socket get client failed");
		return 0;
	}

	ret = cynara_creds_socket_get_user(tcb->fd, USER_METHOD_DEFAULT, &user);
	if (ret != CYNARA_API_SUCCESS) {
		free(client);
		print_cynara_error(ret, "Cynara creds socket get user failed");
		return 0;
	}

	ret = cynara_creds_socket_get_pid(tcb->fd, &pid);
	if (ret != CYNARA_API_SUCCESS) {
		free(client);
		free(user);
		print_cynara_error(ret, "Cynara creds socket get pid failed");
		return 0;
	}

	session = cynara_session_from_pid(pid);
	if (!session) {
		free(client);
		free(user);
		fprintf(stderr, "Cynara session from pid failed");
		return 0;
	}

	tcb->user = user;
	tcb->client = client;
	tcb->session = session;
	return 1;
}

int check_cynara(struct tcb *tcb) {
	int fd = tcb->fd;
	int ret;
	static const char *privilege = "http://tizen.org/privilege/notification";

	ret = cynara_check(tcb->svc_ctx->p_cynara, tcb->client, tcb->session, tcb->user, privilege);

	if (ret == CYNARA_API_ACCESS_ALLOWED) {
		return 1;
	}
	if (ret != CYNARA_API_ACCESS_DENIED) {
		print_cynara_error(ret, "Cynara check failed");
	}

	return 0;
}

/* End of a file */
