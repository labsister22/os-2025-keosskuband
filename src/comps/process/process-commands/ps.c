#include "header/process/process-commands/ps.h"
#include "header/process/process.h"

void ps(ProcessMetadata* info, uint8_t idx) {
    if (idx < PROCESS_COUNT_MAX) {
        if (process_manager_state._process_used[idx]) {
            struct ProcessControlBlock *pcb = &_process_list[idx];
            
            ProcessMetadata pm = {
                .pid = pcb->metadata.pid,
                .state = pcb->metadata.state
            };
    
            // safer with strncpy but we dont have that yey lmao
            memcpy(pm.name, pcb->metadata.name, sizeof(pm.name));
            
            *info = pm;
        }
        else {
            ProcessMetadata pm = {
                .pid = -1,
            };

            *info = pm;
        }
    }
}