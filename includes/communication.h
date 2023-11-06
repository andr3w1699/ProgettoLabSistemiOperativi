#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2001112L
#endif

/********************************/
// header file communication.h   /
/*==============================*/

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

//include
#include <sys/socket.h>
#include <sys/un.h> /* For AF_UNIX sockets */
#include <sys/select.h>
#include <sys/types.h>

#define UNIX_PATH_MAX 108 /* man 7 unix */

//  define SOCKNAME
#if !defined(SOCKNAME)
#define SOCKNAME "./farm.sck"
#endif

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
static inline int readn(long fd, void *buf, size_t size)
{
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    while (left > 0)
    {
        if ((r = read((int)fd, bufptr, left)) == -1)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0)
            return 0; // EOF
        left -= r;
        bufptr += r;
    }
    return size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void *buf, size_t size)
{
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    while (left > 0)
    {
        if ((r = write((int)fd, bufptr, left)) == -1)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0)
            return 0;
        left -= r;
        bufptr += r;
    }
    return 1;
}

#endif // COMMUNICATION_H