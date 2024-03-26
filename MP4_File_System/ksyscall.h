/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"

void SysHalt()
{
	kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
	return op1 + op2;
}
int SysCreate(char *filename ,int size)
{
	// return value
	// 1: success
	// 0: failed
	int rst = kernel->fileSystem->Create(filename, size);
	return rst;
}

//When you finish the function "OpenAFile", you can remove the comment below.

OpenFileId SysOpen(char *name)
{	OpenFileId id = (OpenFileId) kernel->fileSystem->Open(name);
    if(id > 0) return id;
	else return -1;
}

int SysClose(OpenFileId id){
    int rst = kernel->fileSystem->Close(id);
	return rst;
}

int SysWrite(char *buffer, int size, OpenFileId id){
    int rst = kernel->fileSystem->Write(buffer, size, id);
	return rst;
}

int SysRead(char *buffer, int size, OpenFileId id){
    int rst = kernel->fileSystem->Read(buffer, size, id);
	return rst;
}
#ifdef FILESYS_STUB
int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}
#endif

#endif /* ! __USERPROG_KSYSCALL_H__ */
