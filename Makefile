SPKR_OBJS=speakerpipe.o threadedqueue.o audiopipeout.o resampler.o swap.o
MIKE_OBJS=mikepipe.o threadedqueue.o audiopipein.o resampler.o swap.o
CFLAGS=-g -Wall

all: mikepipe speakerpipe

mikepipe: $(MIKE_OBJS)
	$(CC) -g -o $@ $(MIKE_OBJS) -framework CoreAudio

speakerpipe: $(SPKR_OBJS)
	$(CC) -g -o $@ $(SPKR_OBJS) -framework CoreAudio

clean:
	rm -rf $(SPKR_OBJS) $(MIKE_OBJS) speakerpipe mikepipe
