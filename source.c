/***********************************************************************
* Program:
*    Lab ProducerConsumer
*    Brother Jones, CS 345
* Author:
*    Brian Woodruff
* Summary:
*    A threaded semaphore-based consumer/producer problem based off of
*        a similar problem from section 5.7.1 in our textbook.
*    User enters number of consumers producers and program running length.
*    Each consumer and producer runs in their own thread.
*    Mutex and Semaphores are used to coordinate consumers and producers.
*    The data produced/consumed is an integer from 0-999.
*    Producers and consumers display their id and data when handled.
*    Program terminates after number of seconds given from command line.
************************************************************************/
#include <stdlib.h> /* rand() */
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> /* sleep() */
#include <stdlib.h>
#include <stdio.h>

#define BUFFER_SIZE 5 /* number of resources available */

typedef int bufferItem;

bufferItem buffer[BUFFER_SIZE]; /* resources */
int head = 0; /* beginning of buffer */
int tail = 0; /* end of buffer */

pthread_mutex_t mutex;
sem_t empty; /* semaphore */
sem_t full; /* semaphore */

/***********************************************************************
* Attempts to add item to front of the buffer
* Returns 0 if successful
************************************************************************/
int enqueue(bufferItem item)
{
   if (head == ((tail - 1 + BUFFER_SIZE) % BUFFER_SIZE))
      return -1; /* queue is full */

   buffer[head] = item;
   head = (head + 1) % BUFFER_SIZE;
   return 0;
}

/***********************************************************************
* Attempts to remove last element from buffer and store it in item
* Returns 0 if successful
************************************************************************/
int dequeue(bufferItem *item)
{
   if (head == tail)
      return -1; /* queue is empty */

   *item = buffer[tail];
   tail = (tail + 1) % BUFFER_SIZE;
   return 0;
}

/***********************************************************************
* Producer waits a random amount of time before attempting to produce
*     an item and store it in the buffer
* Repeats
************************************************************************/
void *producer(void *param)
{
   bufferItem itemProduced;
   int *id = (int*)param;

   while (1) /* run until cancelled */
   {
      /* sleep for a random period of time */
      usleep(rand() % 150000);
      /* generate a random number */
      itemProduced = rand() % 1000; /* range is 0-999 */

      sem_wait(&empty); /* the consumer will call signal(empty) */
      pthread_mutex_lock(&mutex);

      /* START: Critical section */
      if (enqueue(itemProduced))
         fprintf(stderr, "error!-Buffer probably full\n");
      else
         printf("   %3d      P%d\n", itemProduced, *id);
      /* END: Critical section */

      pthread_mutex_unlock(&mutex);
      sem_post(&full); /* the consumer called wait(full) */
   }
}

/***********************************************************************
* Consumer waits a random amount of time before attempting to consume
*     an item from the buffer
* Repeats
************************************************************************/
void *consumer(void *param)
{
   bufferItem consumedItem;
   int *id = (int*)param;

   while (1) /* run until cancelled */
   {
      /* sleep for a random period of time */
      usleep(rand() % 150000);

      sem_wait(&full); /* the producer will call signal(full) */
      pthread_mutex_lock(&mutex);

      /* START: Critical section */
      if (dequeue(&consumedItem))
         fprintf(stderr, "error!-Buffer probably empty\n");
      else
         printf("                   %3d      C%d\n", consumedItem, *id);
      /* END: Critical section */

      pthread_mutex_unlock(&mutex);
      sem_post(&empty); /* the producer called wait(empty) */
   }
}

/***********************************************************************
* Attempts to initialize mutex and semaphore variables
* Program exits if unsuccessful
************************************************************************/
void initializeMutexAndSemaphores()
{
   int s;

   /* Initialize 'mutex' mutex */
   s = pthread_mutex_init(&mutex, NULL);
   if (s != 0)
   {
      fprintf(stderr, "Couldn't initialize 'mutex' mutex\n");
      exit(-1);
   }

   /* Initialize 'empty' semaphore */
   s = sem_init(&empty, 0, BUFFER_SIZE - 1);
   if (s != 0)
   {
      fprintf(stderr, "Couldn't initialize 'empty' semaphore\n");
      exit(-1);
   }

   /* Initialize 'full' semaphore */
   s = sem_init(&full, 0, 0);
   if (s != 0)
   {
      fprintf(stderr, "Couldn't initialize 'full' semaphore\n");
      exit(-1);
   }
}

/***********************************************************************
* Attempts to create producer and consumer thread arrays
* Program exits if unsuccessful
************************************************************************/
void createThreads(pthread_t **pThreads, int pNum,
                   pthread_t **cThreads, int cNum)
{
   int i;
   int s;

   /* Create producer threads */
   *pThreads = (pthread_t*)malloc(pNum * sizeof (pthread_t));
   if (pThreads == NULL)
   {
      fprintf(stderr, "malloc error\n");
      exit(-1);
   }
   for (i = 0; i < pNum; i++)
   {
      int *id = (int*)malloc(sizeof (int));
      *id = i + 1; /* Producer id (Not related to process id) */
      s = pthread_create(pThreads[i], NULL, producer, (void*)id);

      /* Check to see if error in thread creation. Exit if an error. */
      if (s != 0)
      {
         fprintf(stderr, "Couldn't create thread\n");
         exit(-1);
      }
   }

   /* Create consumer threads */
   *cThreads = (pthread_t*)malloc(cNum * sizeof (pthread_t));
   if (cThreads == NULL)
   {
      fprintf(stderr, "malloc error\n");
      exit(-1);
   }
   for (i = 0; i < cNum; i++)
   {
      int *id = (int*)malloc(sizeof (int));
      *id = i + 1; /* Consumer id (Not related to process id) */
      s = pthread_create(cThreads[i], NULL, consumer, (void*)id);

      /* Check to see if error in thread creation. Exit if an error. */
      if (s != 0)
      {
         fprintf(stderr, "Couldn't create thread\n");
         exit(-1);
      }
   }
}

/***********************************************************************
* Attempts to cancel all consumer and producer threads
* Program exits if unsuccessful
************************************************************************/
void cancelThreads(pthread_t **pThreads, int pNum,
                   pthread_t **cThreads, int cNum)
{
   int i;

   /* Cancel all producer threads */
   for (i = 0; i < pNum; i++)
   {
      int s = pthread_cancel(*pThreads[i]);
      if (s != 0)
      {
         fprintf(stderr, "Couldn't cancel thread\n");
         exit(-1);
      }
   }

   /* Cancel all consumer threads */
   for (i = 0; i < cNum; i++)
   {
      int s = pthread_cancel(*cThreads[i]);
      if (s != 0)
      {
         fprintf(stderr, "Couldn't cancel thread\n");
         exit(-1);
      }
   }
}

/***********************************************************************
* Get command line arguments for number of threads and sleep time
* Initialize mutex and semaphore variables
* Creates producer and consumer threads
* Sleep
* Cancel threads
* Exit
************************************************************************/
int main(int argc, char **argv)
{
   int i;

   pthread_t *producers;
   pthread_t *consumers;

   /* 1. Check and get command line arguments argv[1], argv[2], argv[3] */
   if (argc != 4)
   {
      printf("Use:\n a.out [run time] [# producers] [# consumers]\n");
      return 0;
   }
   int sleepTime = atoi(argv[1]); /* number of seconds to run program */
   int nProducers = atoi(argv[2]); /* number of producers */
   int nConsumers = atoi(argv[3]); /* number of consumers */

   /* 2. Initialize buffer [good for error checking but not really needed]*/
   for (i = 0; i < BUFFER_SIZE; i++)
      buffer[i] = 0;

   /* 3. Initialize the mutex lock and semaphores */
   initializeMutexAndSemaphores();

   printf("Produced  by P#  Consumed  by C#\n");
   printf("========  =====  ========  =====\n");

   /* 4. Create producer threads(s) */
   /* 5. Create consumer threads(s) */
   createThreads(&producers, nProducers, &consumers, nConsumers);

   /* 6. Sleep [ to read manual page, do: > man 3 sleep ] */
   sleep(sleepTime);

   /* 7. Cancel threads [ pthread_cancel() ] */
   cancelThreads(&producers, nProducers, &consumers, nConsumers);

   /* 8. Exit */
   free(producers); /* clean up */
   free(consumers);
   return 0;
}