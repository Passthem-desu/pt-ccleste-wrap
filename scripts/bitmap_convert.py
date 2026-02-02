from pathlib import Path

import cv2
import numpy as np


data_path = Path(__file__).parent.parent / "libs" / "ccleste" / "data"
bmp_font = data_path / "font.bmp"
bmp_gfx = data_path / "gfx.bmp"


img_font = cv2.imread(bmp_font.resolve())
img_gfx = cv2.imread(bmp_gfx.resolve())

assert img_font is not None
assert img_gfx is not None

bitmap_font = np.sum(np.abs(np.array([[[
    [0, 0, 0],
    [255, 255, 255],
]]]) - img_font[:, :, None, :].astype(np.int16)), axis=3)
bitmap_gfx = np.sum(np.abs(np.array([[[
    [0x00, 0x00, 0x00],
    [0x53, 0x2b, 0x1d],
    [0x53, 0x25, 0x7e],
    [0x51, 0x87, 0x00],
    [0x36, 0x52, 0xab],
    [0x4f, 0x57, 0x5f],
    [0xc7, 0xc3, 0xc2],
    [0xe8, 0xf1, 0xff],
    [0x4d, 0x00, 0xff],
    [0x00, 0xa3, 0xff],
    [0x27, 0xec, 0xff],
    [0x36, 0xe4, 0x00],
    [0xff, 0xad, 0x29],
    [0x9c, 0x76, 0x83],
    [0xa8, 0x77, 0xff],
    [0xaa, 0xcc, 0xff],
]]]) - img_gfx[:, :, None, :].astype(np.int16)), axis=3)

bitmap_font = np.argmin(bitmap_font, axis=2)
bitmap_gfx = np.argmin(bitmap_gfx, axis=2)

# Generate Header for Data
def bitmap2data(bitmap: np.ndarray) -> str:
    return "{" + ",".join((
        "{" + ",".join(map(str, line)) + "}"
        for line in bitmap
    )) + "}"

header_text = f"""#ifndef __BITMAP_DATA_H__
#define __BITMAP_DATA_H__

#include <stddef.h>

#define DATA_BITMAP_FONT_HEIGHT {bitmap_font.shape[0]}
#define DATA_BITMAP_FONT_WIDTH {bitmap_font.shape[1]}
#define DATA_BITMAP_GFX_HEIGHT {bitmap_gfx.shape[0]}
#define DATA_BITMAP_GFX_WIDTH {bitmap_gfx.shape[1]}

static unsigned char DATA_BITMAP_FONT[DATA_BITMAP_FONT_HEIGHT][DATA_BITMAP_FONT_WIDTH] = {bitmap2data(bitmap_font)};
static unsigned char DATA_BITMAP_GFX[DATA_BITMAP_GFX_HEIGHT][DATA_BITMAP_GFX_WIDTH] = {bitmap2data(bitmap_gfx)};

#endif // __BITMAP_DATA_H__"""

header_path = Path(__file__).parent.parent / "bitmap_data.h"
header_path.write_text(header_text)
