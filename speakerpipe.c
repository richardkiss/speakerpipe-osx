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
#include "audiopipeout.h"
#include "swap.h"
#include "version.h"

static char *tool;

static void usage() {
  fprintf(stderr, "usage: %s [-v] [-c channelCount (1 or 2)] [-s|-u|-f] [-b|-w|-l] [-x] [-r rate]\n", tool);
  fprintf(stderr, " -v : show version and exit\n");
  fprintf(stderr, " -s : signed samples\n");
  fprintf(stderr, " -u : unsigned\n");
  fprintf(stderr, " -f : float\n");
  fprintf(stderr, " -b : 1 byte per sample\n");
  fprintf(stderr, " -w : 2 bytes per sample\n");
  fprintf(stderr, " -l : 4 bytes per sample\n");
  fprintf(stderr, " -x : use opposite endian\n");
  fprintf(stderr, " -r : sample rate, defaults to 44.1 kHz\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  char ch;
  enum { SIGNED, UNSIGNED, FLOAT };
  int swapEndian = 0;
  int sampleFormat = SIGNED;
  int channelCount = 2;
  float sampleRate = 44100;
  int bytesPerSample = 2;

  audiopipeout ap;

  tool = argv[0];
  while ((ch = getopt(argc, argv, "c:sufbwlxr:v")) != -1)
    switch(ch) {
    case 'v':
      fprintf(stderr, "%s version %s\n", tool, VERSION);
      exit(-1);
      break;
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

  if ((channelCount < 1) || (channelCount > 2)) usage();

  init_audiopipeout(&ap, sampleRate, channelCount == 1, 131072);

  /* a couple of macros to make life easier */

#define FEEDLOOP(BUFFERTYPE, WRITE) \
while (1) {\
    count = fread(buf, bytesPerSample, 4096, stdin);\
    if (count == 0) break;\
    WRITE(&ap, (BUFFERTYPE*)buf, count);}

#define FEEDSWAPLOOP(BUFFERTYPE, SWAP, WRITE) \
while (1) {\
    count = fread(buf, bytesPerSample, 4096, stdin);\
    if (count == 0) break;\
    if (swapEndian) SWAP((BUFFERTYPE*)buf, count);\
    WRITE(&ap, (BUFFERTYPE*)buf, count);}

  switch (sampleFormat) {
    char buf[4096*8];
    unsigned count;
    
  case SIGNED:
    if (bytesPerSample == 1) FEEDLOOP(char, write_s8_samples)
    else if (bytesPerSample == 2) FEEDSWAPLOOP(short, swap_16_samples, write_s16_samples)
    else if (bytesPerSample == 4) FEEDSWAPLOOP(long, swap_32_samples, write_s32_samples);
    break;
  case UNSIGNED:
    if (bytesPerSample == 1) FEEDLOOP(char, write_u8_samples)
    else if (bytesPerSample == 2) FEEDSWAPLOOP(unsigned short, swap_16_samples, write_u16_samples)
    else if (bytesPerSample == 4) FEEDSWAPLOOP(unsigned long, swap_32_samples, write_u32_samples);
    break;
  case FLOAT:
    FEEDLOOP(float, write_float_samples);
  }

  while (1) sleep(5);
}
