package main

/*
#cgo CFLAGS: -I${SRCDIR}/libs/ccleste
#cgo LDFLAGS: -L./build -l:celeste.o
#include "./wrapper.h"
#include <stdlib.h>
*/
import "C"
import (
	"image"
	"image/color"
	"image/gif"
	"os"
)

var pico8Palette = color.Palette{
    color.RGBA{0x00, 0x00, 0x00, 0xff},
    color.RGBA{0x1d, 0x2b, 0x53, 0xff},
    color.RGBA{0x7e, 0x25, 0x53, 0xff},
    color.RGBA{0x00, 0x87, 0x51, 0xff},
    color.RGBA{0xab, 0x52, 0x36, 0xff},
    color.RGBA{0x5f, 0x57, 0x4f, 0xff},
    color.RGBA{0xc2, 0xc3, 0xc7, 0xff},
    color.RGBA{0xff, 0xf1, 0xe8, 0xff},
    color.RGBA{0xff, 0x00, 0x4d, 0xff},
    color.RGBA{0xff, 0xa3, 0x00, 0xff},
    color.RGBA{0xff, 0xec, 0x27, 0xff},
    color.RGBA{0x00, 0xe4, 0x36, 0xff},
    color.RGBA{0x29, 0xad, 0xff, 0xff},
    color.RGBA{0x83, 0x76, 0x9c, 0xff},
    color.RGBA{0xff, 0x77, 0xa8, 0xff},
    color.RGBA{0xff, 0xcc, 0xaa, 0xff},
}

//export go_btn
func go_btn(b C.int) C.int {
    if b == 4 || b == 5 || b == 1 || b == 2 {
    	return 1
    }
    return 0
}

func draw() []byte {
	C.Celeste_P8_draw()

	canvas := make([]byte, 128 * 128)
	C.wrapper_get_canvas((*C.uchar)(&canvas[0]))

	return canvas
}

func createFrame(canvas []byte) *image.Paletted {
	rect := image.Rect(0, 0, 128, 128)
	img := image.NewPaletted(rect, pico8Palette)
	copy(img.Pix, canvas)
	return img
}

func main() {
	C.wrapper_init()
	C.Celeste_P8_set_rndseed(0)
	C.Celeste_P8_init()

	outGif := &gif.GIF {
		LoopCount: 0,
	}

	for i := 0; i < 180; i++ {
		C.Celeste_P8_update()
		pixelData := draw()
		frame := createFrame(pixelData)
		outGif.Image = append(outGif.Image, frame)
		outGif.Delay = append(outGif.Delay, 3)
	}

	f, _ := os.Create("replay.gif")
	defer f.Close()
	gif.EncodeAll(f, outGif)
}
