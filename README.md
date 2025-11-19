# Assignment_4_comp521

# COMP 521 / 521L – Assignment 4  
## The Sleeping Teaching Assistant Problem

This assignment implements a classic concurrency problem: a **teaching assistant (TA)** who helps students during office hours. The TA either **helps students** one at a time or **sleeps** when no one needs help. Multiple **student threads** arrive randomly to request help and may have to **wait in hallway chairs** or **come back later** if the hallway is full.

You will complete the provided skeleton C file using **POSIX threads**, a **mutex**, and a **semaphore** to correctly synchronize the TA and students.

---

## 1. Problem Overview (English Only)

Imagine office hours as a small simulation:

- There is exactly **one TA** in an office.
- There are **x chairs** in the hallway outside.
- There are **y students** who periodically get stuck on their programming and go to see the TA.

The rules of this “mini-world” are:

- If a student arrives and the TA is **sleeping** (no one is being helped and no one is waiting),  
  → The student **wakes the TA** and gets help right away.

- If the TA is **busy helping someone** when a student arrives:
  - If there is at least **one empty chair** in the hallway,  
    → The student **sits down and waits**.
  - If **all chairs are taken**,  
    → The student **leaves and will try again later** after working more on their assignment.

- The TA:
  - If there is **at least one waiting student**,  
    → The TA continuously **calls in the next student** and helps them.
  - If there are **no students in the office and no one in the chairs**,  
    → The TA **goes to sleep** until someone wakes them.

Your program should print messages that make this “story” visible:
- When students are programming.
- When they go to the TA.
- When they sit and wait.
- When they give up temporarily because the hallway is full.
- When the TA sleeps, wakes up, and helps students one by one.

---

## 2. What You Have to Implement

You are given a skeleton file:

- `asmt4_skeleton_f25.c` – contains comments and TODO-style hints for:
  - Declaring global variables, mutex, and semaphore.
  - Implementing `main`.
  - Implementing the TA thread function.
  - Implementing the student thread function.

You need to **fill in the missing pieces** so that the simulation works correctly and safely with multiple threads.

### 2.1. Global Data & Synchronization

You must declare and use:

- A **mutex** (from `pthread.h`)  
  - To protect shared variables such as:
    - Number of students currently waiting.
    - Any other shared counters or state.

- A **semaphore** (from `semaphore.h`)  
  - So that **students can notify the TA** that someone is waiting.
  - The TA uses it to block when there are no students and wake when a student arrives.

- **Global variables**, at minimum:
  - Number of chairs in the hallway.
  - Number of students.
  - Number of waiting students.
  - Any other state you need to track who’s waiting.

---

### 2.2. `main()` Responsibilities

In `main()` you must:

1. **Prompt the user** for:
   - The number of students.
   - The number of chairs in the hallway.

2. **Allocate storage** for student thread handles:
   - Typically an array of `pthread_t`, one per student.

3. **Initialize synchronization primitives**:
   - Initialize the mutex.
   - Initialize the semaphore (initial value usually 0).

4. **Create the TA thread**:
   - A single thread that runs the TA’s loop.

5. **Create all student threads**:
   - Loop from 0 to `num_students - 1`.
   - Each student gets an ID (0, 1, 2, …) so you can distinguish them in output.

6. **Wait for student threads to finish**:
   - Use `pthread_join` on each student thread.

7. **Shut down the TA thread**:
   - Once you know students are done, you need some mechanism to exit the TA’s loop (e.g., a global “done” flag plus a final wake-up).

8. **Clean up**:
   - Destroy the mutex and semaphore.
   - Free any dynamically allocated memory.

---

### 2.3. TA Thread Behavior

The TA thread should conceptually:

1. **Loop** and wait for students:
   - Block on the semaphore until a student signals they need help.

2. When signaled:
   - Lock the mutex.
   - Check how many students are waiting.
   - If at least one is waiting:
     - Decrease the count of waiting students.
     - Print a message like  
       `TA is helping a student. Waiting students: N`
   - Unlock the mutex.

3. Simulate helping that student:
   - Sleep for some time so output is readable.

4. If no more students are waiting and all work is done:
   - Exit the loop so the TA thread can terminate.
   - Otherwise, go back to waiting (which acts like “sleeping”).

---

### 2.4. Student Thread Behavior

Each student thread should:

1. **Loop** over:
   - “Programming time” (simulated with sleep).
   - Attempting to go to the TA.

2. When a student decides to seek help:
   - Lock the mutex.
   - If the number of waiting students is **less than** the number of chairs:
     - Increment the waiting student count.
     - Print something like  
       `Student X is waiting. Waiting students: N`
     - Unlock the mutex.
     - Signal the TA via the semaphore.
     - Simulate “waiting in the hallway” (sleep).
   - Else (chairs are full):
     - Print something like  
       `Hallway full. Student X will try later.`
     - Unlock the mutex.
     - Sleep before trying again.

3. Decide when each student is “done”:
   - For example, after a certain number of help visits.
   - Once done, exit the thread.

---

## 3. Requirements & Constraints

- Use **POSIX threads** (`pthread.h`), **mutexes**, and **semaphores** (`semaphore.h`).
- Protect **all shared data** with the mutex.
- Use the semaphore **only for signaling** between students and the TA.
- Your output should clearly show:
  - Students arriving, waiting, leaving due to no space.
  - TA sleeping, waking, and helping students.
- The simulation should run long enough to demonstrate the behavior clearly (use delays/sleeps to make the output readable).

---

## 4. How to Compile and Run

On a Linux-like environment or WSL/macOS terminal:

```bash
# Compile (note: -pthread is important)
gcc -pthread asmt4_skeleton_f25.c -o sleeping_ta

# Run
./sleeping_ta
