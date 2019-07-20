/*
 * http.h
 *
 *  Created on: 2019年5月10日
 *      Author: G
 */

#ifndef ESP_HTTP_H_
#define ESP_HTTP_H_

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

typedef void (*user_tcp_recv_cb_t)(char *data,unsigned short len);
typedef void (*user_tcp_discon_cb_t)(void);

#define packet_size   (2 * 1024)
#define GET_Buf "GET /%s HTTP/1.1\r\nContent-Type: text/html;charset=utf-8\r\nAccept: */*\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n"
#define POST_Buf "POST /%s HTTP/1.1\r\nAccept: */*\r\nContent-Length: %d\r\nContent-Type: application/json\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n%s"
LOCAL struct espconn user_tcp_conn;

void tcp_init(struct ip_addr *remote_ip,struct ip_addr *local_ip,int remote_port);

void user_dns_check_cb(void);
void http_parse_request_url(char *URL, char *host,char *filename, unsigned short *port);
void user_esp_dns_found(const char *name, ip_addr_t *ipaddr,void *arg);

void ICACHE_FLASH_ATTR http_get(char *URL, user_tcp_recv_cb_t u_tcp_recv_cb, user_tcp_discon_cb_t u_tcp_discon_cb);
void ICACHE_FLASH_ATTR http_post(char *URL,char *method,char *postdata);

#endif /* APP_INCLUDE_DRIVER_ESP_HTTP_H_ */
