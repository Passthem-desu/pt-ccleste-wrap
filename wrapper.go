package main

/*
#cgo CFLAGS: -I${SRCDIR}/libs/ccleste
#cgo LDFLAGS: -L./build -l:celeste.o
#include "./wrapper.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"image"
	"image/color"
	"image/gif"
	"os"
	"unicode"

	"github.com/spf13/cobra"
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

var keyPressStatus uint8 = 0

//export go_btn
func go_btn(b C.int) C.int {
    var status = (keyPressStatus >> b) & 0x01
    return C.int(status)
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

var gifFilePath string
var actionData string
var preactData string

type Action struct {
	keys uint8
	hold uint16
}

func parseActionData(actionData string) ([]Action, error) {
	var actions []Action = []Action {}
	var currentKeys uint8 = 0
	var currentHold uint16 = 0

	push := func () {
		if currentHold == 0 { currentHold = 1 }
		actions = append(actions, Action{
			currentKeys,
			currentHold,
		})
		currentKeys = 0;
		currentHold = 0;
	}

	for _, ch := range actionData {
		switch {
		case unicode.IsSpace(ch):
			if currentKeys != 0 || currentHold != 0 { push() }
		case unicode.IsDigit(ch):
			if currentHold >= 10000 {
				return nil, fmt.Errorf("操作持续时间太长了，请试着减小操作耗时")
			}
			currentHold *= 10
			currentHold += uint16(ch - '0')
		default:
			if currentHold > 0 { push() }
			switch ch {
			case 'a':
				currentKeys |= 0x01
			case 'd':
   				currentKeys |= 0x02
       		case 'w':
       		 	currentKeys |= 0x04
            case 's':
           	    currentKeys |= 0x08
            case 'z', 'c':
            	currentKeys |= 0x10
            case 'x':
           		currentKeys |= 0x20
			}
		}
	}

	if currentHold > 0 || currentKeys > 0 {
		push()
	}

	return actions, nil
}

var rootCmd = &cobra.Command{
	Use: "ccleste-wrap",
	Args: cobra.MaximumNArgs(1),
	Short: "对 Ccleste 的简单包装",
	Run: func(cmd *cobra.Command, args []string) {
		outGif := &gif.GIF {
			LoopCount: 0,
		}

		preacts, err := parseActionData(preactData)
		if err != nil {
			fmt.Printf("解析操作时出错：%v", err)
			os.Exit(1)
		}
		actions, err := parseActionData(actionData)
		if err != nil {
			fmt.Printf("解析操作时出错：%v", err)
			os.Exit(1)
		}

		for _, action := range preacts {
			keyPressStatus = action.keys
			for range action.hold {
				C.Celeste_P8_update()
				draw()
			}
		}
		for _, action := range actions {
			keyPressStatus = action.keys
			for range action.hold {
				C.Celeste_P8_update()
				pixelData := draw()
				frame := createFrame(pixelData)
				outGif.Image = append(outGif.Image, frame)
				outGif.Delay = append(outGif.Delay, 3)
			}
		}

		f, _ := os.Create(gifFilePath)
		defer f.Close()
		gif.EncodeAll(f, outGif)
	},
}

func init() {
	C.wrapper_init()
	C.Celeste_P8_set_rndseed(0)
	C.Celeste_P8_init()

	rootCmd.PersistentFlags().StringVarP(&gifFilePath, "output", "o", "./output/replay.gif", "回放文件保存的地址")
	rootCmd.PersistentFlags().StringVarP(&actionData, "action", "a", "xc 120", "操作")
	rootCmd.PersistentFlags().StringVarP(&preactData, "preact", "p", "", "提前完成的不渲染的操作清单")
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		os.Exit(1)
	}
}
