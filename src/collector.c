/*****************************************************************************************/
/** Progetto Farm
 * Laboratorio di sistemi Operativi
 * @author Andrea Lepori
 * @file : collector.c
 * @brief :
 */
/*=======================================================================================*/

// include
#include <util.h>
#include <communication.h>
#include <sortedlist.h>

/*
This function iterates through file descriptors from 0 to FD_SETSIZE - 1 and checks if each file descriptor
is set in the given fd_set. If it is set and greater than the current max_fd, max_fd is updated with the new value.
Finally, the function returns the maximum file descriptor found. Remember that FD_SETSIZE is a constant
representing the maximum number of file descriptors that can be stored in an fd_set.
*/
int aggiorna(fd_set *set)
{
    int max_fd = -1; // Initialize max_fd to an invalid value

    for (int i = 0; i < FD_SETSIZE; ++i)
    {
        if (FD_ISSET(i, set))
        {
            if (i > max_fd)
            {
                max_fd = i; // Update max_fd if the current FD is greater
            }
        }
    }

    return max_fd; // Return the maximum file descriptor value
}

// main
int main(int argc, char *argv[])
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
    {
        fprintf(stderr, "FATAL ERROR\n");
        exit(EXIT_FAILURE);
    }

    // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket
    struct sigaction s;
    memset(&s, '0', sizeof(struct sigaction));
    s.sa_handler = SIG_IGN;
    if ((sigaction(SIGPIPE, &s, NULL)) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct Node *head = NULL; // puntatore alla testa della lista
    struct Node *temp = NULL; // variabile nodo temporanea
    long codice = -1;         // 0 --> aggiungi coda, 1 --> stampa, 2 --> termina
    int termina = 0;          // flag di terminazione
    int listenfd;
    int fdmax;
    int open_connections = 0; // contatore connessioni aperte
    // create a new socket of type SOCK_STREAM in domain AF_UNIX, if protocol we use the default protocol
    if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '0', sizeof(struct sockaddr_un));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, UNIX_PATH_MAX);

    unlink(SOCKNAME);

    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("bind");
        return EXIT_FAILURE;
    }
    // printf("%s\n",serv_addr.sun_path );

    if (listen(listenfd, SOMAXCONN) == -1)
    {
        perror("listen");
        return EXIT_FAILURE;
        ;
    }

    fd_set set, tmpset;
    long result = 0;
    long messagelength = 0;
    char *message = NULL;
    int n;

    FD_ZERO(&set);
    FD_ZERO(&tmpset);

    FD_SET(listenfd, &set); // aggiungo il listener fd al master set

    fdmax = listenfd;

    while (!termina || open_connections > 0)
    {

        // copio il set nella variabile temporanea per la select
        tmpset = set;

        // use select to check if there are any file descriptors ready for reading
        int ready_fds = select(fdmax + 1, &tmpset, NULL, NULL, NULL);

        if (ready_fds == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // cerchiamo di capire da quale fd abbiamo ricevuto una richiesta
        for (int i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &tmpset))
            {
                int connfd;
                if (i == listenfd)
                { // e' una nuova richiesta di connessione
                    if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1)
                    {
                        perror("accept");
                        return EXIT_FAILURE;
                    }
                    FD_SET(connfd, &set);
                    //printf("Connection opened by client on socket %d\n", connfd);
                    open_connections++;
                    if (connfd > fdmax)
                        fdmax = connfd;
                }
                else
                { /*/* sock I/0 pronto */
                    // per prima cosa leggo il codice del comando che è un long
                    if ((n = readn(i, &codice, sizeof(long))) == -1)
                    {
                        perror("readn");
                        break;
                    }

                    // Check for EOF condition (client closed the connection)
                    if (n == 0)
                    {
                        //printf("Connection closed by client on socket %d\n", i);
                        //  Remove the socket from the set and update fdmax
                        FD_CLR(i, &set);
                        fdmax = aggiorna(&set);
                        close(i); // Close the socket
                        open_connections--;
                        break;
                    }

                    // printf("codice : %ld\n", codice);
                    // se il codice indica la terminazione metto termina a 1
                    if (codice == 2)
                    {
                        termina = 1;
                    }
                    else if (codice == 1)
                    { //  codice 1 --> stampo la lista
                        printList(head);
                        fflush(stdout);
                    }
                    else
                    { // codice 0 --> inserimento nella lista

                        // per prima cosa leggo il long (risultato/somma)
                        if ((n = readn(i, &result, sizeof(long))) == -1)
                        {
                            perror("readn");
                            break;
                        }
                        // printf("result : %ld\n", result);
                        //  poi leggo la lunghezza del messaggio contenente il pathname del file
                        if ((n = readn(i, &messagelength, sizeof(long))) == -1)
                        {
                            perror("readn");
                            break;
                        }
                        // printf("message length : %ld\n", messagelength);
                        //  poi leggo il messaggio della giusta lunghezza
                        message = malloc(sizeof(char) * (messagelength));
                        if ((n = readn(i, message, messagelength)) == -1)
                        {

                            perror("readn");
                            free(message);
                            break;
                        }
                        // printf("message  : %s\n", message);

                        // creo un nuovo nodo con i campi giusti
                        temp = newNode(result, message);

                        // devo inserire ordinatamente in lista temp
                        insertion_sort(&head, temp);

                        // DEBUG
                        // printList(head);

                        free(message);
                    }
                }
            }
        }
    }

    printList(head);
    fflush(stdout);
    free_list(head);

    // printf("collector FINITO\n");
    // fflush(stdout);

    unlink(SOCKNAME);

    exit(EXIT_SUCCESS);
}
