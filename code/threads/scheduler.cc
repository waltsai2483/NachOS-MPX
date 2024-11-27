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

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler() {
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
    if (thread->getPriority() <= 49) {
        if (readyL1.IsInList(thread)) readyL1.Remove(thread);
        readyL1.Push(thread);
        DEBUG(dbgScheduler, "[A] Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L1");
    } else if (thread->getPriority() <= 99) {
        if (readyL2.IsInList(thread)) readyL2.Remove(thread);
        readyL2.Push(thread);
        DEBUG(dbgScheduler, "[A] Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L2");
    } else if (thread->getPriority() <= 149) {
        if (readyL3.IsInList(thread)) readyL3.Remove(thread);
        readyL3.Push(thread);
        DEBUG(dbgScheduler, "[A] Tick " << kernel->stats->totalTicks << ": Thread " << thread->getID() << " is inserted into queue L3");
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
        DEBUG(dbgScheduler, "[B] Tick " << kernel->stats->totalTicks << ": Thread " << t->getID() << " is removed from queue L1");
        return t;
    }

    if (kernel->currentThread->getSchedulerLevel() <= 1 && kernel->currentThread->getStatus() == RUNNING) return NULL; // L2 Queue cannot preempt another L2

    if ((t = readyL2.RemoveBest()) != NULL) {
        DEBUG(dbgScheduler, "[B] Tick " << kernel->stats->totalTicks << ": Thread " << t->getID() << " is removed from queue L2");
        return t;
    }
    
    if ((t = readyL3.RemoveBest()) != NULL) { // Round-Robin CAN preempt
        DEBUG(dbgScheduler, "[B] Tick " << kernel->stats->totalTicks << ": Thread " << t->getID() << " is removed from queue L3");
        return t;
    }

    return NULL;
}

void Scheduler::UpdateThreadLevel(Thread* thread) {
    if (thread->getSchedulerLevel() == 0 && readyL2.IsInList(thread)) {
        readyL2.Remove(thread);
        readyL1.Push(thread);
    } else if (thread->getSchedulerLevel() == 1 && readyL3.IsInList(thread)) {
        readyL3.Remove(thread);
        readyL2.Push(thread);
    }
}

void Scheduler::ElevateThreads() {
    ListIterator<Thread*> iter1(readyL1.list);
    for (; !iter1.IsDone(); iter1.Next()) {
        iter1.Item()->UpdatePriority();
    }
    ListIterator<Thread*> iter2(readyL2.list);
    for (; !iter2.IsDone(); iter2.Next()) {
        iter2.Item()->UpdatePriority();
    }
    ListIterator<Thread*> iter3(readyL3.list);
    for (; !iter3.IsDone(); iter3.Next()) {
        iter3.Item()->UpdatePriority();
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

    nextThread->StartRunning(); // Update start running tick of the thread
                                // in order to calculate CPU burst later
    DEBUG(dbgScheduler, "[E] Tick " << kernel->stats->totalTicks << ": Thread " << nextThread->getID() << " is now selected for execution, thread " << oldThread->getID() << " is replaced, and it has executed " << oldThread->getRunningTick() << " ticks")
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
    readyL1.list->Apply(ThreadPrint);
    cout << "\tL2 Queue: ";
    readyL2.list->Apply(ThreadPrint);
    cout << "\tL3 Queue: ";
    readyL3.list->Apply(ThreadPrint);
}


Thread* SJFQueue::RemoveBest() {
    if (list->IsEmpty()) return NULL;
    ListIterator<Thread*> iter(list);
    Thread *best = NULL;
    while (!iter.IsDone()) {
        if (best == NULL || iter.Item()->getApproRemainingTick() < best->getApproRemainingTick() || iter.Item()->getApproRemainingTick() == best->getApproRemainingTick() && iter.Item()->getID() < best->getID()) {
            best = iter.Item();
        }
        iter.Next();
    }
    list->Remove(best);
    return best;
}

Thread* PriorityQueue::RemoveBest() {
    if (list->IsEmpty()) return NULL;
    ListIterator<Thread*> iter(list);
    Thread *best = NULL;
    while (!iter.IsDone()) {
        if (best == NULL || iter.Item()->getPriority() < best->getPriority() || iter.Item()->getPriority() == best->getPriority() && iter.Item()->getID() < best->getID()) {
            best = iter.Item();
        }
        iter.Next();
    }
    list->Remove(best);
    return best;
}

Thread* RRQueue::RemoveBest() {
    if (list->IsEmpty()) return NULL;
    return list->RemoveFront();
}