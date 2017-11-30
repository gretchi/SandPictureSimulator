#include "GameMain.h"
#include <stdio.h>

int px_table[SCREEN_WIDTH][SCREEN_HEIGHT];


double fluid_moov_n[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];
double fluid_moov_x[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];
double fluid_moov_y[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];

int murky_sc_handle;

GameMain::GameMain() {
	// ================================================================
	// キーインプットクラス初期化
	// ================================================================
	// ヒートマップ切り替え
	this->change_heat_map_key = new ToggleKey(KEY_INPUT_M);
	
	// リセット
	this->reset_key = new ToggleKey(KEY_INPUT_R);

	// フレームレート制限解除
	this->max_frame_rate_key = new ToggleKey(KEY_INPUT_X);

}

GameMain::~GameMain() {
	// ================================================================
	// 開放
	// ================================================================
	delete(this->change_heat_map_key);
	delete(this->reset_key);
	delete(this->max_frame_rate_key);
	
}


void GameMain::Load() {
	// ================================================================
	// スクリーンハンドル作成
	// ================================================================
	// 濁りスクリーンハンドル
	murky_sc_handle = MakeScreen(WIDTH_GRID_NUM, HEIGHT_GRID_NUM, 1);

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

	this->init_hue = double(GetRand(36000)) / 100.0;
	double preset_s[MAX_PRESET] = { 0.0, 0.0, 0.9, 0.0, 0.2, 0.0};
	double preset_v[MAX_PRESET] = { 0.8, 0.2, 1.0, 0.8, 0.2, 0.2};
	int preset_s_rate[MAX_PRESET] = { 1000, 50, 200, 300, 25, 75};

	for (int i = 0; i < MAX_PRESET; i++) {
		this->preset[i].massa = (double(GetRand(280)) + 30.0) / 100.0;
		this->preset[i].h = this->init_hue;
		this->preset[i].s = preset_s[i];
		this->preset[i].v = preset_v[i];
		this->preset[i].shineing_rate = preset_s_rate[i];
	}


	// ポジショニングプリセット
	int position_num = GetRand(MAX_POSITION-1);
	int positions[MAX_POSITION];
	for (int i = 0; i < MAX_POSITION; i++) {
		positions[i] = GetRand(SCREEN_WIDTH-50);
	}

	// ================================================================
	// 砂初期化
	// ================================================================
	for (int i = 0; i < MAX_SAND; i++) {
		int preset_id = GetRand(MAX_PRESET);
		int position_id = GetRand(position_num);
		double offset = pow(SCREEN_WIDTH / MAX_SAND, i);

		this->sand[i].o_x = this->sand[i].x;
		this->sand[i].o_y = this->sand[i].y;

		this->sand[i].massa = this->preset[preset_id].massa * (1.25 - double(GetRand(400))/1000.0);
		this->sand[i].pressure = 0.0;
		this->sand[i].x_speed = 0.0;
		this->sand[i].y_speed = 0.0;
		this->sand[i].h = this->preset[preset_id].h;
		this->sand[i].s = this->preset[preset_id].s;
		this->sand[i].v = this->preset[preset_id].v;
		this->sand[i].shineing_rate = this->preset[preset_id].shineing_rate;
		this->sand[i].color = GetColorHSV(this->sand[i].h, this->sand[i].s, this->sand[i].v);
		this->sand[i].is_fix = false;

		this->sand[i].y = 0.0 - GetRand(150) - position_id * 30;
		this->sand[i].x = positions[position_id] + 25 + (offset * SCREEN_WIDTH / MAX_SAND) + ((this->sand[i].y / 3) * GetRand(2) - 1);

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
			fluid_moov_n[i][j] = 0;
		}
	}

	// ================================================================
	// 落下処理
	// ================================================================
	for (int i = 0; i < MAX_SAND; i++) {
		double x_diff = 0.0;
		double y_diff = 1.0;

		// 移動処理
		if(this->sand[i].is_fix == false) {			
			// メモリチェック
			if ((this->sand[i].x / FLUID_GRID) - 1 > 0 && (this->sand[i].y / FLUID_GRID) - 1 > 0 && (this->sand[i].x / FLUID_GRID) + 1 < WIDTH_GRID_NUM && (this->sand[i].y / FLUID_GRID) + 1 < WIDTH_GRID_NUM ) {
				// X軸処理
				if(GetRand(1)) {
					if (abs(fluid_moov_n[int(this->sand[i].x / FLUID_GRID) - 1][int(this->sand[i].y / FLUID_GRID)] - fluid_moov_n[int(this->sand[i].x / FLUID_GRID) + 1][int(this->sand[i].y / FLUID_GRID)]) > 1) {
						if (fluid_moov_n[int(this->sand[i].x / FLUID_GRID) - 1][int(this->sand[i].y / FLUID_GRID)] - fluid_moov_n[int(this->sand[i].x / FLUID_GRID) + 1][int(this->sand[i].y / FLUID_GRID)] < 0) {
							x_diff = 0.012 * fluid_moov_n[int(this->sand[i].x / FLUID_GRID)][int(this->sand[i].y / FLUID_GRID)] * -1;
						}
						else {
							x_diff = 0.012 * fluid_moov_n[int(this->sand[i].x / FLUID_GRID)][int(this->sand[i].y / FLUID_GRID)];
						}
					}
				}
				else{
					if(GetRand(1)) {
						x_diff = GetRand(350) * 0.005;
					}
					else {
						x_diff = GetRand(350) * 0.005 * -1;
					}
				}

				// Y軸処理
				double pressure_average = 1.0;
				for (int i = 0; i < 3; i++) {
					pressure_average += fluid_moov_n[int(this->sand[i].x / FLUID_GRID) + i - 1][int(this->sand[i].y / FLUID_GRID)];
				}
				this->sand[i].pressure += pressure_average / 3.0;
			}

			double pressure = this->sand[i].pressure;
			if (pressure >= 80) {
				pressure = 80;
			}


			// X軸反映
			this->sand[i].x += (4.9 - this->sand[i].massa) * x_diff * (this->sand[i].y / SCREEN_HEIGHT) * (pressure * 0.035);
			// Y軸反映
			this->sand[i].y += GRAVITY * this->sand[i].massa * ((pressure + 1) * 0.05) * y_diff;

			// フィールド反映
			if (this->sand[i].x > 0 && this->sand[i].x < SCREEN_WIDTH && this->sand[i].y > 0 && this->sand[i].y < SCREEN_HEIGHT) {
				fluid_moov_n[int(this->sand[i].x / FLUID_GRID)][int(this->sand[i].y / FLUID_GRID)] += 1;
			}
		}

		// ピクセル当たり判定
		if(this->sand[i].x > 0 && this->sand[i].x < SCREEN_WIDTH && this->sand[i].y > 0 && this->sand[i].y < SCREEN_HEIGHT) {
			double pressure = this->sand[i].pressure;
			if (pressure >= 25) {
				pressure = 25;
			}

			if (px_table[int(this->sand[i].x)][int(this->sand[i].y) + 1] > FIX_BORDER) {
				if (px_table[int(this->sand[i].x) - 1][int(this->sand[i].y) + 1] < FIX_BORDER) {
					this->sand[i].x -= (2.8 - this->sand[i].massa) * (this->sand[i].y / SCREEN_HEIGHT) * ((pressure + 1) * 0.15);
					this->sand[i].y -= GRAVITY * this->sand[i].massa * ((pressure + 1) * 0.1);
				}
				else if (px_table[int(this->sand[i].x) + 1][int(this->sand[i].y) + 1] < FIX_BORDER) {
					this->sand[i].x += (2.8 - this->sand[i].massa) * (this->sand[i].y / SCREEN_HEIGHT);
					this->sand[i].y -= GRAVITY * this->sand[i].massa * ((pressure + 1) * 0.1) * ((pressure + 1) * 0.15);
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
	// ----------------------------------------------------------------
	// 濁り表現
	// ----------------------------------------------------------------
	// 描画先を濁りスクリーンハンドルにセット
	SetDrawScreen(murky_sc_handle);
	FillMaskScreen(0);
	for (int i = 0; i < WIDTH_GRID_NUM; i++) {
		for (int j = 0; j < HEIGHT_GRID_NUM; j++) {
			int hm_b_mode_alpha = fluid_moov_n[i][j] * 0.50f;
			if (hm_b_mode_alpha >= 255) {
				hm_b_mode_alpha = 255;
			}
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, hm_b_mode_alpha);
			DrawPixel(i, j, GetColorHSV(this->init_hue, 0.3f, 0.8f));
		}
	}
	// 描画先に裏画面を再セット
	SetDrawScreen(DX_SCREEN_BACK);
	FillMaskScreen(0);
	SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
	DrawExtendGraph(0, 0, SCREEN_WIDTH + FLUID_GRID, SCREEN_HEIGHT + FLUID_GRID, murky_sc_handle, true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	// ----------------------------------------------------------------
	// 砂描画
	// ----------------------------------------------------------------
	for (int i = 0; i < MAX_SAND; i++) {
		DrawPixel(int(this->sand[i].x), int(this->sand[i].y), this->sand[i].color);
	}

// #ifdef _DEBUG
	// ----------------------------------------------------------------
	// ヒートマップ描画
	// ----------------------------------------------------------------
	if(this->change_heat_map_key->GetToggle()) {
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
// #endif // _DEBUG

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
	char frame_state[MAX_STRING];
	bool reset_seq_flg = false;
	int reset_time = 0;

	#ifdef _DEBUG
		// Debug
		sprintf_s(build_state, MAX_STRING, "%s", "Debug Build");
	#else
		// Release
		sprintf_s(build_state, MAX_STRING, "%s", "");
	#endif // _DEBUG

	sprintf_s(frame_state, MAX_STRING, "%s", "");
	
	while (true) {
		// ロード
		this->Load();

		while (true) {
			// フレームレート制限解除
			if (this->max_frame_rate_key->GetToggle()) {
				sprintf_s(frame_state, MAX_STRING, "%s", "V-Sync: Off");
				WaitVSync(0);
			}
			else {
				sprintf_s(frame_state, MAX_STRING, "%s", "V-Sync: On");
				// WaitVSync(1);
				WaitVSync(0);
			}


			sprintf_s(title_buf, MAX_STRING, "%s [%.02fFPS] %s %s", WINDOW_TITLE, GetFPS(), frame_state, build_state);
			SetMainWindowText(title_buf);

			// キーマップリフレッシュ
			this->change_heat_map_key->Refresh();
			this->reset_key->Refresh();
			this->max_frame_rate_key->Refresh();

			// リセットボタン
			if (reset_seq_flg == false && this->reset_key->GetFrameOnce()) {
				// リセットシーケンス突入
				reset_seq_flg = true;
				reset_time = 0;
			}

			// 画面初期化
			ClearDrawScreen();

			// 描画
			this->Draw();

			// リセットシーケンス処理
			if (reset_seq_flg) {
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, reset_time);
				DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x000000, true);
				reset_time += 4;
				if (reset_time > 255) {
					reset_seq_flg = false;
					break;
				}
			}

			// 裏画面描画
			ScreenFlip();

			// Windowmメッセージング処理
			if (ProcessMessage() == -1) {
				return 0;
			}
		}
	}
	return 0;
}
