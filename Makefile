OBJS=speakerpipe.o threadedqueue.o

speakerpipe: $(OBJS)
	$(CC) -o $@ $(OBJS) -framework CoreAudio

clean:
	rm -rf $(OBJS) speakerpipe
