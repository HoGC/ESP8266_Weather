#include "driver/json_pdata.h"

void ICACHE_FLASH_ATTR weather_josn(char *pdata,weather_json *wpade) {
	//const char needle[10] = typ;
	josn_dat(pdata,tempe_type,wpade->tempe_date);
	josn_dat(pdata,text_type,wpade->text_date);
	josn_dat(pdata,code_type,wpade->code_date);
}

void ICACHE_FLASH_ATTR josn_dat(char *pdata,const char *typ,char *dat) {
	//const char needle[10] = typ;
	char *PA;
	char *PB;
	PA = strstr(pdata, typ);
	PA =PA+os_strlen(typ)+3;
	PB = os_strchr(PA, '"');
	dat = strncpy(dat,PA,os_strlen(PA) - os_strlen(PB));
	dat[(os_strlen(PA) - os_strlen(PB))]='\0';
}

void ICACHE_FLASH_ATTR time_josn(char *pdata,time_json *dat) {
	u8 i=0;
 	char month_str_buf[4];
	char month_str[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	os_strncpy(dat->day_week,pdata,3);
	dat->day_week[3]='\0';
	os_strncpy(dat->day_dat,pdata+8,2);
	dat->day_dat[2]='\0';
	os_strncpy(month_str_buf,pdata+4,3);
	month_str_buf[3]='\0';
	for(i=0;i<12;i++){
		if(strcmp(month_str_buf,month_str[i])==0){
			if(i<10){
				dat->day_month[0]='0';
				dat->day_month[1]='0'+i+1;
				dat->day_month[2]='.';
				dat->day_month[3]='\0';
			}else{
				dat->day_month[0]='1';
				dat->day_month[1]='0'+i+10-1;
				dat->day_month[2]='.';
				dat->day_month[3]='\0';
			}
			
		}
	}
	// os_strncpy(dat->day_month,pdata+4,3);
	// dat->day_month[3]='\0';
	os_strncpy(dat->time_date,pdata+11,5);
	dat->time_date[5]='\0';
	os_strncpy(dat->time_min,pdata+15,1);
	dat->time_min[1]='\0';
}
