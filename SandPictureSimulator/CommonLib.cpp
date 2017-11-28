#include "CommonLib.h"


void GetData(char input[MAX_DATA_LENGTH], int *seed_id, int *entry_id, int *generation_id) {
	char *tok;
	int i = 0;
	int data_buf;

	// printf("GetData->%s\n", input);

	tok = strtok(input, ",");
	while (tok != NULL) {

		data_buf = atoi(tok);

		if (i == 0) {
			*seed_id = data_buf;
		}
		else if (i == 1) {
			*entry_id = data_buf;
		}
		else if (i == 2) {
			*generation_id = data_buf;
		}

		tok = strtok(NULL, ",");

		i++;
	}

}



double GetFPS() {
	static int last_count = GetNowCount();
	static int flame_count = 0;
	static int start_time = GetNowCount();
	static double show_fps = 0.0;
	static double average_buf[128] = {};

	double diff = GetNowCount() - last_count;
	last_count = GetNowCount();
	double fps = 60.0 * (16.666666666666 / diff);

	average_buf[flame_count] = fps;

	if (GetNowCount() - start_time > 500) {
		double max = 0.0;
		for (int i = 0; i < flame_count; i++) {
			max += average_buf[i];

		}
		show_fps = (max / flame_count);

		start_time = GetNowCount();
		flame_count = -1;

	}

	flame_count++;

	return show_fps;
}


void ShowStatusWithDirectX(int color, char *message) {
	ClearDrawScreen();
	DrawFormatString(5, 5, color, message);
	ScreenFlip();

}

/*
class StdPrint {
	void init() {
		// 標準出力
		AllocConsole();
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONIN$", "r", stdin);
	}

	void std_out() {
		printf("開始\n");
	}
}
*/