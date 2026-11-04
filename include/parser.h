#pragma once

#include "types.h"

bool parse_file_event(FileEvent *file_event, CooldownQueue *cooldown_queue);
bool parse_command(Rules *rule, FileEvent *file_event, char *final_command);
