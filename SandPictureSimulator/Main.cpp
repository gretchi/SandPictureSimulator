#include "GameMain.h"


int end_flag = 0;
int FrameCount = 0;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	//ログ出力する
	SetOutApplicationLogValidFlag(TRUE);
#else
	//ログ出力しない
	SetOutApplicationLogValidFlag(FALSE);
#endif // _DEBUG

	// 画面モードのセット
	SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
	//ウィンドウモードで起動
	ChangeWindowMode(TRUE);
	//カーソル
	SetMouseDispFlag(TRUE);
	//タイトル
	SetMainWindowText(WINDOW_TITLE);
	//アクティブでなくても動くか
	SetAlwaysRunFlag(TRUE);

	// VSyncをまつ
	SetWaitVSyncFlag(TRUE);

	//ＤＸライブラリ初期化処理
	if (DxLib_Init() == -1) {
		return -1;
	}
	// 描画先画面を裏画面にセット
	SetDrawScreen(DX_SCREEN_BACK);
	//描画をバイリニアで
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	//	SetDrawMode(DX_DRAWMODE_NEAREST);

	
	GameMain *gm_p = new GameMain();
	
	gm_p->Main();

	delete gm_p;


	DxLib_End();

	printf("終了\n");
	return 0;
}