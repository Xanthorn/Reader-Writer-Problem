#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define WRITER_STARVATION 0
#define READER_STARVATION 1
#define NO_STARVATION 2

#define MAX_READ_TIME 10
#define MAX_WRITE_TIME 6

const char READER_NAME[] = "Reader";
const char WRITER_NAME[] = "Writer";
const char ARRIVAL_NAME[] = "comes in";
const char DEPARTURE_NAME[] ="leaves";

volatile int readerQueue = 15;
volatile int writerQueue = 4;

volatile int readersIn = 0;
volatile int writersIn = 0;
pthread_mutex_t mutex;
sem_t semaphore;
pthread_cond_t canRead;
pthread_cond_t canWrite;

void print_stats()
{
    printf("ReaderQ: %-2d WriterQ: %-2d [in: R:%-2d W:%-2d] ", readerQueue, writerQueue, readersIn, writersIn);
    fflush(stdout);
}

void print_change(int id, char name[], char action[])
{
    printf("%s %-2d %s\n", name, id, action);
    fflush(stdout);
}

#pragma region writer_starvation
void *writer(void* arg)
{
    int id = *(int *)arg;
    int sleepTime = rand() % MAX_WRITE_TIME + 1;

    //block semaphore
    sem_wait(&semaphore);
    writersIn++;
    writerQueue--;
    print_stats();
    print_change(id, WRITER_NAME, ARRIVAL_NAME);

    //simulate work
    sleep(sleepTime);

    writersIn--;
    writerQueue++;
    print_stats();
    print_change(id, WRITER_NAME, DEPARTURE_NAME);
    //unlock semaphore when finished
    sem_post(&semaphore);

    //simulate waiting before coming again
    sleep(sleepTime);
    writer(arg);
}

void *reader(void* arg)
{
    int id = *(int *)arg;
    int sleepTime = rand() % MAX_READ_TIME + 1;

    //lock before incrementing readersIn;
    pthread_mutex_lock(&mutex);
    readersIn++;
    readerQueue--;
    //block writer access when reader come in
    if(readersIn == 1)
        sem_wait(&semaphore);

    print_stats();
    print_change(id, READER_NAME, ARRIVAL_NAME);

    //unlock mutex when finished
    pthread_mutex_unlock(&mutex);

    //simulate work
    sleep(sleepTime);

    //lock before decrementing readersIN;
    pthread_mutex_lock(&mutex);
    readersIn--;
    readerQueue++;
    //unlock semaphore when the last reader leaves
    if(readersIn == 0)
        sem_post(&semaphore);

    print_stats();
    print_change(id, READER_NAME, DEPARTURE_NAME);
    
    //unlock mutex when finished
    pthread_mutex_unlock(&mutex);

    //simulate waiting before coming again
    sleep(sleepTime);
    reader(arg);
}

void writer_starvation()
{
    int totalCount = readerQueue + writerQueue;
    int readerCount = readerQueue;
    int writerCount = writerQueue;

    pthread_t threads[totalCount];

    int i;  
    if (pthread_mutex_init(&mutex, NULL) != 0)
        return 1;
    if (sem_init(&semaphore, 0, 1) != 0)
        return 2;

    print_stats();
    printf("\n");
    fflush(stdout);
    for(i = 0; i < totalCount; i++)
    {
        int *id = malloc(sizeof(int));
        if(i < readerCount)
        {
            *id = i;
            pthread_create(&threads[i], NULL, reader, (void*) id);
        }
        else
        {
            *id = i - readerCount;
            pthread_create(&threads[i], NULL, writer, (void*) id);
        }

        //simulate time to enter
        sleep(1);
    }

    for(i = 0; i < totalCount; i++)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    sem_destroy(&semaphore);
}
#pragma endregion writer_starvation

#pragma region reader_starvation


void *rs_reader(void* arg)
{
    int id = *(int *)arg;
    int sleepTime = rand() % MAX_READ_TIME + 1;
    
    pthread_mutex_lock(&mutex);

    // wait if a writer is in or are still writers in the queue
    if(writersIn == 1 || writerQueue > 0)
        pthread_cond_wait(&canRead, &mutex);
    
    // start reading and print arrival message
    readersIn++;
    readerQueue--;
    print_stats();
    print_change(id, READER_NAME, ARRIVAL_NAME);

    pthread_mutex_unlock(&mutex);


    //simulate work
    sleep(sleepTime);


    pthread_mutex_lock(&mutex);

    // end reading and print departure message
    readersIn--;
    print_stats();
    print_change(id, READER_NAME, DEPARTURE_NAME);
    pthread_mutex_unlock(&mutex);
    // if any writers are waiting and library is empty then send a signal to waiting writer
    if(writerQueue > 0 && readersIn == 0)
        pthread_cond_signal(&canWrite);
}

void *rs_writer(void* arg)
{
    int id = *(int *)arg;
    int sleepTime = rand() % MAX_WRITE_TIME + 1;
    
    pthread_mutex_lock(&mutex);

    // wait if there is another writer in the library or if there is a reader in the library
    if(writersIn == 1 || readersIn > 0)
        pthread_cond_wait(&canWrite, &mutex);

    // start writing and print arrival message
    writersIn++;
    writerQueue--;
    print_stats();
    print_change(id, WRITER_NAME, ARRIVAL_NAME);

    pthread_mutex_unlock(&mutex);    


    //simulate work
    sleep(sleepTime);


    // end writing and print departure message
    pthread_mutex_lock(&mutex);
    writersIn--;
    print_stats();
    print_change(id, WRITER_NAME, DEPARTURE_NAME);
    pthread_mutex_unlock(&mutex);
    // if any writers are still waiting and library is empty then send signal to waiting writer
    if(writerQueue > 0 && writersIn == 0)
        pthread_cond_signal(&canWrite);
    // else send broadcast to all waiting readers
    else
        pthread_cond_broadcast(&canRead);
}

void reader_starvation()
{
    int totalCount = readerQueue + writerQueue;
    int readerCount = readerQueue;
    int writerCount = writerQueue;

    pthread_t threads[totalCount];
    int i;  
    if (pthread_mutex_init(&mutex, NULL) != 0)
        return 1;
    if (pthread_cond_init(&canRead, NULL))
        return 2;
    if (pthread_cond_init(&canWrite, NULL))
        return 3;
    
    print_stats();
    printf("\n");
    fflush(stdout);
    for(i = 0; i < totalCount; i++)
    {
        int *id = malloc(sizeof(int));
        if(i < readerCount)
        {
            *id = i;
            pthread_create(&threads[i], NULL, rs_reader, (void*) id);
        }
        else
        {
            *id = i - readerCount;
            pthread_create(&threads[i], NULL, rs_writer, (void*) id);
        }

        //simulate time to enter
        sleep(1);
    }
    for(i = 0; i < totalCount; i++)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&canWrite);
    pthread_cond_destroy(&canRead);
}

#pragma endregion reader_starvation

#pragma region no_starvation
// initializes conditional variables and mutex
void ns_init()
{
    pthread_cond_init(&canRead, NULL);
    pthread_cond_init(&canWrite, NULL);
    pthread_mutex_init(&mutex, NULL);
}

// signals that a reader wants to access library, which he does upon signal from other thread on canRead variable
void ns_start_reading(int id)
{
    pthread_mutex_lock(&mutex);

    // wait if writers are in or waiting
    if (writersIn == 1 || writerQueue > 0)
    {
        pthread_cond_wait(&canRead, &mutex);
    }

    // start reading and broadcast that reading is available
    readerQueue--;
    readersIn++;
    print_stats();
    print_change(id, READER_NAME, ARRIVAL_NAME);
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&canRead);
}

// signals that a reader is leaving, which means a writer can enter (only if no readers reside)
void ns_stop_reading(int id)
{
    pthread_mutex_lock(&mutex);
    readersIn--;
    // if this is the last reader, signal that a writer can enter
    if (readersIn == 0)
    {
        pthread_cond_signal(&canWrite);
    }

    // print departure message
    print_stats();
    print_change(id, READER_NAME, DEPARTURE_NAME);
    pthread_mutex_unlock(&mutex);
}

// signals that a writer wants to access library, which he does upon signal from other thread on canWrite variable
void ns_start_writing(int id)
{
    pthread_mutex_lock(&mutex);

    // wait if a writer is in or readers are waiting
    if (writersIn == 1 || readersIn > 0)
    {
        pthread_cond_wait(&canWrite, &mutex);
    }

    // start writing and print arrival message
    writerQueue--;
    writersIn = 1;
    print_stats();
    print_change(id, WRITER_NAME, ARRIVAL_NAME);
    pthread_mutex_unlock(&mutex);
}

// signals that a writer is leaving, which means a reader/writer can enter
void ns_stop_writing(int id)
{
    pthread_mutex_lock(&mutex);
    writersIn = 0;

    // if no readers are waiting, signal a writer to enter, else signal reader
    if (readerQueue == 0)
    {
        pthread_cond_signal(&canWrite);
    }
    else
    {
        pthread_cond_signal(&canRead);
    }

    // print departure message
    print_stats();
    print_change(id, WRITER_NAME, DEPARTURE_NAME);
    pthread_mutex_unlock(&mutex);
}

// execute reading procedure
void *ns_reader(void* arg)
{
    int id = *(int*)arg;
    int sleepTime = rand() % MAX_READ_TIME + 1;
    sleep(1);
    ns_start_reading(id);
    
    // simulate reading
    sleep(sleepTime);
    
    ns_stop_reading(id);
}

// execute writing procedure
void *ns_writer(void* arg)
{
    int id = *(int*)arg;
    int sleepTime = rand() % MAX_WRITE_TIME + 1;
    sleep(1);
    ns_start_writing(id);

    // simulate writing
    sleep(sleepTime);

    ns_stop_writing(id);
}

void no_starvation()
{
    int totalCount = readerQueue + writerQueue;
    int readerCount = readerQueue;
    int writerCount = writerQueue;
    int readerIterator;
    int writerIterator;
    int i;
    pthread_t threads[totalCount];

    ns_init();

    print_stats();
    printf("\n");
    fflush(stdout);
    readerIterator = 0;
    writerIterator = 0;
    i = 0;

    // create reader and writer threads
    while(readerIterator + writerIterator < totalCount)
    {
        int *id;

        if (readerIterator < readerCount)
        {
            id = malloc(sizeof(int));
            *id = i++;
            readerIterator++;
            pthread_create(&threads[*id], NULL, ns_reader, (void*) id);

            //simulate time to enter
            sleep(1);
        }

        if (writerIterator < writerCount)
        {
            id = malloc(sizeof(int));
            *id = i++;
            writerIterator++;
            pthread_create(&threads[*id], NULL, ns_writer, (void*) id);
            
            //simulate time to enter
            sleep(1);
        }
    }

    for(i = 0; i < totalCount; i++)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&canWrite);
    pthread_cond_destroy(&canRead);
}
#pragma endregion no_starvation

int main(int argc, char* argv[])
{
    int variant = READER_STARVATION;
    if(argc == 4)
    {
        readerQueue = atoi(argv[1]);
        writerQueue = atoi(argv[2]);
        variant = atoi(argv[3]);
    }
    srand(time(NULL));
    
    switch(variant)
    {
        case WRITER_STARVATION:
            writer_starvation();
            break;
        case READER_STARVATION:
            reader_starvation();
            break;
        case NO_STARVATION:
            no_starvation();
            break;
        default:
            printf("This variant does not exist :(\n");
    }
    return 0;
}