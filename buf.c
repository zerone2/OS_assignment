#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "queue.h"
#include "Disk.h"

int bufReadCount = 0;
int bufWriteCount = 0;

void BufInit(void)
{
	TAILQ_INIT(&pLruListHead);
	TAILQ_INIT(&ppStateListHead[BUF_LIST_CLEAN]);
	TAILQ_INIT(&ppStateListHead[BUF_LIST_DIRTY]);
	TAILQ_INIT(&pBufList);

	DevCreateDisk();
}

/*search for the buffer if it's in the pBuflist*/
Buf* bufFind(int blkno)	
{
	Buf* temp = (Buf*)malloc(sizeof(Buf));
	TAILQ_FOREACH(temp, &pBufList, blist)
	{
		if (temp->blkno == blkno)
			return temp;
	}
	return NULL;
}

void BufRead(int blkno, char* pData)
{
	int i=0;

	Buf* buffer = (Buf*)malloc(sizeof(Buf));
	Buf* tmp_buf = (Buf*)malloc(sizeof(Buf));
	buffer = bufFind(blkno);

	if (buffer == NULL)				//if buffer is not in pBuflist (blist)
	{
		TAILQ_FOREACH(buffer, &pBufList, blist)	//using FOREACH to notice how many buffers are in pBuflist(blist)
		{
			i++;
			if (i == MAX_BUF_NUM)	//if buf comes more than max buf
			{
				buffer = TAILQ_FIRST(&pLruListHead);
				TAILQ_REMOVE(&pLruListHead, buffer, llist);		//remove from lru list
				TAILQ_REMOVE(&pBufList,buffer,blist);			//remove from buf list

				if(buffer->state==BUF_STATE_DIRTY)
				{
					DevWriteBlock(buffer->blkno, buffer->pMem);
					TAILQ_REMOVE(&ppStateListHead[BUF_LIST_DIRTY],buffer,slist);	//if state is dirty, remove from dirty list
				}
				else if(buffer->state==BUF_STATE_CLEAN)
				{
					TAILQ_REMOVE(&ppStateListHead[BUF_LIST_CLEAN],buffer,slist);	//if state is clean, remove from clean list
				}
				free(buffer);
			}
		}
		buffer = (Buf*)malloc(sizeof(Buf));
		buffer->blkno = blkno;
		buffer->pMem = malloc(BLOCK_SIZE);
		buffer->state = BUF_STATE_CLEAN;		//there was no buf, so it's first time reading from disk. should be clean state

		DevReadBlock(blkno, buffer->pMem);

		TAILQ_INSERT_HEAD(&pBufList, buffer, blist);							//insert into buf list
		TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_CLEAN], buffer, slist);		//insert into clean list

		memcpy(pData, buffer->pMem, BLOCK_SIZE);

		TAILQ_INSERT_TAIL(&pLruListHead, buffer, llist);	//insert into lru list
	}
	else
	{
		memcpy(pData, buffer->pMem, BLOCK_SIZE);
		TAILQ_FOREACH(tmp_buf,&pLruListHead,llist)		//if block is already in Lru list, remove.
		{
			if(tmp_buf->blkno == buffer->blkno)
				TAILQ_REMOVE(&pLruListHead,tmp_buf,llist);
		}
		TAILQ_INSERT_TAIL(&pLruListHead, buffer, llist);	//insert into lru list
	}	

}


void BufWrite(int blkno, char* pData)
{
	int i=0;
	Buf* buffer = (Buf*)malloc(sizeof(Buf));
	Buf* tmp_buf = (Buf*)malloc(sizeof(Buf));
	buffer = bufFind(blkno);

	if (buffer == NULL)
	{
		TAILQ_FOREACH(buffer, &pBufList, blist)	//using FOREACH to notice how many buffers are in pBuflist(blist)
		{
			i++;
			if (i == MAX_BUF_NUM)	//if buf comes more than max buf
			{
				buffer = TAILQ_FIRST(&pLruListHead);
				TAILQ_REMOVE(&pLruListHead, buffer, llist);		//remove from lru list
				TAILQ_REMOVE(&pBufList,buffer,blist);			//remove from buf list

				if(buffer->state==BUF_STATE_DIRTY)
				{
					DevWriteBlock(buffer->blkno, buffer->pMem);
					TAILQ_REMOVE(&ppStateListHead[BUF_LIST_DIRTY],buffer,slist);	//if state is dirty, remove from dirty list
				}
				else if(buffer->state==BUF_STATE_CLEAN)
				{
					TAILQ_REMOVE(&ppStateListHead[BUF_LIST_CLEAN],buffer,slist);	//if state is clean, remove from clean list
				}
				free(buffer);
			}
		}
		buffer = (Buf*)malloc(sizeof(Buf));
		buffer->blkno = blkno;
		buffer->pMem = malloc(BLOCK_SIZE);
		buffer->state = BUF_STATE_DIRTY;
		memcpy(buffer->pMem, pData, BLOCK_SIZE);
		TAILQ_INSERT_HEAD(&pBufList, buffer, blist);							//insert into buf list
		TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_DIRTY], buffer, slist);		//insert into dirty list
		TAILQ_INSERT_TAIL(&pLruListHead, buffer, llist);						//insert into lru list
	}
	else
	{
		if (buffer->state == BUF_STATE_DIRTY)
		{
			memcpy(buffer->pMem, pData, BLOCK_SIZE);
			TAILQ_FOREACH(tmp_buf,&pLruListHead,llist)		//if block is already in LruList, remove.
			{
				if(tmp_buf->blkno==buffer->blkno)
					TAILQ_REMOVE(&pLruListHead,tmp_buf,llist);
			}
			TAILQ_INSERT_TAIL(&pLruListHead, buffer, llist);
		}
		else
		{

			memcpy(buffer->pMem, pData, BLOCK_SIZE);			//copy pData to buffer->pMem
			TAILQ_FOREACH(tmp_buf,&ppStateListHead[BUF_LIST_CLEAN],slist)		//find in clean list and if found, remove.
			{
				if(tmp_buf->blkno==buffer->blkno)
					TAILQ_REMOVE(&ppStateListHead[BUF_LIST_CLEAN],tmp_buf,slist);
			}

			buffer->state=BUF_STATE_DIRTY;										//state change to dirty, because it's written now
			TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_DIRTY], buffer, slist);	//insert into dirty list

			TAILQ_FOREACH(tmp_buf,&pLruListHead,llist)		//if block is already in Lru list, remove.
			{
				if(tmp_buf->blkno==buffer->blkno)
				TAILQ_REMOVE(&pLruListHead,tmp_buf,llist);
			}
			TAILQ_INSERT_TAIL(&pLruListHead, buffer, llist);		//insert into Lru list.
		 }
	 }
}

void BufSync(void)
{
	Buf* buffer = (Buf*)malloc(sizeof(Buf));
	Buf* tmp_buf[MAX_BUF_NUM];
	int i=0;
	int tmp;

	TAILQ_FOREACH(buffer, &ppStateListHead[BUF_LIST_DIRTY], slist)
	{
		buffer->state = BUF_STATE_CLEAN;
		tmp_buf[i] = buffer;
		DevWriteBlock(buffer->blkno,buffer->pMem);								//write to disk
		TAILQ_REMOVE(&ppStateListHead[BUF_LIST_DIRTY], buffer, slist);			//and remove from dirty list
		i++;
	}
	tmp=i;

	for(i=0;i<tmp;i++)
	{
		TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_CLEAN],tmp_buf[i],slist);	//insert into clean list
	}

}



/*
 * GetBufInfoByListNum: Get all buffers in a list specified by listnum.
 *                      This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
void GetBufInfoByListNum(StateList listnum, Buf** ppBufInfo, int* pNumBuf)
{
	int i=0;
	Buf* buffer = (Buf*)malloc(sizeof(Buf));
	TAILQ_FOREACH(buffer,&ppStateListHead[listnum],slist)
	{
		ppBufInfo[i]=buffer;
		i++;
	}
	*pNumBuf=i;
}



/*
 * GetBufInfoInLruList: Get all buffers in a list specified at the LRU list.
 *                         This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
void GetBufInfoInLruList(Buf** ppBufInfo, int* pNumBuf)
{
	int i=0;
	Buf* buffer = (Buf*)malloc(sizeof(Buf*));
	TAILQ_FOREACH(buffer,&pLruListHead,llist)
	{
		ppBufInfo[i]=buffer;
		i++;
	}
	*pNumBuf=i;
}


/*
 * GetBufInfoInBufferList: Get all buffers in the buffer list.
 *                         This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
void GetBufInfoInBufferList(Buf** ppBufInfo, int* pNumBuf)
{
	int i=0;
	Buf* buffer = (Buf*)malloc(sizeof(Buf*));
	TAILQ_FOREACH(buffer,&pBufList,blist)
	{
		ppBufInfo[i]=buffer;
		i++;
	}
	*pNumBuf=i;
}
