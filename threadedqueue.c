/*
* Copyright (c) 2002 By Richard Kiss
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify,
* merge, publish, distribute, sublicense, and or sell copies of
* the Software, and to permit persons to whom the Software is furnished
* to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*/

#include "threadedqueue.h"

void init_threadedqueue(threadedqueue *q, unsigned bufferSize)
{
  pthread_mutex_init(&q->dataLock, NULL);
  pthread_cond_init(&q->addDataLock, NULL);
  pthread_cond_init(&q->removeDataLock, NULL);
  q->buffer = malloc(bufferSize);
  q->headPointer = 0;
  q->tailPointer = 0;
  q->bytesInQueue = 0;
  q->maxDataSize = bufferSize;
}

void destroy_threadedqueue(threadedqueue *q)
{
  pthread_cond_destroy(&q->removeDataLock);
  pthread_cond_destroy(&q->addDataLock);
  pthread_mutex_destroy(&q->dataLock);
  free(q->buffer);
}

void addBytes(threadedqueue *q, const void *bytesPtr, unsigned length)
{
  const char *mem = (const char*)bytesPtr;

  while (length > 0) {
    unsigned bytesToAdd;
    unsigned bytesAvailable;
    unsigned bytesToAddInPlace;
    int isWrapping;
    /* wait until it's safe to add something */
    pthread_mutex_lock(&q->dataLock);
    while (1) {
      /* figure out how much we can add now */
      bytesAvailable = q->maxDataSize - q->bytesInQueue;
      if (bytesAvailable > 0) break;
      pthread_cond_wait(&q->removeDataLock, &q->dataLock);
    }
    bytesToAdd = length;
    if (bytesToAdd > bytesAvailable) bytesToAdd = bytesAvailable;

    /* copy to circular buffer */
    bytesToAddInPlace = bytesToAdd;
    isWrapping = (bytesToAdd >= q->maxDataSize - q->headPointer);
    if (isWrapping) bytesToAddInPlace = q->maxDataSize - q->headPointer;
    memcpy(((char*)q->buffer)+q->headPointer, mem, bytesToAddInPlace);
    if (isWrapping) memcpy(q->buffer, mem+bytesToAddInPlace, bytesToAdd-bytesToAddInPlace);
    q->headPointer += bytesToAdd;
    if (isWrapping) q->headPointer -= q->maxDataSize;
    q->bytesInQueue += bytesToAdd;
    
    if (bytesToAdd > 0) {
      pthread_cond_broadcast(&q->addDataLock);
    }
    pthread_mutex_unlock(&q->dataLock);

    mem += bytesToAdd;
    bytesAvailable -= bytesToAdd;
    length -= bytesToAdd;
  }
}

unsigned peekBytes(threadedqueue *q, void **aBytesPtr)
{
  unsigned bytesToReturn;
  unsigned bytesAvailableAtEnd;
  
  /* wait until we have some data to remove */
  pthread_mutex_lock(&q->dataLock);
  while (1) {
    /* figure out how much we can add now */
    if (q->bytesInQueue > 0) break;
    pthread_cond_wait(&q->addDataLock, &q->dataLock);
  }

  bytesToReturn = q->bytesInQueue;
  bytesAvailableAtEnd = q->maxDataSize - q->tailPointer;
  *aBytesPtr = ((char*)q->buffer) + q->tailPointer;
  pthread_mutex_unlock(&q->dataLock);
  if (bytesToReturn > bytesAvailableAtEnd) bytesToReturn = bytesAvailableAtEnd;
  return bytesToReturn;
}

void removeBytes(threadedqueue *q, unsigned aByteCount)
{
  waitForMinimumBytes(q,aByteCount);
  pthread_mutex_lock(&q->dataLock);

  while (aByteCount > 0) {
    unsigned bytesToTake;
    unsigned bytesAvailableAtEnd;
    
    bytesAvailableAtEnd = q->maxDataSize - q->tailPointer;
    bytesToTake = q->bytesInQueue;
    if (bytesToTake > aByteCount) bytesToTake = aByteCount;
    if (bytesToTake >= bytesAvailableAtEnd) {
      bytesToTake = bytesAvailableAtEnd;
      q->tailPointer = 0;
    } else {
      q->tailPointer += bytesToTake;
    }
    aByteCount -= bytesToTake;
    q->bytesInQueue -= bytesToTake;
    pthread_cond_broadcast(&q->removeDataLock);
  }
  pthread_mutex_unlock(&q->dataLock);
}

unsigned spaceAvailable(threadedqueue *q)
{
  unsigned r;
  pthread_mutex_lock(&q->dataLock);
  r = q->maxDataSize - q->bytesInQueue;
  pthread_mutex_unlock(&q->dataLock);
  return r;
}

unsigned spaceUsed(threadedqueue *q)
{
  unsigned r;
  pthread_mutex_lock(&q->dataLock);
  r = q->bytesInQueue;
  pthread_mutex_unlock(&q->dataLock);
  return r;
}

unsigned waitForMinimumBytes(threadedqueue *q, unsigned minimum)
{
  unsigned r;
  pthread_mutex_lock(&q->dataLock);
  while ((r = q->bytesInQueue) < minimum) {
    pthread_cond_wait(&q->addDataLock, &q->dataLock);
  }
  pthread_mutex_unlock(&q->dataLock);
  return r;
}

unsigned removeBytesTo(threadedqueue *q, void *bytesPtr, unsigned minimum, unsigned maximum)
{
  unsigned totalWrit = 0;
  char *dest = (char*)bytesPtr;
  void *src;
  unsigned available;

  do {
    unsigned bytesAvailableAtEnd;
    if (minimum > 0) waitForMinimumBytes(q, minimum);

    pthread_mutex_lock(&q->dataLock);
    available = q->bytesInQueue;
    bytesAvailableAtEnd = q->maxDataSize - q->tailPointer;
    src = ((char*)q->buffer) + q->tailPointer;

    if (available > bytesAvailableAtEnd) available = bytesAvailableAtEnd;

    if (available > maximum) available = maximum;
    memcpy(dest, src, available);

    // remove 'em
    q->tailPointer += available;
    if (q->tailPointer >= q->maxDataSize) q->tailPointer -= q->maxDataSize;
    q->bytesInQueue -= available;

    pthread_cond_broadcast(&q->removeDataLock);

    pthread_mutex_unlock(&q->dataLock);

    minimum -= available;
    maximum -= available;
    totalWrit += available;
    dest += available;
  } while (minimum > 0);
  return totalWrit;
}

