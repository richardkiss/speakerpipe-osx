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

#include "audiopipeout.h"
#include <CoreAudio/CoreAudio.h>
#include <string.h>

static OSStatus audioProc(AudioDeviceID inDevice,
			  const AudioTimeStamp *inNow,
			  const AudioBufferList *inInputData,
			  const AudioTimeStamp *inInputTime,
			  AudioBufferList *outOutputData, 
			  const AudioTimeStamp *inOutputTime,
			  void *inClientData)
{
    audiopipeout *pipe = (audiopipeout *)inClientData;
    AudioBuffer *buffer = outOutputData->mBuffers;

    unsigned readBytes = removeBytesTo(&pipe->tq, buffer->mData, buffer->mDataByteSize, buffer->mDataByteSize);
    if (readBytes < buffer->mDataByteSize) {
      /* fill remainder with nulls */
      bzero(((char*)buffer->mData) + readBytes, buffer->mDataByteSize - readBytes);
    }
    return 0;
}

static void resamplerCallback(void *context, const float *resampledData, unsigned resampledDataCount)
{
  threadedqueue *tq = (threadedqueue *)context;
  addBytes(tq, resampledData, resampledDataCount * sizeof(float));
}

audiopipeout *apo_new(float rate, int isMono, int frameBufferSize)
{
  OSStatus s;
  AudioDeviceID outputDevice;
  Boolean writeable;
  UInt32 ioPropertyDataSize;
  audiopipeout *ap = (audiopipeout*)malloc(sizeof(audiopipeout));
  init_threadedqueue(&ap->tq, frameBufferSize * sizeof(float));
  if (isMono) rate = rate / 2.0;
  ap->resampler = NULL;
  if (rate != 44100.0) {
    ap->resampler = resampler_new(rate, 44100.0, resamplerCallback);
    resampler_set_buffer_size(ap->resampler, 1024);
    resampler_set_context(ap->resampler, ap);
  }
  s = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultOutputDevice, &ioPropertyDataSize, &writeable);

  s = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &ioPropertyDataSize, &outputDevice);

  s = AudioDeviceAddIOProc(outputDevice, audioProc, ap);
  s = AudioDeviceStart(outputDevice, audioProc);
  return ap;
}

#define DECLARE(NAME, TYPE, SUBTRACTAND, DIVISOR) \
void NAME(audiopipeout *ap, TYPE samples[], unsigned frameCount) { \
  const int kMaxSamples = 1024; \
  float fBuf[kMaxSamples]; \
  float *dst; \
  TYPE *src = samples; \
  while (frameCount > 0) { \
    unsigned toConvert = frameCount; \
    unsigned count; \
    if (toConvert > kMaxSamples) toConvert = kMaxSamples; \
    dst = fBuf; \
    count = toConvert; \
    while (count-- > 0) { \
      *dst++ = (*src++ - SUBTRACTAND) / ((float)DIVISOR); \
    } \
    apo_write_float_samples(ap, fBuf, toConvert); \
    frameCount -= toConvert; \
  } \
}

DECLARE(apo_write_s8_samples, char, 0, 128)
DECLARE(apo_write_u8_samples, unsigned char, 128, 128)
DECLARE(apo_write_s16_samples, short, 0, 32768)
DECLARE(apo_write_u16_samples, unsigned short, 32768, 32768)
DECLARE(apo_write_s32_samples, long, 0, 0x80000000)
DECLARE(apo_write_u32_samples, unsigned long, 2147483648.0, 0x80000000)

void apo_write_float_samples(audiopipeout *ap, float samples[], unsigned frameCount)
{
  /* should we adjust for rate shift? */
  if (ap->resampler != NULL) {
    resampler_scale_data(ap->resampler, samples, frameCount);
    resampler_flush(ap->resampler);
  } else {
    addBytes(&ap->tq, samples, frameCount * sizeof(float));
  }
}

void apo_wait_until_done(audiopipeout *ap)
{
  /* TODO */
}

void apo_free(audiopipeout *ap)
{
  destroy_threadedqueue(&ap->tq);
  if (ap->resampler) resampler_free(ap->resampler);
  free(ap);
}

