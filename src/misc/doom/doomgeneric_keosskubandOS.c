#include "doomkeys.h"
#include "doomgeneric.h"

#define KEYQUEUE_SIZE 16
static uint64_t doom_start_time;
static int framebuffer_fd, async_serial_fd;
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static unsigned char convertToDoomKey(unsigned int key) {
    switch (key) {
        case '\n':
            key = KEY_ENTER;
            break;
        case 127: // Backspace
            key = KEY_ESCAPE;
            break;
        case 'a':
        case 'A':
            key = KEY_LEFTARROW;
            break;
        case 'd':
        case 'D':
            key = KEY_RIGHTARROW;
            break;
        case 'w':
        case 'W':
            key = KEY_UPARROW;
            break;
        case 's':
        case 'S':
            key = KEY_DOWNARROW;
            break;
        case 'l':
        case 'L':
            key = KEY_FIRE;
            break;
        case ' ':
            key = KEY_USE;
            break;
        case 'j':
        case 'J':
            key = KEY_RSHIFT;
            break;
        default:
            break;
    }

    return key;
}

static void addKeyToQueue(int pressed, unsigned int keyCode) {
    unsigned char key = convertToDoomKey(keyCode);

    unsigned short keyData = (pressed << 8) | key;

    s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
    s_KeyQueueWriteIndex++;
    s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

void DG_Init() {
    
}

void DG_DrawFrame() {
    
}

void DG_SleepMs(uint32_t ms) {
}

uint32_t DG_GetTicksMs() {
    
}

int DG_GetKey(int *pressed, unsigned char *doomKey) {
    if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
        //key queue is empty

        return 0;
    } else {
        unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
        s_KeyQueueReadIndex++;
        s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

        if ((keyData >> 8) == 2) {
            return 0;
        }

        *pressed = keyData >> 8;
        *doomKey = keyData & 0xFF;

        return 1;
    }
}

void DG_SetWindowTitle(const char *title) {
}

int main(int argc, char **argv) {
    doom_start_time = time();
    doomgeneric_Create(argc, argv);
    for (int i = 0; ; i++)
        doomgeneric_Tick();

    return 0;
}