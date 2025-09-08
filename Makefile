all:
	gcc png2mode2sprites.c -lSDL2 -lSDL2_image -o png2mode2sprites

clean:
	rm png2mode2sprites *.exe

install:
	cp png2mode2sprites /usr/local/bin

windows:
	x86_64-w64-mingw32-gcc png2mode2sprites.c -o png2mode2sprites.exe -lmingw32 -lSDL2main -lSDL2 -lSDL2_image

