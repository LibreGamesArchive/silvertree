#include "SDL.h"
#include "SDL_ttf.h"

TTF_Font *font = NULL;

void close_font() {
    if(font) {
        TTF_CloseFont(font);
    }
}

int string_height(const char *text) {
    const char *p;
    int max_h;

    max_h = 0;
    for(p=text;*p;++p) {
        int char_h;
        TTF_GlyphMetrics(font, *p, NULL, NULL, NULL, &char_h, NULL);
        if(char_h > max_h) {
            max_h = char_h;
        }
    }
    return max_h;
}

int main(int argc, char **argv) {
    int count;
    const char *test_text[] = {
        "ggggg",
        "hhhhh",
        "ooooo",
        "MMMMM",
        "MMMaaa",
        "aaaMMM",
        "yyyooo",
        "oooyyy",
        "iiiIII",
        "iiiggg",
        NULL
    };
    const char **cur;

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error initialising SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);
    if(TTF_Init() < 0) {
        fprintf(stderr,"Error initialising SDL_ttf: %s\n", TTF_GetError());
        exit(1);
    }
    atexit(TTF_Quit);

    font = TTF_OpenFont("FreeSans.ttf",14);
    if(!font) {
        fprintf(stderr,"Error opening font file: %s\n", TTF_GetError());
        exit(1);
    }
    atexit(close_font);

    count = 0;
    for(cur = test_text;*cur;++cur) {
        static const SDL_Color white = { 0xFF, 0xFF, 0xFF, 0xFF };
        SDL_Surface *rendered;
        int calc_h, real_h, measure_h, theory_h, min_h, ascent;

        ++count;

        rendered = TTF_RenderUTF8_Blended(font, *cur, white); 
        calc_h = string_height(*cur);
        real_h = rendered->h;
        TTF_SizeUTF8(font, *cur, NULL, &measure_h);
        theory_h = TTF_FontLineSkip(font);
        min_h = TTF_FontHeight(font);
        ascent = TTF_FontAscent(font);

        printf("Test case %d: \"%s\"\n", count, *cur);
        printf("Rendered surface height: %d\n", real_h);
        printf("Required (theoretical) line height: %d (%d min)\n",
               theory_h, min_h);
        printf("Font ascent: %d\n", ascent);
        printf("Ascent of string: %d\n", calc_h);
        printf("Calculated offset: %d\n", ascent - calc_h);
        printf("\n");
        SDL_FreeSurface(rendered);
    }
}
