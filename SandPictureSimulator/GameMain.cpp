#include "GameMain.h"
#include <stdio.h>
#include <omp.h>
#include <immintrin.h>
#include <cstring>
#include <cstdint>
#include <cmath>

// ============================================================
// Globals
// ============================================================

static constexpr int MT_MAX_THREADS = 16;

int px_table[SCREEN_WIDTH][SCREEN_HEIGHT];

alignas(64) double fluid_moov_n[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];
double fluid_moov_x[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];
double fluid_moov_y[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];
alignas(64) double fluid_murly_n[MAX_PRESET][WIDTH_GRID_NUM][HEIGHT_GRID_NUM];

// Double buffer: previous frame's fluid density (read-only during parallel particle update)
alignas(64) static double prev_fluid_moov_n[WIDTH_GRID_NUM][HEIGHT_GRID_NUM];

// Per-thread local fluid accumulators (zeroed each frame, then AVX2-reduced into globals)
alignas(64) static double tls_moov_n[MT_MAX_THREADS][WIDTH_GRID_NUM][HEIGHT_GRID_NUM];
alignas(64) static double tls_murly_n[MT_MAX_THREADS][MAX_PRESET][WIDTH_GRID_NUM][HEIGHT_GRID_NUM];

// Per-thread XorShift32 RNGs — each state is separated by 16 ints (64 bytes) to prevent false sharing
static uint32_t tls_rng_state[MT_MAX_THREADS * 16];

static inline uint32_t xorshift32(uint32_t* s) noexcept {
    *s ^= *s << 13;
    *s ^= *s >> 17;
    *s ^= *s << 5;
    return *s;
}

static inline int tls_rand(int tid, int n) noexcept {
    return (int)(xorshift32(&tls_rng_state[tid * 16]) % (uint32_t)(n + 1));
}

int murky_sc_handle[MAX_PRESET];
int heat_sc_handle;


// ============================================================
// GameMain
// ============================================================

GameMain::GameMain() {
    this->change_heat_map_key = new ToggleKey(KEY_INPUT_M);
    this->reset_key = new ToggleKey(KEY_INPUT_R);
    this->max_frame_rate_key = new ToggleKey(KEY_INPUT_X);

    for (int t = 0; t < MT_MAX_THREADS; t++) {
        uint32_t seed = (uint32_t)(19937u + (uint32_t)t * 6301u + 1u);
        tls_rng_state[t * 16] = seed;
    }
}

GameMain::~GameMain() {
    delete(this->change_heat_map_key);
    delete(this->reset_key);
    delete(this->max_frame_rate_key);
}


void GameMain::Load() {
    for (int i = 0; i < MAX_PRESET; i++) {
        murky_sc_handle[i] = MakeScreen(WIDTH_GRID_NUM, HEIGHT_GRID_NUM, 1);
    }
    heat_sc_handle = MakeScreen(WIDTH_GRID_NUM, HEIGHT_GRID_NUM, 1);

    for (int i = 0; i < SCREEN_WIDTH; i++) {
        for (int j = 0; j < SCREEN_HEIGHT; j++) {
            px_table[i][j] = 0;
        }
    }

    this->init_hue = double(GetRand(36000)) / 100.0;
    double preset_s[MAX_PRESET] = { 0.0, 0.0, 0.9, 0.0, 0.2, 0.0 };
    double preset_v[MAX_PRESET] = { 0.8, 0.2, 1.0, 0.8, 0.2, 0.2 };
    int preset_s_rate[MAX_PRESET] = { 1000, 50, 200, 300, 25, 75 };

    for (int i = 0; i < MAX_PRESET; i++) {
        this->preset[i].massa = (double(GetRand(280)) + 30.0) / 100.0;
        this->preset[i].h = this->init_hue;
        this->preset[i].s = preset_s[i];
        this->preset[i].v = preset_v[i];
        this->preset[i].shineing_rate = preset_s_rate[i];
    }

    int position_num = GetRand(MAX_POSITION - 1);
    int positions[MAX_POSITION];
    for (int i = 0; i < MAX_POSITION; i++) {
        positions[i] = GetRand(SCREEN_WIDTH - 50);
    }

    for (int i = 0; i < MAX_SAND; i++) {
        int preset_id = GetRand(MAX_PRESET - 1);
        int position_id = GetRand(position_num);
        double offset = pow(SCREEN_WIDTH / MAX_SAND, i);

        this->sand[i].o_x = this->sand[i].x;
        this->sand[i].o_y = this->sand[i].y;
        this->sand[i].massa = this->preset[preset_id].massa * (1.25 - double(GetRand(400)) / 1000.0);
        this->sand[i].pressure = 0.0;
        this->sand[i].x_speed = 0.0;
        this->sand[i].y_speed = 0.0;
        this->sand[i].h = this->preset[preset_id].h;
        this->sand[i].s = this->preset[preset_id].s;
        this->sand[i].v = this->preset[preset_id].v;
        this->sand[i].preset_id = preset_id;
        this->sand[i].shineing_rate = this->preset[preset_id].shineing_rate;
        this->sand[i].color = GetColorHSV(this->sand[i].h, this->sand[i].s, this->sand[i].v);
        this->sand[i].is_fix = false;
        this->sand[i].y = 0.0 - GetRand(150) - position_id * 30;
        this->sand[i].x = positions[position_id] + 25 + (offset * SCREEN_WIDTH / MAX_SAND) + ((this->sand[i].y / 3) * GetRand(2) - 1);
    }

    memset(prev_fluid_moov_n, 0, sizeof(prev_fluid_moov_n));
}


int GameMain::Draw() {
    int frame_count = 0;
    static int active = 0;

    const int num_threads = (omp_get_max_threads() < MT_MAX_THREADS)
                          ? omp_get_max_threads() : MT_MAX_THREADS;

    // HEIGHT_GRID_NUM rounded down to nearest multiple of 4 for AVX2 loop
    constexpr int height_vec = (HEIGHT_GRID_NUM / 4) * 4;

    // ================================================================
    // Clear per-thread fluid accumulators (parallel)
    // ================================================================
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int t = 0; t < num_threads; t++) {
        memset(tls_moov_n[t],  0, sizeof(tls_moov_n[0]));
        memset(tls_murly_n[t], 0, sizeof(tls_murly_n[0]));
    }

    // ================================================================
    // Parallel particle simulation
    //   - Reads prev_fluid_moov_n (double-buffered, written only after this block)
    //   - Writes to tls_moov_n[tid] / tls_murly_n[tid]   (thread-private, no races)
    //   - Reads px_table (read-only within this block, safe)
    //   - Writes px_table with #pragma omp atomic (rare fix events)
    // ================================================================
    #pragma omp parallel num_threads(num_threads)
    {
        const int tid = omp_get_thread_num();

        #pragma omp for schedule(static)
        for (int si = 0; si < MAX_SAND; si++) {
            double x_diff = 0.0;

            if (!this->sand[si].is_fix) {
                const int gx = int(this->sand[si].x / FLUID_GRID);
                const int gy = int(this->sand[si].y / FLUID_GRID);

                if (gx - 1 > 0 && gy - 1 > 0 && gx + 1 < WIDTH_GRID_NUM && gy + 1 < HEIGHT_GRID_NUM) {
                    if (tls_rand(tid, 1)) {
                        const double left  = prev_fluid_moov_n[gx - 1][gy];
                        const double right = prev_fluid_moov_n[gx + 1][gy];
                        if (std::abs(left - right) > 1.0) {
                            const double sign = (left - right < 0.0) ? -1.0 : 1.0;
                            x_diff = 0.012 * prev_fluid_moov_n[gx][gy] * sign;
                        }
                    } else {
                        const double r = tls_rand(tid, 350) * 0.005;
                        x_diff = tls_rand(tid, 1) ? r : -r;
                    }

                    // Average fluid density across 3 columns around this particle
                    double pressure_average = 1.0;
                    for (int k = 0; k < 3; k++) {
                        pressure_average += prev_fluid_moov_n[gx + k - 1][gy];
                    }
                    this->sand[si].pressure += pressure_average / 3.0;
                }

                double pressure = this->sand[si].pressure;
                if (pressure >= 80.0) pressure = 80.0;

                this->sand[si].x += (4.9 - this->sand[si].massa) * x_diff * (this->sand[si].y / SCREEN_HEIGHT) * (pressure * 0.035);
                this->sand[si].y += GRAVITY * this->sand[si].massa * ((pressure + 1.0) * 0.05);

                if (this->sand[si].x > 0.0 && this->sand[si].x < SCREEN_WIDTH &&
                    this->sand[si].y > 0.0 && this->sand[si].y < SCREEN_HEIGHT) {
                    const int ngx = int(this->sand[si].x / FLUID_GRID);
                    const int ngy = int(this->sand[si].y / FLUID_GRID);
                    tls_moov_n[tid][ngx][ngy] += 1.0;
                    tls_murly_n[tid][this->sand[si].preset_id][ngx][ngy] += 1.0;
                }
            }

            // Pixel table collision check
            if (this->sand[si].x > 0.0 && this->sand[si].x < SCREEN_WIDTH &&
                this->sand[si].y > 0.0 && this->sand[si].y < SCREEN_HEIGHT) {
                double pressure = this->sand[si].pressure;
                if (pressure >= 25.0) pressure = 25.0;

                const int ix = int(this->sand[si].x);
                const int iy = int(this->sand[si].y);

                if (px_table[ix][iy + 1] > FIX_BORDER) {
                    if (px_table[ix - 1][iy + 1] < FIX_BORDER) {
                        this->sand[si].x -= (2.8 - this->sand[si].massa) * (this->sand[si].y / SCREEN_HEIGHT) * ((pressure + 1.0) * 0.15);
                        this->sand[si].y -= GRAVITY * this->sand[si].massa * ((pressure + 1.0) * 0.1);
                    } else if (px_table[ix + 1][iy + 1] < FIX_BORDER) {
                        this->sand[si].x += (2.8 - this->sand[si].massa) * (this->sand[si].y / SCREEN_HEIGHT);
                        this->sand[si].y -= GRAVITY * this->sand[si].massa * ((pressure + 1.0) * 0.1) * ((pressure + 1.0) * 0.15);
                    } else {
                        this->sand[si].is_fix = true;
                        #pragma omp atomic
                        px_table[ix][iy] += 1;
                    }
                }
            }

            // Ground fix
            if (this->sand[si].y >= SCREEN_HEIGHT - 1) {
                this->sand[si].is_fix = true;
                this->sand[si].y = SCREEN_HEIGHT - 1;
                if (this->sand[si].x > 0.0 && this->sand[si].x < SCREEN_WIDTH) {
                    const int ix = int(this->sand[si].x);
                    const int iy = int(this->sand[si].y);
                    #pragma omp atomic
                    px_table[ix][iy] += 1;
                }
            }

            this->sand[si].acceleration = std::abs(this->sand[si].o_x - this->sand[si].x)
                                        + std::abs(this->sand[si].o_y - this->sand[si].y);
            this->sand[si].o_x = this->sand[si].x;
            this->sand[si].o_y = this->sand[si].y;
        }
    }

    // ================================================================
    // Reduce per-thread fluid grids with AVX2 SIMD
    // ================================================================
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int j = 0; j < WIDTH_GRID_NUM; j++) {
        // Reduce moov_n
        {
            double* __restrict dst = fluid_moov_n[j];
            for (int k = 0; k < height_vec; k += 4) {
                __m256d sum = _mm256_setzero_pd();
                for (int t = 0; t < num_threads; t++) {
                    sum = _mm256_add_pd(sum, _mm256_loadu_pd(&tls_moov_n[t][j][k]));
                }
                _mm256_storeu_pd(&dst[k], sum);
            }
            for (int k = height_vec; k < HEIGHT_GRID_NUM; k++) {
                double s = 0.0;
                for (int t = 0; t < num_threads; t++) s += tls_moov_n[t][j][k];
                dst[k] = s;
            }
        }
        // Reduce murly_n per preset
        for (int p = 0; p < MAX_PRESET; p++) {
            double* __restrict dst = fluid_murly_n[p][j];
            for (int k = 0; k < height_vec; k += 4) {
                __m256d sum = _mm256_setzero_pd();
                for (int t = 0; t < num_threads; t++) {
                    sum = _mm256_add_pd(sum, _mm256_loadu_pd(&tls_murly_n[t][p][j][k]));
                }
                _mm256_storeu_pd(&dst[k], sum);
            }
            for (int k = height_vec; k < HEIGHT_GRID_NUM; k++) {
                double s = 0.0;
                for (int t = 0; t < num_threads; t++) s += tls_murly_n[t][p][j][k];
                dst[k] = s;
            }
        }
    }

    // Snapshot current fluid density for next frame's double buffer
    memcpy(prev_fluid_moov_n, fluid_moov_n, sizeof(fluid_moov_n));

    // ================================================================
    // Update particle pressures from reduced grid (parallel)
    // ================================================================
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int si = 0; si < MAX_SAND; si++) {
        if (!this->sand[si].is_fix) {
            const int gx = int(this->sand[si].x / FLUID_GRID);
            const int gy = int(this->sand[si].y / FLUID_GRID);
            if (gx >= 0 && gx < WIDTH_GRID_NUM && gy >= 0 && gy < HEIGHT_GRID_NUM) {
                this->sand[si].pressure = fluid_moov_n[gx][gy];
            }
        }
    }

    // ================================================================
    // Color update (parallel — GetColorHSV is a pure function, thread-safe)
    // ================================================================
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int si = 0; si < MAX_SAND; si++) {
        if (this->sand[si].acceleration > 0) {
            const int tid = omp_get_thread_num();
            this->sand[si].color = GetColorHSV(this->sand[si].h, this->sand[si].s, this->sand[si].v);
            if ((tls_rand(tid, 99) + 1) % (this->sand[si].shineing_rate + 1) == 1) {
                this->sand[si].color = GetColorHSV(this->sand[si].h, 0.0f, 1.0f);
            }
        }
    }

    // ================================================================
    // Drawing (single-threaded — DxLib uses global render state)
    // ================================================================
    FillMaskScreen(0);

    for (int i = 0; i < MAX_PRESET; i++) {
        float h = this->preset[i].h;
        float s = this->preset[i].s;
        float v = this->preset[i].v;
        int col = GetColorHSV(h, s, v);

        SetDrawScreen(murky_sc_handle[i]);
        FillMaskScreen(0);
        for (int j = 0; j < WIDTH_GRID_NUM; j++) {
            for (int k = 0; k < HEIGHT_GRID_NUM; k++) {
                int hm_b_mode_alpha = (int)(fluid_murly_n[i][j][k] * 0.50f);
                if (hm_b_mode_alpha >= 100) hm_b_mode_alpha = 100;
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, hm_b_mode_alpha);
                DrawPixel(j, k, col);
            }
        }
    }

    SetDrawScreen(DX_SCREEN_BACK);
    FillMaskScreen(0);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 64);
    for (int i = 0; i < MAX_PRESET; i++) {
        DrawExtendGraph(0, 0, SCREEN_WIDTH + FLUID_GRID, SCREEN_HEIGHT + FLUID_GRID, murky_sc_handle[i], TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    for (int i = 0; i < MAX_SAND; i++) {
        DrawPixel(int(this->sand[i].x), int(this->sand[i].y), this->sand[i].color);
    }

    if (this->change_heat_map_key->GetToggle()) {
        for (int i = 0; i < WIDTH_GRID_NUM; i++) {
            for (int j = 0; j < HEIGHT_GRID_NUM; j++) {
                int hm_b_mode_alpha = 16 + (int)(fluid_moov_n[i][j] * 1.75f);
                if (hm_b_mode_alpha >= 255) hm_b_mode_alpha = 255;
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, hm_b_mode_alpha - 16);

                double h = 225.0f - fluid_moov_n[i][j] * 0.50f;
                float s = 1.0f;
                if (h < 0.0) {
                    s += (float)(h * 0.0008);
                    h = 0.0;
                    if (s < 0) s = 0;
                }
                DrawBox(i * FLUID_GRID, j * FLUID_GRID, i * FLUID_GRID + FLUID_GRID, j * FLUID_GRID + FLUID_GRID, GetColorHSV(h, s, 1.0f), true);
            }
        }
    }

    active += ADD_ACTIVE_SAND;
    if (MAX_SAND >= active) active = MAX_SAND;

    frame_count++;
    return 0;
}


int GameMain::Main() {
    char title_buf[MAX_STRING];
    char build_state[MAX_STRING];
    char frame_state[MAX_STRING];
    bool reset_seq_flg = false;
    int reset_time = 0;

    #ifdef _DEBUG
        sprintf_s(build_state, MAX_STRING, "%s", "Debug Build");
    #else
        sprintf_s(build_state, MAX_STRING, "%s", "");
    #endif

    sprintf_s(frame_state, MAX_STRING, "%s", "");

    while (true) {
        this->Load();

        while (true) {
            if (this->max_frame_rate_key->GetToggle()) {
                sprintf_s(frame_state, MAX_STRING, "%s", "V-Sync: Off");
                WaitVSync(0);
            } else {
                sprintf_s(frame_state, MAX_STRING, "%s", "V-Sync: On");
                WaitVSync(0);
            }

            SetMainWindowText(title_buf);

            this->change_heat_map_key->Refresh();
            this->reset_key->Refresh();
            this->max_frame_rate_key->Refresh();

            if (reset_seq_flg == false && this->reset_key->GetFrameOnce()) {
                reset_seq_flg = true;
                reset_time = 0;
            }

            ClearDrawScreen();

            this->Draw();

            if (reset_seq_flg) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, reset_time);
                DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x000000, true);
                reset_time += 4;
                if (reset_time > 255) {
                    reset_seq_flg = false;
                    break;
                }
            }

            ScreenFlip();

            if (ProcessMessage() == -1) {
                return 0;
            }
        }
    }
    return 0;
}
