/*
 * http.c
 *
 *  Created on: 2019年5月10日
 *      Author: G
 */
#include "osapi.h"
#include "c_types.h"
#include "iconv.h"

#include "driver/esp_http.h"
#include "driver/json_pdata.h"

//#define HTTP_DEBUG_ON

#if defined(HTTP_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif


char host[32];
char filename[208];
unsigned short port;
char buffer[1024];
unsigned short re_dns;
struct ip_info info;
ip_addr_t addr;
os_timer_t dns_check_timer;

unsigned short re_dns_conut;

user_tcp_recv_cb_t tcp_recv_cb = NULL;
user_tcp_discon_cb_t tcp_discon_cb = NULL;

//成功接收到服务器返回数据函数
void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg, char *pdata,unsigned short len) {
	char *PA;
	char data[1024]="\0";
	struct espconn *pespconn = arg;
	PA = os_strchr(pdata, '{');
	os_strcpy(data,PA);
	INFO("wpdata:%s\n",data);
	tcp_recv_cb(data,os_strlen(data));
	//espconn_disconnect(&user_tcp_conn);
}

//发送数据到服务器成功的回调函数
void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg) {
	INFO("tcp_sent\r\n ");
}

//断开服务器成功的回调函数
void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg) {
	INFO("tcp_discon\r\n ");
	if(tcp_discon_cb != NULL){
		tcp_discon_cb();
	}
}

//连接失败的回调函数，err为错误代码
void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg, sint8 err) {
	INFO("tcp_errcode:%d\r\n", err);
	if(err == (-11)){
		espconn_gethostbyname(&user_tcp_conn,host, &addr,user_esp_dns_found);
	}else{
		espconn_connect((struct espconn *) arg);
	}
}

//成功连接到服务器的回调函数
void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg) {
	struct espconn *pespconn = arg;
	espconn_regist_recvcb(pespconn, user_tcp_recv_cb);
	espconn_regist_sentcb(pespconn, user_tcp_sent_cb);
	espconn_regist_disconcb(pespconn, user_tcp_discon_cb);

	INFO("buffer:%s\n",buffer);

    char *pbuf = (char *)os_zalloc(packet_size);
    //格式化字符串
    os_sprintf(pbuf, buffer);
    //建立tcp的基础上发送数据
    espconn_sent(pespconn, pbuf, os_strlen(pbuf));
    //释放内存
    os_free(pbuf);

}
void ICACHE_FLASH_ATTR tcp_init(struct ip_addr *remote_ip,
		struct ip_addr *local_ip, int remote_port) {
	//配置
	user_tcp_conn.type = ESPCONN_TCP;
	user_tcp_conn.state = ESPCONN_NONE;
	user_tcp_conn.proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp));
	os_memcpy(user_tcp_conn.proto.tcp->local_ip, local_ip, 4);
	os_memcpy(user_tcp_conn.proto.tcp->remote_ip, remote_ip, 4);
	user_tcp_conn.proto.tcp->local_port = espconn_port();
	user_tcp_conn.proto.tcp->remote_port = remote_port;
	espconn_regist_connectcb(&user_tcp_conn, user_tcp_connect_cb);
	espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb);
	//连接服务器
	espconn_connect(&user_tcp_conn);
	os_free(user_tcp_conn.proto.tcp);
}

//剖析URL
void ICACHE_FLASH_ATTR http_parse_request_url(char *URL, char *host, char *path,
		unsigned short *port) {

	char *PA;
	char *PB;

	memset(path, 0, sizeof(path));
	memset(host, 0, sizeof(host));
	*port = 0;

	if (!(*URL)) {
		INFO("\r\n ----- URL return -----  \r\n");
		return;
	}

	PA = URL;

	if (!strncmp(PA, "http://", strlen("http://"))) {
		PA = URL + strlen("http://");
	} else if (!strncmp(PA, "https://", strlen("https://"))) {
		PA = URL + strlen("https://");
	}

	PB = strchr(PA, '/');

	if (PB) {
		memcpy(host, PA, strlen(PA) - strlen(PB));
		if (PB + 1) {
			memcpy(path, PB + 1, strlen(PB - 1));
			path[strlen(PB) - 1] = 0;
		}
		host[strlen(PA) - strlen(PB)] = 0;
		INFO("host:%s",host);

	} else {
		memcpy(host, PA, strlen(PA));
		host[strlen(PA)] = 0;
		INFO("host:%s",host);
	}

	PA = strchr(host, ':');

	if (PA) {
		*port = atoi(PA + 1);
		host[strlen(host) - strlen(PA)] = 0;
	} else {
		*port = 80;
	}
}

//寻找DNS解析，并且配置
void ICACHE_FLASH_ATTR user_esp_dns_found(const char *name, ip_addr_t *ipaddr,void *arg) {

	if (ipaddr == NULL) {
		INFO("user_dns_found NULL \r\n");
		if(re_dns++ < 30){
			espconn_gethostbyname(&user_tcp_conn, host, &addr, user_esp_dns_found);
		}
		return;
	}else{
		addr.addr = ipaddr->addr;
	}
	u8 server_ip[4];
	memcpy(server_ip, &addr, 4);
	INFO("ip:%d.%d.%d.%d\n",server_ip[0],server_ip[1],server_ip[2],server_ip[3]);
	wifi_get_ip_info(STATION_IF, &info);
	tcp_init(ipaddr, &info.ip, port);
	re_dns = 0;

}


//定义Get请求的实现
void ICACHE_FLASH_ATTR http_get(char *URL, user_tcp_recv_cb_t u_tcp_recv_cb, user_tcp_discon_cb_t u_tcp_discon_cb){
	static char host_buf[32];

	memset(buffer,0,1024);
	tcp_recv_cb = u_tcp_recv_cb;
	tcp_discon_cb = u_tcp_discon_cb;

	http_parse_request_url(URL,host,filename,&port);
	os_sprintf(buffer,GET_Buf,filename,host);
	if(strcmp(host_buf,host) == 0 && addr.addr != 0){
		INFO("tcp_to_ip\n");
		wifi_get_ip_info(STATION_IF, &info);
		tcp_init(&addr, &info.ip, port);
	}else{
		INFO("tcp_to_host\n");
		strcpy(host_buf,host);
		espconn_gethostbyname(&user_tcp_conn,host, &addr,user_esp_dns_found);
	}
}


//定义Post请求的实现
void ICACHE_FLASH_ATTR http_post(char *URL,char *method,char *postdata){
	struct ip_addr addr;
	static char host_buf[32];
	memset(buffer,0,1024);
	http_parse_request_url(URL,host,filename,&port);
	os_sprintf(buffer,POST_Buf,filename,strlen(postdata),host,postdata);
	if(strcmp(host_buf,host) == 0 && addr.addr != 0){
		INFO("tcp_to_ip\n");
		tcp_init(&addr, &info.ip, port);
	}else{
		INFO("tcp_to_host\n");
		strcpy(host_buf,host);
		espconn_gethostbyname(&user_tcp_conn,host, &addr,user_esp_dns_found);
	}
}




