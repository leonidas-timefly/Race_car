#ifndef __XPT2046_H
#define __XPT2046_H
#include "main.h"

#define TP_PRES_DOWN 0x80  //触屏被按下	  
#define TP_CATH_PRES 0x40  //有按键按下了 
#define CT_MAX_TOUCH  5    //电容屏支持的点数,固定为5点

//FLASH起始地址
#define STM32_FLASH_BASE_ILI 0x08000000 	//STM32 FLASH的起始地址
#define FLASH_WAITETIME_ILI  50000          //FLASH等待超时时间

//FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0_ILI     ((uint32_t)0x08000000) 	//扇区0起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1_ILI     ((uint32_t)0x08004000) 	//扇区1起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2_ILI     ((uint32_t)0x08008000) 	//扇区2起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3_ILI     ((uint32_t)0x0800C000) 	//扇区3起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4_ILI     ((uint32_t)0x08010000) 	//扇区4起始地址, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5_ILI     ((uint32_t)0x08020000) 	//扇区5起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_6_ILI     ((uint32_t)0x08040000) 	//扇区6起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7_ILI     ((uint32_t)0x08060000) 	//扇区7起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8_ILI     ((uint32_t)0x08080000) 	//扇区8起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_9_ILI     ((uint32_t)0x080A0000) 	//扇区9起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_10_ILI    ((uint32_t)0x080C0000) 	//扇区10起始地址,128 Kbytes  
#define ADDR_FLASH_SECTOR_11_ILI    ((uint32_t)0x080E0000) 	//扇区11起始地址,128 Kbytes 

//触摸屏控制器
typedef struct
{
	uint16_t x[CT_MAX_TOUCH]; 		//当前坐标
	uint16_t y[CT_MAX_TOUCH];		//电容屏有最多5组坐标,电阻屏则用x[0],y[0]代表:此次扫描时,触屏的坐标,用
								//x[4],y[4]存储第一次按下时的坐标. 
	uint8_t  sta;					//笔的状态 
								//b7:按下1/松开0; 
								//b6:0,没有按键按下;1,有按键按下. 
								//b5:保留
								//b4~b0:电容触摸屏按下的点数(0,表示未按下,1表示按下)
/////////////////////触摸屏校准参数(电容屏不需要校准)//////////////////////								
	float xfac;
	float yfac;
	short xoff;
	short yoff;
	//新增的参数,当触摸屏的左右上下完全颠倒时需要用到.
	//b0:0,竖屏(适合左右为X坐标,上下为Y坐标的TP)
	//   1,横屏(适合左右为Y坐标,上下为X坐标的TP) 
	//b1~6:保留.
	//b7:0,电阻屏
	//   1,电容屏 
	uint8_t touchtype;
}_m_tp_dev;

extern _m_tp_dev tp_dev;	 	//触屏控制器在touch.c里面定义

extern uint16_t IcArr[20][4];

//电阻屏函数
void TP_Write_Byte(uint8_t num);						//向控制芯片写入一个数据
uint16_t TP_Read_AD(uint8_t CMD);							//读取AD转换值
uint16_t TP_Read_XOY(uint8_t xy);							//带滤波的坐标读取(X/Y)
uint8_t TP_Read_XY(uint16_t* x, uint16_t* y);					//双方向读取(X+Y)
uint8_t TP_Read_XY2(uint16_t* x, uint16_t* y);					//带加强滤波的双方向坐标读取
void TP_Drow_Touch_Point(uint16_t x, uint16_t y, uint16_t color);//画一个坐标校准点
void TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color);	//画一个大点
void TP_Save_Adjdata(void);						//保存校准参数
uint8_t TP_Get_Adjdata(void);						//读取校准参数
void TP_Adjust(void);							//触摸屏校准
void TP_Adj_Info_Show(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t fac);//显示校准信息
//电阻屏/电容屏 共用函数
uint8_t TOUCH_Scan(uint8_t tp);								//扫描
uint8_t TOUCH_Init(void);								//初始化

void Load_Drow_Dialog(void);
void Screen_Draw_Line(void);

uint8_t LCD_Feedback_Whole(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, int* temp, uint8_t size);//全屏反馈
uint8_t LCD_Feedback(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, float* result, uint8_t size);
uint8_t IC_Feedback(uint8_t num);
uint16_t LCD_ICRec(uint16_t x, uint16_t y, uint16_t xmax, uint8_t str[][20], uint8_t num, uint8_t size);
void LCD_ICSel(uint16_t x, uint16_t y, uint16_t xmax, uint8_t str[][20], uint8_t len, uint8_t cs, uint8_t size);
void LCD_Draw_Keyboard(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size);
void LCD_ShowFloatnum(uint16_t x, uint16_t y, float num, uint8_t size);

#endif
