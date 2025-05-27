#include "header/process/process-commands/sleep.h"

bool sleep(uint32_t sleep_time) {
    lluint wakeup_time = time_since_running + sleep_time;
    
    // prioqueue malas implement hehe (lebih tepatnya takut salah sih)
    // should be impossible but idk
    if (sleep_manager.sleep_process_count >= PROCESS_COUNT_MAX) return false;

    // empty queue no need for sort
    if (sleep_manager.sleep_process_count == 0) {
        sleep_manager.tail = (sleep_manager.tail + 1) % PROCESS_COUNT_MAX;
        sleep_manager.sleep_queue_pid[sleep_manager.tail] = current_process_idx;
        sleep_manager.wakeup_time[sleep_manager.tail] = wakeup_time;
        sleep_manager.sleep_process_count++;
        _process_list[current_process_idx].metadata.state = SLEEPING;
        
        return true;
    }
    
    // Extend rear to make space
    sleep_manager.tail = (sleep_manager.tail + 1) % PROCESS_COUNT_MAX;
    uint32_t insert_pos = sleep_manager.tail;
    
    // Find correct position and shift elements to maintain sorted order
    // We want highest priority at front, so shift elements with lower priority backward
    uint32_t current = (sleep_manager.tail - 1 + PROCESS_COUNT_MAX) % PROCESS_COUNT_MAX;
    uint32_t last = (sleep_manager.head - 1 + PROCESS_COUNT_MAX) % PROCESS_COUNT_MAX;
    
    while (current != last && 
           sleep_manager.wakeup_time[current] > wakeup_time) {
        uint32_t next = (current + 1) % PROCESS_COUNT_MAX;
        sleep_manager.sleep_queue_pid[next] = sleep_manager.sleep_queue_pid[current];
        sleep_manager.wakeup_time[next] = sleep_manager.wakeup_time[current];
        insert_pos = current;
        current = (current - 1 + PROCESS_COUNT_MAX) % PROCESS_COUNT_MAX;
    }
    
    // Insert the new element at the found position
    sleep_manager.sleep_queue_pid[insert_pos] = current_process_idx;
    sleep_manager.wakeup_time[insert_pos] = wakeup_time;
    sleep_manager.sleep_process_count++;

    _process_list[current_process_idx].metadata.state = SLEEPING;



    return true;
}