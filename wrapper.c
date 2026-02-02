#include "wrapper.h"
#include "math.h"
#include "./libs/ccleste/celeste.h"
#include "./libs/ccleste/tilemap.h"
#include <stdarg.h>
#include <stdlib.h>
#include "bitmap_data.h"

#define check_c(c) if(c < 0 || c >= 16) return

unsigned char canvas[128][128];
unsigned char palette_def[16];
int camera_x;
int camera_y;

inline void pt_rect_fill(int x, int y, int x2, int y2, int c) {
    check_c(c);
    for (int i = x; i <= x2; i++) {
        if (i < 0 || i >= 128) continue;
        for (int j = y; j <= y2; j++) {
            if (j < 0 || j >= 128) continue;
            canvas[j][i] = palette_def[c];
        }
    }
}

static inline unsigned char sample_bitmap_gfx(int x, int y) {
    if (x < 0 || x >= DATA_BITMAP_GFX_WIDTH || y < 0 || y >= DATA_BITMAP_GFX_HEIGHT) return 0;
    return DATA_BITMAP_GFX[y][x];
}

static inline unsigned char sample_bitmap_font(int x, int y) {
    if (x < 0 || x >= DATA_BITMAP_FONT_WIDTH || y < 0 || y >= DATA_BITMAP_FONT_HEIGHT) return 0;
    return DATA_BITMAP_FONT[y][x];
}

static inline void pt_spr(int sprite, int x, int y, int cols, int rows, int flipx, int flipy) {
    if (cols != 1 || rows != 1) return;
    if (sprite < 0) return;
    int sprite_x = (sprite % 16) * 8;
    int sprite_y = (sprite / 16) * 8;

    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            int dx = x + i;
            int dy = y + j;
            if (dx < 0 || dx >= 128 || dy < 0 || dy >= 128) continue;
            int sx = flipx ? (7 - i) : i;
            int sy = flipy ? (7 - j) : j;
            unsigned char pixel = sample_bitmap_gfx(sprite_x + sx, sprite_y + sy);
            if (pixel > 0 && pixel < 16) canvas[dy][dx] = palette_def[pixel];
        }
    }
}

inline void pt_pal(int a, int b) {
    check_c(a);
    check_c(b);

    palette_def[a] = b;
}

inline void pt_pal_reset() {
    for (int i = 0; i < 16; i++) {
        palette_def[i] = i;
    }
}

inline void pt_line(int x, int y, int x2, int y2, int c) {
    check_c(c);

    int dx = abs(x - x2);
    int dy = abs(y - y2);
    int sx = x < x2 ? 1 : -1;
    int sy = y < y2 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        if (x >= 0 & x < 128 && y >= 0 && y < 128) {
            canvas[y][x] = palette_def[c];
        }
        if (x == x2 && y == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

inline void pt_circfill(int x, int y, int r, int c) {
    check_c(c);

    if (r <= 1) {
        pt_rect_fill(x - 1, y, x + 1, y, c);
        pt_rect_fill(x, y - 1, x, y + 1, c);
    } else if (r <= 2) {
        pt_rect_fill(x - 2, y - 1, x + 2, y + 1, c);
        pt_rect_fill(x - 1, y - 2, x + 1, y + 2, c);
    } else if (r <= 3) {
        pt_rect_fill(x - 3, y - 1, x + 3, y + 1, c);
        pt_rect_fill(x - 1, y - 3, x + 1, y + 3, c);
        pt_rect_fill(x - 2, y - 2, x + 2, y + 2, c);
    } else {
        int f = 1 - r;
        int ddFx = 1;
        int ddFy = -2 * r;
        int ox = 0;
        int oy = r;

        // 绘制初始直径
        pt_line(x, y - oy, x, y + r, c);
        pt_line(x + r, y, x - r, y, c);

        while (ox < oy) {
            if (f >= 0) {
                oy--;
                ddFy += 2;
                f += ddFy;
            }
            ox++;
            ddFx += 2;
            f += ddFx;

            // 使用水平线填充
            pt_line(x + ox, y + oy, x - ox, y + oy, c);
            pt_line(x + ox, y - oy, x - ox, y - oy, c);
            pt_line(x + oy, y + ox, x - oy, y + ox, c);
            pt_line(x + oy, y - ox, x - oy, y - ox, c);
        }
    }
}

static inline void pt_print(const char *str, const int x, const int y, const int c) {
    check_c(c);

    const char *_str = str;
    int _x = x;

    for (char ch = *_str; ch; ch = *(++_str)) {
        ch &= 0x7F;
        int rx = 8 * (ch % 16);
        int ry = 8 * (ch / 16);

        for (int j = 0; j < 8; j++) {
            for (int i = 0; i < 8; i++) {
                int dx = _x + i;
                int dy = y + j;
                if (dx < 0 || dx >= 128 || dy < 0 || dy >= 128) continue;
                unsigned char pixel = sample_bitmap_font(rx + i, ry + j);
                if (pixel > 0) canvas[dy][dx] = *(palette_def + c);
            }
        }

        _x += 4;
    }
}

static inline int gettileflag(int tile, int flag) {
	return tile < sizeof(tile_flags)/sizeof(*tile_flags) && (tile_flags[tile] & (1 << flag)) != 0;
}

static inline void pt_map(int mx, int my, int tx, int ty, int mw, int mh, int mask) {
    for (int y = 0; y < mh; y++) {
        for (int x = 0; x < mw; x++) {
            int map_x = mx + x;
            int map_y = my + y;
            if (map_x < 0 || map_x >= 128 || map_y < 0 || map_y >= 128) {
                continue;
            }
            int tile = tilemap_data[map_x + map_y * 128];

            if (mask != 0) {
                if (mask == 4) {
                    if (tile >= (int)(sizeof(tile_flags)/sizeof(*tile_flags)) || tile_flags[tile] != 4) {
                        continue;
                    }
                } else {
                    if (!gettileflag(tile, mask - 1)) {
                        continue;
                    }
                }
            }

            int screen_x = tx + x * 8;
            int screen_y = ty + y * 8;

            int sprite_x = (tile % 16) * 8;
            int sprite_y = (tile / 16) * 8;

            for (int dy = 0; dy < 8; dy++) {
                for (int dx = 0; dx < 8; dx++) {
                    int dst_x = screen_x + dx;
                    int dst_y = screen_y + dy;

                    if (dst_x < 0 || dst_x >= 128 || dst_y < 0 || dst_y >= 128) continue;

                    unsigned char pixel = sample_bitmap_gfx(sprite_x + dx, sprite_y + dy);
                    if (pixel != 0) {
                        canvas[dst_y][dst_x] = palette_def[pixel];
                    }
                }
            }
        }
    }
}

int wrapped_callback(CELESTE_P8_CALLBACK_TYPE calltype, ...) {
    va_list args;
    va_start(args, calltype);

    int result = 0;

    switch (calltype) {
        case CELESTE_P8_MUSIC: {
            va_arg(args, int);
            va_arg(args, int);
            va_arg(args, int);
            break;
        }
        case CELESTE_P8_SPR: {
            int sprite = va_arg(args, int);
            int x = va_arg(args, int);
            int y = va_arg(args, int);
            int cols = va_arg(args, int);
            int rows = va_arg(args, int);
            int flipx = va_arg(args, int);
            int flipy = va_arg(args, int);
            pt_spr(sprite, x, y, cols, rows, flipx, flipy);
            break;
        }
        case CELESTE_P8_BTN: {
            int b = va_arg(args, int);
            result = go_btn(b);
            break;
        }
        case CELESTE_P8_SFX: {
            va_arg(args, int);
            break;
        }
        case CELESTE_P8_PAL: {
            int a = va_arg(args, int);
            int b = va_arg(args, int);
            pt_pal(a, b);
            break;
        }
        case CELESTE_P8_PAL_RESET: {
            pt_pal_reset();
            break;
        }
        case CELESTE_P8_CIRCFILL: {
            int x = va_arg(args, int);
            int y = va_arg(args, int);
            int r = va_arg(args, int);
            int c = va_arg(args, int);
            pt_circfill(x, y, r, c);
            break;
        }
        case CELESTE_P8_PRINT: {
            const char* str = va_arg(args, const char*);
            int x = va_arg(args, int);
            int y = va_arg(args, int);
            int c = va_arg(args, int);
            pt_print((char *)str, x, y, c);
            break;
        }
        case CELESTE_P8_RECTFILL: {
            int x = va_arg(args, int);
            int y = va_arg(args, int);
            int x2 = va_arg(args, int);
            int y2 = va_arg(args, int);
            int c = va_arg(args, int);
            pt_rect_fill(x, y, x2, y2, c);
            break;
        }
        case CELESTE_P8_LINE: {
            int x = va_arg(args, int);
            int y = va_arg(args, int);
            int x2 = va_arg(args, int);
            int y2 = va_arg(args, int);
            int c = va_arg(args, int);
            pt_line(x, y, x2, y2, c);
            break;
        }
        case CELESTE_P8_MGET: {
            int x = va_arg(args, int);
            int y = va_arg(args, int);
            result = tilemap_data[x + y * 128];
            break;
        }
        case CELESTE_P8_CAMERA: {
            camera_x = va_arg(args, int);
            camera_y = va_arg(args, int);
            break;
        }
        case CELESTE_P8_FGET: {
            int t = va_arg(args, int);
            int f = va_arg(args, int);
            result = gettileflag(t, f);
            break;
        }
        case CELESTE_P8_MAP: {
            int mx = va_arg(args, int);
            int my = va_arg(args, int);
            int tx = va_arg(args, int);
            int ty = va_arg(args, int);
            int mw = va_arg(args, int);
            int mh = va_arg(args, int);
            int mask = va_arg(args, int);
            pt_map(mx, my, tx, ty, mw, mh, mask);
            break;
        }
    }

    va_end(args);
    return result;
}

void wrapper_init() {
    Celeste_P8_set_call_func(wrapped_callback);

    camera_x = 0;
    camera_y = 0;
    pt_pal_reset();
    pt_rect_fill(0, 0, 127, 127, 0);
}

void wrapper_get_canvas(unsigned char *out_canvas) {
    for (int i = 0; i < 128; i++) {
        int ii = (i + camera_x) & 0x7F;
        for (int j = 0; j < 128; j++) {
            int jj = (j + camera_y) & 0x7F;
            out_canvas[jj * 128 + ii] = canvas[j][i];
        }
    }
}
