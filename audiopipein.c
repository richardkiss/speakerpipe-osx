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

#include "audiopipein.h"
#include <CoreAudio/CoreAudio.h>

static void resamplerCallback(void *context, const float *resampledData, unsigned resampledDataCount)
{
  threadedqueue *tq = (threadedqueue *)context;
  addBytes(tq, resampledData, resampledDataCount * sizeof(float));
}

static OSStatus audioProc(AudioDeviceID 	inDevice,
                const AudioTimeStamp*	inNow,
		const AudioBufferList*	inInputData,
		const AudioTimeStamp*	inInputTime,
		AudioBufferList* 	outOutputData, 
		const AudioTimeStamp*	inOutputTime,
		void*			inClientData)
{
  audiopipein *ap = (audiopipein *)inClientData;
  unsigned byteCount = inInputData->mBuffers[0].mDataByteSize;
  float *samples = (float*)inInputData->mBuffers[0].mData;

  if (ap->resampler != NULL) {
    scaleData(ap->resampler, samples, byteCount / sizeof(float));
    flushBuffer(ap->resampler);
  } else {
    addBytes(&ap->tq, samples, byteCount);
  }
  
  /* TODO -- pipe data into the threaded queue */
  /*
    BOOL shouldContinue = [pipe->callbackObject gotAudioBytes:(float*)inInputData->mBuffers[0].mData withSize:inInputData->mBuffers[0].mDataByteSize withContext:pipe->callbackContext];

    if (!shouldContinue)
    {
        // turn it off!
        AudioDeviceStop(inDevice, audioProc);
    }
    */
    return 0;
}

void init_audiopipein(audiopipein *ap, float rate, int isMono, int frameBufferSize)
{
  OSStatus s;
  AudioDeviceID inputDevice;
  Boolean writeable;
  UInt32 ioPropertyDataSize;
  
  init_threadedqueue(&ap->tq, frameBufferSize * sizeof(float));
  if (isMono) rate = rate / 2.0;
  ap->resampler = NULL;
  if (rate != 44100.0) {
    ap->resampler = createResampler(rate, 44100.0, resamplerCallback);
    setBufferSize(ap->resampler, 1024);
    setContext(ap->resampler, ap);
  }
  s = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultInputDevice, &ioPropertyDataSize, &writeable);

  s = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &ioPropertyDataSize, &inputDevice);

  s = AudioDeviceAddIOProc(inputDevice, audioProc, ap);
  s = AudioDeviceStart(inputDevice, audioProc);
}
#include <stdio.h>
unsigned read_s16_samples(audiopipein *ap, short *samples, unsigned maxFrameCount)
{
  unsigned samplesRead, loop;
  float *fsamples = (float*)alloca(2048 * sizeof(float));
  if (maxFrameCount > 2048) maxFrameCount = 2048;
  fprintf(stderr,"trying to read %d\n", maxFrameCount);
  samplesRead = read_float_samples(ap, fsamples, maxFrameCount);
  fprintf(stderr,"did read %d\n", samplesRead);

  loop = samplesRead;
  while (loop-- > 0) {
    *samples++ = *fsamples++ * 32768.0;
  }
  return samplesRead;
}

unsigned read_float_samples(audiopipein *ap, float *samples, unsigned maxFrameCount)
{
  /* read 'em from queue */
  unsigned bytesToMove = waitForMinimumBytes(&ap->tq, sizeof(float));
  bytesToMove = bytesToMove & (~7);
  if (bytesToMove > maxFrameCount * sizeof(float)) bytesToMove = maxFrameCount * sizeof(float);
  bytesToMove = removeBytesTo(&ap->tq, samples, bytesToMove, bytesToMove);
  return bytesToMove / sizeof(float);
}


#if 0
#define kMinMax 1024
#define kDefaultCapacity 163840

@implementation AudioInputPipe

static OSStatus audioProc(AudioDeviceID 	inDevice,
                const AudioTimeStamp*	inNow,
		const AudioBufferList*	inInputData,
		const AudioTimeStamp*	inInputTime,
		AudioBufferList* 	outOutputData, 
		const AudioTimeStamp*	inOutputTime,
		void*			inClientData)
{
    AudioInputPipe *pipe = (AudioInputPipe *)inClientData;

    BOOL shouldContinue = [pipe->callbackObject gotAudioBytes:(float*)inInputData->mBuffers[0].mData withSize:inInputData->mBuffers[0].mDataByteSize withContext:pipe->callbackContext];

    if (!shouldContinue)
    {
        // turn it off!
        AudioDeviceStop(inDevice, audioProc);
    }
    return 0;
}

-(void)precacheValues
{
    OSStatus s;
    UInt32 ioPropertyDataSize;
    Boolean writeable;
    AudioDeviceID inputDevice;
    s = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultInputDevice, &ioPropertyDataSize, &writeable);
    s = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &ioPropertyDataSize, &inputDevice);
    s = AudioDeviceGetPropertyInfo(inputDevice, 0, false, kAudioDevicePropertyBufferSize, &ioPropertyDataSize, &writeable);

    if (sizeof(bufferSizeInBytes) <= ioPropertyDataSize)
    {
        s = AudioDeviceGetProperty(inputDevice, 0, false, kAudioDevicePropertyBufferSize, &ioPropertyDataSize, &bufferSizeInBytes);
    }
}

-(id)initWithObject:(id)anObject withContext:(id)aContext
{
    [super init];
    callbackObject = [anObject retain];
    callbackContext = [aContext retain];
    [self precacheValues];
    return self;
}

-(void)dealloc
{
    [callbackObject release];
    [callbackContext release];
    [super dealloc];
}

-(void)startAudio:(double)aSampleRate
{
    OSStatus s;
    AudioStreamBasicDescription asbd;
    AudioDeviceID inputDevice;
    Boolean writeable;
    UInt32 ioPropertyDataSize;
    
    s = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultInputDevice, &ioPropertyDataSize, &writeable);

    s = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &ioPropertyDataSize, &inputDevice);

    s = AudioDeviceGetPropertyInfo(inputDevice, 0, false, kAudioDevicePropertyBufferSize, &ioPropertyDataSize, &writeable);
    if (sizeof(bufferSizeInBytes) <= ioPropertyDataSize)
    {
        s = AudioDeviceGetProperty(inputDevice, 0, false, kAudioDevicePropertyBufferSize, &ioPropertyDataSize, &bufferSizeInBytes);
    }
    s = AudioDeviceGetPropertyInfo(inputDevice, 0, false, kAudioDevicePropertyDeviceIsRunning, &ioPropertyDataSize, &writeable);
    s = AudioDeviceGetProperty(inputDevice, 0, false, kAudioDevicePropertyDeviceIsRunning, &ioPropertyDataSize, &writeable);

    s = AudioDeviceGetPropertyInfo(inputDevice, 0, false, kAudioDevicePropertyStreamFormat, &ioPropertyDataSize, &writeable);
    s = AudioDeviceGetProperty(inputDevice, 0, false, kAudioDevicePropertyStreamFormat, &ioPropertyDataSize, &asbd);

    asbd.mSampleRate = aSampleRate;

    s = AudioDeviceSetProperty(inputDevice, NULL, 0, false, kAudioDevicePropertyStreamFormat, sizeof(asbd), &asbd);
NSLog(@"asbd: %f, %d", aSampleRate, s);

    s = AudioDeviceAddIOProc(inputDevice, audioProc, self);

    s = AudioDeviceStart(inputDevice, audioProc);
}

+(id)listenWithSampleRateInHZ:(unsigned)aHZ withObject:(id<AudioInputCallback>)anObject withContext:(id)aContext
{
    id ai = [[[self alloc] initWithObject:anObject withContext:aContext] autorelease];
    [ai startAudio:aHZ];
    return ai;
}

@end
#endif
