/***********************************************************************/
//  file util.c                                                        //
// implementation of the functions contained in the header file util.h //
/*=====================================================================*/

// include
#include <util.h>

/**
 * function isNumber
 * \brief Controlla se la stringa passata come primo argomento e' un numero.
 * \param s la stringa da controllare, n indirizzo di un long dove salvare l'eventuale numero letto
 * \return  0 ok, 1 non e' un numero, 2 overflow/underflow
 */
int isNumber(const char *s, long *n)
{
  if (s == NULL)
    return 1;
  if (strlen(s) == 0)
    return 1;
  char *e = NULL;
  errno = 0;
  /*
  from the man ...
  long strtol(const char *nptr, char **endptr, int base);
  DESCRIPTION
  The  strtol()  function  converts the initial part of the string in nptr to a long integer
  value according to the given base.
  The string may begin with an arbitrary amount of white space (as determined by isspace(3))  followed
  by  a single optional '+' or '-' sign.  If base is zero or 16, the string may then include a "0x" or
  "0X" prefix, and the number will be read in base 16; otherwise, a zero base is taken as 10 (decimal)
  unless the next character is '0', in which case it is taken as 8 (octal).

  The  remainder  of  the  string  is converted to a long value in the obvious manner, stopping at the
  first character which is not a valid digit in the given base. (In bases above 10, the letter 'A' in
  either  uppercase or lowercase represents 10, 'B' represents 11, and so forth, with 'Z' representing
  35.)

  If endptr is not NULL, strtol() stores the address of the first invalid character  in  *endptr.   If
  there  were no digits at all, strtol() stores the original value of nptr in *endptr (and returns 0).
  In particular, if *nptr is not '\0' but **endptr is '\0' on return, the entire string is valid.
  RETURN VALUE
  The  strtol()  function returns the result of the conversion, unless the value would underflow or overflow.
  If  an  underflow  occurs,  strtol() returns LONG_MIN.  If an overflow occurs, strtol() returns LONG_MAX.
  In both cases, errno is set to  ERANGE.
  */
  long val = strtol(s, &e, 10);
  if (errno == ERANGE)
    return 2; // overflow
  if (e != NULL && *e == (char)0)
  {
    *n = val;
    return 0; // successo
  }
  return 1; // non e' un numero
}

/**
 * function print_error
 * \brief Procedura di utilita' per la stampa degli errori
 * \param str stringa che contiene il messaggio di errore eventualmente formatto con dei placeholder
 *        e un numero variabile di argomenti per riempire i placeholder
 */
void print_error(const char *str, ...)
{
  const char err[] = "ERROR: ";
  va_list argp;
  char *p = (char *)malloc(strlen(str) + strlen(err) + EXTRA_LEN_PRINT_ERROR);
  if (!p)
  {
    perror("malloc");
    fprintf(stderr, "FATAL ERROR nella funzione 'print_error'\n");
    return;
  }
  strcpy(p, err);               // copio all' inizio della stringa "ERROR: "
  strcpy(p + strlen(err), str); // ci appendo la stringa col messaggio di errore
  va_start(argp, str);
  vfprintf(stderr, p, argp); // stampo su stderr la stringa ottenuta passando anche gli eventuali argomenti variabili (a riempire gli eventuali placeholder)
  va_end(argp);
  free(p);
}

/**
 * funzione isdot
 * @brief funzione di utilità che controlla se la directory passata come parametro è . oppure ..
 * @param dir è  una stringa che rappresenta un pathname di una directory
 * @return 1 se la stringa passata come parametro identifica la directory . o .. altrimenti restituisce 0
 */
int isdot(const char dir[])
{
  int l = strlen(dir);
  if ((l > 0 && dir[l - 1] == '.')) // se la lunghezza della stringa non è 0 e il penultimo carattere è '.' (l'ultimo è il terminatore obv )
    return 1;
  return 0;
}

/**
 * funzione cwd
 * @brief restituisce pathname della directory corrente
 * @return restituisce la stringa contente il pathname della directory corrente se l'operazione va a buon fine, null altrimenti
 *
 */
char *cwd()
{
  char *buf = malloc(NAME_MAX * sizeof(char));
  if (!buf)
  {
    perror("cwd malloc");
    return NULL;
  }
  if (getcwd(buf, NAME_MAX) == NULL)
  {
    if (errno == ERANGE)
    { // il buffer e' troppo piccolo, lo allargo
      char *buf2 = realloc(buf, 2 * NAME_MAX * sizeof(char));
      if (!buf2)
      {
        perror("cwd realloc");
        free(buf);
        return NULL;
      }
      buf = buf2;
      if (getcwd(buf, 2 * NAME_MAX) == NULL)
      { // mi arrendo....
        perror("cwd eseguendo getcwd");
        free(buf);
        return NULL;
      }
    }
    else
    {
      perror("cwd eseguendo getcwd");
      free(buf);
      return NULL;
    }
  } // getcwd è andato a buon fine restituisco la stringa contenente il pathname della directory corrente
  return buf;
}

/**
 * @brief function to sleep tms ms
 * @return 0 on succesfully sleeping, -1 and errno set to EINVAL if param tms < 0
 */
int msleep(long tms)
{

  // The timespec struct contains two member variables:
  // tv_sec – The variable of the time_t type made to store time in seconds.
  // tv_nsec – The variable of the long type used to store time in nanoseconds.

  struct timespec ts;
  int ret;

  if (tms < 0)
  {
    errno = EINVAL;
    return -1;
  }

  ts.tv_sec = tms / 1000;
  ts.tv_nsec = (tms % 1000) * 1000000;

  // int nanosleep(const struct timespec *req, struct timespec *_Nullable rem);
  /* DESCRIPTION
     nanosleep() suspends the execution of the calling thread until
     either at least the time specified in *req has elapsed, or the
     delivery of a signal that triggers the invocation of a handler in
     the calling thread or that terminates the process.

     If the call is interrupted by a signal handler, nanosleep()
     returns -1, sets errno to EINTR, and writes the remaining time
     into the structure pointed to by rem unless rem is NULL.  The
     value of *rem can then be used to call nanosleep() again and
     complete the specified pause (but see NOTES).

     The timespec(3) structure is used to specify intervals of time
     with nanosecond precision.

    RETURN VALUE
     On successfully sleeping for the requested interval, nanosleep()
     returns 0.  If the call is interrupted by a signal handler or
     encounters an error, then it returns -1, with errno set to
     indicate the error.
  */

  do
  {
    ret = nanosleep(&ts, &ts);
  } while (ret && errno == EINTR);

  return ret;
}
