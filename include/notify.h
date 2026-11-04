#pragma once

#include "types.h"

void write_to_log(EventFlag flag, const char *title, const char *message);
void dispatch_event(EventFlag flag, const char *title, const char *message);