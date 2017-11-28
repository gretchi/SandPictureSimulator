#include "GameMain.h"
#include <stdio.h>




GameMain::GameMain() {
}

GameMain::~GameMain() {
}


void GameMain::Load() {
	// ================================================================
	// プリセット初期化
	// ================================================================
	// [0] 白砂 
	// [1] 黒砂
	// [2] 色砂 1
	// [3] 色砂 2
	// [4] 金砂

	double init_hue = double(GetRand(36000)) / 100.0;
	double preset_s[MAX_PRESET] = { 0.0, 0.0, 0.9, 0.3, 0.8};
	double preset_v[MAX_PRESET] = { 0.8, 0.2, 1.0, 0.8, 0.2};
	int preset_s_rate[MAX_PRESET] = { 1000, 50, 200, 300, 25 };

	for (int i = 0; i < MAX_PRESET; i++) {
		this->preset[i].massa = (double(GetRand(140)) + 30.0) / 100.0;
		this->preset[i].h = init_hue;
		this->preset[i].s = preset_s[i];
		this->preset[i].v = preset_v[i];
		this->preset[i].shineing_rate = preset_s_rate[i];
	}

	// ================================================================
	// 砂初期化
	// ================================================================
	for (int i = 0; i < MAX_SAND; i++) {
		int preset_id = GetRand(MAX_PRESET);

		this->sand[i].y = 0.0 - GetRand(300);
		this->sand[i].x = GetRand(SCREEN_WIDTH);
		
		this->sand[i].massa = this->preset[preset_id].massa * (1.25 - double(GetRand(500))/1000.0);
		this->sand[i].h = this->preset[preset_id].h;
		this->sand[i].s = this->preset[preset_id].s;
		this->sand[i].v = this->preset[preset_id].v;
		this->sand[i].shineing_rate = this->preset[preset_id].shineing_rate;
		this->sand[i].color = GetColorHSV(this->sand[i].h, this->sand[i].s, this->sand[i].v);
	}
}


int GameMain::Draw() {
	// ================================================================
	// 落下処理
	// ================================================================
	for (int i = 0; i < MAX_SAND; i++) {
		this->sand[i].y += GRAVITY * this->sand[i].massa;
		this->sand[i].x += (2.8 - this->sand[i].massa) * (GetRand(2) - 1) * (this->sand[i].y / SCREEN_HEIGHT);

	}

	// ================================================================
	// 色彩処理
	// ================================================================
	for (int i = 0; i < MAX_SAND; i++) {
		// 基本色
		this->sand[i].color = GetColorHSV(this->sand[i].h, this->sand[i].s, this->sand[i].v);

		// キラキラ処理
		if ((GetRand(99)+1)%(this->sand[i].shineing_rate+1) == 1) {
			this->sand[i].color = GetColorHSV(this->sand[i].h, 0.0f, 1.0f);
		}

	}

	// ================================================================
	// 描画
	// ================================================================
	for (int i = 0; i < MAX_SAND; i++) {
		DrawPixel(int(this->sand[i].x), int(this->sand[i].y), this->sand[i].color);
	}

	return 0;
}


int GameMain::Main() {
	char title_buf[MAX_STRING];

	this->Load();

	while(true) {
		sprintf_s(title_buf, MAX_STRING, "%s  [%.02fFPS]", WINDOW_TITLE, GetFPS());
		SetMainWindowText(title_buf);

		// 画面初期化
		ClearDrawScreen();

		// 描画
		this->Draw();

		// 裏画面描画
		ScreenFlip();

		// Windowmメッセージング処理
		if (ProcessMessage() == -1) {
			return 0;
		}

	}
	return 0;
}
