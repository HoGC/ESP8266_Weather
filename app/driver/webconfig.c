#include "os_type.h"
#include "driver/webconfig.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "spi_flash.h"
#include "mem.h"

#define  WEBCONFIG_DEBUG_ON

#if defined(WEBCONFIG_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

config_cd_t config_cd = NULL;

u8 location[20];

void data_send(void *arg, bool responseOK, char *psend) {
	uint16 length = 0;
	char *pbuf = NULL;
	char httphead[256];
	struct espconn *ptrespconn = arg;
	os_memset(httphead, 0, 256);

	if (responseOK) {
		os_sprintf(httphead,
				"HTTP/1.0 200 OK\r\nContent-Length: %d\r\nServer: lwIP/1.4.0\r\n",
				psend ? os_strlen(psend) : 0);

		if (psend) {
			os_sprintf(httphead + os_strlen(httphead),
					"Content-type: text/html\r\nExpires: Fri, 10 Apr 2008 14:00:00 GMT\r\nPragma: no-cache\r\n\r\n");
			length = os_strlen(httphead) + os_strlen(psend);
			pbuf = (char *) os_zalloc(length + 1);
			os_memcpy(pbuf, httphead, os_strlen(httphead));
			os_memcpy(pbuf + os_strlen(httphead), psend, os_strlen(psend));
		} else {
			os_sprintf(httphead + os_strlen(httphead), "\n");
			length = os_strlen(httphead);
		}
	} else {
		os_sprintf(httphead,
				"HTTP/1.0 400 BadRequest\r\n\
				Content-Length: 0\r\nServer: lwIP/1.4.0\r\n\n");
		length = os_strlen(httphead);
	}

	if (psend) {
#ifdef SERVER_SSL_ENABLE
		espconn_secure_sent(ptrespconn, pbuf, length);
#else
		espconn_sent(ptrespconn, pbuf, length);
#endif
	} else {
#ifdef SERVER_SSL_ENABLE
		espconn_secure_sent(ptrespconn, httphead, length);
#else
		espconn_sent(ptrespconn, httphead, length);
#endif
	}

	if (pbuf) {
		os_free(pbuf);
		pbuf = NULL;
	}
}

void parse_url(char *precv, URL_Frame *purl_frame) {
	char *str = NULL;
	uint8 length = 0;
	char *pbuffer = NULL;
	char *pbufer = NULL;

	if (purl_frame == NULL || precv == NULL) {
		return;
	}

	pbuffer = (char *) os_strstr(precv, "Host:");

	if (pbuffer != NULL) {
		length = pbuffer - precv;
		pbufer = (char *) os_zalloc(length + 1);
		pbuffer = pbufer;
		os_memcpy(pbuffer, precv, length);
		os_memset(purl_frame->pSelect, 0, URLSize);
		os_memset(purl_frame->pCommand, 0, URLSize);
		os_memset(purl_frame->pFilename, 0, URLSize);

		if (os_strncmp(pbuffer, "GET ", 4) == 0) {
			purl_frame->Type = GET;
			pbuffer += 4;
		} else if (os_strncmp(pbuffer, "POST ", 5) == 0) {
			purl_frame->Type = POST;
			pbuffer += 5;
		}

		pbuffer++;
		str = (char *) os_strstr(pbuffer, "?");

		if (str != NULL) {
			length = str - pbuffer;
			os_memcpy(purl_frame->pSelect, pbuffer, length);
			str++;
			pbuffer = (char *) os_strstr(str, "=");

			if (pbuffer != NULL) {
				length = pbuffer - str;
				os_memcpy(purl_frame->pCommand, str, length);
				pbuffer++;
				str = (char *) os_strstr(pbuffer, "&");

				if (str != NULL) {
					length = str - pbuffer;
					os_memcpy(purl_frame->pFilename, pbuffer, length);
				} else {
					str = (char *) os_strstr(pbuffer, " HTTP");

					if (str != NULL) {
						length = str - pbuffer;
						os_memcpy(purl_frame->pFilename, pbuffer, length);
					}
				}
			}
		}

		os_free(pbufer);
	} else {
		return;
	}
}

void webserver_recv(void *arg, char *pusrdata, unsigned short length) {
	URL_Frame *pURL_Frame = NULL;
	char *pParseBuffer = NULL;
	char *index = NULL;

	SpiFlashOpResult ret = 0;
	INFO("len:%u\r\n", length);
	INFO("Webserver recv:-------------------------------\r\n%s\r\n",
			pusrdata);
	pURL_Frame = (URL_Frame *) os_zalloc(sizeof(URL_Frame));
	parse_url(pusrdata, pURL_Frame);

	switch (pURL_Frame->Type) {
	case GET:
		INFO("We have a GET request.\r\n");
		if (pURL_Frame->pFilename[0] == 0) {
			index = (char *) os_zalloc(FLASH_READ_SIZE+1);
			if (index == NULL) {
				INFO("os_zalloc error!\r\n");
				goto _temp_exit;
			}
			// Flash read/write has to be aligned to the 4-bytes boundary
			ret = spi_flash_read(0xD0 * SPI_FLASH_SEC_SIZE, (uint32 *) index,
			FLASH_READ_SIZE);  // start address:0x10000 + 0xC0000
			if (ret != SPI_FLASH_RESULT_OK) {
				//ERR_LOG("spi_flash_read err:%d\r\n", ret);
				os_free(index);
				index = NULL;
				goto _temp_exit;
			}
			index[HTML_FILE_SIZE] = 0;   // put 0 to the end
			data_send(arg, true, index);
			os_free(index);
			index = NULL;
		}
		break;

	case POST:
		INFO("We have a POST request.\r\n");

		pParseBuffer = (char *) os_strstr(pusrdata, "\r\n\r\n");
		INFO("pusrdata=", pusrdata);
		if (pParseBuffer == NULL) {
			data_send(arg, false, NULL);
			break;
		}
		config_cd(pusrdata,length);
		break;
	}

	_temp_exit: ;
	if (pURL_Frame != NULL) {
		os_free(pURL_Frame);
		pURL_Frame = NULL;
	}
}

void webserver_discon(void *arg) {
	struct espconn *pesp_conn = arg;

	INFO("webserver's %d.%d.%d.%d:%d disconnect\n",
	pesp_conn->proto.tcp->remote_ip[0],
	pesp_conn->proto.tcp->remote_ip[1],
	pesp_conn->proto.tcp->remote_ip[2],
	pesp_conn->proto.tcp->remote_ip[3],
	pesp_conn->proto.tcp->remote_port);
}

void webserver_sent(void *arg) {
	struct espconn *pesp_conn = arg;

	INFO("webserver's %d.%d.%d.%d:%d disconnect\n",
	pesp_conn->proto.tcp->remote_ip[0],
	pesp_conn->proto.tcp->remote_ip[1],
	pesp_conn->proto.tcp->remote_ip[2],
	pesp_conn->proto.tcp->remote_ip[3],
	pesp_conn->proto.tcp->remote_port);
}

void webserver_listen(void *arg) {
	struct espconn *pesp_conn = arg;

	espconn_regist_recvcb(pesp_conn, webserver_recv);
	espconn_regist_reconcb(pesp_conn, webserver_recon);
	espconn_regist_disconcb(pesp_conn, webserver_discon);
	espconn_regist_sentcb(pesp_conn, webserver_sent);
}

void webserver_recon(void *arg, sint8 err) {
	struct espconn *pesp_conn = arg;

	INFO("webserver's %d.%d.%d.%d:%d err %d reconnect\n",
	pesp_conn->proto.tcp->remote_ip[0],
	pesp_conn->proto.tcp->remote_ip[1],
	pesp_conn->proto.tcp->remote_ip[2],
	pesp_conn->proto.tcp->remote_ip[3],
	pesp_conn->proto.tcp->remote_port, err);
}

//网页配置初始化
void webconfig_init(char *ssidname, config_cd_t u_config_cd) {

	config_cd = u_config_cd;
	wifi_set_opmode(SOFTAP_MODE);
	struct softap_config stationConf;
	wifi_softap_get_config(&stationConf);
	os_strcpy(stationConf.ssid, ssidname);
	stationConf.ssid_len = os_strlen(ssidname);
	wifi_softap_set_config_current(&stationConf);

	LOCAL struct espconn esp_conn;
	LOCAL esp_tcp esptcp;
	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = SERVER_PORT;
	espconn_regist_connectcb(&esp_conn, webserver_listen);
	espconn_accept(&esp_conn);
}

