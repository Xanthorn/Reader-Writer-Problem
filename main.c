#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_READ_TIME 10;
#define MAX_WRITE_TIME 6;

const char READER_NAME[] = "Reader";
const char WRITER_NAME[] = "Writer";
const char ARRIVAL_NAME[] = "comes in";
const char DEPARTURE_NAME[] ="leaves";


const int readerCount = 15;
const int writerCount = 4;
volatile int readersIn = 0;
volatile int writersIn = 0;
pthread_mutex_t mutex;
sem_t semaphore;

void print_stats()
{
    printf("ReaderQ: %-2d WriterQ: %-2d [in: R:%-2d W:%-2d] ", readerCount - readersIn, writerCount - writersIn, readersIn, writersIn);
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

int main(int argc, char* argv[])
{
    int totalCount = readerCount + writerCount;
    pthread_t threads[totalCount];
    int i;
    srand(time(NULL));

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
    return 0;
}