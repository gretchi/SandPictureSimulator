#include "GameMain.h"
#include <stdio.h>

int px_table[SCREEN_WIDTH][SCREEN_HEIGHT];


double fluid_moov_n[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];
double fluid_moov_x[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];
double fluid_moov_y[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];


GameMain::GameMain() {
	// ================================================================
	// キーインプットクラス初期化
	// ================================================================
	// ヒートマップ切り替え
	this->change_heat_map_key = new ToggleKey(KEY_INPUT_M);


}

GameMain::~GameMain() {
	// ================================================================
	// 開放
	// ================================================================
	delete(this->change_heat_map_key);
}


void GameMain::Load() {
	// ================================================================
	// ピクセルテーブル初期化
	// ================================================================
	for (int i = 0; i < SCREEN_WIDTH; i++) {
		for (int j = 0; j < SCREEN_HEIGHT; j++) {
			px_table[i][j] = 0;
		}
	}

	// ================================================================
	// プリセット初期化
	// ================================================================
	// [0] 白砂 
	// [1] 黒砂
	// [2] 色砂 1
	// [3] 色砂 2
	// [4] 金砂

	double init_hue = double(GetRand(36000)) / 100.0;
	double preset_s[MAX_PRESET] = { 0.0, 0.0, 0.9, 0.0, 0.2};
	double preset_v[MAX_PRESET] = { 0.8, 0.2, 1.0, 0.8, 0.2};
	int preset_s_rate[MAX_PRESET] = { 1000, 50, 200, 300, 25 };

	for (int i = 0; i < MAX_PRESET; i++) {
		this->preset[i].massa = (double(GetRand(280)) + 30.0) / 100.0;
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
		double offset = pow(SCREEN_WIDTH / MAX_SAND, i);

		this->sand[i].y = 0.0 - GetRand(300);
		this->sand[i].x = SCREEN_WIDTH / 2 + (offset * SCREEN_WIDTH / MAX_SAND);
		

		this->sand[i].o_x = this->sand[i].x;
		this->sand[i].o_y = this->sand[i].y;

		this->sand[i].massa = this->preset[preset_id].massa * (1.25 - double(GetRand(500))/1000.0);
		this->sand[i].pressure = 0.0;
		this->sand[i].x_speed = 0.0;
		this->sand[i].y_speed = 0.0;
		this->sand[i].h = this->preset[preset_id].h;
		this->sand[i].s = this->preset[preset_id].s;
		this->sand[i].v = this->preset[preset_id].v;
		this->sand[i].shineing_rate = this->preset[preset_id].shineing_rate;
		this->sand[i].color = GetColorHSV(this->sand[i].h, this->sand[i].s, this->sand[i].v);
		this->sand[i].is_fix = false;
	}
	
}


int GameMain::Draw() {
	// ================================================================
	// 内部変数初期化
	// ================================================================
	static int active = 0;
	
	// ================================================================
	// フィールド状態処理
	// ================================================================
	for (int i = 0; i < WIDTH_GRID_NUM; i++) {
		for (int j = 0; j < HEIGHT_GRID_NUM; j++) {
			// DrawLine(i * FLUID_GRID, 0, i * FLUID_GRID, SCREEN_HEIGHT, 0x202020);
			// DrawLine(0, j * FLUID_GRID, SCREEN_WIDTH, j * FLUID_GRID, 0x202020);
			fluid_moov_n[i][j] = 0;
		}
	}

	// ================================================================
	// 落下処理
	// ================================================================
	for (int i = 0; i < MAX_SAND; i++) {
		// 移動処理
		if(this->sand[i].is_fix == false) {
			this->sand[i].y += GRAVITY * this->sand[i].massa * ((this->sand[i].pressure+1) * 0.05);
			this->sand[i].x += (4.9 - this->sand[i].massa) * (GetRand(2) - 1) * (this->sand[i].y / SCREEN_HEIGHT) * ((this->sand[i].pressure + 1) * 0.035);

			// フィールド反映
			if (this->sand[i].x > 0 && this->sand[i].x < SCREEN_WIDTH && this->sand[i].y > 0 && this->sand[i].y < SCREEN_HEIGHT) {
				fluid_moov_n[int(this->sand[i].x / FLUID_GRID)][int(this->sand[i].y / FLUID_GRID)] += 1;
			}
		}

		// ピクセル当たり判定
		if(this->sand[i].x > 0 && this->sand[i].x < SCREEN_WIDTH && this->sand[i].y > 0 && this->sand[i].y < SCREEN_HEIGHT) {
			if (px_table[int(this->sand[i].x)][int(this->sand[i].y) + 1] > FIX_BORDER) {
				if (px_table[int(this->sand[i].x) - 1][int(this->sand[i].y) + 1] < FIX_BORDER) {
					this->sand[i].x -= (2.8 - this->sand[i].massa) * (this->sand[i].y / SCREEN_HEIGHT);
				}
				else if (px_table[int(this->sand[i].x) + 1][int(this->sand[i].y) + 1] < FIX_BORDER) {
					this->sand[i].x += (2.8 - this->sand[i].massa) * (this->sand[i].y / SCREEN_HEIGHT);
				}
				else {
					this->sand[i].is_fix = true;
					px_table[int(this->sand[i].x)][int(this->sand[i].y)] += 1;
				}
			}
		}

		// 地面当たり判定
		if (this->sand[i].y >= SCREEN_HEIGHT-1) {
			this->sand[i].is_fix = true;
			this->sand[i].y = SCREEN_HEIGHT-1;
			if (this->sand[i].x > 0 && this->sand[i].x < SCREEN_WIDTH && this->sand[i].y > 0 && this->sand[i].y < SCREEN_HEIGHT) {
				px_table[int(this->sand[i].x)][int(this->sand[i].y)] += 1;
			}
		}


		// 最終処理
		this->sand[i].acceleration = abs(this->sand[i].o_x - this->sand[i].x) + abs(this->sand[i].o_y - this->sand[i].y);
		this->sand[i].o_x = this->sand[i].x;
		this->sand[i].o_y = this->sand[i].y;
	}

	// フィールドサンド反映
	for (int i = 0; i < MAX_SAND; i++) {
		if (this->sand[i].is_fix == false) {
			this->sand[i].pressure = fluid_moov_n[int(this->sand[i].x / FLUID_GRID)][int(this->sand[i].y / FLUID_GRID)];
		}
	}


	// ================================================================
	// 色彩処理
	// ================================================================
	for (int i = 0; i < MAX_SAND; i++) {
		// 加速度変化処理
		if (this->sand[i].acceleration > 0) {
			// 基本色
			this->sand[i].color = GetColorHSV(this->sand[i].h, this->sand[i].s, this->sand[i].v);

			// キラキラ処理
			if ((GetRand(99)+1)%(this->sand[i].shineing_rate+1) == 1) {
				this->sand[i].color = GetColorHSV(this->sand[i].h, 0.0f, 1.0f);
			}
		}

	}

	// ================================================================
	// 描画
	// ================================================================
	// 砂描画
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	for (int i = 0; i < MAX_SAND; i++) {
		DrawPixel(int(this->sand[i].x), int(this->sand[i].y), this->sand[i].color);
	}

#ifdef _DEBUG
	// ヒートマップ描画
	if(this->change_heat_map_key->GetResult()) {
		for (int i = 0; i < WIDTH_GRID_NUM; i++) {
			for (int j = 0; j < HEIGHT_GRID_NUM; j++) {
				int hm_b_mode_alpha = 16 + fluid_moov_n[i][j] * 1.75f;
				if (hm_b_mode_alpha >= 255) {
					hm_b_mode_alpha = 255;
				}
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, hm_b_mode_alpha - 16);
				DrawBox(i * FLUID_GRID, j * FLUID_GRID, i * FLUID_GRID + FLUID_GRID, j * FLUID_GRID + FLUID_GRID, GetColorHSV(abs(225.0f - fluid_moov_n[i][j] * 0.50f), 1.0f, 1.0f), true);
			}
		}
	}
#endif // _DEBUG

	// ================================================================
	// アクティブ砂インクリメント
	// ================================================================
	active += ADD_ACTIVE_SAND;
	if (MAX_SAND >= active) {
		active = MAX_SAND;
	}

	return 0;
}


int GameMain::Main() {
	char title_buf[MAX_STRING];
	char build_state[MAX_STRING];

#ifdef _DEBUG
	sprintf_s(build_state, MAX_STRING, "%s", "Debug Build");
#endif // _DEBUG

	// ロード
	this->Load();

	while(true) {
		sprintf_s(title_buf, MAX_STRING, "%s [%.02fFPS] %s", WINDOW_TITLE, GetFPS(), build_state);
		SetMainWindowText(title_buf);

		// キーマップリフレッシュ
		this->change_heat_map_key->Refresh();

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
