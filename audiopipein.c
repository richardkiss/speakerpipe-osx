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
    resampler_scale_data(ap->resampler, samples, byteCount / sizeof(float));
    resampler_flush(ap->resampler);
  } else {
    addBytes(&ap->tq, samples, byteCount);
  }
  return 0;
}

audiopipein *api_new(float rate, int isMono, int frameBufferSize)
{
  OSStatus s;
  AudioDeviceID inputDevice;
  Boolean writeable;
  UInt32 ioPropertyDataSize;
  audiopipein *ap = (audiopipein*)malloc(sizeof(audiopipein));
  
  init_threadedqueue(&ap->tq, frameBufferSize * sizeof(float));
  if (isMono) rate = rate / 2.0;
  ap->resampler = NULL;
  if (rate != 44100.0) {
    ap->resampler = resampler_new(44100.0, rate, resamplerCallback);
    resampler_set_buffer_size(ap->resampler, 1024);
    resampler_set_context(ap->resampler, ap);
  }
  s = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultInputDevice, &ioPropertyDataSize, &writeable);

  s = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &ioPropertyDataSize, &inputDevice);

  s = AudioDeviceAddIOProc(inputDevice, audioProc, ap);
  s = AudioDeviceStart(inputDevice, audioProc);
  return ap;
}

#define DECLARE(NAME, TYPE, ADDITOR, MULTIPLIER) \
unsigned NAME(audiopipein *ap, TYPE samples[], unsigned maxFrameCount) { \
  unsigned samplesRead, loop; \
  float *fsamples = (float*)alloca(2048 * sizeof(float)); \
  if (maxFrameCount > 2048) maxFrameCount = 2048; \
  samplesRead = api_read_float_samples(ap, fsamples, maxFrameCount); \
  loop = samplesRead; \
  while (loop-- > 0) { \
    *samples++ = *fsamples++ * MULTIPLIER + ADDITOR; \
  } \
  return samplesRead; \
}

DECLARE(api_read_s8_samples, char, 0, 0x7f)
DECLARE(api_read_s16_samples, short, 0, 0x7fff)
DECLARE(api_read_s32_samples, long, 0, 0x7fffffff)
DECLARE(api_read_u8_samples, unsigned char, 0x80, 0x7f)
DECLARE(api_read_u16_samples, unsigned short, 0x8000, 0x7fff)
DECLARE(api_read_u32_samples, unsigned long, ((unsigned)0x80000000), 0x7fffffff)

unsigned api_read_float_samples(audiopipein *ap, float *samples, unsigned maxFrameCount)
{
  /* read 'em from queue */
  unsigned bytesToMove = waitForMinimumBytes(&ap->tq, sizeof(float));
  bytesToMove = bytesToMove & (~(sizeof(float)-1));
  if (bytesToMove > maxFrameCount * sizeof(float)) bytesToMove = maxFrameCount * sizeof(float);
  bytesToMove = removeBytesTo(&ap->tq, samples, bytesToMove, bytesToMove);
  return bytesToMove / sizeof(float);
}

void api_free(audiopipein *ap)
{
  destroy_threadedqueue(&ap->tq);
  if (ap->resampler) resampler_free(ap->resampler);
  free(ap);
}

