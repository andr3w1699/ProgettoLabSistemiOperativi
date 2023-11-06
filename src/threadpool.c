/**
 * @file threadpool.c
 * @brief File di implementazione dell'interfaccia threadpool.h
 */

// include
#include <util.h>
#include <communication.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <threadpool.h>

/**
 * @function void *workerpool_thread(void *threadpool)
 * @brief funzione eseguita dal thread worker che appartiene al pool
 */
static void *workerpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool; // cast
    taskfun_t task;                                  // generic task

    pthread_t self = pthread_self(); // restituisce l' identificatore del thread (lo stesso restituito dalla pthread_create)
    int myid = -1;

    do
    {
        for (int i = 0; i < pool->numthreads; ++i)
            if (pthread_equal(pool->threads[i], self))
            {
                myid = i; // indice nell' array dei thread di questo thread
                break;
            }
    } while (myid < 0);

    // ciascun thread worker del threadpool ha una connessione col processo collector

    // stabilisco la connessione

    // creo e connetto il socket
    struct sockaddr_un serv_addr;
    int serverfd;
    SYSCALL_EXIT("socket", serverfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket", "");
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, UNIX_PATH_MAX);
    // printf("%s\n",serv_addr.sun_path );

    while (connect(serverfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        if (!pool->exiting && errno == ENOENT)
            msleep(100); /* sock non esiste */
        else
        {
            close(serverfd);
            return NULL;
        }
    }

    // sono connesso

    // acquisisco la lock
    LOCK_RETURN(&(pool->lock), NULL);

    for (;;)
    {

        // in attesa di un messaggio, controllo spurious wakeups.
        while ((pool->count == 0) && (!pool->exiting))
        {                                                             // finchè non ci sono task e non devo uscire
            pthread_cond_wait(&(pool->cond_consumer), &(pool->lock)); // mi metto in attesa sulla variabile di condizione (not-empty)
        }

        if (pool->exiting > 1)
        {
            close(serverfd);
            break; // exit forzato, esco immediatamente
        }

        if (pool->exiting == 1 && !pool->count)
        {
            close(serverfd);
            break; // devo uscire E NON ci sono messaggi pendenti ALLORA ESCO
        }
        // nuovo task
        task.fun = pool->pending_queue[pool->head].fun; // prendo la funzione da eseguire dal task in testa alla coda
        task.arg = pool->pending_queue[pool->head].arg; // prendo l'argomento dal task in testa alla coda

        pool->head++;
        pool->count--;                                                       // sposto il puntatore alla testa, diminuisco il contatore dei task pendenti
        pool->head = (pool->head == abs(pool->queue_size)) ? 0 : pool->head; // la coda è implementata circolarmente

        pool->taskonthefly++; // incremento il contatore dei task serviti al momento

        int r;
        if ((r = pthread_cond_signal(&(pool->cond_producer))) != 0)
        { // faccio una signal per svegliare un producer in attesa xk ho liberato un posto nella coda
            UNLOCK_RETURN(&(pool->lock), NULL);
            close(serverfd);
            errno = r;
            return NULL;
        }

        UNLOCK_RETURN(&(pool->lock), NULL); // rilascio la lock xk non ho più bisogno della mutua esclusione

        // variable to store the result of the computation
        long sum;
        // return value of the function
        int ret_val;
        // eseguo la funzione, passo come argomento il pathname del file su cui lavorare e il puntatore a dove salvare il risultato
        ret_val = (*(task.fun))(task.arg, &sum);

        if (ret_val != 0)
        {
            perror("error with the compute function");
            close(serverfd);
            return NULL;
        }

        // int notused;
        // SYSCALL_EXIT("connect", notused, connect(serverfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect", "");

        /*******************************/
        /* communication of the result */
        /*******************************/

        // lenght of the message with the filename
        long message_length = strlen(task.arg) + 1;

        long codice = 0;                         // code 0 --> message with (result, filename) to store
        writen(serverfd, &codice, sizeof(long)); // send the code

        writen(serverfd, &sum, sizeof(long)); // send result
        // printf("%ld\n", sum );
        // fflush(stdout);
        writen(serverfd, &message_length, sizeof(long)); // send a long with the lenght of the filename
        // printf("%ld\n", message_length );
        // fflush(stdout);
        // send filename
        writen(serverfd, task.arg, sizeof(char) * message_length);
        // printf("%s\n", task.arg );
        // fflush(stdout);

        free(task.arg);
        // riacquisisco la lock
        LOCK_RETURN(&(pool->lock), NULL);
        pool->taskonthefly--; // diminuisco il contatore dei task serviti correntemente
    }
    UNLOCK_RETURN(&(pool->lock), NULL); // rilascio la lock

    // fprintf(stderr, "thread %d exiting\n", myid);
    return NULL;
}

static int freePoolResources(threadpool_t *pool)
{
    if (pool->threads)
    {
        free(pool->threads);
        free(pool->pending_queue);

        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->cond_producer));
        pthread_cond_destroy(&(pool->cond_consumer));
    }
    free(pool);
    return 0;
}

/**
 * @function createThreadPool
 * @brief Crea un oggetto thread pool.
 * @param numthreads è il numero di thread del pool
 * @param pending_size è la size delle richieste che possono essere pendenti. Questo parametro è 0 se si vuole utilizzare un modello per il pool con 1 thread 1 richiesta, cioe' non ci sono richieste pendenti.
 *
 * @return un nuovo thread pool oppure NULL ed errno settato opportunamente
 */
threadpool_t *createThreadPool(int numthreads, int pending_size)
{
    // controllo che i parametri siano validi
    if (numthreads <= 0 || pending_size < 0)
    {
        errno = EINVAL;
        return NULL;
    }

    // alloco spazio per un oggetto threadpool
    threadpool_t *pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    if (pool == NULL)
        return NULL;

    // condizioni iniziali
    pool->numthreads = 0;
    pool->taskonthefly = 0;
    pool->queue_size = (pending_size == 0 ? -1 : pending_size);
    pool->head = pool->tail = pool->count = 0;
    pool->exiting = 0;

    /* Allocate thread and task queue */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * numthreads);
    if (pool->threads == NULL)
    {
        free(pool);
        return NULL;
    }
    pool->pending_queue = (taskfun_t *)malloc(sizeof(taskfun_t) * abs(pool->queue_size));
    if (pool->pending_queue == NULL)
    {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    // initialize mutex & condition variable
    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) || (pthread_cond_init(&(pool->cond_producer), NULL) != 0) || (pthread_cond_init(&(pool->cond_consumer), NULL)))
    {
        free(pool->threads);
        free(pool->pending_queue);
        free(pool);
        return NULL;
    }

    for (int i = 0; i < numthreads; i++)
    {
        if (pthread_create(&(pool->threads[i]), NULL, workerpool_thread, (void *)pool) != 0)
        {
            /* errore fatale, libero tutto forzando l'uscita dei threads */
            destroyThreadPool(pool, 1);
            errno = EFAULT;
            return NULL;
        }
        pool->numthreads++;
    }
    return pool;
}

/**
 * @function destroyThreadPool
 * @brief stoppa tutti i thread e distrugge l'oggetto pool
 * @param pool  oggetto da liberare
 * @param force se 1 forza l'uscita immediatamente di tutti i thread e libera subito le risorse, se 0 aspetta che i thread finiscano tutti e soli i lavori pendenti (non accetta altri lavori).
 *
 * @return 0 in caso di successo <0 in caso di fallimento ed errno viene settato opportunamente
 */
int destroyThreadPool(threadpool_t *pool, int force)
{
    // controllo i parametri
    if (pool == NULL || force < 0)
    {
        errno = EINVAL;
        return -1;
    }

    LOCK_RETURN(&(pool->lock), -1); // acquisisco la lock

    pool->exiting = 1 + force; // imposto la variabile di terminazione

    if (pthread_cond_broadcast(&(pool->cond_consumer)) != 0)
    { // risveglio tutti i thread bloccati sulla variabile di condizione
        UNLOCK_RETURN(&(pool->lock), -1);
        errno = EFAULT;
        return -1;
    }

    if (pthread_cond_broadcast(&(pool->cond_producer)) != 0)
    { // risveglio tutti i producer bloccati sulla variabile di condizione
        UNLOCK_RETURN(&(pool->lock), -1);
        errno = EFAULT;
        return -1;
    }

    UNLOCK_RETURN(&(pool->lock), -1); // rilascio la lock

    for (int i = 0; i < pool->numthreads; i++)
    { // aspetto la terminazione di tutti i thread worker
        if (pthread_join(pool->threads[i], NULL) != 0)
        {
            errno = EFAULT;
            UNLOCK_RETURN(&(pool->lock), -1);
            return -1;
        }
    }
    freePoolResources(pool); // libero le risorse
    return 0;
}

/**
 * @function addToThreadPool
 * @brief aggiunge un task al pool
 * @param pool oggetto thread pool
 * @param fun  funzione da eseguire per eseguire il task
 * @param arg  argomento della funzione (pathname del file su cui si deve lavorare )
 * @return 0 se successo, 1 se non ci sono thread disponibili e/o la coda è piena, -1 in caso di fallimento, errno viene settato opportunamente.
 */
int addToThreadPool(threadpool_t *pool, int (*f)(void *, void *), void *arg)
{
    if (pool == NULL || f == NULL || arg == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    LOCK_RETURN(&(pool->lock), -1);
    int queue_size = abs(pool->queue_size);
    int nopending = (pool->queue_size == -1); // non dobbiamo gestire messaggi pendenti

    // finchè la coda è piena e non devo uscire mi sospendo
    while (pool->count >= queue_size && (!pool->exiting))
    {
        pthread_cond_wait(&(pool->cond_producer), &(pool->lock));
    }

    // in fase di uscita
    if (pool->exiting)
    {
        UNLOCK_RETURN(&(pool->lock), -1);
        return 1; // esco con valore "coda piena" (non ho aggiunto)
    }

    if (pool->taskonthefly >= pool->numthreads) // tutti i thread sono occupati
    {
        if (nopending)
        {
            // tutti i thread sono occupati e non si gestiscono task pendenti
            assert(pool->count == 0);

            UNLOCK_RETURN(&(pool->lock), -1);
            return 1; // esco con valore "coda piena"
        }
    }
    // inserisco in coda
    pool->pending_queue[pool->tail].fun = (int (*)(void *, void *))f;
    char *str_temp = (char *)malloc(sizeof(char) * NAME_MAX);
    if (str_temp == NULL)
    {
        perror("malloc");
        UNLOCK_RETURN(&(pool->lock), -1);
        return -1;
    }
    pool->pending_queue[pool->tail].arg = strcpy(str_temp, (char *)arg);
    pool->count++; // incremento il numero dei task pendenti
    pool->tail++;  // incremento il puntatore alla coda
    if (pool->tail >= queue_size)
        pool->tail = 0;

    int r;
    if ((r = pthread_cond_signal(&(pool->cond_consumer))) != 0)
    { // faccio una signal per svegliare un worker in attesa
        UNLOCK_RETURN(&(pool->lock), -1);
        errno = r;
        return -1;
    }

    UNLOCK_RETURN(&(pool->lock), -1);
    return 0;
}
