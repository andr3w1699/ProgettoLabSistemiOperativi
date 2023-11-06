/*****************************************************************************************/
/** Progetto Farm
 * Laboratorio di sistemi Operativi
 * @author Andrea Lepori
 * @file : masterWorkerMain.c
 * @brief : il file masterWorkerMain.c
 */
/*=======================================================================================*/

#define _GNU_SOURCE

/*
1.3.4 Feature Test Macros

The exact set of features available when you compile a source file is controlled by which feature test
macros you define. If you compile your programs using ‘gcc -ansi’, you get only the ISO C library features,
unless you explicitly request additional features by defining one or more of the feature macros.
You should define these macros by using ‘#define’ preprocessor directives at the top of your source code files.
These directives must come before any #include of a system header file. It is best to make them the very first
thing in the file, preceded only by comments. You could also use the ‘-D’ option to GCC, but it’s better if you make
the source files indicate their own meaning in a self-contained way. This system exists to allow the library to
conform to multiple standards. Although the different standards are often described as supersets of each other,
they are usually incompatible because larger standards require functions with names that smaller ones reserve to
the user program. This is not mere pedantry — it has been a problem in practice. For instance, some non-GNU programs
define functions named getline that have nothing to do with this library’s getline. They would not be compilable if
all features were enabled indiscriminately.
This should not be used to verify that a program conforms to a limited standard. It is insufficient for this
purpose, as it will not protect you from including header files outside the standard, or relying on semantics
undefined within the standard.
...
Macro: _POSIX_C_SOURCE

Define this macro to a positive integer to control which POSIX functionality is made available. The greater the
value of this macro, the more functionality is made available.
...
If you define this macro to a value greater than or equal to 200112L, then the functionality from the 2001 edition
of the POSIX standard (IEEE Standard 1003.1-2001) is made available.
...
Macro: _GNU_SOURCE

If you define this macro, everything is included: ISO C89, ISO C99, POSIX.1, POSIX.2, BSD, SVID, X/Open, LFS,
and GNU extensions. In the cases where POSIX.1 conflicts with BSD, the POSIX definitions take precedence.
...

reference : https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html#index-_005fPOSIX_005fC_005fSOURCE
*/

/*******************************************/
// Direttive al preprocessore
/*=========================================*/

// include
#include <util.h>
#include <communication.h>
#include <threadpool.h>
#include <getopt.h>
#include <worker.h>

// define
// alcuni valori di default
#define NTHREAD 4 // numero dei thread worker di default
#define QLEN 8    // lunghezza di default della coda concorrente dei task pendenti
#define DELAY 0   // distanza di sottomissione dei task dal master ai worker espressa in ms

/*******************************************/
// Alcune variabili globali
/*=========================================*/

/*
sig_atomic_t --> An integer type which can be accessed as an atomic entity even in the presence
of asynchronous interrupts made by signals.
*/
static volatile sig_atomic_t termina = 0;

// file descriptor del socket di comunicazione tra sig_handler e collector
static int serverfd;

// codice per il messaggio di stampa nel protocollo prestabilito di comunicazione tra master e collector
static long codice_stampa = 1;

// dichiarazione funzione compute
int compute(char *file_name, long *result);

/*******************************************/
// signal handler
/*=========================================*/
static void sighandler(int signum)
{
  switch (signum)
  {
  case SIGINT:
    // printf("Received SIGINT signal.\n");
    termina = 1;
    break;
  case SIGQUIT:
    // printf("Received SIGQUIT signal.\n");
    termina = 1;
    break;
  case SIGTERM:
    // printf("Received SIGTERM signal.\n");
    termina = 1;
    break;
  case SIGHUP:
    // printf("Received SIGHUP signal.\n");
    termina = 1;
    break;
  case SIGUSR1:
    // printf("Received SIGUSR1 signal.\n");
    // signal safety --> https://man7.org/linux/man-pages/man7/signal-safety.7.html --> write is asynchronous signal safe
    writen(serverfd, &codice_stampa, sizeof(long));
    break;
  default:;
    // printf("Received signal number: %d\n", signum);
  }
}

/*******************************************/
// Funzioni
/*=========================================*/

// funzione che stampa il messaggio d'uso
int arg_h(const char *programname)
{
  printf("usage: %s -n <num_worker> -q <qlen> -t <delay> [-d <nomedir>] nomefile [nomefile...] -h\n", programname);
  return -1;
}

// funzione arg_n
int arg_n(const char *n, long *nthread)
{
  long tmp;
  if (isNumber(n, &tmp) != 0)
  {
    printf("l'argomento di '-n' non e' valido\n");
    return -1;
  }
  *nthread = tmp;
  return 0;
}

// funzione arg_q
int arg_q(const char *m, long *qlen)
{
  long tmp;
  if (isNumber(m, &tmp) != 0)
  {
    printf("l'argomento di '-q' non e' valido\n");
    return -1;
  }
  *qlen = tmp;
  return 0;
}

// funzione arg_t
int arg_t(const char *t, long *delay)
{
  long tmp;
  if (isNumber(t, &tmp) != 0)
  {
    printf("l'argomento di '-t' non e' valido\n");
    return -1;
  }
  *delay = tmp;
  return 0;
}

// funzione arg_d
int arg_d(char *d, char **dir_name)
{
  *dir_name = d;
  return 0;
}

/** funzione find
 * @brief: esplora il file system a partire dalla directory passata come parametro in cerca di file regolari
 *       da sottomettere al threadpool
 * @param nomedir directory di partenza
 * @param tp threadpool a cui sottomettere i task
 * @param delay ritardo in ms tra una sottomissione e l'altra
 * @return :
 *   1 successo
 *   -1 errore
 */
int find(const char nomedir[], threadpool_t *tp, long delay)
{
  DIR *dir;

  if ((dir = opendir(nomedir)) == NULL)
  {
    print_error("Errore aprendo la directory %s\n", nomedir);
    return -1;
  }
  else
  {
    // sono dentro la directory
    struct dirent *file;

    while ((errno = 0, file = readdir(dir)) != NULL)
    { // finchè ci sono entry nella directory
      struct stat statbuf;
      char *new_dir = (char *)malloc(sizeof(char) * (NAME_MAX));
      new_dir[0] = '\0';
      strcat(new_dir, nomedir);
      if (new_dir[strlen(new_dir) - 1] != '/')
      {
        strcat(new_dir, "/");
      }
      strcat(new_dir, file->d_name);
      if (!termina)
      {
        if (stat(new_dir, &statbuf) == -1)
        { // faccio la stat
          perror("stat");
          print_error("Errore facendo stat di %s\n", file->d_name);
          return -1;
        }
        if (S_ISDIR(statbuf.st_mode))
        { // se è una directory
          if (!isdot(file->d_name))
          { // se non è la directory corrente o padre
            find(new_dir, tp, delay);
            // chiamo la find sul sottoalbero
          }
        }
        else
        { // se non è una directory
          if (S_ISREG(statbuf.st_mode))
          { // se il file è un file regolare
            msleep(delay);
            // printf("dormito per %ld ms\n", delay);

            if (!termina)
            {
              addToThreadPool(tp, (int (*)(void *, void *))compute, new_dir);
              // printf("Sottomesso al threadpool file : %s\n", new_dir);
            }
          }
        }
      }
      free(new_dir);
    }
    if (errno != 0)
    {
      perror("readdir");
      closedir(dir);
      return -1;
    }
    closedir(dir);
    return 1;
  }
}

// funzione main
int main(int argc, char *argv[])
{
  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGQUIT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGPIPE);
  sigaddset(&mask, SIGUSR1);

  /*
    pthread_sigmask - examine and change mask of blocked signals
  */
  if (pthread_sigmask(SIG_BLOCK, &mask, &oldmask) != 0)
  {
    fprintf(stderr, "FATAL ERROR, pthread_sigmask\n");
    return EXIT_FAILURE;
  }

  pid_t pid = fork();

  if (pid == 0)
  { // figlio
    char *argv_for_program[] = {"collector", NULL};
    if (execvp("./collector", argv_for_program) == -1)
    {
      perror("execvp");
      return EXIT_FAILURE;
    }
  }
  else
  { // padre
    if (pid == -1)
    { // errore padre
      perror("fork");
      return EXIT_FAILURE;
    }

    // creo e connetto il socket
    struct sockaddr_un serv_addr;
    SYSCALL_EXIT("socket", serverfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket", "");
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, UNIX_PATH_MAX);
    // printf("%s\n",serv_addr.sun_path );

    // connetto il socket
    while (connect(serverfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
      if (errno == ENOENT)
        msleep(50); /* sock non esiste */
      else
        exit(EXIT_FAILURE);
    }
    // socket connesso

    // installo il signal handler per tutti i segnali che mi interessano
    struct sigaction sa;
    // resetto la struttura
    memset(&sa, '0', sizeof(sa));
    sa.sa_handler = sighandler;
    int notused;
    SYSCALL_EXIT("sigaction", notused, sigaction(SIGINT, &sa, NULL), "sigaction", "");
    SYSCALL_EXIT("sigaction", notused, sigaction(SIGQUIT, &sa, NULL), "sigaction", "");
    SYSCALL_EXIT("sigaction", notused, sigaction(SIGTERM, &sa, NULL), "sigaction", "");
    SYSCALL_EXIT("sigaction", notused, sigaction(SIGHUP, &sa, NULL), "sigaction", "");
    SYSCALL_EXIT("sigaction", notused, sigaction(SIGUSR1, &sa, NULL), "sigaction", "");
    // ignoro sigpipe
    memset(&sa, '0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    ec_meno1(sigaction(SIGPIPE, &sa, NULL), "sigaction");

    if (pthread_sigmask(SIG_SETMASK, &oldmask, NULL) != 0)
    {
      fprintf(stderr, "FATAL ERROR\n");
      return EXIT_FAILURE;
    }

    // setto i valori di default
    long nthread = NTHREAD, qlen = QLEN, delay = DELAY;

    char *dir_name = NULL;

    int opt;

    while ((opt = getopt(argc, argv, ":n:q:t:d:h:")) != -1)
    {
      switch (opt)
      {
      case 'n':
        arg_n(optarg, &nthread);
        break;
      case 'q':
        arg_q(optarg, &qlen);
        break;
      case 't':
        arg_t(optarg, &delay);
        break;
      case 'd':
        arg_d(optarg, &dir_name);
        break;
      case 'h':
        arg_h(argv[0]);
        break;
      case ':':
      { // restituito se manca il valore corrispondente ad un' opzione
        // printf("l'opzione '-%c' richiede un argomento\n", optopt);
      }
      break;
      case '?':
      { // restituito se getopt trova una opzione non riconosciuta
        //  printf("l'opzione '-%c' non e' gestita\n", optopt);
      }
      break;
      default:;
      }
    }

    /*
    // Stampe di prova
    printf("-n : %ld\n", nthread);
    printf("-q : %ld\n", qlen);
    printf("-t : %ld\n", delay);
    if (dir_name != NULL)
      printf("-d : \"%s\"\n", dir_name);
    */

    // creo il threadpool
    threadpool_t *tp = createThreadPool(nthread, qlen);
    // printf("Threadpool creato\n");

    for (int index = optind; index < argc; index++)
    {

      msleep(delay);
      // printf("dormito per %ld ms\n", delay);

      struct stat statbuf;
      if (stat(argv[index], &statbuf) == -1)
      { // faccio la stat
        perror("stat");
        print_error("Errore facendo stat di %s\n", argv[index]);
        return EXIT_FAILURE;
      }
      if (!termina && S_ISREG(statbuf.st_mode))
      {
        addToThreadPool(tp, (int (*)(void *, void *))compute, argv[index]);
        // printf("Sottomesso al threadpool file : %s\n", argv[index]);
      }

      if (termina)
        break;
    }

    if (dir_name != NULL && !termina)
    {
      struct stat statbuf;
      int r;

      SYSCALL_EXIT("stat", r, stat(dir_name, &statbuf), "Facendo stat del nome %s: errno=%d\n", dir_name, errno);
      if (!S_ISDIR(statbuf.st_mode))
      {
        fprintf(stderr, "%s non e' una directory\n", dir_name);
        return EXIT_FAILURE;
      }
      else
        find(dir_name, tp, delay);
      // printf("iniziata l'esplorazione della cartella %s\n", dir_name);
    }
    // distruggo il threadpool , terminando i task in coda senza accettarne di nuovi 
    destroyThreadPool(tp, 0);

    // invio codice di terminazione al collector
    // utilizzo un' altra connessione

    // creo e connetto il socket
    int serverfd2;
    struct sockaddr_un serv_addr2;
    SYSCALL_EXIT("socket", serverfd2, socket(AF_UNIX, SOCK_STREAM, 0), "socket", "");
    memset(&serv_addr2, '0', sizeof(serv_addr2));

    serv_addr2.sun_family = AF_UNIX;
    strncpy(serv_addr2.sun_path, SOCKNAME, UNIX_PATH_MAX);
    // printf("%s\n",serv_addr.sun_path );

    // connetto il socket
    while (connect(serverfd2, (struct sockaddr *)&serv_addr2, sizeof(serv_addr2)) == -1)
    {
      if (errno == ENOENT)
        msleep(50); /* sock non esiste */
      else
        exit(EXIT_FAILURE);
    }
    // socket connesso

    long codice;
    codice = 2;
    writen(serverfd2, &codice, sizeof(long));
    // chiudo il socket del MasterMain
    close(serverfd2);

    // chiudo il socket del signal_handler
    close(serverfd);

    int status;
    pid_t child_pid = waitpid(pid, &status, 0); // Wait for the child process to terminate

    if (child_pid == -1)
    {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }

    /*
          if (WIFEXITED(status)) {
              printf("Child process %d terminated normally with exit status: %d\n", pid, WEXITSTATUS(status));
          } else if (WIFSIGNALED(status)) {
              printf("Child process %d terminated by signal %d\n", pid, WTERMSIG(status));
          }

    */

    exit(EXIT_SUCCESS);
  }
}
