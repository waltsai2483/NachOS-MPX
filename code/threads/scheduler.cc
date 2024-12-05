// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "scheduler.h"

#include "copyright.h"
#include "debug.h"
#include "main.h"
#include "utility.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler() : priorityIntervalSize(4) {
    priorityInterval[0] = 0;
    priorityInterval[1] = 50;
    priorityInterval[2] = 100;
    priorityInterval[3] = 150;
    toBeDestroyed = NULL;
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler() {
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void Scheduler::ReadyToRun(Thread *thread) {
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName() << " (" << thread->getID() << ")");
    // cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    int lv = ScheduleLevel(thread->getPriority());
    if (lv == READYL1_LEVEL) {
        readyL1.Push(thread);
    } else if (lv == READYL2_LEVEL) {
        readyL2.Push(thread);
    } else if (lv == READYL3_LEVEL) {
        readyL3.Push(thread);
    }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun() {
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    Thread* t;
    if ((t = readyL1.RemoveBest()) != NULL) { // L1 Queue CAN preempt
        return t;
    }

    // handled cases are:
    //   * running L1 thread is prior to ready L2/L3 thread
    //   * running L2 thread is prior to ready L2/L3 thread
    if (ScheduleLevel(kernel->currentThread->getPriority()) >= READYL2_LEVEL &&
        kernel->currentThread->getStatus() == RUNNING)
        return NULL;

    if ((t = readyL2.RemoveBest()) != NULL) {
        return t;
    }
    
    if ((t = readyL3.RemoveBest()) != NULL) { // Round-Robin CAN preempt
        return t;
    }

    return NULL;
}

int Scheduler::ScheduleLevel(int priority) {
    ASSERT(priority >= priorityInterval[0] &&
           priority < priorityInterval[priorityIntervalSize - 1]);

    int i = 0;
    for (; i < priorityIntervalSize; ++i) {
        if (priorityInterval[i] > priority)
            break;
    }
    return i - 1;
}

void Scheduler::Aging(Thread *thread) {
    Scheduler *scheduler = kernel->scheduler;
    if (kernel->stats->totalTicks - thread->getPriorityUptTick() >= AGING_PERIOD) {
        thread->setPriorityUptTick(kernel->stats->totalTicks);

        int cap = scheduler->priorityInterval[scheduler->priorityIntervalSize - 1] - 1;
        int prevPriority = thread->getPriority();
        int priority = min(prevPriority + AGING_FACTOR, cap);
        thread->setPriority(priority);

        DEBUG(dbgScheduler, "[C] Tick [" << kernel->stats->totalTicks
            << "]: Thread [" << thread->getID() << "] changes its priority from ["
            << prevPriority << "] to [" << priority << "]");

        if (scheduler->ScheduleLevel(prevPriority) != scheduler->ScheduleLevel(priority))
            scheduler->UpgradeThreadLevel(thread);
    }
}

void Scheduler::UpgradeThreadLevel(Thread* thread) {
    // FIXME: hard code the queue to remove
    int lv = ScheduleLevel(thread->getPriority());
    if (lv == READYL1_LEVEL) {
        readyL2.Remove(thread);
        readyL1.Push(thread);
    } else if (lv == READYL2_LEVEL) {
        readyL3.Remove(thread);
        readyL2.Push(thread);
    }
}

void Scheduler::ElevateThreads() {
    readyL1.Apply(Aging);
    readyL2.Apply(Aging);
    readyL3.Apply(Aging);
}

const char *Scheduler::QueueName(JobQueue *q) {
    if (q == &readyL1) {
        return "L[1]";
    } else if (q == &readyL2) {
        return "L[2]";
    } else if (q == &readyL3) {
        return "L[3]";
    } else {
        ASSERT(false);
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void Scheduler::Run(Thread *nextThread, bool finishing) {
    Thread *oldThread = kernel->currentThread;

    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {  // mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
        toBeDestroyed = oldThread;
    }

    if (oldThread->space != NULL) {  // if this thread is a user program,
        oldThread->SaveUserState();  // save the user's CPU registers
        oldThread->space->SaveState();
    }

    oldThread->CheckOverflow();  // check if the old thread
                                 // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running

    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());

    // This is a machine-dependent assembly language routine defined
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    int tick = oldThread->getAccumTickWithResetCheck();
    DEBUG(dbgScheduler, "[E] Tick [" << kernel->stats->totalTicks << "]: Thread [" <<
          nextThread->getID() << "] is now selected for execution, thread [" <<
          oldThread->getID() << "] is replaced, and it has executed [" <<
          tick << "] ticks");
    SWITCH(oldThread, nextThread);

    // we're back, running oldThread

    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();  // check if thread we were running
                           // before this one has finished
                           // and needs to be cleaned up

    if (oldThread->space != NULL) {     // if there is an address space
        oldThread->RestoreUserState();  // to restore, do it.
        oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void Scheduler::CheckToBeDestroyed() {
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
        toBeDestroyed = NULL;
    }
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void Scheduler::Print() {
    cout << "Ready list contents:\n";
    cout << "\tL1 Queue: ";
    readyL1.Apply(ThreadPrint);
    cout << "\tL2 Queue: ";
    readyL2.Apply(ThreadPrint);
    cout << "\tL3 Queue: ";
    readyL3.Apply(ThreadPrint);
}

void JobQueue::Push(Thread* thread) {
    const char *name = kernel->scheduler->QueueName(this);
    DEBUG(dbgScheduler, "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" <<
          thread->getID() << "] is inserted into queue " << name);
    list->Append(thread);
}

void JobQueue::Remove(Thread *thread) {
    const char *name = kernel->scheduler->QueueName(this);
    DEBUG(dbgScheduler, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" <<
          thread->getID() << "] is removed from queue " << name);
    list->Remove(thread);
}

Thread* SJFQueue::RemoveBest() {
    if (list->IsEmpty()) return NULL;
    ListIterator<Thread*> iter(list);
    Thread *best = NULL;
    while (!iter.IsDone()) {
        if (best == NULL || iter.Item()->getApproBurstTick() < best->getApproBurstTick() || iter.Item()->getApproBurstTick() == best->getApproBurstTick() && iter.Item()->getID() < best->getID()) {
            best = iter.Item();
        }
        iter.Next();
    }

    Thread *curr = kernel->currentThread;
    int lv = kernel->scheduler->ScheduleLevel(curr->getPriority());
    if (lv == Scheduler::READYL1_LEVEL && curr->getStatus() == RUNNING) {
        double curr_tick = curr->getApproRemainingTick();
        double best_tick = best->getApproBurstTick();
        if (curr_tick < best_tick ||
            (curr_tick == best_tick && curr->getID() < best->getID()))
            return NULL;
    }

    Remove(best);
    return best;
}

Thread* PriorityQueue::RemoveBest() {
    if (list->IsEmpty()) return NULL;
    ListIterator<Thread*> iter(list);
    Thread *best = NULL;
    while (!iter.IsDone()) {
        if (best == NULL || iter.Item()->getPriority() > best->getPriority() || iter.Item()->getPriority() == best->getPriority() && iter.Item()->getID() < best->getID()) {
            best = iter.Item();
        }
        iter.Next();
    }
    Remove(best);
    return best;
}

Thread* RRQueue::RemoveBest() {
    if (list->IsEmpty()) return NULL;
    Thread *t = list->Front();
    Remove(t);
    return t;
}