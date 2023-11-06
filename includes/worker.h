/**************************/
//  header file worker.h   /
/*========================*/

/**
 * @brief: questo header file contiene la dichiarazione della funzione compute che è la funzione principale
 * eseguita dai thread worker del threadpool e che esegue la computazione richiesta sui dati del file binario
 * passato come argomento alla funzione.
 */

// include
#include <stdio.h>
#include <threadpool.h>

/**
 * @brief: la funzione compute implementa il lavoro che un thread worker deve compiere,
 *         la funzione prende in ingresso il pathname di un file regolare, il file viene interpretato
 *         come un file binario contenente N long, i long contenuti nel file vengono letti uno ad uno e viene
 *         eseguita una computazione. Il calcolo che deve essere effettuato su ogni file è il seguente:
 *         result = sommatoria (per i che va da 0 a N-1) di (i * file[i])
 *         N è il numero di long presenti nel file e file[i] è l' i-esimo long
 *         Se la computazione è stata eseguita con successo il risultato viene memorizzato nella variabile puntata
 *         dall' argomento result.
 * @param file_name --> pathname del file su cui operare
 * @param result --> puntatore a long dove memorizzare il risultato della computazione
 * @return: 0 (int) se la funzione è stata eseguita con successo e  il risultato della computazione nella variabile puntata da result
 *         -1 altrimenti;
 */
int compute(char *file_name, long *result);