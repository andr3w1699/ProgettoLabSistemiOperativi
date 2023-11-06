/**
 * @file threadpool.h
 * @brief Interfaccia per il ThreadPool
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

// include
#include <pthread.h>

/**
 *  @struct taskfun_t
 *  @brief generico task che un thread del threadpool deve eseguire
 *
 *  @var fun Puntatore alla funzione da eseguire
 *  @var arg Argomento della funzione (pathname del file)
 */
typedef struct taskfun_t
{
    int (*fun)(void *, void *);
    void *arg;
} taskfun_t;

/**
 *  @struct threadpool_t
 *  @brief Rappresentazione dell'oggetto threadpool
 */
typedef struct threadpool_t
{
    pthread_mutex_t lock;         // mutua esclusione nell'accesso all'oggetto
    pthread_cond_t cond_producer; // variabile di condizione (not-full) per il produttore
    pthread_cond_t cond_consumer; // variabile di condizione (not-empty) per il consumatore
    pthread_t *threads;           // array di worker id
    int numthreads;               // numero di thread (size dell'array threads)
    taskfun_t *pending_queue;     // coda interna per task pendenti
    int queue_size;               // massima size della coda, puo' essere anche -1 ad indicare che non si vogliono gestire task pendenti
    int taskonthefly;             // numero di task attualmente in esecuzione
    int head, tail;               // riferimenti della coda
    int count;                    // numero di task nella coda dei task pendenti
    int exiting;                  // se > 0 e' iniziato il protocollo di uscita, se 1 il thread aspetta che non ci siano piu' lavori in coda
} threadpool_t;

/**
 * @function createThreadPool
 * @brief Crea un oggetto thread pool.
 * @param numthreads è il numero di thread del pool
 * @param pending_size è la size delle richieste che possono essere pendenti. Questo parametro è 0 se si vuole utilizzare un modello per il pool con 1 thread 1 richiesta, cioe' non ci sono richieste pendenti.
 *
 * @return un nuovo thread pool oppure NULL ed errno settato opportunamente
 */
threadpool_t *createThreadPool(int numthreads, int pending_size);

/**
 * @function destroyThreadPool
 * @brief stoppa tutti i thread e distrugge l'oggetto pool
 * @param pool  oggetto da liberare
 * @param force se 1 forza l'uscita immediatamente di tutti i thread e libera subito le risorse, se 0 aspetta che i thread finiscano tutti e soli i lavori pendenti (non accetta altri lavori).
 *
 * @return 0 in caso di successo <0 in caso di fallimento ed errno viene settato opportunamente
 */
int destroyThreadPool(threadpool_t *pool, int force);

/**
 * @function addToThreadPool
 * @brief aggiunge un task al pool
 * @param pool oggetto thread pool
 * @param fun  funzione da eseguire per eseguire il task
 * @param arg  argomento della funzione (pathname del file su cui si deve lavorare )
 * @return 0 se successo, 1 se non ci sono thread disponibili e/o la coda è piena, -1 in caso di fallimento, errno viene settato opportunamente.
 */
int addToThreadPool(threadpool_t *pool, int (*fun)(void *, void*), void *arg);

#endif /* THREADPOOL_H_ */
