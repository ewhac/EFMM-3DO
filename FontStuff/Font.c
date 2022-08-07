
#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "graphics.h"
#include "event.h"
#include "stdio.h"

#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) /* { printf x ; } */

void* 
MyAllocMem(int32 size, uint32 typebits)
{
	register void	*MemPtr;

	MemPtr = ALLOCMEM(size,typebits);
	if (!MemPtr) 
		printf("ALLOCMEM failed\n") ;
	return MemPtr;
}

void
MyFreeMem(void	*MemPtr,int32 size)
{
	FreeMem(MemPtr,size);
}



