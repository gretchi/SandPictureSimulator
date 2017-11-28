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

// セルサイズ
#define CELL_SIZE 32
#define CELL_X_NUM 16
#define CELL_Y_NUM 16
#define DEBUG

#define LISTEN_PORT 8500

// ============================================================
// 画面解像度定義
// ============================================================
#ifdef __DOT_BY_DOT__
// ドットバイドット
#define SCREEN_WIDTH	::GetSystemMetrics(SM_CXSCREEN)
#define SCREEN_HEIGHT	::GetSystemMetrics(SM_CYSCREEN)
#else __DOT_BY_DOT__
// 解像度定義
#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	640
#endif __DOT_BY_DOT__

#define WINDOW_TITLE "3D Sound"

#define MAX_STRING 255



class GameMain {

public:
	GameMain();
	~GameMain();
	int Main();

private:
	void Load();
	int Draw();

};


#endif __GAME_MAIN_H__