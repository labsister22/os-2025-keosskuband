#ifndef APPLE_H 
#define APPLE_H
#include "header/filesys/ext2.h"
#include "header/usermode/user-shell.h"

typedef struct {
    int size;
    uint8_t font_color;
    uint8_t bg_color;
} PrintRequest;

void apple(CP* cursor);

#endif