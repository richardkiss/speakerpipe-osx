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

#ifndef __threaded_queue_h__
#define __threaded_queue_h__

#include <stdlib.h>
#include <pthread.h>

typedef struct
{
  pthread_mutex_t dataLock;
  pthread_cond_t addDataLock;
  pthread_cond_t removeDataLock;
  void *buffer;
  unsigned headPointer;
  unsigned tailPointer;
  unsigned bytesInQueue;
  unsigned maxDataSize;
} threadedqueue;

void init_threadedqueue(threadedqueue *q, unsigned bufferSize);
void addBytes(threadedqueue *q, const void *bytesPtr, unsigned length);
unsigned peekBytes(threadedqueue *q, void **aBytesPtr);
void removeBytes(threadedqueue *q, unsigned aByteCount);
unsigned waitForMinimumBytes(threadedqueue *q, unsigned minimum);
unsigned removeBytesTo(threadedqueue *q, void *bytesPtr, unsigned minimum, unsigned maximum);
void destroy_threadedqueue(threadedqueue *q);

#endif /* __threaded_queue_h__ */
