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

#ifndef __resampler_h__
#define __resampler_h__

typedef void (*outputCallback)(void *context, const float *resampledData, unsigned resampledDataCount);

typedef struct
{
    float inputRate;
    float outputRate;
    float currentSampleCountRemaining;
    float currentSampleCumulative;
    void *context;
    outputCallback callback;
    unsigned outBufferSize;
    unsigned outBufferUsed;
    float *outBuffer;
} resampler;

resampler *resampler_new(float inputRate, float outputRate, outputCallback callback);
void resampler_free(resampler *rs);
void resampler_set_context(resampler *rs, void *context);
void resampler_scale_data(resampler *rs, float *inputData, unsigned inputDataCount);
void resampler_flush(resampler *rs);
void resampler_set_buffer_size(resampler *rs, unsigned newSize);

unsigned resampler_get_available_data(resampler *rs, float **bufferReference);
void resampler_clear_available_data(resampler *rs);

#endif /* __resampler_h__ */
