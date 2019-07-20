/*
* 0.96 OLED  显示程序
*/
#include "driver/i2c_oled.h"
#include "driver/i2c_master.h"
#include "driver/i2c_oled_fonts.h"

#define MAX_SMALL_FONTS 21 //小字体
#define MAX_BIG_FONTS 16 //大字体

static bool oledstat = false;//oled状态初始标识为假

//向某寄存器中写入命令（数据），置位寄存器实现特定功能
bool ICACHE_FLASH_ATTR OLED_writeReg(uint8_t reg_addr,uint8_t val)
{
	//启动iic通信
 	i2c_master_start();
  	//写OLED作为从设备的地址
  	i2c_master_writeByte(OLED_ADDRESS);
  	//如未收到回应信号则停止iic通信，返回失败标识
  	if(!i2c_master_checkAck()) {
		i2c_master_stop();
    	return 0;
  	}
  	//写寄存器地址
  	i2c_master_writeByte(reg_addr);
  	//如未收到回应信号则停止iic通信，返回失败标识
  	if(!i2c_master_checkAck()) {
		i2c_master_stop();
    	return 0;
  	}
  	//写数据，更新各位的值
  	i2c_master_writeByte(val&0xff);
  	//如未收到回应信号则停止iic通信，返回失败标识
  	if(!i2c_master_checkAck()) {
	  	i2c_master_stop();
    	return 0;
  	}
  	//停止iic通信
  	i2c_master_stop();

  	//更新oled状态为真，即初始化完毕，可用了
  	if (reg_addr==0x00)
    	oledstat=true;
    return 1;
}

//向显示控制寄存器中写命令，各命令值及含义见“0.96OLED显示屏_驱动芯片手册”文档的28页
void ICACHE_FLASH_ATTR OLED_writeCmd(unsigned char I2C_Command)
{
	OLED_writeReg(0x00,I2C_Command);
}

//写待显示的数据，也就是点亮要显示的分辨率点
void ICACHE_FLASH_ATTR OLED_writeDat(unsigned char I2C_Data)
{
	OLED_writeReg(0x40,I2C_Data);
}

//Oled初始化相关操作，参考51例程和中景园文档中给出的步骤，具体为什么这样初始化不甚明了，也许是模块自身规定的
bool ICACHE_FLASH_ATTR OLED_Init(void)
{

	i2c_master_gpio_init();

  	//延时10秒
  	os_delay_us(60000);
  	os_delay_us(40000);


  	OLED_writeCmd(0xae); // 关闭屏幕显示，进入休眠状态
  	OLED_writeCmd(0x00); // 设置列地址低位
  	OLED_writeCmd(0x10); // 设置列地址高位
  	OLED_writeCmd(0x40); // 设置起始行地址
  	OLED_writeCmd(0x81); // 设置对照控制寄存器地址
  	OLED_writeCmd(0xcf); // 设置SEG输出电流亮度

  	OLED_writeCmd(0xa1); // 列地址0重置
	//OLED_writeCmd(0xa0); // 0xa0左右反置 0xa1正常
	OLED_writeCmd(0xc8); // 设置COM口为普通扫描模式
  	//OLED_writeCmd(0xc0); // 0xc0上下反置 0xc8正常
  
	OLED_writeCmd(0xa6); // 设置为普通显示模式
	OLED_writeCmd(0xa8); // 设置1至64多样化比例
  	OLED_writeCmd(0x3f); // 1/64 占空比
  	OLED_writeCmd(0xd3); // 设置显示补偿
  	OLED_writeCmd(0x00); //
  	OLED_writeCmd(0xd5); // 设置显示时钟划分比例
  	OLED_writeCmd(0x80); // 设置划分比例
  	OLED_writeCmd(0xd9); // 设置显示时钟的周期
  	OLED_writeCmd(0xf1); 
  	OLED_writeCmd(0xda); // 设置COM脚硬件配置
  	OLED_writeCmd(0x12); 
  	OLED_writeCmd(0xdb); // 设置 vcomh 级别
  	OLED_writeCmd(0x40); 
	OLED_writeCmd(0x20); // 设置页面寻址模式(0x00/0x01/0x02)
  	OLED_writeCmd(0x02); //
    OLED_writeCmd(0x8d); // 设置电池不可用
  	OLED_writeCmd(0x14); //
	OLED_writeCmd(0xa4); // 禁用整个显示(0xa4/0xa5)
  	OLED_writeCmd(0xa6); // 在(0xa6/a7)上禁用反向显示
  	OLED_writeCmd(0xaf); // 打开显示屏
    
  	OLED_Fill(0x00);  //清屏
  
  	return oledstat;
}

//设置屏幕上的显示位置
void ICACHE_FLASH_ATTR OLED_SetPos(unsigned char x, unsigned char y)
{ 
	OLED_writeCmd(0xb0+y);
	OLED_writeCmd(((x&0xf0)>>4)|0x10);
	OLED_writeCmd((x&0x0f)|0x01);
}

//屏幕填充
void ICACHE_FLASH_ATTR OLED_Fill(unsigned char fill_Data)
{
	unsigned char m,n;
	for(m=0;m<8;m++)
	{
		OLED_writeCmd(0xb0+m);		//第几页,也就是第几行
		OLED_writeCmd(0x00);		//列低位的起始地址
		OLED_writeCmd(0x10);		//列高位的起始地址
		for(n=0;n<128;n++)
		{
			OLED_writeDat(fill_Data);
		}
	}
}
//清第N到M行
void ICACHE_FLASH_ATTR OLED_CLS_N(unsigned char N,unsigned char M)
{
	unsigned char n;
	for(;N<=M;N++)
	{
		OLED_writeCmd(0xb0+N);		//第几页,也就是第几行
		OLED_writeCmd(0x00);		//列低位的起始地址
		OLED_writeCmd(0x10);		//列高位的起始地址
		for(n=0;n<128;n++)
		{
			OLED_writeDat(0x00);
		}
	}
}

//清屏
void ICACHE_FLASH_ATTR OLED_CLS(void)
{
	OLED_Fill(0x00);
}

//打开LED屏幕
void ICACHE_FLASH_ATTR OLED_ON(void)
{
	OLED_writeCmd(0X8D);
	OLED_writeCmd(0X14);
	OLED_writeCmd(0XAF);
}

//关闭LED屏幕
void ICACHE_FLASH_ATTR OLED_OFF(void)
{
	OLED_writeCmd(0X8D);
	OLED_writeCmd(0X10);
	OLED_writeCmd(0XAE);
}


//在屏幕上的指定位置显示出指定的字符串，参数为列位置、行位置，待显示字符串，字体大小，也就是点亮相关分辨率点的过程
void ICACHE_FLASH_ATTR OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
{
	unsigned char c = 0,i = 0,j = 0;
	switch(TextSize)
	{
		case 1:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 126)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<6;i++)
					OLED_writeDat(F6x8[c][i]);
				x += 6;
				j++;
			}
		}break;
		case 2:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 120)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<8;i++)
				{
					OLED_writeDat(F8X16[c*16+i]);
				}
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
				{
					OLED_writeDat(F8X16[c*16+i+8]);
				}
				x += 8;
				j++;
			}
		}break;
	}
}
//显示16*16点阵  显示的坐标（x,y），0<=x<8,  0<=y<4
void ICACHE_FLASH_ATTR OLED_P16x16Ch(unsigned char x,unsigned char y,unsigned char N)
{
	unsigned char wm=0;
	unsigned int adder=32*N;
	OLED_SetPos(x*16 , y*2);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_writeDat(F16X16[adder]);
		adder += 1;
	}
	OLED_SetPos(x*16,y*2 + 1);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_writeDat(F16X16[adder]);
		adder += 1;
	}
}

//显示位图，由取模工具得位图数据，x0为起始列的点，y0为起始行的的点，x1/y1分别为终止行的点，BMP为位图数据，也就是点亮相关分辨率点的过程
//可以将汉字设计为大小合适的位图，在合适的位置显示
void ICACHE_FLASH_ATTR OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char i)//
{
	unsigned int j=0;
	unsigned char x,y;

	if (x1>128)
		x1=128;
	if (y1>8)
		y1=8;

	for(y=y0;y<y1;y++)
	{
		OLED_SetPos(x0,y);
    for(x=x0;x<x1;x++)
		{
			OLED_writeDat(BMP[i][j++]);
		}
	}
}
