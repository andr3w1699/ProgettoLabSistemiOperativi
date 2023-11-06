/***********************/
// header file util.h   /
/*=====================*/

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2001112L
#endif

#if !defined(_UTIL_H)
#define _UTIL_H

// include
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <stddef.h>

// NAME_MAX
#if !defined(NAME_MAX)
#define NAME_MAX 256
#endif

// EXTRA_LEN_PRINT_ERROR
#if !defined(EXTRA_LEN_PRINT_ERROR)
#define EXTRA_LEN_PRINT_ERROR 512
#endif

// SYSCALL_EXIT
#define SYSCALL_EXIT(name, r, sc, str, ...) \
  if ((r = sc) == -1)                       \
  {                                         \
    perror(#name);                          \
    int errno_copy = errno;                 \
    print_error(str, __VA_ARGS__);          \
    exit(errno_copy);                       \
  }

// LOCK_RETURN
#define LOCK_RETURN(l, r)                    \
  if (pthread_mutex_lock(l) != 0)            \
  {                                          \
    fprintf(stderr, "ERRORE FATALE lock\n"); \
    return r;                                \
  }

// UNLOCK_RETURN
#define UNLOCK_RETURN(l, r)                    \
  if (pthread_mutex_unlock(l) != 0)            \
  {                                            \
    fprintf(stderr, "ERRORE FATALE unlock\n"); \
    return r;                                  \
  }

/* controlla -1; stampa errore e termina */
#define ec_meno1(s, m) \
  if ((s) == -1)       \
  {                    \
    perror(m);         \
    exit(errno);       \
  }

/* controlla NULL; stampa errore e termina (NULL) */
#define ec_null(s, m) \
  if ((s) == NULL)    \
  {                   \
    perror(m);        \
    exit(errno);      \
  }

/* controlla -1; stampa errore ed esegue c */
#define ec_meno1_c(s, m, c) \
  if ((s) == -1)            \
  {                         \
    perror(m);              \
    c;                      \
  }

// prototipi di alcune funzioni di utilità

/**
 * \brief Controlla se la stringa passata come primo argomento, s, rappresenti o meno un numero
 *        (long) ed eventualmente la converte a long, memorizzandolo nel secondo argomento n
 * \param s è una stringa
 * \param n è un puntatore a long
 * \return  0 ok,  1 non e' un numero,   2 overflow/underflow
 */
int isNumber(const char *s, long *n);

/**
 * \brief Procedura di utilita' per la stampa degli errori
 *
 */
void print_error(const char *str, ...);

/**
 * funzione isdot
 * @brief funzione di utilità che controlla se la directory passata come parametro è . oppure ..
 * @param dir è una stringa che rappresenta il pathname di una directory
 * @return 1 se la stringa passata come parametro identifica la directory . o .. altrimenti restituisce 0
 */
int isdot(const char dir[]);

/**
 * funzione cwd
 * @brief restituisce il pathname della directory corrente
 * @return restituisce la stringa contente il pathname della directory corrente se l'operazione va a buon fine, null altrimenti
 *
 */
char *cwd();

/**
 * @brief function to sleep tms milliseconds
 */
int msleep(long tms);

#endif // _UTIL_H