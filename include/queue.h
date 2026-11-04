#pragma once

#include "types.h"

bool queue_empty(EventQueue *queue);
bool queue_full(EventQueue *queue);
bool enqueue_event(EventQueue *queue, FileEvent *file_event);
FileEvent dequeue_event(EventQueue *queue);

void enqueue_cooldown(CooldownQueue *cooldown_queue, const char *filename);
bool search_queue_cooldown(CooldownQueue *cooldown_queue, const char *filename);