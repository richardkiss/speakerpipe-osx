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

static char *tool;

static void usage() {
  printf("usage: %s [-c channelCount (1 or 2)] [-s|-u|-f] [-b|-w|-l] [-x] [-r rate]\n", tool);
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

  if ((channelCount < 1) || (channelCount > 2)) usage();

  init_audiopipein(&ap, sampleRate, channelCount == 1, 4096);

  while (1) {
    short samples[4096];
    unsigned frames = read_s16_samples(&ap, samples, 4096);
    fprintf(stderr, "Read %d frames\n", frames);
    write(1, samples, frames * sizeof(short));
  }

  while (1) sleep(5);
}
