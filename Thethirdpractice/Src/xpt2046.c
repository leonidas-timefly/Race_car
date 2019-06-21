#include "xpt2046.h"
#include "ili9341.h"
#include "delay.h"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "main.h"
//#include "key.h"

//保存在FLASH里面的地址区间基址,占用13个字节(RANGE:SAVE_ADDR_BASE~SAVE_ADDR_BASE+12)
#define SAVE_ADDR_BASE 0x08040000

_m_tp_dev tp_dev =
{
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};
//默认为touchtype=0的数据.
uint8_t CMD_RDX = 0XD0;
uint8_t CMD_RDY = 0X90;

uint16_t IcArr[20][4];

//读取指定地址的字节(8位数据) 
//faddr:读地址 
//返回值:对应数据.
uint8_t FLASH_ReadOneByte_ILI(uint32_t faddr)
{
	return *(__IO uint8_t*)faddr;
}

//获取某个地址所在的flash扇区
//addr:flash地址
//返回值:0~11,即addr所在的扇区
uint8_t FLASH_GetFlashSector_ILI(uint32_t addr)
{
	if (addr < ADDR_FLASH_SECTOR_1_ILI)return FLASH_SECTOR_0;
	else if (addr < ADDR_FLASH_SECTOR_2_ILI)return FLASH_SECTOR_1;
	else if (addr < ADDR_FLASH_SECTOR_3_ILI)return FLASH_SECTOR_2;
	else if (addr < ADDR_FLASH_SECTOR_4_ILI)return FLASH_SECTOR_3;
	else if (addr < ADDR_FLASH_SECTOR_5_ILI)return FLASH_SECTOR_4;
	else if (addr < ADDR_FLASH_SECTOR_6_ILI)return FLASH_SECTOR_5;
	else if (addr < ADDR_FLASH_SECTOR_7_ILI)return FLASH_SECTOR_6;
	else if (addr < ADDR_FLASH_SECTOR_8_ILI)return FLASH_SECTOR_7;
	else if (addr < ADDR_FLASH_SECTOR_9_ILI)return FLASH_SECTOR_8;
	else if (addr < ADDR_FLASH_SECTOR_10_ILI)return FLASH_SECTOR_9;
	else if (addr < ADDR_FLASH_SECTOR_11_ILI)return FLASH_SECTOR_10;
	return FLASH_SECTOR_11;
}

//从指定地址开始写入指定长度的数据
//特别注意:因为STM32F4的扇区实在太大,没办法本地保存扇区数据,所以本函数
//         写地址如果非0XFF,那么会先擦除整个扇区且不保存扇区数据.所以
//         写非0XFF的地址,将导致整个扇区数据丢失.建议写之前确保扇区里
//         没有重要数据,最好是整个扇区先擦除了,然后慢慢往后写. 
//该函数对OTP区域也有效!可以用来写OTP区!
//OTP区域地址范围:0X1FFF7800~0X1FFF7A0F(注意：最后16字节，用于OTP数据块锁定，别乱写！！)
//WriteAddr:起始地址(此地址必须为4的倍数!!)
//pBuffer:数据指针
//NumToWrite:字节(8位)数(就是要写入的8位数据的个数.) 
void FLASH_WritexByte_ILI(uint32_t WriteAddr, uint8_t* pBuffer, uint32_t NumToWrite)
{
	FLASH_EraseInitTypeDef FlashEraseInit;
	HAL_StatusTypeDef FlashStatus = HAL_OK;
	uint32_t SectorError = 0;
	uint32_t addrx = 0;
	uint32_t endaddr = 0;

	if (WriteAddr < STM32_FLASH_BASE_ILI)return;	//非法地址

	HAL_FLASH_Unlock();             //解锁	
	addrx = WriteAddr;				//写入的起始地址
	endaddr = WriteAddr + NumToWrite;	//写入的结束地址

	FlashStatus = FLASH_WaitForLastOperation(FLASH_WAITETIME_ILI);            //等待上次操作完成
	if (FlashStatus == HAL_OK)
	{
		while (WriteAddr < endaddr)//写数据
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, WriteAddr, *pBuffer) != HAL_OK)//写入数据
			{
				break;	//写入异常
			}
			WriteAddr++;
			pBuffer++;
		}
	}
	HAL_FLASH_Lock();           //上锁
}

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToRead:字节(8位)数
void FLASH_ReadxByte_ILI(uint32_t ReadAddr, uint8_t* pBuffer, uint32_t NumToRead)
{
	for (uint32_t i = 0; i < NumToRead; i++)
	{
		pBuffer[i] = FLASH_ReadOneByte_ILI(ReadAddr);//读取1个字节.
		ReadAddr++;//偏移1个字节.	
	}
}

float float_abs(float x)
{
	if (x < 0)
		return -x;
	return x;
}

//SPI写数据
//向触摸屏IC写入1byte数据
//num:要写入的数据
void TP_Write_Byte(uint8_t num)
{
	uint8_t count = 0;
	for (count = 0; count < 8; count++)
	{
		if (num & 0x80)
			HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_RESET);
		num <<= 1;
		HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_RESET);
		delay_us(1);
		HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_SET); //上升沿有效
	}
}
//SPI读数据
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读到的数据
uint16_t TP_Read_AD(uint8_t CMD)
{
	uint8_t count = 0;
	uint16_t Num = 0;
	HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_RESET);   //先拉低时钟
	HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_RESET); //拉低数据线
	HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_RESET);     //选中触摸屏IC
	TP_Write_Byte(CMD);                                              //发送命令字
	delay_us(6);                                                     //ADS7846的转换时间最长为6us
	HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_RESET);
	delay_us(1);
	HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_SET); //给1个时钟，清除BUSY
	delay_us(1);
	HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_RESET);
	for (count = 0; count < 16; count++) //读出16位数据,只有高12位有效
	{
		Num <<= 1;
		HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_RESET); //下降沿有效
		delay_us(1);
		HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_SET);
		if (HAL_GPIO_ReadPin(T_MISO_GPIO_Port, T_MISO_Pin) == GPIO_PIN_SET)
			Num++;
	}
	Num >>= 4;                                                 //只有高12位有效.
	HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_SET); //释放片选
	return (Num);
}
//读取一个坐标值(x或者y)
//连续读取READ_TIMES次数据,对这些数据升序排列,
//然后去掉最低和最高LOST_VAL个数,取平均值
//xy:指令（CMD_RDX/CMD_RDY）
//返回值:读到的数据
#define READ_TIMES 5 //读取次数
#define LOST_VAL 1   //丢弃值
uint16_t TP_Read_XOY(uint8_t xy)
{
	uint16_t i, j;
	uint16_t buf[READ_TIMES];
	uint16_t sum = 0;
	uint16_t temp;
	for (i = 0; i < READ_TIMES; i++)
		buf[i] = TP_Read_AD(xy);
	for (i = 0; i < READ_TIMES - 1; i++) //排序
	{
		for (j = i + 1; j < READ_TIMES; j++)
		{
			if (buf[i] > buf[j]) //升序排列
			{
				temp = buf[i];
				buf[i] = buf[j];
				buf[j] = temp;
			}
		}
	}
	sum = 0;
	for (i = LOST_VAL; i < READ_TIMES - LOST_VAL; i++)
		sum += buf[i];
	temp = sum / (READ_TIMES - 2 * LOST_VAL);
	return temp;
}
//读取x,y坐标
//最小值不能少于100.
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
uint8_t TP_Read_XY(uint16_t * x, uint16_t * y)
{
	uint16_t xtemp, ytemp;
	xtemp = TP_Read_XOY(CMD_RDX);
	ytemp = TP_Read_XOY(CMD_RDY);
	//if(xtemp<100||ytemp<100)return 0;//读数失败
	*x = xtemp;
	*y = ytemp;
	return 1; //读数成功
}
//连续2次读取触摸屏IC,且这两次的偏差不能超过
//ERR_RANGE,满足条件,则认为读数正确,否则读数错误.
//该函数能大大提高准确度
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
#define ERR_RANGE 50 //误差范围
uint8_t TP_Read_XY2(uint16_t * x, uint16_t * y)
{
	uint16_t x1, y1;
	uint16_t x2, y2;
	uint8_t flag;
	flag = TP_Read_XY(&x1, &y1);
	if (flag == 0)
		return (0);
	flag = TP_Read_XY(&x2, &y2);
	if (flag == 0)
		return (0);
	if (((x2 <= x1 && x1 < x2 + ERR_RANGE) || (x1 <= x2 && x2 < x1 + ERR_RANGE)) //前后两次采样在+-50内
		&& ((y2 <= y1 && y1 < y2 + ERR_RANGE) || (y1 <= y2 && y2 < y1 + ERR_RANGE)))
	{
		*x = (x1 + x2) / 2;
		*y = (y1 + y2) / 2;
		return 1;
	}
	else
		return 0;
}
//////////////////////////////////////////////////////////////////////////////////
//与LCD部分有关的函数
//画一个触摸点
//用来校准用的
//x,y:坐标
//color:颜色
void TP_Drow_Touch_Point(uint16_t x, uint16_t y, uint16_t color)
{
	POINT_COLOR = color;
	LCD_DrawLine(x - 12, y, x + 13, y); //横线
	LCD_DrawLine(x, y - 12, x, y + 13); //竖线
	LCD_DrawPoint(x + 1, y + 1);
	LCD_DrawPoint(x - 1, y + 1);
	LCD_DrawPoint(x + 1, y - 1);
	LCD_DrawPoint(x - 1, y - 1);
	LCD_Draw_Circle(x, y, 6); //画中心圈
}
//画一个大点(2*2的点)
//x,y:坐标
//color:颜色
void TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color)
{
	POINT_COLOR = color;
	LCD_DrawPoint(x, y); //中心点
	LCD_DrawPoint(x + 1, y);
	LCD_DrawPoint(x, y + 1);
	LCD_DrawPoint(x + 1, y + 1);
}
//////////////////////////////////////////////////////////////////////////////////
//触摸按键扫描
//tp:0,屏幕坐标;1,物理坐标(校准等特殊场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
uint8_t TOUCH_Scan(uint8_t tp)
{
	if (HAL_GPIO_ReadPin(T_PEN_GPIO_Port, T_PEN_Pin) == GPIO_PIN_RESET) //有按键按下
	{
		if (tp)
			TP_Read_XY2(&tp_dev.x[0], &tp_dev.y[0]);        //读取物理坐标
		else if (TP_Read_XY2(&tp_dev.x[0], &tp_dev.y[0])) //读取屏幕坐标
		{
			tp_dev.x[0] = tp_dev.xfac * tp_dev.x[0] + tp_dev.xoff; //将结果转换为屏幕坐标
			tp_dev.y[0] = tp_dev.yfac * tp_dev.y[0] + tp_dev.yoff;
		}
		if ((tp_dev.sta & TP_PRES_DOWN) == 0) //之前没有被按下
		{
			tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES; //按键按下
			tp_dev.x[4] = tp_dev.x[0];                //记录第一次按下时的坐标
			tp_dev.y[4] = tp_dev.y[0];
		}
	}
	else
	{
		if (tp_dev.sta & TP_PRES_DOWN) //之前是被按下的
		{
			tp_dev.sta &= ~(1 << 7); //标记按键松开
		}
		else //之前就没有被按下
		{
			tp_dev.x[4] = 0;
			tp_dev.y[4] = 0;
			tp_dev.x[0] = 0xffff;
			tp_dev.y[0] = 0xffff;
		}
	}
	return tp_dev.sta& TP_PRES_DOWN; //返回当前的触屏状态
}
//////////////////////////////////////////////////////////////////////////
//保存校准参数
void TP_Save_Adjdata(void)
{
	int32_t temp;
	//保存校正结果!
	temp = tp_dev.xfac * 100000000; //保存x校正因素
	FLASH_WritexByte_ILI(SAVE_ADDR_BASE, (uint8_t*)& temp, 4);
	temp = tp_dev.yfac * 100000000; //保存y校正因素
	FLASH_WritexByte_ILI(SAVE_ADDR_BASE + 4, (uint8_t*)& temp, 4);
	//保存x偏移量
	FLASH_WritexByte_ILI(SAVE_ADDR_BASE + 8, (uint8_t*)& tp_dev.xoff, 2);
	//保存y偏移量
	FLASH_WritexByte_ILI(SAVE_ADDR_BASE + 10, (uint8_t*)& tp_dev.yoff, 2);
	//保存触屏类型
	FLASH_WritexByte_ILI(SAVE_ADDR_BASE + 12, (uint8_t*)& tp_dev.touchtype, 1);
	temp = 0X0A; //标记校准过了
	FLASH_WritexByte_ILI(SAVE_ADDR_BASE + 13, (uint8_t*)& temp, 1);
}
//得到保存在EEPROM里面的校准值
//返回值：1，成功获取数据
//        0，获取失败，要重新校准
uint8_t TP_Get_Adjdata(void)
{
	int32_t tempfac;
	tempfac = 0;
	FLASH_ReadxByte_ILI(SAVE_ADDR_BASE + 13, (uint8_t*)& tempfac, 1); //读取标记字,看是否校准过！
	if (tempfac == 0X0A)                                          //触摸屏已经校准过了
	{
		FLASH_ReadxByte_ILI(SAVE_ADDR_BASE, (uint8_t*)& tempfac, 4);
		tp_dev.xfac = (float)tempfac / 100000000; //得到x校准参数
		FLASH_ReadxByte_ILI(SAVE_ADDR_BASE + 4, (uint8_t*)& tempfac, 4);
		tp_dev.yfac = (float)tempfac / 100000000; //得到y校准参数
			//得到x偏移量
		FLASH_ReadxByte_ILI(SAVE_ADDR_BASE + 8, (uint8_t*)& tp_dev.xoff, 2);
		//得到y偏移量
		FLASH_ReadxByte_ILI(SAVE_ADDR_BASE + 10, (uint8_t*)& tp_dev.yoff, 2);
		FLASH_ReadxByte_ILI(SAVE_ADDR_BASE + 12, (uint8_t*)& tp_dev.touchtype, 1); //读取触屏类型标记
		if (tp_dev.touchtype)                                                  //X,Y方向与屏幕相反
		{
			CMD_RDX = 0X90;
			CMD_RDY = 0XD0;
		}
		else //X,Y方向与屏幕相同
		{
			CMD_RDX = 0XD0;
			CMD_RDY = 0X90;
		}
		return 1;
	}
	return 0;
}
//提示字符串
uint8_t* const TP_REMIND_MSG_TBL = "Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";

//提示校准结果(各个参数)
void TP_Adj_Info_Show(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t fac)
{
	POINT_COLOR = RED;
	LCD_ShowString(40, 160, lcddev.width, lcddev.height, 16, "x1:");
	LCD_ShowString(40 + 80, 160, lcddev.width, lcddev.height, 16, "y1:");
	LCD_ShowString(40, 180, lcddev.width, lcddev.height, 16, "x2:");
	LCD_ShowString(40 + 80, 180, lcddev.width, lcddev.height, 16, "y2:");
	LCD_ShowString(40, 200, lcddev.width, lcddev.height, 16, "x3:");
	LCD_ShowString(40 + 80, 200, lcddev.width, lcddev.height, 16, "y3:");
	LCD_ShowString(40, 220, lcddev.width, lcddev.height, 16, "x4:");
	LCD_ShowString(40 + 80, 220, lcddev.width, lcddev.height, 16, "y4:");
	LCD_ShowString(40, 240, lcddev.width, lcddev.height, 16, "fac is:");
	LCD_ShowNum(40 + 24, 160, x0, 4, 16);      //显示数值
	LCD_ShowNum(40 + 24 + 80, 160, y0, 4, 16); //显示数值
	LCD_ShowNum(40 + 24, 180, x1, 4, 16);      //显示数值
	LCD_ShowNum(40 + 24 + 80, 180, y1, 4, 16); //显示数值
	LCD_ShowNum(40 + 24, 200, x2, 4, 16);      //显示数值
	LCD_ShowNum(40 + 24 + 80, 200, y2, 4, 16); //显示数值
	LCD_ShowNum(40 + 24, 220, x3, 4, 16);      //显示数值
	LCD_ShowNum(40 + 24 + 80, 220, y3, 4, 16); //显示数值
	LCD_ShowNum(40 + 56, 240, fac, 3, 16);     //显示数值,该数值必须在95~105范围之内.
}

//触摸屏校准代码
//得到四个校准参数
void TP_Adjust(void)
{
	uint16_t pos_temp[4][2]; //坐标缓存值
	uint8_t cnt = 0;
	uint16_t d1, d2;
	uint32_t tem1, tem2;
	double fac;
	uint16_t outtime = 0;
	cnt = 0;
	POINT_COLOR = BLUE;
	BACK_COLOR = WHITE;
	LCD_Clear(WHITE);  //清屏
	POINT_COLOR = RED; //红色
	LCD_Clear(WHITE);  //清屏
	POINT_COLOR = BLACK;
	LCD_ShowString(40, 40, 160, 100, 16, (uint8_t*)TP_REMIND_MSG_TBL); //显示提示信息
	TP_Drow_Touch_Point(20, 20, RED);                                   //画点1
	tp_dev.sta = 0;                                                     //消除触发信号
	tp_dev.xfac = 0;                                                    //xfac用来标记是否校准过,所以校准之前必须清掉!以免错误
	while (1)                                                           //如果连续10秒钟没有按下,则自动退出
	{
		TOUCH_Scan(1);                              //扫描物理坐标
		if ((tp_dev.sta & 0xc0) == TP_CATH_PRES) //按键按下了一次(此时按键松开了.)
		{
			outtime = 0;
			tp_dev.sta &= ~(1 << 6); //标记按键已经被处理过了.

			pos_temp[cnt][0] = tp_dev.x[0];
			pos_temp[cnt][1] = tp_dev.y[0];
			cnt++;
			switch (cnt)
			{
			case 1:
				TP_Drow_Touch_Point(20, 20, WHITE);              //清除点1
				TP_Drow_Touch_Point(lcddev.width - 20, 20, RED); //画点2
				break;
			case 2:
				TP_Drow_Touch_Point(lcddev.width - 20, 20, WHITE); //清除点2
				TP_Drow_Touch_Point(20, lcddev.height - 20, RED);  //画点3
				break;
			case 3:
				TP_Drow_Touch_Point(20, lcddev.height - 20, WHITE);              //清除点3
				TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, RED); //画点4
				break;
			case 4:                                        //全部四个点已经得到
														   //对边相等
				tem1 = abs(pos_temp[0][0] - pos_temp[1][0]); //x1-x2
				tem2 = abs(pos_temp[0][1] - pos_temp[1][1]); //y1-y2
				tem1 *= tem1;
				tem2 *= tem2;
				d1 = sqrt(tem1 + tem2); //得到1,2的距离

				tem1 = abs(pos_temp[2][0] - pos_temp[3][0]); //x3-x4
				tem2 = abs(pos_temp[2][1] - pos_temp[3][1]); //y3-y4
				tem1 *= tem1;
				tem2 *= tem2;
				d2 = sqrt(tem1 + tem2); //得到3,4的距离
				fac = (float)d1 / d2;
				if (fac < 0.95 || fac > 1.05 || d1 == 0 || d2 == 0) //不合格
				{
					cnt = 0;
					TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);                                                                                           //清除点4
					TP_Drow_Touch_Point(20, 20, RED);                                                                                                                            //画点1
					TP_Adj_Info_Show(pos_temp[0][0], pos_temp[0][1], pos_temp[1][0], pos_temp[1][1], pos_temp[2][0], pos_temp[2][1], pos_temp[3][0], pos_temp[3][1], fac * 100); //显示数据
					continue;
				}
				tem1 = abs(pos_temp[0][0] - pos_temp[2][0]); //x1-x3
				tem2 = abs(pos_temp[0][1] - pos_temp[2][1]); //y1-y3
				tem1 *= tem1;
				tem2 *= tem2;
				d1 = sqrt(tem1 + tem2); //得到1,3的距离

				tem1 = abs(pos_temp[1][0] - pos_temp[3][0]); //x2-x4
				tem2 = abs(pos_temp[1][1] - pos_temp[3][1]); //y2-y4
				tem1 *= tem1;
				tem2 *= tem2;
				d2 = sqrt(tem1 + tem2); //得到2,4的距离
				fac = (float)d1 / d2;
				if (fac < 0.95 || fac > 1.05) //不合格
				{
					cnt = 0;
					TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);                                                                                           //清除点4
					TP_Drow_Touch_Point(20, 20, RED);                                                                                                                            //画点1
					TP_Adj_Info_Show(pos_temp[0][0], pos_temp[0][1], pos_temp[1][0], pos_temp[1][1], pos_temp[2][0], pos_temp[2][1], pos_temp[3][0], pos_temp[3][1], fac * 100); //显示数据
					continue;
				} //正确了

				//对角线相等
				tem1 = abs(pos_temp[1][0] - pos_temp[2][0]); //x1-x3
				tem2 = abs(pos_temp[1][1] - pos_temp[2][1]); //y1-y3
				tem1 *= tem1;
				tem2 *= tem2;
				d1 = sqrt(tem1 + tem2); //得到1,4的距离

				tem1 = abs(pos_temp[0][0] - pos_temp[3][0]); //x2-x4
				tem2 = abs(pos_temp[0][1] - pos_temp[3][1]); //y2-y4
				tem1 *= tem1;
				tem2 *= tem2;
				d2 = sqrt(tem1 + tem2); //得到2,3的距离
				fac = (float)d1 / d2;
				if (fac < 0.95 || fac > 1.05) //不合格
				{
					cnt = 0;
					TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);                                                                                           //清除点4
					TP_Drow_Touch_Point(20, 20, RED);                                                                                                                            //画点1
					TP_Adj_Info_Show(pos_temp[0][0], pos_temp[0][1], pos_temp[1][0], pos_temp[1][1], pos_temp[2][0], pos_temp[2][1], pos_temp[3][0], pos_temp[3][1], fac * 100); //显示数据
					continue;
				} //正确了
				//计算结果
				tp_dev.xfac = (float)(lcddev.width - 40) / (pos_temp[1][0] - pos_temp[0][0]);       //得到xfac
				tp_dev.xoff = (lcddev.width - tp_dev.xfac * (pos_temp[1][0] + pos_temp[0][0])) / 2; //得到xoff

				tp_dev.yfac = (float)(lcddev.height - 40) / (pos_temp[2][1] - pos_temp[0][1]);       //得到yfac
				tp_dev.yoff = (lcddev.height - tp_dev.yfac * (pos_temp[2][1] + pos_temp[0][1])) / 2; //得到yoff
				if (float_abs(tp_dev.xfac) > 2 || float_abs(tp_dev.yfac) > 2)                                    //触屏和预设的相反了.
				{
					cnt = 0;
					TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE); //清除点4
					TP_Drow_Touch_Point(20, 20, RED);                                  //画点1
					LCD_ShowString(40, 26, lcddev.width, lcddev.height, 16, "TP Need readjust!");
					tp_dev.touchtype = !tp_dev.touchtype; //修改触屏类型.
					if (tp_dev.touchtype)                 //X,Y方向与屏幕相反
					{
						CMD_RDX = 0X90;
						CMD_RDY = 0XD0;
					}
					else //X,Y方向与屏幕相同
					{
						CMD_RDX = 0XD0;
						CMD_RDY = 0X90;
					}
					continue;
				}
				POINT_COLOR = BLUE;
				LCD_Clear(WHITE);                                                                    //清屏
				LCD_ShowString(35, 110, lcddev.width, lcddev.height, 16, "Touch Screen Adjust OK!"); //校正完成
				delay_ms(1000);
				TP_Save_Adjdata();
				LCD_Clear(WHITE); //清屏
				return;           //校正完成
			}
		}
		delay_ms(10);
		outtime++;
		if (outtime > 1000)
		{
			TP_Get_Adjdata();
			break;
		}
	}
}
//触摸屏初始化
//返回值:0,没有进行校准
//       1,进行过校准
uint8_t TOUCH_Init(void)
{
	if (TP_Get_Adjdata())
		return 0; //已经校准
	else        //未校准
	{
		LCD_Clear(WHITE); //清屏
		TP_Adjust();      //屏幕校准
		TP_Save_Adjdata();
	}
	TP_Get_Adjdata();

	return 1;
}

//清空屏幕并在右上角显示"RST"
void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);                                         //清屏
	POINT_COLOR = RED;                                        //设置画笔蓝色
}

void Screen_Draw_Line(void)
{
	TOUCH_Scan(0);
	if (tp_dev.sta & TP_PRES_DOWN) //触摸屏被按下
	{
		if (tp_dev.x[0] < lcddev.width && tp_dev.y[0] < lcddev.height)
		{
			if (tp_dev.x[0] > (lcddev.width - 24) && tp_dev.y[0] < 16)
				Load_Drow_Dialog(); //清除
			else
				TP_Draw_Big_Point(tp_dev.x[0], tp_dev.y[0], RED); //画图
		}
	}
	else
		delay_ms(10);       //没有按键按下的时候
}

//设计触屏反馈
uint8_t LCD_Feedback(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, float* result, uint8_t size)
{
	static uint8_t touch_up = 1;
	static uint8_t i = 0;              //i记录已输入元素个数
	uint8_t j;
	static uint8_t str[20] = { '\0' }; //字符串数组

	uint16_t x = x2 - x1; //矩形的宽和长
	uint16_t y = y2 - y1;
	TOUCH_Scan(0);
	/*触屏反馈*/
	if (touch_up && (tp_dev.x[0] != 0xFFFF))
	{
		delay_ms(10);
		touch_up = 0;

		if (tp_dev.x[0] > (lcddev.width - 24) && tp_dev.y[0] < 20)
		{
			LCD_Clear(WHITE);
			LCD_Draw_Keyboard(x1, y1, x2, y2, size);
			i = 0;
		}
		//判断键值
		if (tp_dev.x[0] > x1 && tp_dev.x[0] < x2 && tp_dev.y[0] < y2 && tp_dev.y[0] > y1)
		{
			i++;
			//触碰1
			if (tp_dev.x[0] > x1 && tp_dev.x[0] < (x1 + x / 3) && tp_dev.y[0] > y1 && tp_dev.y[0] < (y1 + y / 5))
			{
				str[i - 1] = '1';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '1', size, 0);
			}
			//触碰2
			else if (tp_dev.x[0] > (x1 + x / 3) && tp_dev.x[0] < (x1 + 2 * x / 3) && tp_dev.y[0] > y1 && tp_dev.y[0] < (y1 + y / 5))
			{
				str[i - 1] = '2';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '2', size, 0);
			}
			//触碰3
			else if (tp_dev.x[0] > (x1 + 2 * x / 3) && tp_dev.x[0] < (x1 + x) && tp_dev.y[0] > y1 && tp_dev.y[0] < (y1 + y / 5))
			{
				str[i - 1] = '3';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '3', size, 0);
			}
			//触碰4
			else if (tp_dev.x[0] > (x1) && tp_dev.x[0] < (x1 + x / 3) && tp_dev.y[0] > (y1 + y / 5) && tp_dev.y[0] < (y1 + 2 * y / 5))
			{
				str[i - 1] = '4';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '4', size, 0);
			}
			//触碰5
			else if (tp_dev.x[0] > (x1 + x / 3) && tp_dev.x[0] < (x1 + 2 * x / 3) && tp_dev.y[0] > (y1 + y / 5) && tp_dev.y[0] < (y1 + 2 * y / 5))
			{
				str[i - 1] = '5';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '5', size, 0);
			}
			//触碰6
			else if (tp_dev.x[0] > (x1 + 2 * x / 3) && tp_dev.x[0] < (x1 + x) && tp_dev.y[0] > (y1 + y / 5) && tp_dev.y[0] < (y1 + 2 * y / 5))
			{
				str[i - 1] = '6';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '6', size, 0);
			}
			//触碰7
			else if (tp_dev.x[0] > (x1) && tp_dev.x[0] < (x1 + x / 3) && tp_dev.y[0] > (y1 + 2 * y / 5) && tp_dev.y[0] < (y1 + 3 * y / 5))
			{
				str[i - 1] = '7';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '7', size, 0);
			}
			//触碰8
			else if (tp_dev.x[0] > (x1 + x / 3) && tp_dev.x[0] < (x1 + 2 * x / 3) && tp_dev.y[0] > (y1 + 2 * y / 5) && tp_dev.y[0] < (y1 + 3 * y / 5))
			{
				str[i - 1] = '8';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '8', size, 0);
			}
			//触碰9
			else if (tp_dev.x[0] > (x1 + 2 * x / 3) && tp_dev.x[0] < (x1 + x) && tp_dev.y[0] > (y1 + 2 * y / 5) && tp_dev.y[0] < (y1 + 3 * y / 5))
			{
				str[i - 1] = '9';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '9', size, 0);
			}
			//触碰.
			else if (tp_dev.x[0] > (x1) && tp_dev.x[0] < (x1 + x / 3) && tp_dev.y[0] > (y1 + 3 * y / 5) && tp_dev.y[0] < (y1 + 4 * y / 5))
			{
				str[i - 1] = '.';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '.', size, 0);
			}
			//触碰0
			else if (tp_dev.x[0] > (x1 + x / 3) && tp_dev.x[0] < (x1 + 2 * x / 3) && tp_dev.y[0] > (y1 + 3 * y / 5) && tp_dev.y[0] < (y1 + 4 * y / 5))
			{
				str[i - 1] = '0';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '0', size, 0);
			}
			//触碰back
			else if (tp_dev.x[0] > (x1 + 2 * x / 3) && tp_dev.x[0] < (x1 + x) && tp_dev.y[0] > (y1 + 3 * y / 5) && tp_dev.y[0] < (y1 + 4 * y / 5))
			{
				LCD_ShowChar(x1 + 5 + size * (i - 1) / 2 + x / 3, 9 * y / 10 - size / 2 + y1, ' ', size, 0);
				if (i >= 2)
					i -= 2;
				else
					i -= 1;
				str[i] = '\0';
			}
			//触碰-		
			else if (tp_dev.x[0] > (x1) && tp_dev.x[0]<(x1 + x / 3) && tp_dev.y[0]>(y1 + 4 * y / 5) && tp_dev.y[0] < (y1 + 5 * y / 5))
			{
				str[i - 1] = '-';
				LCD_ShowChar(x1 + 5 + size * i / 2 + x / 3, 9 * y / 10 - size / 2 + y1, '-', size, 0);

			}
			//触碰enter
			//按下enter后函数返回输入的浮点数，显示浮点数并清除屏幕数据
			else if (tp_dev.x[0] > (x1 + x / 3) && tp_dev.x[0] < (x1 + x) && tp_dev.y[0] > (y1 + 4 * y / 5) && tp_dev.y[0] < (y1 + 5 * y / 5))
			{
				*result = atof(str);
				for (j = 1; j < i + 1; j++)
				{
					LCD_ShowChar(x1 + 5 + size * (j - 1) / 2 + x / 3, 9 * y / 10 - size / 2 + y1, ' ', size, 0);
				}
				for (j = 0; j < 20; j++)
					str[j] = '\0';
				//状态判断变量复位
				i = 0;
				return 1;
			}

		}
	}
	else if (tp_dev.x[0] == 0xFFFF)touch_up = 1;

	return 0;
}

uint8_t IC_Feedback(uint8_t num)
{
	static uint8_t cs = -1;
	TOUCH_Scan(0);
	for (uint8_t i = 0; i < num; i++)
		if (tp_dev.x[0] > IcArr[i][0] && tp_dev.x[0] < IcArr[i][2] && tp_dev.y[0] > IcArr[i][1] && tp_dev.y[0] < IcArr[i][3] && cs != i)
			cs = i;
	return cs;
}

//绘制片选界面
/*
返回字符串长度
*/

uint16_t LCD_ICRec(uint16_t x, uint16_t y, uint16_t xmax, uint8_t str[][20], uint8_t num, uint8_t size)
{
	uint8_t* p;
	uint8_t count = 0;
	uint16_t len;
	p = str[num];
	while (*p != '\0')
	{
		count++;
		p++;
	}
	len = size * count / 2;
	if (x + len + size + 5 > xmax)
		return 0xFFFF;
	LCD_DrawRectangle(x, y, x + len + size + 5, y + 2 * size + 5);
	IcArr[num][0] = x;
	IcArr[num][1] = y;
	IcArr[num][2] = x + len + size + 5;
	IcArr[num][3] = y + 2 * size + 5;
	LCD_ShowString(x + (size + 5) / 2, y + (size + 5) / 2, len, size, size, str[num]);
	return (x + len + size + 5);
}
/*
入口参数:
x,y:LCD坐标
str:显示的字符串
len:字符串数量
*/
void LCD_ICSel(uint16_t x, uint16_t y, uint16_t xmax, uint8_t str[][20], uint8_t len, uint8_t cs, uint8_t size)
{
	uint8_t icnum = 0;
	uint16_t x0 = x;
	for (uint8_t i = 0; i < len; i++)
	{
		if (cs == i)
			POINT_COLOR = RED;
		x = LCD_ICRec(x, y, xmax, str, i, size);
		if (x == 0xFFFF) {
			y += 2 * size + 5;
			x = x0;
			x = LCD_ICRec(x, y, xmax, str, i, size);
		}
		if (cs == i)
			POINT_COLOR = BLACK;
	}
}
//绘制图形界面
void LCD_Draw_Keyboard(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size)
{
	uint8_t buff[] = { '1','2','3','4','5','6','7','8','9','.','0',' ','-',' ',' ' };
	POINT_COLOR = BLACK;
	uint16_t x = x2 - x1;
	uint16_t y = y2 - y1;

	LCD_DrawRectangle(x1, y1, x2, y2);
	for (uint8_t i = 1; i < 5; i++)
		LCD_DrawLine(x1, i * y / 5 + y1, x2, i * y / 5 + y1);
	//纵向长线
	for (uint8_t i = 1; i < 2; i++)
		LCD_DrawLine(i * x / 3 + x1, y1, i * x / 3 + x1, y + y1);
	//纵向短线
	for (uint8_t i = 2; i < 3; i++)
		LCD_DrawLine(i * x / 3 + x1, y1, i * x / 3 + x1, 4 * y / 5 + y1);

	uint8_t k = 0;
	for (uint8_t i = 0; i < 5; i++)
	{
		for (uint8_t j = 0; j < 3; j++)
		{
    		LCD_ShowChar(j * x / 3 + x / 6 - size / 4 + x1, i * y / 5 + y / 10 - size / 2 + y1, buff[k], size, 0);
			k++;
		}
	}
    LCD_ShowString(5 * x / 6 - size + x1, 7 * y / 10 - size / 2 + y1, 2 * size, size, size, "back");
}

void LCD_ShowFloatnum(uint16_t x, uint16_t y, float num, uint8_t size)
{
	char buff[20];
	sprintf(buff, "%.5f", num);
	LCD_ShowString(x, y, 200, size, size, buff);
}
//设计触屏全屏反馈
uint8_t LCD_Feedback_Whole(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, int* temp, uint8_t size)
{
    TOUCH_Scan(0);
    /*触屏反馈*/
    //双按键
    if (tp_dev.x[0] > x1 && tp_dev.x[0] < x2 && tp_dev.y[0] > y1 && tp_dev.y[0] < y2)
    {
        (*temp)++; 
        tp_dev.x[0] = 0xFFFF;
    }
    if (tp_dev.x[0] > (x1+75) && tp_dev.x[0] < (x2+75) && tp_dev.y[0] > y1 && tp_dev.y[0] < y2)
    {
        (*temp)--;
        tp_dev.x[0] = 0xFFFF;
    }
    return 0;
}
