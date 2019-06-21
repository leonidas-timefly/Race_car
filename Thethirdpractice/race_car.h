#pragma once
#include"main.h"
#include"stdlib.h"
#include"stdio.h"
#include"xpt2046.h"
#include"ili9341.h"
#include"delay.h"
#include"time.h"
#include"tim.h"

#define line1 0
#define line2 1
#define line3 2

#define creat   1
#define nocreat 0

#define in_screen  1 
#define out_screen 0

#define car_living 1
#define car_dead 0

typedef struct{
    uint16_t left_up_x;
    int      left_up_y;
    uint16_t width;
    uint16_t height;
    int color;
    uint16_t condition;
}Moving_car;

void Draw_Still_Frame(void);
void Draw_crash_car(Moving_car* car);
void Draw_all_car(void);
void Clean_crash_car(Moving_car* car);
void Clean_all_car(void);
void Draw_score(void);
void Draw_player_car(Moving_car* car);
void Detect_all_crash(Moving_car* game_car1, Moving_car* game_car2, Moving_car* game_car3,
                      Moving_car* game_car4, Moving_car* game_car5, Moving_car* game_car6, Moving_car* play_car);
void Detect_one_crash(Moving_car* game_car, Moving_car* player_car);
void Touch_screen(Moving_car* car);
void race_car_main(void);