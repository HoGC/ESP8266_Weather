
#ifndef __JSON_PDATA_H
#define	__JSON_PDATA_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

static const char* tempe_type="temperature";
static const char* text_type="text";
static const char* code_type="code";

typedef struct weather_json{
	char tempe_date[3];
	char code_date[3];
	char  text_date[20];
}weather_json;

typedef struct time_json{
	char time_date[6];
	char day_week[4];
	char day_month[4];
	char day_dat[3];
	char time_min[2];
}time_json;

void weather_josn(char *pdata,weather_json *wpade);
void josn_dat(char *pdata,const char *typ,char *dat);
void time_josn(char *pdata,time_json *dat);

#endif
