#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "gl.h"
#include "glu.h"
#include "boost/version.hpp"

#ifdef SDL_VERSION
void print_sdl_version(char *txt, const SDL_version *version) {
  printf("%s : %d.%d.%d\n", txt, version->major, 
	 version->minor, version->patch);
}
#endif

void status_sdl() {
#ifdef SDL_VERSION
  SDL_version sdl_compile_version;
  const SDL_version *sdl_link_version;
  SDL_VERSION(&sdl_compile_version);
  sdl_link_version = SDL_Linked_Version();
  print_sdl_version("SDL compiled version", &(sdl_compile_version));
  print_sdl_version("SDL linked version", sdl_link_version);
#else
  printf("SDL has no versioning information\n");
#endif
}

void status_sdl_ttf() {
#ifdef TTF_VERSION
  SDL_version ttf_compile_version;
  const SDL_version *ttf_link_version;
  TTF_VERSION(&ttf_compile_version);
  ttf_link_version=TTF_Linked_Version();
  print_sdl_version("SDL_ttf compiled version", &(ttf_compile_version));
  print_sdl_version("SDL_ttf linked version", ttf_link_version);
#else
  printf("SDL_ttf has no versioning information\n");
#endif
}

void status_sdl_image() {
#ifdef SDL_IMAGE_VERSION
  SDL_version img_compile_version;
  const SDL_version *img_link_version;
  SDL_IMAGE_VERSION(&img_compile_version);
  img_link_version=IMG_Linked_Version();
  print_sdl_version("SDL_image compiled version", &(img_compile_version));
  print_sdl_version("SDL_image linked version", img_link_version);
#else
  printf("SDL_image has no versioning information\n");
#endif
}

void status_boost() {
#ifdef BOOST_VERSION
  int major = BOOST_VERSION / 100000;
  int minor = (BOOST_VERSION / 100) % 1000;
  int tiny = BOOST_VERSION % 100;
  printf("boost compiled version: %d.%d.%d\n", major, minor, tiny);

  /* can't find any information on how to find the linked version
     versus the compiled version */
#else
  printf("boost has no versioning information\n");
#endif
}

void status_gl(int detailed) {
  const char *version = (const char*)glGetString(GL_VERSION);
  printf("OpenGL linked version: %s\n", version);

  Uint8 comp_version_major = 1;
  Uint8 comp_version_minor = 0;
#ifdef GL_VERSION_1_1
  comp_version_minor = 1;
#endif
#ifdef GL_VERSION_1_2
  comp_version_minor = 2;
#endif
#ifdef GL_VERSION_1_3
  comp_version_minor = 3;
#endif
#ifdef GL_VERSION_1_4
  comp_version_minor = 4;
#endif
#ifdef GL_VERSION_1_5
  comp_version_minor = 5;
#endif
#ifdef GL_VERSION_2_0
  comp_version_major = 2;
  comp_version_minor = 0;
#endif

  printf("OpenGL compiled version: %d.%d\n", 
	 comp_version_major, comp_version_minor);
  
  if(detailed) {
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    char *extensions = strdup((const char *)glGetString(GL_EXTENSIONS));
    char *ext; int count;

    printf("OpenGL vendor: %s\n", vendor);
    printf("OpenGL renderer: %s\n", renderer);
    printf("OpenGL extensions:\n");
    ext = strtok(extensions, " ");
    count = 0;
    while(ext) {
      printf("%03d: %s\n", ++count, ext);
      ext = strtok(NULL, " ");
    }
    puts("");
    free(extensions);
  }
}

void status_glu(int detailed) {
  const char *version = (const char*)gluGetString(GLU_VERSION);
  printf("GLU linked version: %s\n", version);

  Uint8 comp_version_major = 1;
  Uint8 comp_version_minor = 0;
#ifdef GLU_VERSION_1_1
  comp_version_minor = 1;
#endif
#ifdef GLU_VERSION_1_2
  comp_version_minor = 2;
#endif
#ifdef GLU_VERSION_1_3
  comp_version_minor = 3;
#endif
#ifdef GLU_VERSION_1_4
  comp_version_minor = 4;
#endif
#ifdef GLU_VERSION_1_5
  comp_version_minor = 5;
#endif
#ifdef GLU_VERSION_2_0
  comp_version_major = 2;
  comp_version_minor = 0;
#endif
  printf("GLU compiled version: %d.%d\n", 
	 comp_version_major, comp_version_minor);
  
  if(detailed) {
    char *extensions = strdup((const char *)gluGetString(GLU_EXTENSIONS));
    char *ext; int count;

    printf("GLU extensions:\n");
    ext = strtok(extensions, " ");
    count = 0;
    while(ext) {
      printf("%03d: %s\n", ++count, ext);
      ext = strtok(NULL, " ");
    }
    puts("");
    free(extensions);
  }
}

int main(int argc, char **argv) {
  int gl_only = 0;

  if(argc > 1) {
    if(!strcmp(argv[1], "--gl")) {
      gl_only = 1;
    }
  }

  SDL_Init(SDL_INIT_VIDEO);
  SDL_SetVideoMode(1, 1, 32, SDL_OPENGL);
  atexit(SDL_Quit);

  if(!gl_only) {
    status_sdl();
    status_sdl_ttf();
    status_sdl_image();
    status_boost();
  }
  status_gl(gl_only);
  status_glu(gl_only);

  if(!gl_only) {
    printf("Use \"%s --gl\" for detailed OpenGL information\n", argv[0]);
  }

  exit(0);
}
