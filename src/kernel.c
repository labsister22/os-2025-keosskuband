#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/kernel-entrypoint.h"
#include "header/cpu/gdt.h"
#include "header/graphics/graphics.h"  
#include "header/interrupt/interrupt.h"
#include "header/interrupt/idt.h"
#include "header/driver/keyboard.h"
#include "header/driver/disk.h"
#include "header/filesys/ext2.h"
#include "header/text/framebuffer.h"
#include "header/memory/paging.h"
#include "header/process/process.h"
#include "header/scheduler/scheduler.h"

void boot_animation(void) {
    int FRAME_COUNT = 44;
    int FRAME_PER_SEGMENT = 4;
    int segment_count = FRAME_COUNT / FRAME_PER_SEGMENT;
    
    for (int counter = 0; counter < 2; counter++) {
        for (int i = 0; i < segment_count; i++) {
            char ikuyokita_frames[4][200*512];
            struct EXT2DriverRequest request;
            request.buf = (uint8_t*) &ikuyokita_frames;
            request.parent_inode = 2;
            request.buffer_size = FRAME_PER_SEGMENT * 200 * 512;
            request.is_directory = 0;

            if (i == 0) {
                request.name = "ikuyokita0-3";
                request.name_len = 12;
            } else if (i == 1) {
                request.name = "ikuyokita4-7";
                request.name_len = 12;
            } else if (i == 2) {
                request.name = "ikuyokita8-11";
                request.name_len = 13;
            } else if (i == 3) {
                request.name = "ikuyokita12-15";
                request.name_len = 14;
            } else if (i == 4) {
                request.name = "ikuyokita16-19";
                request.name_len = 14;
            } else if (i == 5) {
                request.name = "ikuyokita20-23";
                request.name_len = 14;
            } else if (i == 6) {
                request.name = "ikuyokita24-27";
                request.name_len = 14;
            } else if (i == 7) {
                request.name = "ikuyokita28-31";
                request.name_len = 14;
            } else if (i == 8) {
                request.name = "ikuyokita32-35";
                request.name_len = 14;
            } else if (i == 9) {
                request.name = "ikuyokita36-39";
                request.name_len = 14;
            } else if (i == 10) {
                request.name = "ikuyokita40-43";
                request.name_len = 14;
            }

            int8_t read_result = read(request);
            if (read_result != 0) {
                continue;
            }

            for (int j = 0; j < FRAME_PER_SEGMENT; j++) {
                for (int y = 0; y < 200 && y < VGA_HEIGHT; y++) {
                    for (int x = 0; x < 320 && x < VGA_WIDTH; x++) {
                        int data_index = y * 512 + x;
                        if (data_index < 200 * 512) {
                            uint8_t pixel_color = ikuyokita_frames[j][data_index];
                            graphics_pixel(x, y, pixel_color);
                        }
                    }
                }
                
                for (volatile int delay = 0; delay < 300000; delay++);

                char skip_key = 0;
                get_keyboard_buffer(&skip_key);
                if (skip_key != 0) {
                    return;
                }
            }
        }
    }
}

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    
    initialize_filesystem_ext2();
    gdt_install_tss();
    set_tss_register();

    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

    graphics_initialize();
    
    keyboard_state_activate();
    
    boot_animation();
    
    for (int fade_step = 0; fade_step < 20; fade_step++) {
        for (int y = fade_step * 10; y < (fade_step + 1) * 10 && y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                graphics_pixel(x, y, COLOR_BLACK);
            }
        }
        for (volatile int delay = 0; delay < 80000; delay++);
    }

    struct EXT2DriverRequest request2 = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .parent_inode          = 1,
        .buffer_size           = 0x100000,
        .name_len              = 5,
        .is_directory          = false,
    };

    struct EXT2DriverRequest request_test = {
        .buf                   = "something1",
        .name                  = "check1",
        .parent_inode          = 1,
        .buffer_size           = 11,
        .name_len              = 6,
        .is_directory          = false,
    };
    write(&request_test);

    request_test.buf = 
"Saya lagi membayangkan prabowo mendaki semeru\n"
"Terus pas nyampe di puncak,\n"
"Dia mengibarkan bendera merah putih lalu dia berteriak\n"
"Titik!!! kembalilah ke pelukanku!!!\n"
"Terus habis itu dia meng gelinding ke bawah seperti se ekor landak\n"
"pas nyampe di bawah, dia langsung membeli jagung bakar\n"
"Udah dibela-belain jadi mualaf\n"
"eh malah dicerain istri\n"
"punya anak homo\n"
"biji tinggal satu\n"
"gagal nyapres\n"
"duit abis\n"
"aduh!!\n"
"Akun fufufafa, penuh dengan komedi\n"
"Habib Rizieq yang bilang penyubur korupsi\n"
"Sayang seribu sayang, yang tidak ada banding\n"
"Wong foto orangnya nempel di setiap dinding\n"
"Kasian sekali Pak prabowo ini\n"
"Punya wakil umurnya masih dini\n"
"Baru kemaren debat sama pak anis\n"
"Tapi sekarang plis, aku mau sprei gratis\n"
"Baru juga kemaren pak prabowo debut\n"
"Langsung dikatain mana makan siang gratisnya dut\n"
"Pak prabowo ayo jangan sampai kerja pelan\n"
"ini kira kira situasi rimspek atau godvlan\n"
"Saya lagi membayangkan prabowo mendaki seméru\n"
"Terus pas nyampe di puncak,\n"
"Dia mengibarkan bendera merah putih lalu dia berteriak\n"
"Titik!!! kembalilah ke pelukanku!!!\n"
"Terus habis itu dia meng gelinding ke bawah seperti se ekor landak\n"
"pas nyampe di bawah, dia langsung membeli jagung bakar\n"
"Yasudahlah, sebaiknya kita\n"
"Ayo, ayo, ganyang Fufufafa!\n"
"ganyang fufufafa sekarang juga!\n"
"Ayo, ayo, ganyang Fufufafa!\n"
"Ganyang FufuFafa sekarang juga\n"
"Fufufafa musuh demokrasi\n"
"Fufufafa penyubur korupsi\n"
"Fufufafa anak haram konstitusi\n"
"Fufufafa cukuplah sampai disini";
    request_test.name = "ikanaide";
    request_test.buffer_size = 1430;
    request_test.name_len = 8;
    write(&request_test);

    set_tss_kernel_current_stack();

    process_create_user_process(request2);
    scheduler_init();
    scheduler_switch_to_next_process();
}