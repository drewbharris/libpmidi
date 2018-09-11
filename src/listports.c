#include <unistd.h>
#include <alsa/asoundlib.h>

#include "pmidi/pmidi.h"

int exiting = 0;

int main() {
    // memory is allocated in getports
    pmidi_port_list_t* portlist = pmidi_getports();

    for (int i = 0; i < portlist->nports; i++) {
        pmidi_port_t *curport = &portlist->ports[i];
        printf("client: %d, port: %d, name: %s\n", curport->client, curport->port, curport->name);
    }

    // clear allocated memory
    pmidi_destroyports(portlist);
    return 0;
}
