#ifndef WRAPPER_H
#define WRAPPER_H

#include <stddef.h>
#include "./libs/ccleste/celeste.h"

extern int go_btn(int b);

int unified_callback(CELESTE_P8_CALLBACK_TYPE calltype, ...);
void wrapper_init();
void wrapper_get_canvas(unsigned char *out_canvas);

#endif
