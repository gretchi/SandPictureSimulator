#include "GameMain.h"
#include <stdio.h>

GameMain::GameMain() {
}

GameMain::~GameMain() {
}

	
void GameMain::Load() {
}


int GameMain::Draw() {
	return 0;
}


int GameMain::Main() {
	char title_buf[MAX_STRING];

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
