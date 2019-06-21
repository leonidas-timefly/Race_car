#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int main()
{
    //float number=20.12;
    //char temp[50] = "The number is:";
    //char char2[10];
    //sprintf_s(char2, "%6.2f", number);
    //strcat_s(temp, char2);
    //puts(temp);
    int ADC_Value[40];
    float voltage1 = 100, current1 = 100,      //输入电压、电流   PA0、PA2
        voltage2 = 0, current2 = 0;          //输出电压、电流   PA1、PA3
    float input_power, output_power, transfer_rate;    //输入功率，输出功率，转化率
    char char_voltage1[50] = "The input voltage:";
    char char_voltage2[50] = "The output voltage:";
    char char_current1[50] = "The input current:";
    char char_current2[50] = "The output current:";
    char char_input_power[50] = "The input power:";
    char char_output_power[50] = "The output power:";
    char char_transfer_rate[50] = "The transfer rate:";
    char temp_input_voltage[10];
    char temp_input_current[10];
    char temp_output_voltage[10];
    char temp_output_current[10];
    char temp_input_power[10];
    char temp_output_power[10];
    char temp_transfer_rate[10];
    float output_voltage = 2;            //PA5输出2V
    float temp;
    //HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);  //输出PWM波
    //HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&ADC_Value, 40);
    //HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
      //while (1)
      //{
      //    LCD_ShowString(5, 240, 200, 30, 16, show7);
      //}
    while (1)
    {
        /*       for (uint16_t i = 0; i < 40; ++i)
               {
                   if (i % 4 == 0)
                   {
                       voltage1 = ADC_Value[i] + voltage1;
                   }
                   else if (i % 4 == 1)
                   {
                       voltage2 = ADC_Value[i] + voltage2;
                   }
                   else if (i % 4 == 2)
                   {
                       current1 = ADC_Value[i] + current1;
                   }
                   else
                   {
                       current2 = ADC_Value[i] + current2;
                   }
               }
               voltage1 = 0.1*voltage1;
               voltage2 = 0.1*voltage2;
               current1 = 0.1*current1;
               current2 = 0.1*current2;*/
        input_power = voltage1 * current1;
        output_power = voltage2 * current2;
        transfer_rate = output_power / input_power;
        sprintf_s(temp_input_voltage, "%6.2f", voltage1);
        sprintf_s(temp_input_current, "%6.2f", current1);
        sprintf_s(temp_output_voltage, "%6.2f", voltage2);
        sprintf_s(temp_output_current, "%6.2f", current2);
        sprintf_s(temp_input_power, "%6.2f", input_power);
        sprintf_s(temp_output_power, "%6.2f", output_power);
        sprintf_s(temp_transfer_rate, "%6.2f", transfer_rate);
        strcat_s(char_voltage1, temp_input_voltage);
        strcat_s(char_current1, temp_input_current);
        strcat_s(char_voltage2, temp_output_voltage);
        strcat_s(char_current2, temp_output_current);
        strcat_s(char_input_power, temp_input_power);
        strcat_s(char_output_power, temp_output_power);
        strcat_s(char_transfer_rate, temp_transfer_rate);
        puts(char_voltage1);
        puts(char_current1);
        puts(char_voltage2);
        puts(char_current2);
        puts(char_input_power);
        puts(char_output_power);
        puts(char_transfer_rate);
        while (1);
        //LCD_ShowString(5, 60, 200, 30, 16, char_voltage1);
        //LCD_ShowString(5, 90, 200, 30, 16, char_current1);
        //LCD_ShowString(5, 120, 200, 30, 16, char_voltage2);
        //LCD_ShowString(5, 150, 200, 30, 16, char_current2);
        //LCD_ShowString(5, 180, 200, 30, 16, char_input_power);
        //LCD_ShowString(5, 210, 200, 30, 16, char_output_power);
        //LCD_ShowString(5, 240, 200, 30, 16, char_transfer_rate);
        //temp = temp * 4096 / 3.3;
        //if (current1 >= 1.5)
        //{
        //    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
        //}
        //else
        //{
        //    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, temp);
        //}
    }
    system("pause");
    return 0;

}