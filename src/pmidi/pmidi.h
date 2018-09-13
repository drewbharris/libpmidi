#ifndef PMIDI_H_INCLUDED
#define PMIDI_H_INCLUDED

#include <alsa/asoundlib.h>
#include "seqlib.h"

typedef struct {
    int client;
    int port;
    char *name;
} pmidi_port_t;

typedef struct {
    int nports;
    pmidi_port_t *ports;
} pmidi_port_list_t;

seq_context_t *pmidi_openports(char *);
void pmidi_playfile(seq_context_t *, char *);
int pmidi_close(seq_context_t *);
pmidi_port_list_t* pmidi_getports();
void pmidi_destroyports(pmidi_port_list_t *);
#endif