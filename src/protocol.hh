/* defines the simple protocol between the daemon and the PAM module
 *
 * Copyright 2021 Daniel "q66" Kolesa <q66@chimera-linux.org>
 * License: BSD-2-Clause
 */

#ifndef TURNSTILED_PROTOCOL_HH
#define TURNSTILED_PROTOCOL_HH

#include <sys/un.h>

#include "config.hh"

#ifndef RUN_PATH
#error "No RUN_PATH is defined"
#endif

#define DPAM_SERVICE "turnstiled"

#define SOCK_DIR DPAM_SERVICE
#define DAEMON_SOCK RUN_PATH "/" SOCK_DIR "/control.sock"

/* maximum length of a directory path we can receive */
#define DIRLEN_MAX 1024

/* protocol messages
 *
 * this is a simple protocol consisting of uint-sized messages; each
 * message carries the type (4 bits) and optionally auxiliary data
 * (only some messages; MSG_DATA and MSG_REQ_RDATA)
 *
 * turnstiled is the server; the pam module is the client
 *
 * the client connects to DAEMON_SOCK (seqpacket sockets are used)
 *
 * from there, the following sequence happens:
 *
 * CLIENT: sends MSG_START with uid and enters a message loop (state machine)
 * SERVER: if service manager for the user is already running, responds
 *         with MSG_OK_DONE (with export_dbus attached as aux data); else
 *         initiates startup and responds with MSG_OK_WAIT
 * CLIENT: if MSG_OK_WAIT was received, waits for a message
 * SERVER: once service manager starts, MSG_OK_DONE is sent
 * CLIENT: sends MSG_REQ_RLEN
 * SERVER: responds with MSG_DATA with rundir length (0 if not known,
           DIRLEN_MAX will be added to it if managed).
 * loop:
 *   CLIENT: sends MSG_REQ_RDATA with number of remaining bytes of rundir
 *           that are yet to be received
 *   SERVER: responds with a MSG_DATA packet until none is left
 * CLIENT: finishes startup, exports XDG_RUNTIME_DIR if needed as well
 *         as DBUS_SESSION_BUS_ADDRESS, and everything is done
 */

/* this is a regular unsigned int */
enum {
    MSG_OK_WAIT = 0x1, /* login, wait */
    MSG_OK_DONE, /* ready, proceed */
    MSG_REQ_RLEN, /* rundir length request */
    MSG_REQ_RDATA, /* rundir string request + how much is left */
    MSG_DATA,
    MSG_START,
    /* sent by server on errors */
    MSG_ERR,

    MSG_TYPE_BITS = 4,
    MSG_TYPE_MASK = 0xF,
    MSG_DATA_BYTES = sizeof(unsigned int) - 1
};

#define MSG_ENCODE_AUX(v, tp) \
    (tp | (static_cast<unsigned int>(v) << MSG_TYPE_BITS))

#define MSG_ENCODE(v) MSG_ENCODE_AUX(v, MSG_DATA)
#define MSG_SBYTES(len) std::min(int(MSG_DATA_BYTES), int(len))

#endif
