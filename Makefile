all: bouncer

bouncer: bouncer.c
# rebuilds the bouncer application into an executable named "bouncer". 
	gcc -o bouncer bouncer.c `pkg-config --cflags --libs libavutil libavformat libavcodec libswscale`

clean:
# removes the bouncer application, all .o files, all movie files, 
# and all .mpff files from the directory.
	rm -f *.o bouncer *.mp4 *.mpff

movie:
# uses ffmpeg to assemble the 300 frames of the movie into 
# an .mp4 movie named "movie.mp4"
	ffmpeg -i frame%03d.mpff -q:v 0 -r 30 -t 10 movie.mp4

