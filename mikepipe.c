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
#include "audiopipein.h"
#include "swap.h"

static char *tool;

static void usage() {
  fprintf(stderr, "usage: %s [-c channelCount (1 or 2)] [-s|-u|-f] [-b|-w|-l] [-x] [-r rate]\n", tool);
  exit(1);
}

#define MAX_FRAME_COUNT 4096
#define MAX_FRAME_SIZE (sizeof(float))

typedef unsigned (*ReadSamplesFunction)(audiopipein *, void *samples, unsigned maxFrameCount);

int main(int argc, char *argv[]) {
  char ch;
  enum { SIGNED, UNSIGNED, FLOAT };
  int swapEndian = 0;
  int sampleFormat = SIGNED;
  int channelCount = 2;
  float sampleRate = 44100;
  int bytesPerSample = 2;
  char sampleBuffer[MAX_FRAME_COUNT * MAX_FRAME_SIZE];
  ReadSamplesFunction readSamplesFunction;
  audiopipein ap;

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
      sampleRate = atof(optarg);
      break;
    case '?':
    default:
      usage();
    }
  argc -= optind;
  argv += optind;

  if (argc > 0) usage();

  /* figure sample type */
  if ((sampleFormat == FLOAT) && (swapEndian)) {
    fprintf(stderr, "Can't use swap (-x) with float (-f)\n");
    usage();
  }

  switch (sampleFormat) {
  case SIGNED:
    if (bytesPerSample == 1) readSamplesFunction = (ReadSamplesFunction)read_s8_samples;
    else if (bytesPerSample == 2) readSamplesFunction = (ReadSamplesFunction)read_s16_samples;
    else if (bytesPerSample == 4) readSamplesFunction = (ReadSamplesFunction)read_s32_samples;
    break;
  case UNSIGNED:
    if (bytesPerSample == 1) readSamplesFunction = (ReadSamplesFunction)read_u8_samples;
    else if (bytesPerSample == 2) readSamplesFunction = (ReadSamplesFunction)read_u16_samples;
    else if (bytesPerSample == 4) readSamplesFunction = (ReadSamplesFunction)read_u32_samples;
    break;
  case FLOAT:
    readSamplesFunction = (ReadSamplesFunction)read_float_samples;
  }

  if ((channelCount < 1) || (channelCount > 2)) usage();

  init_audiopipein(&ap, sampleRate, channelCount == 1, MAX_FRAME_COUNT);

  while (1) {
    unsigned frames = readSamplesFunction(&ap, sampleBuffer, MAX_FRAME_COUNT);
    if (swapEndian) {
      if (bytesPerSample == 2) swap_16_samples((short*)sampleBuffer, frames);
      if (bytesPerSample == 4) swap_32_samples((long*)sampleBuffer, frames);
    }
    write(1, sampleBuffer, frames * bytesPerSample);
  }

  while (1) sleep(5);
}
