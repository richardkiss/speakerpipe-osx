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

#include "resampler.h"
#include <assert.h>
#include <stdlib.h>

Resampler *createResampler(float inputRate, float outputRate, outputCallback callback)
{
    Resampler *rs = (Resampler *)malloc(sizeof(Resampler));
    rs->inputRate = inputRate;
    rs->outputRate = outputRate;
    rs->context = NULL;
    rs->callback = callback;
    rs->currentSampleCountRemaining = inputRate;
    rs->currentSampleCumulative = 0.0;
    rs->outBufferSize = 2048;
    rs->outBufferUsed = 0;
    rs->outBuffer = (float*)malloc(rs->outBufferSize * sizeof(float));
    return rs;
}

void destroyResampler(Resampler *rs)
{
    free(rs->outBuffer);
    free(rs);
}

void setContext(Resampler *rs, void *context)
{
    rs->context = context;
}

void scaleData(Resampler *rs, float *inputData, unsigned inputDataCount)
{
    unsigned outputDataCount = rs->outBufferUsed;
    float currentSampleCountRemaining = rs->currentSampleCountRemaining;
    float outputRate = rs->outputRate;
    float currentSampleCumulative = rs->currentSampleCumulative;
    float *outBuffer = rs->outBuffer;
    unsigned outBufferSize = rs->outBufferSize;
    float inputRate = rs->inputRate;
    void *context = rs->context;
    outputCallback callback = rs->callback;

    while (inputDataCount > 0)
    {
        // iterate over input sample
        float inputAvailableNumerator = outputRate;
        float currentSample = *inputData;
        // figure out how much of input to use
        while (currentSampleCountRemaining <= inputAvailableNumerator)
        {
            // finish off current sample
            currentSampleCumulative += currentSample * currentSampleCountRemaining;
            inputAvailableNumerator -= currentSampleCountRemaining;
            if (outputDataCount >= outBufferSize)
            {
                if (callback == NULL)
                {
                    // double buffer size
                    outBufferSize += outBufferSize;
                    outBuffer = rs->outBuffer = (float*)realloc(outBuffer, outBufferSize * sizeof(float));
                    rs->outBufferSize = outBufferSize;
                }
                else
                {
                    callback(context, outBuffer, outputDataCount);
                    outputDataCount = 0;
                }
            }
            outBuffer[outputDataCount++] = currentSampleCumulative / inputRate;
            currentSampleCumulative = 0.0;
            currentSampleCountRemaining = rs->inputRate;
        }
        if (inputAvailableNumerator > 0.0)
        {
            currentSampleCountRemaining -= inputAvailableNumerator;
            currentSampleCumulative += currentSample * inputAvailableNumerator;
            inputAvailableNumerator = 0.0;
        }
        inputDataCount--;
        inputData++;
    }
    rs->currentSampleCountRemaining = currentSampleCountRemaining;
    rs->currentSampleCumulative = currentSampleCumulative;
    rs->outBufferUsed = outputDataCount;
}

void flushBuffer(Resampler *rs)
{
    assert(rs->outBuffer != NULL);
    rs->callback(rs->context, rs->outBuffer, rs->outBufferUsed);
    rs->outBufferUsed = 0;
}

void setBufferSize(Resampler *rs, unsigned newSize)
{
    if (newSize < rs->outBufferUsed) flushBuffer(rs);
    rs->outBuffer = (float*)realloc(rs->outBuffer, newSize * sizeof(float));
    rs->outBufferSize = newSize;
}

unsigned getAvailableData(Resampler *rs, float **bufferReference)
{
    *bufferReference = rs->outBuffer;
    return rs->outBufferUsed;
}

void clearAvailableData(Resampler *rs)
{
    rs->outBufferUsed = 0;
}
