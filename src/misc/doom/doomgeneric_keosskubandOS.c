#include "doomkeys.h"
#include "doomgeneric.h"
#include "header/stdlib/sleep.h"

#define COLOR_BLACK       0x00
#define KEYQUEUE_SIZE 16
#define VGA_MEMORY ((uint8_t*)0xC00A0000) // Higher-half mapping for 0xA0000
#define VGA_WIDTH   320
#define VGA_HEIGHT  200
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

void doom_syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

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
    doom_syscall(17, COLOR_BLACK, 0, 0); // Clear screen doom_syscall
    return;
}

void DG_DrawFrame() {
    for(int i = 0 ; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        VGA_MEMORY[i] = DG_ScreenBuffer[i];
    }
    return;
}

void DG_SleepMs(uint32_t ms) {
    sleep(ms);
    return;
}

uint32_t DG_GetTicksMs() {
    uint32_t ticks;
    doom_syscall(36, (uint32_t)&ticks,0,0);
    return ticks;
}

void DG_SetWindowTitle(const char * title)
{
  return;
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

int main(int argc, char **argv) {
    doomgeneric_Create(argc, argv);
    for (int i = 0; ; i++)
        doomgeneric_Tick();

    return 0;
}