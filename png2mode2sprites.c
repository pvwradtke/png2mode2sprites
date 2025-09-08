/*

Copyright 2025, Paulo Vinicius Radtke

This file is distributed under the MIT license.

To compile, install SDL2 and SDL2 Image and run:

	gcc png2mode2sprites.c -lSDL2 -lSDL2_image -o png2mode2sprites

	or run make ot execute the Makefile. "make install" will copy it to a path directory.

*/

#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

uint16_t palette[16];
// Can convert up to 1024 sprites (not really a strong case, but, whatever)
uint8_t spPixels[1024][32];
uint8_t spColors[1024][16];

int main(int argc, char* argv[]) {
    memset(spPixels, 0, 1024*32);     // All to transparente pixels
    memset(spColors, 64, 1024*16);    // All to transparent color with CC bit to 1
    int sprites=0;
    if(argc != 4 && argc != 2){
    	printf("Usage: png2mode2sprites <pngfilename> <raw sprite patterns> <raw sprite colors>\n");
    	printf("To only validate a PNG: png2mode2sprites <pngfilename>\n");
    	return 1;
    }
    // Initialize SDL and SDL_image
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG); // Initialize for PNG and JPG support

    // Load an image
    SDL_Surface* msx = IMG_Load(argv[1]);
    if (!msx) {
        printf("Can't load the file\n");
        return 1;
    }
    if(msx->format->palette->ncolors!=16){
        printf("PNG doesn't have 16 indexed colors\n");
        return 1;
    }
    printf("File %s - Width: %d, Height: %d, %d colors\n",
    	argv[1], msx->w, msx->h, msx->format->palette->ncolors);
    if(msx->w%16 != 0 || msx->h%16 != 0){
        printf("PNG image dimensions are not multiple of 16.\n");
        return 1;
    }
    // How many sprites we're working with
    int numSprites = 0;
    // Converts the picture to sprites
    for(int i=0;i<msx->h/16;i++){
        for(int j=0;j<msx->w/16;j++){
            bool spriteA=false, spriteB=false;
            // Variables to indicate if we need to make one or two sprites
            for(int line=0;line<16;line++){
                // First find the number of colors in the line
                int colors[3];
                int cColors=0;
                // Finds the colors in this line
                for(int column=0;column<16;column++){
                    int pixel = ((char*)msx->pixels)[(i*16+line)*msx->w+j*16+column];
                    if(pixel==0)
                        continue;
                    if(cColors==0){
                        colors[0]=pixel;
                        cColors=1;
                    }
                    else{
                        bool found=false;
                        for(int count=0;count<cColors && !found;count++){
                            if(pixel==colors[count])
                                found=true;
                        }
                        if(!found){
                        // Checks if we're still up to three colors per line
                            if(cColors<3){
                                colors[cColors]=pixel;
                                cColors++;
                            }else{
                                // Clean up as we can´t have more than three colors per line
                                printf("ERROR! Image %s has more colors per line on sprite %d at line %d.\n", argv[1], i*(msx->h/16)+j, line);
                                SDL_FreeSurface(msx);
                                IMG_Quit();
                                SDL_Quit();
                                return 1;
                            }
                        }
                    }
                }
                // Checks the color combinations for sprites A and B to generate color M with OR
                int colorA, colorB, colorM;
                if(cColors==0)
                    continue;
                else if(cColors==1){
                    colorA = colors[0];
                    colorB = 255;
                    colorM = 255;
                    spriteA=true;
                }else if(cColors==2){
                    colorA = colors[0];
                    colorB = colors[1];
                    colorM = 255;
                    spriteA=true;
                    spriteB=true;
                }else if((colors[0] | colors[1]) == colors[2] && colors[0] < colors[2] && colors[1] < colors[2] ){   // If they
                    colorA= colors[0];
                    colorB=colors[1];
                    colorM=colors[2];
                    spriteA=true;
                    spriteB=true;
                }else if((colors[0] | colors[2]) == colors[1] && colors[0] < colors[1] && colors[2] < colors[1] ){
                    colorA= colors[0];
                    colorB=colors[2];
                    colorM=colors[1];
                    spriteA=true;
                    spriteB=true;
                }else if((colors[1] | colors[2]) == colors[0] && colors[1] < colors[0] && colors[2] < colors[0] ){
                    colorA= colors[1];
                    colorB=colors[2];
                    colorM=colors[0];
                    spriteA=true;
                    spriteB=true;
                }else{
                    // Clean up as we can´t have an invalid OR operation
                    printf("ERROR! Image %s has an invalid color combination on sprite %d at line %d (%d on PNG). Colors: %d, %d, %d\n", argv[1], i*(msx->h/16)+j, line, i*16+line, colors[0], colors[1], colors[2]);
                    SDL_FreeSurface(msx);
                    IMG_Quit();
                    SDL_Quit();
                    return 1;
                }
                // Now search the pixels and fill the colors information
                unsigned char byte1=0;
                unsigned char byte2=0;
                // For pixesl 0 to 7 (left side, sprite pattern 0 or 1)
                for(int bitCount=0;bitCount<8;bitCount++){
                    unsigned char pixel = ((unsigned char*)msx->pixels)[(i*16+line)*msx->w+j*16+bitCount];
                    // Sets the leftmost bit to1 and shit up to 7 bits, depending on which pixel we're working with
                    unsigned char bit=128 >> bitCount;
                    // We set the bit to 1 if the color matches the color for sprite 1 or the merged color
                    if(pixel == colorA || pixel == colorM){
                        byte1 = byte1 | bit;
                    }
                    else
                    // Same for color of the sprite 2
                    if(pixel == colorB || pixel == colorM)
                        byte2 = byte2 | bit;
                }
                // Writes byte into the sprite 0 or 1. If we got here, we have AT LEAST the first sprite
                spPixels[numSprites][line]=byte1;
                // If we have a second sprite, the color is different than 0
                if(colorB!=255)
                    spPixels[numSprites+1][line]=byte2;
                // Writes the color information. The first sprite ALWAYS has a color, but if there's colorB, we shoud set the CC bit
                if(colorB == 255)
                    spColors[numSprites][line]= 64 | colorA;
                else if(colorM==255){
                    spColors[numSprites][line]= 64 | colorA;
                    spColors[numSprites+1][line]= 64 | colorB;
                }
                else if(colorM!=255){
                    spColors[numSprites][line]= colorA;
                    spColors[numSprites+1][line]= 64 | colorB;
                }
                byte1=0;
                byte2=0;
                // For pixesl 8 to 15 (left side, sprite pattern 0 or 1)
                for(int bitCount=0;bitCount<8;bitCount++){
                    unsigned char pixel = ((unsigned char*)msx->pixels)[(i+line)*msx->w+j*16+bitCount+8];
                    // Sets the leftmost bit to1 and shit up to 7 bits, depending on which pixel we're working with
                    unsigned char bit=0x128 >> bitCount;
                    // We set the bit to 1 if the color matches the color for sprite 1 or the merged color
                    if(pixel == colorA || pixel == colorM)
                        byte1 = byte1 | bit;
                    // Same for color of the sprite 2
                    if(pixel == colorB || pixel == colorM)
                        byte2 = byte2 | bit;
                }
                // Writes byte into the sprite 0 or 1. If we got here, we have AT LEAST the first sprite
                spPixels[numSprites][line+16]=byte1;
                // If we have a second sprite, the color is different than 0
                if(colorB!=255)
                    spPixels[numSprites+1][line+16]=byte2;
                // Writes the color information. The first sprite ALWAYS has a color, but if there's colorB, we shoud set the CC bit
                if(colorB == 255)
                    spColors[numSprites][line] = 64 | colorA;
                else if(colorM==255){
                    spColors[numSprites][line] = 64 | colorA;
                    spColors[numSprites+1][line] = 64 | colorB;
                }
                else if(colorM!=255){
                    spColors[numSprites][line]= colorA;
                    spColors[numSprites+1][line]= 64 | colorB;
                }
            }
            // Add the sprite counter according to the sprites found
            if(spriteA)
                numSprites++;
            if(spriteB)
                numSprites++;
        }
    }
    // We need AT LEAST one sprite
    if(numSprites==0){
        printf("ERROR! Image %s has no sprites.\n", argv[1]);
        SDL_FreeSurface(msx);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }else
        printf("Found %d sprites in %s.\n", numSprites, argv[1]);
    if(argc==4){
        FILE *out = fopen(argv[2], "wb");
        if (!out) {
            printf("Can't open raw sprite patterns at %s\n", argv[2]);
            return 1;
        }
        // Writes the sprite patterns
        int written = fwrite(spPixels, 1, 32*numSprites, out);
        fclose(out);
        if (written!=numSprites*32) {
            printf("Can't write raw sprite patterns at %s\n", argv[2]);
            SDL_FreeSurface(msx);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        out = fopen(argv[3], "wb");
        if (!out) {
            printf("Can't open raw sprite colors at %s\n", argv[3]);
            SDL_FreeSurface(msx);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        // Writes the sprite colors
        written = fwrite(spColors, 1, numSprites*16, out);
        fclose(out);
        if (written!=numSprites*16) {
            printf("Can't write raw sprite colors at %s - written = %d\n", argv[3], written);
            SDL_FreeSurface(msx);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        printf("Wrote patterns at %s and colors at %s.\n", argv[2], argv[3]);
    }
   // Clean up
    SDL_FreeSurface(msx);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
