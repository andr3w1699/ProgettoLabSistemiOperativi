/**********************************/
//  implementation file worker.c   /
/*================================*/

// include
#include <util.h>
#include <worker.h>

/**
 * @brief: la funzione compute implementa il lavoro che un thread worker deve compiere
 *         la funzione prende in ingresso il pathname di un file regolare, il file viene interpretato come un file binario contenente N long
 *         i long contenuti nel file vengono letti uno ad uno e con essi viene eseguita una computazione
 *         Il calcolo che deve essere effettuato su ogni file è il seguente result = sommatoria (per i che va da 0 a N-1) di (i * file[i])
 *         N è il numero di long presenti nel file e file[i] è l' i-esimo long
 * @param file_name il pathname del file su cui operare
 * @param result puntatore a long dove memorizzare il risultato della computazione
 * @return: 0 se la funzione è stata eseguita con successo ed il risultato della computazione viene memorizzato nella variabile puntata da result
 *          -1 se si è verificato un errore
 */

int compute(char *file_name, long *result)
{
    // controllo che gli argomenti passati non siano null
    if (file_name == NULL || result==NULL)
    {
        perror("ERROR: arguments not valid in compute function");
        return -1; // segnalo l'errore al chiamante 
    }

    // file da aprire
    FILE *fptr;

    // apro il file in lettura
    fptr = fopen(file_name, "r");

    /* from the man ...
    FILE *fopen(const char *pathname, const char *mode);
    DESCRIPTION
       The fopen() function opens the file whose name is the string pointed to by pathname and associates a
       stream with it.

       The argument mode points to a string beginning with one of the following  sequences  (possibly  fol‐
       lowed by additional characters, as described below):

       
       r      Open text file for reading.  The stream is positioned at the beginning of the file.

       r+     Open for reading and writing.  The stream is positioned at the beginning of the file.

       w      Truncate  file  to  zero length or create text file for writing.  The stream is positioned at
              the beginning of the file.

       w+     Open for reading and writing.  The file is created if it does  not  exist,  otherwise  it  is
              truncated.  The stream is positioned at the beginning of the file.

       a      Open  for appending (writing at end of file).  The file is created if it does not exist.  The
              stream is positioned at the end of the file.

       a+     Open for reading and appending (writing at end of file).  The file is created if it does  not
              exist.   Output  is always appended to the end of the file.  POSIX is silent on what the ini‐
              tial read position is when using this mode.  For glibc, the initial file position for reading
              is  at  the  beginning  of the file, but for Android/BSD/MacOS, the initial file position for
              reading is at the end of the file.
    

       ...
    */

    if (fptr == NULL)
    {
        perror("ERROR with fopen in compute function");
        return -1;
    }

    // <uso del file>

    // variabili di supporto
    long tmp;
    long sum = 0;
    long i = 0;
    int r;

    // riazzero errno
    errno = 0;
    // leggo un long alla volta dal file binario 
    /*
    * man fread 
    * size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    * size_t fwrite(const void *ptr, size_t size, size_t nmemb,FILE *stream);
    * DESCRIPTION
       The  function fread() reads nmemb items of data, each size bytes long, from the stream pointed to by
       stream, storing them at the location given by ptr.

       The function fwrite() writes nmemb items of data, each size bytes long, to the stream pointed to  by
       stream, obtaining them from the location given by ptr.

       For nonlocking counterparts, see unlocked_stdio(3).

       RETURN VALUE
       On success, fread() and fwrite() return the number of items read or written. This number equals the
       number of bytes transferred only when size is 1.  If an error occurs, or the  end  of  the  file  is
       reached, the return value is a short item count (or zero). 

       The  file  position indicator for the stream is advanced by the number of bytes successfully read or
       written.

       fread() does not distinguish between end-of-file and error, and callers must use  feof(3)  and  fer‐
       ror(3) to determine which occurred.
    */
    while ((r = fread(&tmp, sizeof(long), 1, fptr)) != 0)
    {
        sum += i * tmp;
        i++;
    }
    if (errno != 0)
    {
        int errtemp = errno;
        // stampa errore
        perror("error in fread in compute function");
        fclose(fptr);
        errno = errtemp;
        return -1;
    }

    fclose(fptr);
    // printf("File : %s --> result :%ld\n", file_name, sum);

    *result = sum;
    
    return 0; //success
}