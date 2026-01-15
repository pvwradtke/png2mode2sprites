/*

Copyright 2025, Paulo Vinicius Radtke

This file is distributed under the MIT license.

To compile, install SDL2 and SDL2 Image and run:

	gcc png2mode2sprites.c -lSDL2 -lSDL2_image -o png2mode2sprites

	or run make ot execute the Makefile. "make install" will copy it to a path directory.

*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <getopt.h>

uint16_t palette[16];
// Can convert up to 1024 sprites (not really a strong case, but, whatever)
uint8_t spPixels[1024][32];
uint8_t spColors[1024][16];
uint8_t transparent = 0 ;

typedef struct option Option;

static Option long_options[] = {
    {"columns", required_argument, NULL, 'c'},
    {"lines", required_argument, NULL, 'l'},
    {"bpp", required_argument, NULL, 'b'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};


int main(int argc, char* argv[]) {
    int option_index = 0;
    int spColumns=1, spLines=1, spBPP=0;
    char *error;

    // Parse arguments
    int c;
    int digit_optind = 0;
    while ((c = getopt_long(argc, argv, "c:l:b:h",
        long_options, &option_index)) != -1) {
        int this_option_optind = optind ? optind : 1;
        switch (c) {
            case 'c':
                spColumns = strtol(optarg, &error, 10);
                if(*error != '\0'){
                    printf("Invalid non-integer value for columns: %s\n", optarg);
                    exit(1);
                }
                break;
            case 'l':
                spLines = strtol(optarg, &error, 10);
                if(*error != '\0'){
                    printf("Invalid non-integer value for lines: %s\n", optarg);
                    exit(1);
                }
                break;
            case 'b':
                spBPP = strtol(optarg, &error, 10);
                if(*error != '\0'){
                    printf("Invalid non-integer value for bpp: %s\n", optarg);
                    exit(1);
                };
                break;
            case '?':
            case 'h':
                break;
            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }
    char *filenames[3];
    int fCount=0;
    if (optind < argc) {
        while (optind < argc) {
            if(fCount<3)
                filenames[fCount++]=argv[optind++];
            else{
                printf("Trying to get more than three file parameters, aborting! Failed at: %s\n", argv[optind]);
                exit(1);
            }
        }
    }

    memset(spPixels, 0, 1024*32);     // All to transparente pixels
    memset(spColors, 0, 1024*16);    // All to transparent color with CC bit to 1
    int sprites=0;
    if(fCount != 1 && fCount != 3){
    	printf("Usage: png2mode2sprites <pngfilename> <raw sprite patterns> <raw sprite colors> -l [lines] -c [columns] -b [BPP]\n");
    	printf("To only validate a PNG: png2mode2sprites <pngfilename>\n");
    	return 1;
    }
    if(spLines != 1 && spLines != 2){
        printf("Mode 2 sprites support only 1 or 2 lines.\n");
        exit(1);
    }
    if(spColumns != 1 && spColumns != 2){
        printf("Mode 2 sprites support only 1 or 2 columns.\n");
        exit(1);
    }
    if(spBPP != 1 && spBPP != 2 && spBPP != 0){
        printf("Mode 2 sprites support only 1 or 2 bpp - use 0 for auto bpp.\n");
        exit(1);
    }

    // Initialize SDL and SDL_image
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG); // Initialize for PNG and JPG support

    printf("Picture: %s, lines: %d, columns: %d, bpp: %d\n", filenames[0], spLines, spColumns, spBPP);
    if(fCount>1)
        printf("Sprites will be output to %s, and palette to %s.\n", filenames[1], filenames[2]);
    // Load an image
    SDL_Surface* msx = IMG_Load(filenames[0]);
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
    if(msx->format->palette->ncolors == 17){
        transparent=16;
/*        int count=0;
        bool found[256];
        int highest=((char*)msx->pixels)[0];
        memset(found, 0, 256);
        for(int i=0;i<msx->w*msx->h;i++){
        	int pixel = ((char*)msx->pixels)[i];
        	if(!found[pixel]){
        		count++;
        		found[pixel]=true;
        		if(pixel > highest)
        			highest=pixel;
        	}
        }
        if(count < 16 && highest < 16)
	        printf("Found a valid 256 color PNG, it has %d colors, and the highest vaule is %d - both are up to 16.\n", count, highest);
        else if(count == 17 && highest == 17){
            printf("Found a valid 256 color PNG, it has %d colors, and the highest vaule is %d - both are up to 16.\n", count, highest);
        }
        else{
            printf("Found an invalid 256 color PNG, it has %d colors, and the highest vaule is %d - both should be up to 16.\n", count, highest);
            SDL_FreeSurface(msx);
                IMG_Quit();
                    SDL_Quit();
            return 1;
        }*/
    }
    printf("File %s - Width: %d, Height: %d, %d colors\n",
    	filenames[0], msx->w, msx->h, msx->format->palette->ncolors);
    if(msx->w%16 != 0 || msx->h%16 != 0){
        printf("PNG image dimensions are not multiple of 16.\n");
        return 1;
    }
    if((msx->w/16) % spColumns != 0 || (msx->h/16) % spLines != 0){
        printf("The sprite matrix is not compatible with %dx%d sprites.\n", spColumns, spLines);
        return 1;
    }

    // How many sprites we're working with
    int numSprites = 0;

    // Calculates how many horizontal and vertical animation frames are
    int hSprites = (msx->w/16)/spColumns;
    int vSprites = (msx->h/16)/spLines;

    printf("Treating %s as a spritesheet with %dx%d objects.\n", filenames[0], hSprites, vSprites);

    // Converts the picture to sprites
    for(int fLine = 0; fLine <vSprites ; fLine++){
        for(int fColumn = 0; fColumn < hSprites; fColumn++){
            for(int i=0;i<spLines;i++){
                for(int j=0;j<spColumns;j++){
                    bool spriteA=false, spriteB=false;
                    int bpp=0;
                    // Sprites are read in a frame from left to right, top to bottom

                    // Variables to indicate if we need to make one or two sprites
                    for(int line=0;line<16;line++){
                        // First find the number of colors in the line
                        int colors[3];
                        int cColors=0;
                        // Finds the colors in this line
                        for(int column=0;column<16;column++){
                            int x, y;
                            x = (j+fColumn*spColumns)*16+column;
                            y = (i+fLine*spLines)*16+line;
                            //printf("X: %d, Y: %d\n", x, y);
                            int pixel = ((char*)msx->pixels)[y*msx->w+x];
                            if(pixel==transparent)
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
                                        printf("ERROR! Image %s has more colors per line on sprite %d at line %d - PNG (%d, %d).\n", filenames[0], i*(msx->w/16)+j, line, i*16+line, j*16);
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
                            printf("ERROR! Image %s has an invalid color combination on sprite %d at line %d (%d on PNG). Colors: %d, %d, %d\n", filenames[0], i*(msx->h/16)+j, line, i*16+line, colors[0], colors[1], colors[2]);
                            SDL_FreeSurface(msx);
                            IMG_Quit();
                            SDL_Quit();
                            return 1;
                        }
                        if(bpp<cColors)
                            bpp=cColors;

                        // Now search the pixels and fill the colors information
                        unsigned char byte1=0;
                        unsigned char byte2=0;
                        // For pixesl 0 to 7 (left side, sprite pattern 0 or 1)
                        for(int bitCount=0;bitCount<8;bitCount++){
                            int x = (j+fColumn*spColumns)*16+bitCount;
                            int y = (i+fLine*spLines)*16+line;
                            unsigned char pixel = ((unsigned char*)msx->pixels)[y*msx->w+x];
                            // Sets the leftmost bit to1 and shit up to 7 bits, depending on which pixel we're working with
                            unsigned char bit=128 >> bitCount;
                            // We set the bit to 1 if the color matches the color for sprite 1 or the merged color
                            if(pixel == colorA || pixel == colorM){
                                byte1 = byte1 | bit;
                            }
                            // Same for color of the sprite 2
                            if(pixel == colorB || pixel == colorM)
                                byte2 = byte2 | bit;
                        }
                        // Writes byte into the sprite 0 or 1. If we got here, we have AT LEAST the first sprite
                        spPixels[numSprites][line]=byte1;
                        // If we have a second sprite, the color is different than 0
                        if(colorB!=255)
                            spPixels[numSprites+1][line]= byte2;
                        // Writes the color information. The first sprite ALWAYS has a color, but if there's colorB, we shoud set the CC bit
                        if(colorB == 255)
                            spColors[numSprites][line]= colorA;
                        else if(colorM==255){
                            spColors[numSprites][line]= colorA;
                            spColors[numSprites+1][line]= colorB;
                        }
                        else if(colorM!=255){
                            spColors[numSprites][line]= colorA;
                            spColors[numSprites+1][line]= 64 | colorB;
                        }
                        //printf("Sprite %d, Color A: %d, Color B: %d, Color M: %d\n", (msx->h/16)*i+j, colorA, colorB, colorM);
                        byte1=0;
                        byte2=0;
                        // For pixesl 8 to 15 (left side, sprite pattern 0 or 1)
                        for(int bitCount=0;bitCount<8;bitCount++){
                            int x = (j+fColumn*spColumns)*16+bitCount+8;
                            int y = (i+fLine*spLines)*16+line;
                            unsigned char pixel = ((unsigned char*)msx->pixels)[y*msx->w+x];
                            // Sets the leftmost bit to1 and shit up to 7 bits, depending on which pixel we're working with
                            unsigned char bit=128 >> bitCount;
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
                    }
                    // Add the sprite counter according to the sprites found
                    if(spriteA)
                        numSprites++;
                    if(spriteB)
                        numSprites++;
                    // In case we found three colors in the same line (still 2bpp)
                    if(bpp==3)
                        bpp=2;
                    if(spBPP!=0 && spBPP!=bpp){
                        printf("Expected the sprite at line %d, column %d to have %dbpp (actually has %d).\n", i, j, spBPP, bpp);
                        SDL_FreeSurface(msx);
                        IMG_Quit();
                        SDL_Quit();
                        return 1;
                    }
                }

            }
        }
    }
    // We need AT LEAST one sprite
    if(numSprites==0){
        printf("ERROR! Image %s has no sprites.\n", filenames[0]);
        SDL_FreeSurface(msx);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }else
        printf("Found %d sprites in %s.\n", numSprites, filenames[0]);
    if(fCount>1){
        FILE *out = fopen(filenames[1], "wb");
        if (!out) {
            printf("Can't open raw sprite patterns at %s\n", filenames[1]);
            return 1;
        }
        // Writes the sprite patterns
        int written=written = fwrite(spPixels, 1, 32*numSprites, out);
        fclose(out);
        if (written!=numSprites*32) {
            printf("Can't write raw sprite patterns at %s\n", filenames[1]);
            SDL_FreeSurface(msx);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        out = fopen(filenames[2], "wb");
        if (!out) {
            printf("Can't open raw sprite colors at %s\n", filenames[2]);
            SDL_FreeSurface(msx);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        // Writes the sprite colors
        written = fwrite(spColors, 1, numSprites*16, out);
        fclose(out);
        if (written!=numSprites*16) {
            printf("Can't write raw sprite colors at %s - written = %d\n", filenames[2], written);
            SDL_FreeSurface(msx);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        printf("Wrote patterns at %s and colors at %s.\n", filenames[1], filenames[2]);
    }
   // Clean up
    SDL_FreeSurface(msx);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
