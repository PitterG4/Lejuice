#pragma once

#include "types.h"

void free_config_struct();

ReplaceAction set_replace_tag(const char *token);
MacroMatch set_macro_match_tag(const char *token);

int comp_rules(const void *a, const void *b);
int comp_tools(const void *a, const void *b);
int comp_macros(const void *a, const void *b);

bool load_config(const char *filepath);