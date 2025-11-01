/*

Copyright 2025, Paulo Vinicius Radtke

This file is distributed under the MIT license.

To compile, install SDL2 and SDL2 Image and run:

	gcc png2mode2sprites.c -lSDL2 -lSDL2_image -o png2mode2sprites

	or run make ot execute the Makefile. "make install" will copy it to a path directory.

*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// Can convert up to 1024 sprites (not really a strong case, but, whatever)
uint8_t spPixels[1024][32];
uint8_t spColors[1024][16];
uint8_t transparent = 0 ;

int main(int argc, char* argv[]) {
    memset(spPixels, 0, 1024*32);     // All to transparente pixels
    memset(spColors, 0, 1024*16);    // All to transparent color with CC bit to 1
    int sprites=0;
    if(argc != 2 ){
    	printf("Usage: palettecount <pngfilename>\n");
    	return 1;
    }
    // Initialize SDL and SDL_image
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG); // Initialize for PNG and JPG support

    // Load an image
    SDL_Surface* msx = IMG_Load(argv[1]);
    if (!msx) {
        printf("Can't load the file\n");
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    if(!SDL_ISPIXELFORMAT_INDEXED(msx->format->format)){
        printf("PNG doesn't have 16 or 256 indexed colors\n");
        SDL_FreeSurface(msx);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    for(int i=0;i<msx->format->palette->ncolors;i++){
        printf("%d: R: %03d, G: %03d, B: %03d, A: %03d\n", i, msx->format->palette->colors[i].r, msx->format->palette->colors[i].g, msx->format->palette->colors[i].b, msx->format->palette->colors[i].a);
    }
   // Clean up
    SDL_FreeSurface(msx);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
