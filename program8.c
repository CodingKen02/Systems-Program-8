/*
----------------------------------------------------------
Program: program8.c
Date: October 17, 2023
Student Name & NetID: Kennedy Keyes kfk38
Description: This program uses multiple threads, mutexes, and condition variables.
It implements a consumer thread and two producer threads. The consumer and both 
producer threads are spawned by the main thread. The program monitors the pitch
and roll of two accelerometers, reporting when they are outside of the acceptable
range of -20.0 to 20.0. This program reads the angl.dat files.
----------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define STACK_SIZE 100
#define RANGE_LOWER -20.0
#define RANGE_UPPER 20.0

// mutex and condition variables
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

// warning stack
typedef struct 
{
    char thread_name[20];
    char warning_type[10];
    char direction[5];
} Warning;

Warning warning_stack[STACK_SIZE];
int stack_count = 0;
int condition_flag = 0;
int thread_counter = 0;

// add a warning to the stack
void addWarning(const char* thread_name, const char* warning_type, const char* direction) 
{
    pthread_mutex_lock(&mutex1);
    if (stack_count < STACK_SIZE) 
    {
        strcpy(warning_stack[stack_count].thread_name, thread_name);
        strcpy(warning_stack[stack_count].warning_type, warning_type);
        strcpy(warning_stack[stack_count].direction, direction);
        stack_count++;
    }
    pthread_mutex_unlock(&mutex1);
}

// producer thread
void* producerThread(void* sensorFileName) 
{
    FILE* sensorFile = fopen((char*)sensorFileName, "r");
    if (sensorFile == NULL) 
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char thread_name[20];
    strcpy(thread_name, (char*)sensorFileName);

    while (1) {
        pthread_mutex_lock(&mutex2);

        double pitch, roll, yaw;
        int result = fscanf(sensorFile, "%lf %lf %lf", &pitch, &roll, &yaw);

        if (result != 3) 
        {
            fclose(sensorFile);
            break;
        }

        if (pitch < RANGE_LOWER || pitch > RANGE_UPPER) 
        {
            addWarning(thread_name, "Pitch", "Outside Range");
        }

        if (roll < RANGE_LOWER || roll > RANGE_UPPER) 
        {
            addWarning(thread_name, "Roll", "Outside Range");
        }

        thread_counter++;

        if (thread_counter == 2) 
        {
            if (stack_count > 0) 
            {
                pthread_mutex_lock(&mutex1);
                condition_flag = 1;
                pthread_cond_signal(&cond1);
                pthread_mutex_unlock(&mutex1);
            }
            thread_counter = 0;
            sleep(1);
        }

        pthread_mutex_unlock(&mutex2);
    }

    pthread_exit(NULL);
}

// consumer thread
void* consumerThreadFunction(void* args) 
{
    while (1) 
    {
        pthread_mutex_lock(&mutex1);
        while (condition_flag == 0) 
        {
            pthread_cond_wait(&cond1, &mutex1);
        }

        for (int i = 0; i < stack_count; i++) 
        {
            printf("Warning from %s: %s value outside acceptable range. Direction: %s.\n", 
            warning_stack[i].thread_name, warning_stack[i].warning_type);
        }

        stack_count = 0;
        condition_flag = 0;
        pthread_mutex_unlock(&mutex1);
    }
}

int main(int argc, char *argv[]) 
{
    // initialize the warning stack    
    memset(warning_stack, 0, sizeof(warning_stack));

    // create the threads
    pthread_t producerThread1, producerThread2, consumerThread;
    char sensor1FileName[] = "angl1.dat";
    char sensor2FileName[] = "angl2.dat";

    if (pthread_create(&producerThread1, NULL, producerThread, sensor1FileName) != 0 ||
        pthread_create(&producerThread2, NULL, producerThread, sensor2FileName) != 0 ||
        pthread_create(&consumerThread, NULL, consumerThreadFunction, NULL) != 0) 
    {
        perror("Error with pthread_create");
        exit(EXIT_FAILURE);
    }

    // join the producer threads
    pthread_join(producerThread1, NULL);
    pthread_join(producerThread2, NULL);

    // cancel and join the consumer thread
    pthread_cancel(consumerThread);
    pthread_join(consumerThread, NULL);

    return 0;
}