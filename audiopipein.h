/*
* Copyright (c) 2000 By Richard Kiss
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*      This product includes software developed by Richard Kiss and
*      possibly other contributors.
* 4. The name Richard Kiss may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/


#ifndef __audiopipein_h__
#define __audiopipein_h__

#include "threadedqueue.h"
#include "resampler.h"

typedef struct {
  threadedqueue tq;
  Resampler *resampler;
} audiopipein;


/* Larger buffers reduces dropout probability. */
void init_audiopipein(audiopipein *ap, float rate, int isMono, int frameBufferSize);

unsigned read_s8_samples(audiopipein *ap, char *samples, unsigned maxFrameCount);
unsigned read_u8_samples(audiopipein *ap, unsigned char *samples, unsigned maxFrameCount);
unsigned read_s16_samples(audiopipein *ap, short *samples, unsigned maxFrameCount);
unsigned read_u16_samples(audiopipein *ap, unsigned short *samples, unsigned maxFrameCount);
unsigned read_s32_samples(audiopipein *ap, long *samples, unsigned maxFrameCount);
unsigned read_u32_samples(audiopipein *ap, unsigned long *samples, unsigned maxFrameCount);
unsigned read_float_samples(audiopipein *ap, float *samples, unsigned maxFrameCount);

/* utilities */
void swap_16_samples(short samples[], unsigned frameCount);
void swap_32_samples(long samples[], unsigned frameCount);

void destroy_audiopipein(audiopipein *ap);

#endif /* __audiopipein_h__ */
