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

#include <CoreAudio/AudioHardware.h>
#include <stdio.h>
#include <unistd.h>

enum { U8BIT_SAMPLE, S8BIT_SAMPLE,
       U16BIT_SAMPLE, S16BIT_SAMPLE,
       U16BIT_SE_SAMPLE, S16BIT_SE_SAMPLE,
       U32BIT_SAMPLE, S32BIT_SAMPLE,
       U32BIT_SE_SAMPLE, S32BIT_SE_SAMPLE,
       FLOAT_SAMPLE };

int sampleType;

int channelCount = 2;
int sampleRate = 44100;

int bytesPerSample = 2;

int sampleRepeatCount;
UInt32 bufferSizeInBytes = 65536;
int readBufferSizeInBytes;
char *readBuffer;
int isStopping = 0;

static OSStatus audioProc(AudioDeviceID 	inDevice,
			  const AudioTimeStamp*	inNow,
			  const AudioBufferList*inInputData,
			  const AudioTimeStamp*	inInputTime,
			  AudioBufferList*	outOutputData, 
			  const AudioTimeStamp*	inOutputTime,
			  void* 		inClientData)
{
    AudioBuffer *buffer = outOutputData->mBuffers;
    float *dst = (float *)buffer->mData;
    char *src = readBuffer;
    unsigned readByteCount = 0;
    unsigned srcSamples;
    signed char *scsrc;
    short *ssrc;
    long *lsrc;
    float *fsrc;
    float sample;
    int r;

    if (isStopping) exit(1);

    while (readByteCount<readBufferSizeInBytes) {
      int count = fread(src+readByteCount, 1, readBufferSizeInBytes - readByteCount, stdin);
      readByteCount += count;
      if (count == 0) {
	/* EOF -- stop next time through */
	bzero(src + readByteCount, readBufferSizeInBytes - readByteCount);
	readByteCount = readBufferSizeInBytes;
	isStopping = 1;
	break;
      }
    }

    switch (sampleType) {
    case U8BIT_SAMPLE:
      srcSamples = readByteCount;
      scsrc = (signed char*)src;
      while (srcSamples-->0) {
	  sample = ((signed char)(*scsrc++ - 128)) / 128.0;
	  r = sampleRepeatCount;
	  while (r-->0) *dst++ = sample;
      }
      break;
    case S8BIT_SAMPLE:
      srcSamples = readByteCount;
      scsrc = (signed char*)src;
      while (srcSamples-->0) {
	sample = (*scsrc++) / 128.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;
    case U16BIT_SAMPLE:
      srcSamples = readByteCount >> 1;
      ssrc = (signed short*)src;
      while (srcSamples-->0) {
	sample = ((signed short)(*ssrc++ - 32768)) / 32768.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;
    case S16BIT_SAMPLE:
      srcSamples = readByteCount >> 1;
      ssrc = (signed short*)src;
      while (srcSamples-->0) {
	sample = *ssrc++ / 32768.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;
    case U16BIT_SE_SAMPLE:
      srcSamples = readByteCount >> 1;
      ssrc = (signed short*)src;
      while (srcSamples-->0) {
	r = *ssrc++;
	r = (unsigned short)((r&0xff)<<8) | ((r&0xff00)>>8);
	sample = ((signed short)(r - 32768)) / 32768.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;
    case S16BIT_SE_SAMPLE:
      srcSamples = readByteCount >> 1;
      ssrc = (signed short*)src;
      while (srcSamples-->0) {
	r = *ssrc++;
	r = (short)((r&0xff)<<8) | ((r&0xff00)>>8);
	sample = r / 32768.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;

    case U32BIT_SAMPLE:
      srcSamples = readByteCount >> 2;
      lsrc = (signed long*)src;
      while (srcSamples-->0) {
	sample = ((signed long)(*lsrc++ - 2147483648)) / 2147483648.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;
    case S32BIT_SAMPLE:
      srcSamples = readByteCount >> 2;
      lsrc = (signed long*)src;
      while (srcSamples-->0) {
	sample = *lsrc++ / 2147483648.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;
    case U32BIT_SE_SAMPLE:
      srcSamples = readByteCount >> 2;
      lsrc = (signed long*)src;
      while (srcSamples-->0) {
	r = *lsrc++;
	r = (unsigned long)((r&0xff)<<24) | ((r&0xff00)<<8) | ((r&0xff0000) >> 8) | ((r&0xff000000)>>24);
	sample = ((signed long)(r - 2147483648)) / 2147483648.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;
    case S32BIT_SE_SAMPLE:
      srcSamples = readByteCount >> 2;
      lsrc = (signed long*)src;
      while (srcSamples-->0) {
	r = *lsrc++;
	r = (unsigned long)((r&0xff)<<24) | ((r&0xff00)<<8) | ((r&0xff0000) >> 8) | ((r&0xff000000)>>24);
	sample = r / 2147483648.0;
	r = sampleRepeatCount;
	while (r-->0) *dst++ = sample;
      }
      break;
    case FLOAT_SAMPLE:
      srcSamples = readByteCount >> 2;
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

    return 0;
}


static char *tool;

static void usage() {
  printf("usage: %s [-c channelCount (1 or 2)] [-s|-u|-f] [-b|-w|-l] [-x] [-r rate]\n", tool);
  exit(1);
}

#define false 0
#define true 1

int main(int argc, char *argv[]) {
  float ratio;
  char ch;
  enum { SIGNED, UNSIGNED, FLOAT };
  int swapEndian = 0;
  int sampleFormat = SIGNED;

  OSStatus s;
  AudioDeviceID outputDevice;
  Boolean writeable;
  UInt32 ioPropertyDataSize;

  tool = argv[0];
  while ((ch = getopt(argc, argv, "c:sufbwlxr:")) != -1)
    switch(ch) {
    case 'c':
      channelCount = atoi(optarg);
      break;
    case 's':
      sampleFormat = SIGNED;
      break;
    case 'u':
      sampleFormat = UNSIGNED;
      break;
    case 'f':
      bytesPerSample = 4;
      sampleFormat = FLOAT;
      break;
    case 'b':
      bytesPerSample = 1;
      break;
    case 'w':
      bytesPerSample = 2;
      break;
    case 'l':
      bytesPerSample = 4;
      break;
    case 'x':
      swapEndian = 1;
      break;
    case 'r':
      sampleRate = atoi(optarg);
      break;
    case '?':
    default:
      usage();
    }
  argc -= optind;
  argv += optind;

  /* figure sample type */
  if ((sampleFormat == FLOAT) && (swapEndian)) {
    fprintf(stderr, "Can't use swap (-x) with float (-f)\n");
    usage();
  }
  switch (sampleFormat) {
  case SIGNED:
    if (bytesPerSample == 1) sampleType = S8BIT_SAMPLE;
    else if (bytesPerSample == 2) {
      if (swapEndian) sampleType = S16BIT_SE_SAMPLE;
      else sampleType = S16BIT_SAMPLE;
    }
    else if (bytesPerSample == 4) {
      if (swapEndian) sampleType = S32BIT_SE_SAMPLE;
      else sampleType = S32BIT_SAMPLE;
    }
    break;
  case UNSIGNED:
    if (bytesPerSample == 1) sampleType = U8BIT_SAMPLE;
    else if (bytesPerSample == 2) {
      if (swapEndian) sampleType = U16BIT_SE_SAMPLE;
      else sampleType = U16BIT_SAMPLE;
    }
    else if (bytesPerSample == 4) {
      if (swapEndian) sampleType = U32BIT_SE_SAMPLE;
      else sampleType = U32BIT_SAMPLE;
    }
    break;
  case FLOAT:
    sampleType = FLOAT_SAMPLE;
  }

  ratio = (44100.0 / sampleRate);
  if (ratio > (int)ratio) usage();

  if ((channelCount < 1) || (channelCount > 2)) usage();

  /* figure sampleRepeatCount */
  sampleRepeatCount = ratio * 2 / channelCount;

  s = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDefaultOutputDevice, &ioPropertyDataSize, &writeable);

  s = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &ioPropertyDataSize, &outputDevice);

  s = AudioDeviceGetPropertyInfo(outputDevice, 0, false, kAudioDevicePropertyBufferSize, &ioPropertyDataSize, &writeable);
  if (sizeof(bufferSizeInBytes) <= ioPropertyDataSize) {
    s = AudioDeviceGetProperty(outputDevice, 0, false, kAudioDevicePropertyBufferSize, &ioPropertyDataSize, &bufferSizeInBytes);
  }

  readBufferSizeInBytes = ((((bytesPerSample * bufferSizeInBytes + (sizeof(float)-1)) / sizeof(float)) + sampleRepeatCount-1)) / sampleRepeatCount;
  readBuffer = (char*)malloc(readBufferSizeInBytes);

  s = AudioDeviceAddIOProc(outputDevice, audioProc, NULL);
  s = AudioDeviceStart(outputDevice, audioProc);

  while (1) {
    sleep(5);
  }
}
