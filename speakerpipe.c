/*
* Copyright (c) 2000 By Richard Kiss
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <CoreAudio/AudioHardware.h>

#define kMinMax 1024
#define kDefaultCapacity 163840

enum { U8BIT_SAMPLE, S8BIT_SAMPLE, U16BIT_SAMPLE, S16BIT_SAMPLE,
       U16BIT_LE_SAMPLE, S16BIT_LE_SAMPLE,
       FLOAT_SAMPLE };

int sampleType = S16BIT_LE_SAMPLE;
int sampleRepeatCount = 2;

#define BUFFER_SIZE 1024

static OSStatus audioProc(AudioDeviceID 	inDevice,
                const AudioTimeStamp*	inNow,
		const AudioBufferList*	inInputData,
		const AudioTimeStamp*	inInputTime,
		AudioBufferList*	outOutputData, 
		const AudioTimeStamp*	inOutputTime,
		void* 			inClientData)
{
    AudioBuffer *buffer = outOutputData->mBuffers;
    float *dst = (float *)buffer->mData;
    char *src[BUFFER_SIZE];
    unsigned readByteCount = read(0, src, BUFFER_SIZE);

    if (readByteCount == 0) exit(1);

    while (readByteCount>0) {
      unsigned bytesToCopy = readByteCount;
      unsigned srcSamples;
      signed char *scsrc;
      short *ssrc;
      float *fsrc;
      float sample;
      int r;

      if (bytesToCopy > readByteCount) bytesToCopy = readByteCount;
      switch (sampleType) {
      case U8BIT_SAMPLE:
	srcSamples = bytesToCopy;
	scsrc = (signed char*)src;
	while (srcSamples-->0) {
	  sample = ((signed char)(*scsrc++ - 128)) / 128.0;
	  r = sampleRepeatCount;
	  while (r-->0) *dst++ = sample;
	}
	break;
      case S8BIT_SAMPLE:
	srcSamples = bytesToCopy;
	scsrc = (signed char*)src;
	while (srcSamples-->0) {
	  sample = (*scsrc++) / 128.0;
	  r = sampleRepeatCount;
	  while (r-->0) *dst++ = sample;
	}
	break;
      case U16BIT_SAMPLE:
	srcSamples = bytesToCopy >> 1;
	ssrc = (signed short*)src;
	while (srcSamples-->0) {
	  sample = ((signed short)(*ssrc++ - 32768)) / 32768.0;
	  r = sampleRepeatCount;
	  while (r-->0) *dst++ = sample;
	}
	break;
      case S16BIT_SAMPLE:
	srcSamples = bytesToCopy >> 1;
	ssrc = (signed short*)src;
	while (srcSamples-->0) {
	  sample = *ssrc++ / 32768.0;
	  r = sampleRepeatCount;
	  while (r-->0) *dst++ = sample;
	}
	break;
      case U16BIT_LE_SAMPLE:
	srcSamples = bytesToCopy >> 1;
	ssrc = (signed short*)src;
	while (srcSamples-->0) {
	  r = *ssrc++;
	  r = (unsigned short)((r&0xff)<<8) | ((r&0xff00)>>8);
	  sample = ((signed short)(r - 32768)) / 32768.0;
	  r = sampleRepeatCount;
	  while (r-->0) *dst++ = sample;
	}
	break;
      case S16BIT_LE_SAMPLE:
	srcSamples = bytesToCopy >> 1;
	ssrc = (signed short*)src;
	while (srcSamples-->0) {
	  r = *ssrc++;
	  r = (short)((r&0xff)<<8) | ((r&0xff00)>>8);
	  sample = r / 32768.0;
	  r = sampleRepeatCount;
	  while (r-->0) *dst++ = sample;
	}
	break;
      case FLOAT_SAMPLE:
	srcSamples = bytesToCopy >> 2;
	fsrc = (float*)src;
	while (srcSamples-->0) {
	  sample = *fsrc++;
	  r = sampleRepeatCount;
	  while (r-->0) *dst++ = sample;
	}
	break;
      default:
	break;
      }
      readByteCount -= bytesToCopy;
    }
    
    return 0;
}


#define false 0
#define true 1

UInt32 bufferSizeInBytes;

int main()
{
  // first, let's just get it working with 44.1 kHz, 16-bit signed stereo samples

  OSStatus s;
  AudioStreamBasicDescription asbd;
  AudioDeviceID outputDevice;
  Boolean writeable;
  UInt32 ioPropertyDataSize;

  s = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultOutputDevice, &ioPropertyDataSize, &writeable);

  s = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &ioPropertyDataSize, &outputDevice);

  s = AudioDeviceGetPropertyInfo(outputDevice, 0, false, kAudioDevicePropertyBufferSize, &ioPropertyDataSize, &writeable);
  if (sizeof(bufferSizeInBytes) <= ioPropertyDataSize) {
    s = AudioDeviceGetProperty(outputDevice, 0, false, kAudioDevicePropertyBufferSize, &ioPropertyDataSize, &bufferSizeInBytes);
  }
  s = AudioDeviceGetPropertyInfo(outputDevice, 0, false, kAudioDevicePropertyDeviceIsRunning, &ioPropertyDataSize, &writeable);
  s = AudioDeviceGetProperty(outputDevice, 0, false, kAudioDevicePropertyDeviceIsRunning, &ioPropertyDataSize, &writeable);

  s = AudioDeviceGetPropertyInfo(outputDevice, 0, false, kAudioDevicePropertyStreamFormat, &ioPropertyDataSize, &writeable);
  s = AudioDeviceGetProperty(outputDevice, 0, false, kAudioDevicePropertyStreamFormat, &ioPropertyDataSize, &asbd);

  s = AudioDeviceAddIOProc(outputDevice, audioProc, NULL);

  s = AudioDeviceStart(outputDevice, audioProc);

  while (1) {
    sleep(5);
  }
}

