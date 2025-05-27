#ifndef TORUS_H
#define TORUS_H


#include "header/usermode/commands/torus.h"
#include "header/stdlib/sleep.h"
#include "header/usermode/user-shell.h"
#include <stdint.h>
#include <stdbool.h>

int fast_floor(float x);
void render_torus_frame(float A, float B);
void torus();
float sin(float x);
float cos(float x);

#endif // TORUS_H