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

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// compare function for SJF queue and priority queue
//----------------------------------------------------------------------
int SJF_cmp(Thread* a, Thread* b){
    double a_app = max(a->approximate_burst_time - a->burst_time, 0.0);
    double b_app = max(b->approximate_burst_time - b->burst_time, 0.0);
    if(a_app < b_app){
        return -1;
    }
    else if(a_app > b_app){
        return 1;
    }
    else{
        return 0;
    }
}

int priority_cmp(Thread* a, Thread* b){
    if(a->thread_priority > b->thread_priority){
        return -1;
    }
    else if(a->thread_priority < b->thread_priority){
        return 1;
    }
    else{
        return 0;
    }
}

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    //readyList = new List<Thread *>; 
    L1 = new SortedList<Thread *>(SJF_cmp);
    L2 = new SortedList<Thread *>(priority_cmp);
    L3 = new List<Thread *>;
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    //delete readyList; 
    delete L1;
    delete L2;
    delete L3;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    //readyList->Append(thread);

    // multilevel feedback queue
    if(thread->thread_priority >= 100 && thread->thread_priority <= 149){
        L1->Insert(thread);
        thread->start_waiting_time = kernel->stats->totalTicks;
        DEBUG(dbgMP3, "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[1]");
    }
    else if(thread->thread_priority >= 50 && thread->thread_priority <= 99){
        L2->Insert(thread);
        thread->start_waiting_time = kernel->stats->totalTicks;
        DEBUG(dbgMP3, "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[2]");
    }
    else if(thread->thread_priority >= 0 && thread->thread_priority <= 49){
        L3->Append(thread);
        thread->start_waiting_time = kernel->stats->totalTicks;
        DEBUG(dbgMP3, "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[3]");
    }
    //CheckPreemptive_R(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    /*if (readyList->IsEmpty()) {
		return NULL;
    } else {
    	return readyList->RemoveFront();
    }*/


    // multi queue
    if(!L1->IsEmpty()){
        Thread *thread = L1->RemoveFront();
        thread->waiting_time = 0;
        DEBUG(dbgMP3, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[1]");
        //DEBUG(dbgMP3, " " << thread->remain_app_burst_time);
        return thread;
    }
    else if(!L2->IsEmpty()){
        Thread *thread = L2->RemoveFront();
        thread->waiting_time = 0;
        DEBUG(dbgMP3, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[2]");
        //DEBUG(dbgMP3, " " << thread->remain_app_burst_time);
        return thread;
    }
    else if(!L3->IsEmpty()){
        Thread *thread = L3->RemoveFront();
        thread->waiting_time = 0;
        DEBUG(dbgMP3, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[3]");
        //DEBUG(dbgMP3, " " << thread->remain_app_burst_time);
        return thread;
    }
    else{
        return NULL;
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

void
Scheduler::Run (Thread *nextThread, bool finishing, bool Sleep)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }

    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow
    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->Set_Start_Time();        // set the time that the thread start
    nextThread->setStatus(RUNNING);      // nextThread is now running

    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // DEBUG code for context switch
    DEBUG(dbgMP3, "[E] Tick [" << kernel->stats->totalTicks << "]: Thread [" << nextThread->getID() << "] is is now selected for execution, thread [" << oldThread->getID() << "] is replaced, and it has executed [" << oldThread->burst_time << "] ticks");
    //DEBUG(dbgMP3, oldThread->approximate_burst_time << "  " << nextThread->approximate_burst_time);


    // reset burst time to 0
    if(Sleep){
        oldThread->Reset_Burst_Time();
    }
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
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

void
Scheduler::CheckToBeDestroyed()
{
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
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    // eadyList->Apply(ThreadPrint);
}

void Scheduler::Aging()
{
    SortedList<Thread *> *newL1 = new SortedList<Thread *>(SJF_cmp);
    SortedList<Thread *> *newL2 = new SortedList<Thread *>(priority_cmp);
    List<Thread *> *newL3 = new List<Thread *>;
    int totalTicks = kernel->stats->totalTicks;
    while (!L1->IsEmpty())
    {
        Thread *thread = L1->RemoveFront();
        thread->waiting_time += totalTicks - thread->start_waiting_time;
        thread->start_waiting_time = totalTicks;

        if (thread->waiting_time >= 1500)
        {
            if (thread->thread_priority + 10 > 149)
            {
                DEBUG(dbgMP3, "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from thread [" << thread->thread_priority << "] to [149] ticks");
                thread->thread_priority = 149;
            }
            else
            {
                DEBUG(dbgMP3, "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from thread [" << thread->thread_priority << "] to [" << thread->thread_priority+10 << "] ticks");
                thread->thread_priority += 10;
            }
            thread->waiting_time -= 1500;
        }
        newL1->Insert(thread);
    }
    while (!L2->IsEmpty())
    {
        Thread *thread = L2->RemoveFront();
        thread->waiting_time += totalTicks - thread->start_waiting_time;
        thread->start_waiting_time = totalTicks;

        if (thread->waiting_time >= 1500)
        {
            thread->waiting_time -= 1500;
            DEBUG(dbgMP3, "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from thread [" << thread->thread_priority << "] to [" << thread->thread_priority+10 << "] ticks");
            thread->thread_priority += 10;
        }
        if (thread->thread_priority > 99)
            newL1->Insert(thread);
        else
            newL2->Insert(thread);
    }
    while (!L3->IsEmpty())
    {
        Thread *thread = L3->RemoveFront();
        thread->waiting_time += totalTicks - thread->start_waiting_time;
        thread->start_waiting_time = totalTicks;
        if (thread->waiting_time >= 1500)
        {
            thread->waiting_time -= 1500;
            DEBUG(dbgMP3, "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from thread [" << thread->thread_priority << "] to [" << thread->thread_priority+10 << "] ticks");
            thread->thread_priority += 10;
        }
        if (thread->thread_priority > 49)
            newL2->Insert(thread);
        else
            newL3->Append(thread);
    }

    delete L1;
    delete L2;
    delete L3;
    L1 = newL1;
    L2 = newL2;
    L3 = newL3;
}



void Scheduler::CheckPreemptive_R(Thread* thread){
    Thread *oldThread = kernel->currentThread;
    // case 1: current thread is L1, but a new thread get in L1 and has shorter burst time
    if(oldThread->thread_priority <50 && (thread->thread_priority>50 && thread->thread_priority<149)){
        oldThread->burst_time += kernel->stats->totalTicks - oldThread->start_time;
        Thread * nextThread = this->FindNextToRun();
        this->Run(nextThread, FALSE, FALSE);
    }

    else if((oldThread->thread_priority <100 && oldThread->thread_priority>50 ) && thread->thread_priority > 100){
        oldThread->burst_time += kernel->stats->totalTicks - oldThread->start_time;
        Thread * nextThread = this->FindNextToRun();
        this->Run(nextThread, FALSE, FALSE);
    }
    else if(oldThread->thread_priority > 100 && thread->thread_priority > 100){
        if(oldThread->approximate_burst_time > thread->approximate_burst_time){
            oldThread->burst_time += kernel->stats->totalTicks - oldThread->start_time;
            Thread * nextThread = this->FindNextToRun();
            this->Run(nextThread, FALSE, FALSE);
        }
    }
}