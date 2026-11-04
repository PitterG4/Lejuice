#include <stdio.h>
#include <string.h>

#include "types.h"
#include "system.h"

bool queue_empty(EventQueue *queue)
{
    return queue->head == queue->tail;
}
bool queue_full(EventQueue *queue)
{
    return ((queue->tail + 1) % queue->size) == queue->head;
}
bool enqueue_event(EventQueue *queue, FileEvent *file_event)
{
    if (queue_full(queue)) return false;

    queue->events[queue->tail] = *file_event;
    queue->tail = (queue->tail + 1) % queue->size;

    return true;
}
FileEvent dequeue_event(EventQueue *queue)
{
    FileEvent file_event = {0};

    if (queue_empty(queue)) return file_event;

    file_event = queue->events[queue->head];
    queue->head = (queue->head + 1) % queue->size;

    return file_event;
}

void enqueue_cooldown(CooldownQueue *cooldown_queue, const char *filename)
{
    CooldownItem item = {
        .expire_time = current_time_ms() + 1500,  // add 1.5s of cooldown
    };
    snprintf(item.filename, MAX_FILENAME_LEN, "%s", filename);

    cooldown_queue->items[cooldown_queue->tail] = item;
    cooldown_queue->tail = (cooldown_queue->tail + 1) % cooldown_queue->size;
}

// iterates through the oldest item to the newest -- beautiful logic be cautious, don't mess it up
bool search_queue_cooldown(CooldownQueue *cooldown_queue, const char *filename)
{
    uint64_t current_time = current_time_ms();

    for (int i = 0; i < cooldown_queue->size; i++) {
        int index = cooldown_queue->tail - 1 - i;
        if (index < 0) index += cooldown_queue->size;

        if (current_time > cooldown_queue->items[index].expire_time) break;
        if (strncmp(cooldown_queue->items[index].filename, filename, MAX_FILENAME_LEN) == 0) return false;
    }

    return true;
}