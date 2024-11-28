// scheduler.h
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// The following class defines the scheduler/dispatcher abstraction --
// the data structures and operations needed to keep track of which
// thread is running, and which threads are ready but not running.

class JobQueue {
   public:
   JobQueue() { list = new List<Thread*>; }
   ~JobQueue() { delete list; }
   virtual void Push(Thread* thread) { list->Append(thread); }
   virtual Thread* RemoveBest() = 0;
   void Remove(Thread *thread) { list->Remove(thread); }
   bool IsEmpty() { return list->IsEmpty(); }
   bool IsInList(Thread *thread) { return list->IsInList(thread); }
   void Apply(void (*f)(Thread *)) const { list->Apply(f); }

   protected:
   List<Thread*>* list;
};

class SJFQueue: public JobQueue {
   public:
   Thread* RemoveBest();
};

class PriorityQueue: public JobQueue {
   public:
   Thread* RemoveBest();
};

class RRQueue: public JobQueue {
   public:
   Thread* RemoveBest();
};

class Scheduler {
   public:
    Scheduler();   // Initialize list of ready threads
    ~Scheduler();  // De-allocate ready list

    void ReadyToRun(Thread* thread);
    // Thread can be dispatched.
    Thread* FindNextToRun();  // Dequeue first thread on the ready
                              // list, if any, and return thread.
    void UpgradeThreadLevel(Thread* thread);
    void ElevateThreads();
    void Run(Thread* nextThread, bool finishing);
    // Cause nextThread to start running
    void CheckToBeDestroyed();  // Check if thread that had been
                                // running needs to be deleted
    void Print();               // Print contents of ready list

    static void Aging(Thread *thread);

    // SelfTest for scheduler is implemented in class Thread

   private:
    static const int AGING_PERIOD = 1500;
    static const int AGING_FACTOR = 10;

    int ScheduleLevel(int priority);

    SJFQueue readyL1;
    PriorityQueue readyL2;
    RRQueue readyL3;
    
    Thread* toBeDestroyed;     // finishing thread to be destroyed
                               // by the next thread that runs
};

#endif  // SCHEDULER_H
