all:
	gcc png2mode2sprites.c -lSDL2 -lSDL2_image -o png2mode2sprites

clean:
	rm png2mode2sprites

install:
	cp png2sc5raw /usr/local/bin

