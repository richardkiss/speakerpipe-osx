OBJS=speakerpipe.o threadedqueue.o audiopipeout.o resampler.o
CFLAGS=-g

speakerpipe: $(OBJS)
	$(CC) -g -o $@ $(OBJS) -framework CoreAudio

clean:
	rm -rf $(OBJS) speakerpipe
