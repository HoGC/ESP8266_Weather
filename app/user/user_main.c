/*
 * ESP8266_Weather
 * Author: HoGC 
 */
#include "osapi.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"

#include "driver/wifi.h"
#include "driver/gpio_key.h"
#include "driver/esp_http.h"
#include "driver/i2c_oled.h"
#include "driver/json_pdata.h"
#include "driver/webconfig.h"

#define MAIN_DEBUG_ON

#if defined(MAIN_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

u8 get_weather[1024];
bool tiem_ten = true;				//十分钟标志
bool weather_up = false;			//天气是否更新标志
bool config_flag = false;			//是否处于配置模式
os_timer_t checkTimer_updata;		//更新信息定时器

//天气代码对应的BMP图片
unsigned short codenum[]={1,2,1,2,3,4,5,3,3,6,7,8,0,9,10,11,12,12,12,0,0,13,13,13,13,13,0,0,0,0,14,14,15,15,0,0,0,0,16};

#define user_key "your_private_key"
//请求天气的url
#define get_weather_buf "https://api.seniverse.com/v3/weather/now.json?key=%s&location=%s&language=en&unit=c"

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

//smartconfig配置回调
void ICACHE_FLASH_ATTR smartconfig_cd(sm_status status){

	switch (status)
	{
		case SM_STATUS_FINISH:
			INFO("smartconfig_finish\n");
			break;
		case SM_STATUS_GETINFO:
			INFO("wifiinfo_error\n");
			break;
		case SM_STATUS_TIMEOUT:
			INFO("smartconfig_timeout\n");
			break;
	}
	config_flag = false;
}

//按键短按回调
LOCAL void ICACHE_FLASH_ATTR key1LongPress(void) {

	INFO("start_smartconfig\n");
	config_flag = true;
	OLED_CLS_N(1,7);
	OLED_ShowStr(0,3,"  smartconfig   ",2);
	//进入smartconfig配置
	start_smartconfig(smartconfig_cd);
}

//按键初始化
LOCAL void ICACHE_FLASH_ATTR keyInit(void) {

	//设置按键数量
	set_key_num(1);
	//长按、短按的按键回调
	key_add(D0, key1LongPress, NULL);

}

//时间服务器初始化
void ICACHE_FLASH_ATTR my_sntp_init(void)
{
	ip_addr_t * addr = (ip_addr_t *) os_zalloc(sizeof(ip_addr_t));

	sntp_setservername(0, "us.pool.ntp.org");
	sntp_setservername(1, "ntp.sjtu.edu.cn");
	ipaddr_aton("210.72.145.44", addr);
	sntp_setserver(2, addr);
	os_free(addr);
	sntp_init();
}

//get请求结束回调
void ICACHE_FLASH_ATTR http_dis_cb(){
	os_timer_arm(&checkTimer_updata, 1000, true);
}

//get请求回调
void ICACHE_FLASH_ATTR http_get_cb(char *data,unsigned short len){

	weather_json wdata;
	unsigned short tempe_len = 0;
	unsigned short text_len = 0;
	unsigned short code_len = 0;

	if(os_strstr(data, "results")!=NULL){
		//INFO("data:%s\n",data);
		INFO("weather show\n");
		weather_josn(data,&wdata);
		code_len=atoi(wdata.code_date);
		tempe_len=(175-((os_strlen(wdata.tempe_date)+2)*8))/2;
		OLED_CLS_N(1,7);
		OLED_DrawBMP(0,1,48,7,codenum[code_len]);
		OLED_ShowStr(tempe_len,2,wdata.tempe_date,2);
		OLED_ShowStr(tempe_len+16,2,"~C",2);
		if(os_strlen(wdata.text_date)<=10)
		{
			text_len=(175-(os_strlen(wdata.text_date)*8))/2;
			OLED_ShowStr(text_len,4,wdata.text_date,2);
		}else
		{
			text_len=(175-(os_strlen(wdata.text_date)*6))/2;
			OLED_ShowStr(text_len,5,wdata.text_date,1);
		}
	}else if(os_strstr(data,"The location can not be found.")!=NULL){
		OLED_CLS_N(1,7);
		OLED_ShowStr(0,2,"    location    ",2);
		OLED_ShowStr(0,4,"  not be found  ",2);
	}else if(os_strstr(data,"The API key is invalid")!=NULL){
		OLED_CLS_N(1,7);
		OLED_ShowStr(0,3," key is invalid ",2);
	}else{
		OLED_CLS_N(1,7);
		OLED_ShowStr(0,3,"      ERROR     ",2);
	}
}

//更新数据
void ICACHE_FLASH_ATTR check_updata(){
	char *PC;
	time_json tdata;
	unsigned short tempe_len = 0;
	unsigned short text_len = 0;
	unsigned short code_len = 0;

	uint32_t time = sntp_get_current_timestamp();
	system_soft_wdt_feed();
	if(time!=0)
	{
		PC = sntp_get_real_time(time);
		time_josn(PC,&tdata);
		OLED_ShowStr(0,0,tdata.time_date,1);
		OLED_ShowStr(56,0,tdata.day_week,1);
		OLED_ShowStr(96,0,tdata.day_month,1);
		OLED_ShowStr(114,0,tdata.day_dat,1);
		if((tiem_ten == true)&&(weather_up == true))
		{
			INFO("updating...\n");
			tiem_ten = false;
			weather_up = false;
			os_timer_disarm(&checkTimer_updata);
			http_get(get_weather,http_get_cb, http_dis_cb);
		}
		if(os_strcmp(tdata.time_min,"9\0")==0)
		{
			tiem_ten = true;
			weather_up = false;
		}else{
			weather_up = true;
		}
	}else{
		OLED_CLS_N(0,1);
	}
	
    
}

//WIFI连接回调
void ICACHE_FLASH_ATTR wifi_connect_cb(void){

	INFO("wifi connect!\r\n");
	tiem_ten = true;
	weather_up = true;
	OLED_CLS_N(1,7);
	OLED_ShowStr(0,3," updating data.. ",2);
	os_timer_disarm(&checkTimer_updata);
	os_timer_setfn(&checkTimer_updata, (os_timer_func_t *) check_updata,NULL);
	os_timer_arm(&checkTimer_updata, 1000, true);
}

//WIFI断开回调
void ICACHE_FLASH_ATTR wifi_disconnect_cb(void){
 
	INFO("WIFI Disconnect\n");
	os_timer_disarm(&checkTimer_updata);
	if(config_flag != true){
		OLED_CLS_N(0,7);
		OLED_ShowStr(0,3,"Waiting For WIFI",2);
	}
	
}

///提取网页返回的城市信息，并保存到flash
void ICACHE_FLASH_ATTR weather_config(char *psend) {
	char *PA;
	char *PB;

	SpiFlashOpResult ret = 0;
	PA = os_strstr(psend, "location");
	PA = PA + os_strlen("location") + 1;
	PB = os_strstr(PA, "&SSID");
	if(os_strlen(PA) - os_strlen(PB) != 0){
		os_strncpy(location, PA, os_strlen(PA) - os_strlen(PB));
		location[(os_strlen(PA) - os_strlen(PB))] = '\0';
		INFO("location:%s\n", location);
		os_sprintf(get_weather,get_weather_buf,user_key,location);
		//保存城市信息
		spi_flash_erase_sector(0xE0);
		ret = spi_flash_write(0xE0 * 4096, (uint32 *) location, 20);
		if (ret == SPI_FLASH_RESULT_OK) {
			INFO("write_ok\r\n");
		}
		os_delay_us(5000);
	}
}

//提取网页返回的wifi信息，并连接到wifi
void ICACHE_FLASH_ATTR webconfig_wifi_connect(char *psend) {
	char *PA;
	char *PB;
	char *PA1;
	char *PB1;

	struct station_config stationConf;

	PA = os_strstr(psend, "SSID");
	PA = PA + os_strlen("SSID") + 1;
	PB = os_strstr(PA, "&password");

	if (os_strlen(PA) - os_strlen(PB) != 0) {

		os_strncpy(stationConf.ssid, PA, os_strlen(PA) - os_strlen(PB));
		stationConf.ssid[(os_strlen(PA) - os_strlen(PB))] = '\0';
		PA1 = os_strstr(psend, "password");
		PA1 = PA1 + os_strlen("password") + 1;
		PB1 = os_strstr(PA1, "&submitOK");
        if(os_strlen(PA) - os_strlen(PB) != 0){
            os_strncpy(stationConf.password, PA1, os_strlen(PA1) - os_strlen(PB1));
		    stationConf.password[(os_strlen(PA1) - os_strlen(PB1))] = '\0';
        }
		INFO("ssid:%s\n",stationConf.ssid);
        INFO("password:%s\n",stationConf.password);
        wifi_set_opmode(STATION_MODE);
		os_delay_us(500);
		wifi_station_set_config(&stationConf);
		os_delay_us(500);
		wifi_station_connect();
	} else {
		wifi_set_opmode(STATION_MODE);
		wifi_station_connect();
	}
}

//网页配置信息接收回调
void ICACHE_FLASH_ATTR webconfig_cb(char *pusrdata, unsigned short length) 
{
	tiem_ten = true;
	weather_up = true;
	//提取wifi信息
	webconfig_wifi_connect(pusrdata);
	//提取城市信息
	weather_config(pusrdata);
}

//网页配置
void ICACHE_FLASH_ATTR webconfig(void) {
	INFO("Webconfig Start!\r\n");
	config_flag = true;
	OLED_CLS_N(1,7);
	OLED_ShowStr(0,3,"    Webconfig   ",2);
	wifi_station_disconnect();
	//设置AP名和回调
	webconfig_init("weather", webconfig_cb);
}

void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(115200, 115200);
	os_delay_us(60000);

    wifi_set_opmode(STATION_MODE); 
	//设置wifi信息最大存储数量
	wifi_station_ap_number_set(2);

	//读取目标城市拼音
    spi_flash_read(0xE0*4096, (uint32 *) &location, 20);
	INFO("location:%s\n",location);
    os_sprintf(get_weather,get_weather_buf,user_key,location);
    INFO("get url:%s\n",get_weather);

	//三次上电进入网页配置模式
	power_key_init(webconfig);

	//按键初始化
	keyInit();

	//时间服务器初始化
	my_sntp_init();

	//OLED初始化
	OLED_Init();
	OLED_ShowStr(0,3,"Waiting For WIFI",2);

    //设置wifi连接、断开回调，并自动尝试连接以储存的多个WIFI
    set_wifistate_cb(wifi_connect_cb, wifi_disconnect_cb);
}
