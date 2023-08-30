#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../src/savefile.h"

#define SAMPLE_FILENAME             "../../samples/skyrim_special_edition.ess"
#define REWRITTEN_SAMPLE_FILENAME   "rewritten_sample"

int main()
{
    struct savegame *save;

    save = cengine_savefile_read(SAMPLE_FILENAME);
    if (!save) {
        return EXIT_FAILURE;
    }

    if (cengine_savefile_write(REWRITTEN_SAMPLE_FILENAME, save) == -1) {
        return EXIT_FAILURE;
    }

    execl("/usr/bin/diff", "-q", SAMPLE_FILENAME, REWRITTEN_SAMPLE_FILENAME, NULL);
    perror("execl");
    return EXIT_FAILURE;
}
