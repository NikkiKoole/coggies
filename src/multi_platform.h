
#ifdef LINUX
#define getResourcePath() "./resources/"
#define GL3
#include <GL/glew.h>
#include <SDL_opengl.h>
#endif

#ifdef OSX
#define getResourcePath() "./resources/"
#define GL3
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL_opengl.h>
#endif

#ifdef RPI
#define GLES
#define getResourcePath() "./resources/"
#include <SDL_opengles2.h>
#endif


// THIS IS JUST TO PLEASE FLYCHECKER, it cant find the GL headers otherwise
#if (!defined(OSX) && !defined(RPI) && !defined(LINUX))
#include <GL/glew.h>
#include <SDL_opengl.h>
#endif


// THIS IS JUST TO PLEASE FLYCHECKER, it looks for opengles headers on osx otherwise

#if 0
#ifndef OSX
#if defined(TARGET_OS_IPHONE) | defined(TARGET_IPHONE_SIMULATOR)
#define IOS
#define GLES
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "ResourcePath.h"
#endif
#endif
#endif

#include "SDL.h"
#include "SDL_mixer.h"
