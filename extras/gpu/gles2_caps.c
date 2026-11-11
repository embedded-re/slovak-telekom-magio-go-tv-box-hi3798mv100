/* Print GLES2 limits/precision/extensions of the lima driver (surfaceless EGL). */
#include <stdio.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
int main(void){
  EGLDisplay d = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
  eglInitialize(d,0,0); eglBindAPI(EGL_OPENGL_ES_API);
  EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
  EGLContext c=eglCreateContext(d,EGL_NO_CONFIG_KHR,EGL_NO_CONTEXT,ca);
  if(!c||!eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,c)){puts("no context");return 1;}
  struct {GLenum e;const char*n;} L[]={
   {GL_MAX_TEXTURE_SIZE,"MAX_TEXTURE_SIZE"},{GL_MAX_CUBE_MAP_TEXTURE_SIZE,"MAX_CUBE_MAP_TEXTURE_SIZE"},
   {GL_MAX_RENDERBUFFER_SIZE,"MAX_RENDERBUFFER_SIZE"},{GL_MAX_VERTEX_ATTRIBS,"MAX_VERTEX_ATTRIBS"},
   {GL_MAX_VERTEX_UNIFORM_VECTORS,"MAX_VERTEX_UNIFORM_VECTORS"},{GL_MAX_FRAGMENT_UNIFORM_VECTORS,"MAX_FRAGMENT_UNIFORM_VECTORS"},
   {GL_MAX_VARYING_VECTORS,"MAX_VARYING_VECTORS"},{GL_MAX_TEXTURE_IMAGE_UNITS,"MAX_TEXTURE_IMAGE_UNITS"},
   {GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,"MAX_VERTEX_TEXTURE_IMAGE_UNITS"},{GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,"MAX_COMBINED_TEXTURE_IMAGE_UNITS"}};
  for(unsigned i=0;i<sizeof L/sizeof*L;i++){GLint v=0;glGetIntegerv(L[i].e,&v);printf("%-34s %d\n",L[i].n,v);}
  GLint vp[2];glGetIntegerv(GL_MAX_VIEWPORT_DIMS,vp);printf("%-34s %dx%d\n","MAX_VIEWPORT_DIMS",vp[0],vp[1]);
  const char*sn[]={"VERTEX","FRAGMENT"};GLenum st[]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
  const char*pn[]={"highp float","mediump float","lowp float","highp int","mediump int","lowp int"};
  GLenum pt[]={GL_HIGH_FLOAT,GL_MEDIUM_FLOAT,GL_LOW_FLOAT,GL_HIGH_INT,GL_MEDIUM_INT,GL_LOW_INT};
  for(int s=0;s<2;s++)for(int p=0;p<6;p++){GLint r[2]={0,0},pr=0;glGetShaderPrecisionFormat(st[s],pt[p],r,&pr);
    printf("%-8s %-14s range 2^%d..2^%d precision %d bits\n",sn[s],pn[p],-r[0],r[1],pr);}
  printf("EXTENSIONS: %s\n",glGetString(GL_EXTENSIONS));
  return 0;}
