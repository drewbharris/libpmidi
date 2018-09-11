#include <unistd.h>
#include <alsa/asoundlib.h>

#include "pmidi/pmidi.h"

int exiting = 0;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: playfile filename client:port\n");
        return -1;
    }

    seq_context_t *pmidi_context = pmidi_openports(argv[2]);
    seq_set_nonblocking(pmidi_context);
    pmidi_playfile(pmidi_context, argv[1]);

    printf("playing...\n");

    usleep(1000*1000*10);
    return 0;
}
