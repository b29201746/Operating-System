// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"


//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
}

//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//----------------------------------------------------------------------

void 
Alarm::CallBack() 
{
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    
    if (status != IdleMode) {
        kernel->scheduler->Aging();
	    interrupt->YieldOnReturn();
        /*if(kernel->currentThread->thread_priority <= 49){
            if(!kernel->scheduler->L1_empty() || !kernel->scheduler->L2_empty()){
                Thread* nextThread = kernel->scheduler->FindNextToRun();
                kernel->currentThread->Update_Burst_Time();
                kernel->scheduler->ReadyToRun(kernel->currentThread);
                kernel->scheduler->Run(nextThread, FALSE, FALSE);
            }
        }
        else if(kernel->currentThread->thread_priority >= 100 && kernel->currentThread->thread_priority <= 149){
            if(!kernel->scheduler->L1_empty()){
                int flag = 0;
                ListIterator<Thread *> *iter = new ListIterator<Thread *>(kernel->scheduler->get_L1());
                for(; !iter->IsDone(); iter->Next()){
                    Thread* thread = iter->Item();
                    int remain_app = thread->approximate_burst_time - thread->burst_time;
                    if(remain_app < 0) remain_app = 0;

                    int current_app = kernel->currentThread->approximate_burst_time - kernel->currentThread->burst_time;
                    if(current_app < 0) current_app = 0;
                    if(current_app > remain_app){
                        flag = 1;
                        break;
                    }
                }
                if(flag){
                    flag = 0;
                    Thread* nextThread = kernel->scheduler->FindNextToRun();
                    kernel->currentThread->Update_Burst_Time();
                    kernel->scheduler->ReadyToRun(kernel->currentThread);
                    kernel->scheduler->Run(nextThread, FALSE, FALSE);
                }
            }
        }*/
    }
}
