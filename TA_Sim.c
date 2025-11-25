//TA_Sim.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

/****************************************************************************
* Global synchronization objects and shared state
****************************************************************************/
pthread_mutex_t mutex;                  //protects access to shared counters/state
sem_t students_sem;                     //counts students waiting & wakes the TA
        
int num_students = 0;                   //total number of student threads
int num_chairs = 0;                     //number of chairs in the hallway
int waiting_students = 0;               //current number of students waiting for the TA
int students_finished = 0;              //how many students are completely done
int all_done = 0;                       //flag set when all students have finished

#define HELP_REQUESTS_PER_STUDENT 3     //how many times each student will ask for help

/****************************************************************************
* Thread function prototypes
****************************************************************************/
void* ta_thread(void* param);
void* student_thread(void* num);

/****************************************************************************
 * Main Function
****************************************************************************/

int main() {
    //Declare local variables
    int i;
    pthread_t ta_handle;
    pthread_t* student_handles;
    int* student_ids;

    //Seed the random number generator so each run looks different
    srand((unsigned int)time(NULL));

    //Prompt for number of students and number of chairs
    printf("Enter number of students: ");
    scanf("%d", &num_students);

    printf("Enter number of chairs in hallway: ");
    scanf("%d", &num_chairs);

    if (num_students <= 0 || num_chairs < 0) {
        printf("Invalid input. Exiting.\n");
        return 1;
    }

    //Allocate arrays for threads and IDs
    student_handles = (pthread_t*)malloc(sizeof(pthread_t) * num_students);
    student_ids = (int*)malloc(sizeof(int) * num_students);
    if (student_handles == NULL || student_ids == NULL) {
        printf("Error: unable to allocate memory for threads.\n");
        free(student_handles);
        free(student_ids);
        return 1;
    }

    //Initialize mutex and semaphore
    pthread_mutex_init(&mutex, NULL);
    sem_init(&students_sem, 0, 0); //start with 0 students waiting

    //Create the TA thread
    if (pthread_create(&ta_handle, NULL, ta_thread, NULL) != 0) {
        printf("Error: unable to create TA thread.\n");
        free(student_handles);
        free(student_ids);
        pthread_mutex_destroy(&mutex);
        sem_destroy(&students_sem);
        return 1;
    }

    //Create the student threads
    for (i = 0; i < num_students; i++) {
        student_ids[i] = i + 1; //give students IDs 1..num_students
        if (pthread_create(&student_handles[i], NULL, student_thread, &student_ids[i]) != 0) {
            printf("Error: unable to create student thread %d.\n", i + 1);
        }
    }

    //Wait for all student threads to finish
    for (i = 0; i < num_students; i++) {
        pthread_join(student_handles[i], NULL);
    }

    //At this point, all students have finished their help cycles
    //Let the TA know that everyone is done
    pthread_mutex_lock(&mutex);
    all_done = 1;
    pthread_mutex_unlock(&mutex);

    //Wake up TA in case it is sleeping on the semaphore
    sem_post(&students_sem);

    //End the TA thread after all students are done
    pthread_join(ta_handle, NULL);

    //Destroy mutex and semaphore, and free memory
    pthread_mutex_destroy(&mutex);
    sem_destroy(&students_sem);
    free(student_handles);
    free(student_ids);

    return 0;
} //end main

/****************************************************************************
* Function: ta_thread
* What it does: Repeatedly sleeps waiting for students, then helps one
*               waiting student at a time, until all students are finished.
* Outputs: NULL when the TA finishes and the thread exits
****************************************************************************/
void* ta_thread(void* param) {
    (void)param; // unused parameter
    while (1) {
        //TA goes to "sleep" by waiting on the semaphore
        printf("TA: Waiting for a student (sleeping)...\n");
        sem_wait(&students_sem);  // block until a student arrives or a final wake-up

        //Lock the mutex when checking/updating shared state
        pthread_mutex_lock(&mutex);

        //If all students are done and no one is waiting, TA can go home
        if (all_done && waiting_students == 0) {
            pthread_mutex_unlock(&mutex);
            printf("TA: All students are done. TA is going home.\n");
            break;
        }

        //Check if students are actually waiting
        if (waiting_students > 0) {
            //"Help" a student by reducing the number of waiting students
            waiting_students--;
            printf("TA: Helping a student. Students still waiting = %d\n",
                   waiting_students);

            //Unlock mutex before simulating help time
            pthread_mutex_unlock(&mutex);

            //Simulate time taken to help a student (delay to make output readable)
            sleep(5);
        } else {
            //No students are actually waiting (possible after final wake-up)
            printf("TA: Woke up but no students are waiting.\n");
            pthread_mutex_unlock(&mutex);

            //Short delay just so output is readable; TA will loop and probably exit
            sleep(1);
        }
    } //end while

    pthread_exit(NULL);
    return NULL; //not reached, but keeps compiler happy
} //end thread function
    
/*************************************
* Function: student_thread
* What it does: Simulates one student alternating between programming and
*               visiting the TA for help a fixed number of times.
* Inputs: num -> pointer to an int representing this student's ID
* Outputs: NULL when the student is done and the thread exits
*************************************/
void* student_thread(void* num) {
    //typcast param to integer id
    int id = *((int*)num);
    int i;

    for (i = 0; i < HELP_REQUESTS_PER_STUDENT; i++) {
        //Simulate time spent programming
        int program_time = (rand() % 5) + 1; //between 1 and 5 seconds
        printf("Student %d: Programming for %d seconds.\n", id, program_time);
        sleep(program_time);

        //Try to get help from the TA by locking mutex
        pthread_mutex_lock(&mutex);

        //If number of waiting students is less than the number of chairs
        if (waiting_students < num_chairs) {
            waiting_students++;
            printf("Student %d: Sitting in hallway. Students waiting = %d\n",
                   id, waiting_students);

            //Unlock mutex before notifying TA
            pthread_mutex_unlock(&mutex);

            //Notify TA through semaphore (student has arrived / is waiting)
            sem_post(&students_sem);

            //Simulate waiting time
            sleep(1);
        } else {
            printf("Student %d: Hallway full. Will try again later.\n", id);
            pthread_mutex_unlock(&mutex);

            //Delay to make output readable and simulate walking away/coming back later
            sleep(1);
        }
    } //end for (each help request)

    //Mark this student as finished
    pthread_mutex_lock(&mutex);
    students_finished++;
    printf("Student %d: Done for the day. Finished count = %d\n",
           id, students_finished);
    if (students_finished == num_students) {
        all_done = 1;
    }
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
    return NULL; //not reached, but keeps compiler happy
} //end thread function