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
    print_stats();
    print_change(id, WRITER_NAME, DEPARTURE_NAME);
    //unlock semaphore when finished
    sem_post(&semaphore);
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
    //unlock semaphore when the last reader leaves
    if(readersIn == 0)
        sem_post(&semaphore);

    print_stats();
    print_change(id, READER_NAME, DEPARTURE_NAME);
    
    //unlock mutex when finished
    pthread_mutex_unlock(&mutex);
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

int main(int argc, char* argv[])
{
    int variant = WRITER_STARVATION;
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
            printf("No implemented yet :/\n");
            break;
        case NO_STARVATION:
            printf("No implemented yet :/\n");
            break;
        default:
            printf("This variant does not exist :(\n");
    }
    return 0;
}