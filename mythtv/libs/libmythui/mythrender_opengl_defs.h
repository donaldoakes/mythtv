#ifndef MYTHRENDER_OPENGL_DEFS_H_
#define MYTHRENDER_OPENGL_DEFS_H_

#ifndef GL_COLOR_EXT
#define GL_COLOR_EXT 0x1800
#endif

#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

typedef ptrdiff_t MYTH_GLsizeiptr;
typedef GLvoid* (APIENTRY * MYTH_GLMAPBUFFERPROC)
    (GLenum target, GLenum access);
typedef GLboolean (APIENTRY * MYTH_GLUNMAPBUFFERPROC)
    (GLenum target);
#endif
