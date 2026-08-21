#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

ReturnCode load_config(const char *filepath, Config *out_config);
void free_config_struct(Config config);

#endif //CONFIG_H