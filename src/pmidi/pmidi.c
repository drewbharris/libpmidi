/*
 * 
 * Copyright (C) 1999-2003 Steve Ratcliffe
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 */


#include "glib.h"
#include "elements.h"
#include "except.h"
#include "intl.h"

#include <alsa/asoundlib.h>

#include <stdio.h>
#include "seqlib.h"
#include "md.h"
#include "midi.h"
#include "pmidi.h"
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define TMPBUFSIZE 30
#define MAXPORTS 50

/* Pointer to root context */
seq_context_t *g_ctxp;

/* Number of elements in an array */
#define NELEM(a) ( sizeof(a)/sizeof((a)[0]) )

#define ADDR_PARTS 4 /* Number of part in a port description addr 1:2:3:4 */
#define SEP ", \t"	/* Separators for port description */

static void play(void *arg, struct element *el);
static void no_errors_please(const char *file, int line, const char *function, int err, const char *fmt, ...);
static void set_signal_handler(seq_context_t *ctxp);
static void signal_handler(int sig);


/*
 * Read a list of client/port specifications and return an
 * array of snd_seq_addr_t that describes them.
 * 
 *  Arguments:
 *    portdesc  - 
 */
seq_context_t *
pmidi_openports(char *portdesc)
{
	char *astr;
	char *cp;
	seq_context_t *ctxp;
	snd_seq_addr_t *addr;
	snd_seq_addr_t *ap;
	int a[ADDR_PARTS];
	int count, naddr;
	int i;

	if (portdesc == NULL)
		return NULL;

	ctxp = seq_create_context();

	addr = g_new(snd_seq_addr_t, strlen(portdesc));

	naddr = 0;
	ap = addr;

	for (astr = strtok(portdesc, SEP); astr; astr = strtok(NULL, SEP)) {
		for (cp = astr, count = 0; cp && *cp; cp++) {
			if (count < ADDR_PARTS)
				a[count++] = atoi(cp);
			cp = strchr(cp, ':');
			if (cp == NULL)
				break;
		}

		switch (count) {
		case 2:
			ap->client = a[0];
			ap->port = a[1];
			break;
		default:
			printf("Addresses in %d parts not supported yet\n", count);
			break;
		}
		ap++;
		naddr++;
	}

	count = 0;
	for (i = 0; i < naddr; i++) {
		int  err;
		
		err = seq_connect_add(ctxp, addr[i].client, addr[i].port);
		if (err < 0) {
			fprintf(stderr, _("Could not connect to port %d:%d\n"),
				addr[i].client, addr[i].port);
		} else
			count++;
	}

	g_free(addr);

	if (count == 0) {
		seq_free_context(ctxp);
		return NULL;
	}

	return ctxp;
}

/*
 * Play a single file.
 *  Arguments:
 *    ctxp      - 
 *    filename  - 
 */
void 
pmidi_playfile(seq_context_t *ctxp, char *filename)
{
	struct rootElement *root;
	struct sequenceState *seq;
	struct element *el;
	unsigned long end;
	snd_seq_event_t *ep;

	if (strcmp(filename, "-") == 0)
		root = midi_read(stdin);
	else
		root = midi_read_file(filename);
	if (!root)
		return;

	int input_buffer = snd_seq_get_input_buffer_size(seq_handle(ctxp));
	int output_buffer = snd_seq_get_output_buffer_size(seq_handle(ctxp));

	printf("input buffer: %d, output buffer: %d\n", input_buffer, output_buffer);

	// try increasing the buffer, see if that fixes reliability
	snd_seq_set_output_buffer_size(seq_handle(ctxp), output_buffer * 2);
	snd_seq_set_input_buffer_size(seq_handle(ctxp), input_buffer * 2);

	/* Loop through all the elements in the song and play them */
	seq = md_sequence_init(root);
	while ((el = md_sequence_next(seq)) != NULL) {
		play(ctxp, el);
	}


	/* Get the end time for the tracks and echo an event to
	 * wake us up at that time
	 */
	end = md_sequence_end_time(seq);
	seq_midi_echo(ctxp, end);

	snd_seq_drain_output(seq_handle(ctxp));

	/* Wait for all the events to be played */
	snd_seq_event_input(seq_handle(ctxp), &ep);

	seq_stop_timer(ctxp);

	md_free(MD_ELEMENT(root));
}

/* Get a list of ports */
pmidi_port_list_t*
pmidi_getports()
{
	int nports = 0;
	int i = 0;

	int cap = (SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE);

    char tmp_name[TMPBUFSIZE];
    int tmp_client = 0;
    int tmp_port = 0;
    snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;
	int  client;
	int  err;
	snd_seq_t *handle;

	err = snd_seq_open(&handle, "hw", SND_SEQ_OPEN_DUPLEX, 0);
	if (err < 0) {
		except(ioError, _("Could not open sequencer %s"), snd_strerror(errno));
	}

	snd_seq_drop_output(handle);

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_client_info_set_client(cinfo, -1);

	// since ALSA doesn't allow us to check for the number of ports, loop through twice
	while (snd_seq_query_next_client(handle, cinfo) >= 0) {
		client = snd_seq_client_info_get_client(cinfo);

		snd_seq_port_info_alloca(&pinfo);
		snd_seq_port_info_set_client(pinfo, client);

		snd_seq_port_info_set_port(pinfo, -1);

		while (snd_seq_query_next_port(handle, pinfo) >= 0) {
			if ((snd_seq_port_info_get_capability(pinfo) & cap) == cap) {
				nports++;
			}
		}
	}

	// printf("found %d ports\n", nports);

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_client_info_set_client(cinfo, -1);

    pmidi_port_list_t *results_list = (pmidi_port_list_t *)malloc(sizeof(pmidi_port_list_t));
    pmidi_port_t *results = (pmidi_port_t *)(malloc(sizeof(pmidi_port_t) * nports));

    while (snd_seq_query_next_client(handle, cinfo) >= 0) {
		client = snd_seq_client_info_get_client(cinfo);

		snd_seq_port_info_alloca(&pinfo);
		snd_seq_port_info_set_client(pinfo, client);

		snd_seq_port_info_set_port(pinfo, -1);

		while (snd_seq_query_next_port(handle, pinfo) >= 0) {
			int cap;

			cap = (SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE);

			if ((snd_seq_port_info_get_capability(pinfo) & cap) == cap) {
				if (i >= MAXPORTS) {
					printf("reached max port limit\n");
					return results_list;
				}

				pmidi_port_t *curport = &results[i];
				curport->client = snd_seq_port_info_get_client(pinfo);
				curport->port = snd_seq_port_info_get_port(pinfo);
				snprintf(tmp_name, TMPBUFSIZE, snd_seq_port_info_get_name(pinfo));
				curport->name = strdup(tmp_name);
				i++;
			}
		}
	}
    
    results_list->nports = nports;
    results_list->ports = results;

    return results_list;
}

/* Free memory allocated for ports */
void
pmidi_destroyports(pmidi_port_list_t *portlist)
{
    int i;
    for(i = 0; i < portlist->nports; i++) {
        pmidi_port_t *p = &portlist->ports[i];
        free(p->name);
    }
    free(portlist->ports);
    free(portlist);
}

static void 
play(void *arg, struct element *el)
{
	seq_context_t *ctxp = arg;
	snd_seq_event_t ev;
	
	seq_midi_event_init(ctxp, &ev, el->element_time, el->device_channel);
	switch (el->type) {
	case MD_TYPE_ROOT:
		seq_init_tempo(ctxp, MD_ROOT(el)->time_base, 120, 1);
		seq_start_timer(ctxp);
		break;
	case MD_TYPE_NOTE:
		seq_midi_note(ctxp, &ev, el->device_channel, MD_NOTE(el)->note, MD_NOTE(el)->vel,
			MD_NOTE(el)->length);
		break;
	case MD_TYPE_CONTROL:
		seq_midi_control(ctxp, &ev, el->device_channel, MD_CONTROL(el)->control,
			MD_CONTROL(el)->value);
		break;
	case MD_TYPE_PROGRAM:
		seq_midi_program(ctxp, &ev, el->device_channel, MD_PROGRAM(el)->program);
		break;
	case MD_TYPE_TEMPO:
		seq_midi_tempo(ctxp, &ev, MD_TEMPO(el)->micro_tempo);
		break;
	case MD_TYPE_PITCH:
		seq_midi_pitchbend(ctxp, &ev, el->device_channel, MD_PITCH(el)->pitch);
		break;
	case MD_TYPE_PRESSURE:
		seq_midi_chanpress(ctxp, &ev, el->device_channel, MD_PRESSURE(el)->velocity);
		break;
	case MD_TYPE_KEYTOUCH:
		seq_midi_keypress(ctxp, &ev, el->device_channel, MD_KEYTOUCH(el)->note,
			MD_KEYTOUCH(el)->velocity);
		break;
	case MD_TYPE_SYSEX:
		seq_midi_sysex(ctxp, &ev, MD_SYSEX(el)->status, MD_SYSEX(el)->data,
			MD_SYSEX(el)->length);
		break;
	case MD_TYPE_TEXT:
	case MD_TYPE_KEYSIG:
	case MD_TYPE_TIMESIG:
	case MD_TYPE_SMPTEOFFSET:
		/* Ones that have no sequencer action */
		break;
	default:
		printf("WARNING: play: not implemented yet %d\n", el->type);
		break;
	}
}

/**
 * alsa-lib has taken to printing errors on system call failures, even
 * though such failed calls are part of normal operation.  So we have to
 * install our own handler to shut it up and prevent it messing up the list
 * output.
 *
 * Possibly not needed any more Dec 2003.
 */
static void
no_errors_please(const char *file, int line,
		const char *function, int err, const char *fmt, ...)
{
}

static void
set_signal_handler(seq_context_t *ctxp)
{
	struct sigaction *sap = calloc(1, sizeof(struct sigaction));

	g_ctxp = ctxp;

	sap->sa_handler = signal_handler;
	sigaction(SIGINT, sap, NULL);
}

/* signal handler */
static void
signal_handler(int sig)
{
	/* Close device */
	if (g_ctxp) {
		seq_free_context(g_ctxp);
	}

	exit(1);
}
