#pragma once

#ifndef __GAME_MAIN_H__
#define __GAME_MAIN_H__
// #define __DOT_BY_DOT__


#include <stdio.h>
#include <iostream>
#include <string>

// ============================================================
// User Header
// ============================================================
#include "DataConst.h"
#include "CommonLib.h"
#include "DxLib.h"


// ============================================================
// 画面解像度定義
// ============================================================
#ifdef __DOT_BY_DOT__
// ドットバイドット
#define SCREEN_WIDTH	::GetSystemMetrics(SM_CXSCREEN)
#define SCREEN_HEIGHT	::GetSystemMetrics(SM_CYSCREEN)
#else __DOT_BY_DOT__
// 解像度定義
#define SCREEN_WIDTH	1024
#define SCREEN_HEIGHT	576
#endif __DOT_BY_DOT__


#define WINDOW_TITLE "SandPictureSimulator"

#define MAX_STRING 255

#define MAX_SAND 200000
#define MAX_PRESET 5

#define FIX_BORDER 1

#define GRAVITY 0.25

#define FLUID_GRID 12

#define WIDTH_GRID_NUM SCREEN_WIDTH / FLUID_GRID + 1
#define HEIGHT_GRID_NUM SCREEN_HEIGHT / FLUID_GRID + 1

#define ADD_ACTIVE_SAND 500

typedef struct sand {
	double x = 0.0;
	double y = 0.0;
	double o_x = 0.0;
	double o_y = 0.0;
	double massa = 1.0;
	double acceleration = 0.0;
	double pressure = 0.0;
	double x_speed = 0.0;
	double y_speed = 0.0;
	float h;
	float s;
	float v;
	int shineing_rate = 100;
	int color = 0xFFFFFF;
	bool is_fix = false;
}sand_t;

class GameMain {

public:
	GameMain();
	~GameMain();
	int Main();

private:
	void Load();
	int Draw();
	sand_t sand[MAX_SAND];
	sand_t preset[MAX_PRESET];
	ToggleKey *change_heat_map_key;
};


#endif __GAME_MAIN_H__