/***************************************************************
**
** Add up memory usage for a task.
**
** WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!
** Watch out for threads changing the list while you traverse!
** D0 NOT use this in an application!  It is not safe and sane.
** Suitable only for debugging memory leaks.
**
** Call ReportMemoryUsage() for a handy report.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "list.h"
#include "mem.h"
#include "stdarg.h"
#include "strings.h"
#include "operror.h"
#include "stdio.h"

#define	PRT(x)	{ printf x; }
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}

/***************************************************************
** Add up the sizes of the nodes in a linked list.
***************************************************************/
uint32 SumListNodes ( List *TheList )
{
	uint32 Sum = 0;
	Node *n;

	for (n = FirstNode( TheList ); IsNode( TheList, n ); n = NextNode(n) )
    {
    	Sum += n->n_Size;
	}
	return Sum;
}
/***************************************************************
** Add up the sizes of the nodes in a linked list, get largest.
***************************************************************/
uint32 SumMaxListNodes ( List *TheList, uint32 *pLargest)
{
	uint32 Sum = 0;
	uint32 Largest = 0;
	Node *n;

	for (n = FirstNode( TheList ); IsNode( TheList, n ); n = NextNode(n) )
    {
    	Sum += n->n_Size;
    	if(n->n_Size > Largest) Largest = n->n_Size;
	}
	
	*pLargest = Largest;
	return Sum;
}

/***************************************************************
** Count the number of nodes in a linked list.
***************************************************************/
uint32 CountListNodes ( List *TheList )
{
	uint32 Count = 0;
	Node *n;

	for (n = FirstNode( TheList ); IsNode( TheList, n ); n = NextNode(n) )
    {
    	Count += 1;
	}
	return Count;
}

/***************************************************************
** Count the number of 1 bits in a int32 word.
***************************************************************/
/* Stolen from shell.c */
uint32 OnesCount( uint32 v)
{
	uint32 ret = 0;
	while (v)
	{
		if (v & 1) ret++;
		v >>= 1;
	}
	return ret;
}

/***************************************************************
** Return total memory allocated for a memlist.
***************************************************************/
uint32 TotalPageMem( MemList *ml )
{
	uint32 *p = ml->meml_OwnBits;
	int ones = 0;
	int32 size;
	int i =  ml->meml_OwnBitsSize;
	while (i--) ones += OnesCount(*p++);
	size = ones*ml->meml_MemHdr->memh_PageSize;
	return size;
}

/***************************************************************
** Find MemList of a given type.
***************************************************************/
/* Stolen from mem.c */
MemList *FindML( List *l, uint32 types)
{
    Node *n;
    MemList *ml;
    uint32 mt;
    types &= MEMTYPE_VRAM|MEMTYPE_DMA|MEMTYPE_CEL;
    for (n = FIRSTNODE(l); ISNODE(l,n); n = NEXTNODE(n))
    {
        ml = (MemList *)n;
        DBUG(("FindML: test %lx nm=%s\n",ml,ml->meml_n.n_Name));
        mt = ml->meml_Types;
        mt &= types;    /* mask out bits we need */
        if (types == mt) break; /* a match? */
    }
    if (!(ISNODE(l,n)))     n = 0;
    return (MemList *)n;
}

/***************************************************************
** BytesOwned is total of all pages owned  of given type.
** BytesFree is sum of available memory nodes given type.
***************************************************************/
int32 mySumAvailMem( List *l, uint32 Type, uint32 *pBytesOwned, uint32 *pBytesFree )
{
	MemList *ml;
	int32 Result = -1;
	
	ml = FindML(l, Type);
	if (ml)
	{
		*pBytesFree = SumListNodes( ml->meml_l );
		*pBytesOwned = TotalPageMem( ml );
		Result = 0;
	}
	return Result;
}

/***************************************************************
** BytesOwned is total of all pages owned  of given type.
** BytesFree is sum of available memory nodes given type.
***************************************************************/
int32 mySumMaxAvailMem( List *l, uint32 Type, uint32 *pBytesOwned, uint32 *pBytesFree, uint32 *pLargest)
{
	MemList *ml;
	int32 Result = -1;
	
	ml = FindML(l, Type);
	if (ml)
	{
		*pBytesFree = SumMaxListNodes( ml->meml_l, pLargest);
		*pBytesOwned = TotalPageMem( ml );
		Result = 0;
	}
	return Result;
}

#define REPORTMEM(List,Type,Msg) \
{ \
	Result = mySumMaxAvailMem( List, Type, &BytesOwned, &BytesFree, &Largest); \
	PRT(("%s: ", Msg)); \
	PRT((" %8d  %8d  %8d %8d\n", BytesOwned, BytesFree, (BytesOwned-BytesFree), Largest)); \
}

/***************************************************************/
int32 ReportMemoryUsage( void )
{
	uint32 BytesFree, BytesOwned, Largest;
	int32 Result;

	PRT(("\nMemory Type------Owned------Free----In Use---Largest\n"));
	REPORTMEM(KernelBase->kb_CurrentTask->t_FreeMemoryLists,
		MEMTYPE_DRAM,"Task's DRAM");
	REPORTMEM(KernelBase->kb_CurrentTask->t_FreeMemoryLists,
		MEMTYPE_VRAM,"Task's VRAM");
	REPORTMEM(KernelBase->kb_MemFreeLists,
		MEMTYPE_DRAM,"Kernel DRAM");
	REPORTMEM(KernelBase->kb_MemFreeLists,
		MEMTYPE_VRAM,"Kernel VRAM");

	return 0;
}
