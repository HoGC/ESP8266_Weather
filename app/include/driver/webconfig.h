/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __WEBCONFIG_H__
#define __WEBCONFIG_H__

typedef void (*config_cd_t)(char *pusrdata, unsigned short length);
extern u8 location[20];

#define SERVER_PORT 80
#define SERVER_SSL_PORT 443

#define FLASH_READ_SIZE 2072
#define HTML_FILE_SIZE 2072

#define URLSize 10

typedef enum Result_Resp {
    RespFail = 0,
    RespSuc,
} Result_Resp;

typedef enum ProtocolType {
    GET = 0,
    POST,
} ProtocolType;

typedef enum _ParmType {
    SWITCH_STATUS = 0,
    INFOMATION,
    WIFI,
    SCAN,
	REBOOT,
    DEEP_SLEEP,
    LIGHT_STATUS,
    CONNECT_STATUS,
    USER_BIN
} ParmType;

typedef struct URL_Frame {
    enum ProtocolType Type;
    char pSelect[URLSize];
    char pCommand[URLSize];
    char pFilename[URLSize];
} URL_Frame;

typedef struct _rst_parm {
    ParmType parmtype;
    struct espconn *pespconn;
} rst_parm;

void webconfig_init(char *ssidname, config_cd_t u_config_cd);
void webserver_recon(void *arg, sint8 err);
void webserver_listen(void *arg);
void webserver_sent(void *arg);
void webserver_discon(void *arg);
void webserver_recv(void *arg, char *pusrdata, unsigned short length);
void data_send(void *arg, bool responseOK, char *psend);
void parse_url(char *precv, URL_Frame *purl_frame);
void webconfig_wifi_connect(char *psend);
void weather_config(char *psend);
#endif
