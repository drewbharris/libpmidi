#include <unistd.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#include "pmidi/pmidi.h"

int exiting = 0;
seq_context_t *pmidi_context;

void *pthread_play(void *pntr) {
    char *filename = (char *)pntr;
    pmidi_playfile(pmidi_context, filename);
    printf("after playfile\n");
    exiting = 1;
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: playfile filename client:port\n");
        return -1;
    }

    pmidi_context = pmidi_openports(argv[2]);

    printf("playing...\n");

    pthread_t thread;
    pthread_create(&thread, NULL, pthread_play, argv[1]);  

    while(1) { usleep(1000 * 1000); }

    return 0;
}
