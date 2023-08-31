#include "log.h"

FILE *debug_log_file;

void logging(void)
{
    if (!debug_log_file) {
        debug_log_file = fopen("/tmp/cegse_debug.log", "w");
    }
}
