#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <time.h>
static const char *vs = "attribute vec2 p; void main(){gl_Position=vec4(p,0.0,1.0);}";
static const char *fs = "precision mediump float; void main(){gl_FragColor=vec4(0.0,1.0,0.0,1.0);}";
static GLuint sh(GLenum t, const char *s){GLuint x=glCreateShader(t);glShaderSource(x,1,&s,0);glCompileShader(x);return x;}
int main(void){
  PFNEGLGETPLATFORMDISPLAYEXTPROC gpd=(void*)eglGetProcAddress("eglGetPlatformDisplayEXT");
  EGLDisplay d=gpd(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,0); eglInitialize(d,0,0);
  EGLint ca[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_NONE},n; EGLConfig c; eglChooseConfig(d,ca,&c,1,&n);
  eglBindAPI(EGL_OPENGL_ES_API); EGLint xa[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
  EGLContext x=eglCreateContext(d,c,EGL_NO_CONTEXT,xa); eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,x);
  GLuint fb,rb; glGenFramebuffers(1,&fb); glBindFramebuffer(GL_FRAMEBUFFER,fb);
  glGenRenderbuffers(1,&rb); glBindRenderbuffer(GL_RENDERBUFFER,rb); glRenderbufferStorage(GL_RENDERBUFFER,GL_RGB565,1024,1024);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,rb); glViewport(0,0,1024,1024);
  GLuint p=glCreateProgram(); glAttachShader(p,sh(GL_VERTEX_SHADER,vs)); glAttachShader(p,sh(GL_FRAGMENT_SHADER,fs));
  glBindAttribLocation(p,0,"p"); glLinkProgram(p); glUseProgram(p);
  float v[]={-1,-1, 1,-1, 0,1}; glVertexAttribPointer(0,2,GL_FLOAT,0,0,v); glEnableVertexAttribArray(0);
  struct timespec a,b; clock_gettime(CLOCK_MONOTONIC,&a); int N=200;
  for(int i=0;i<N;i++){glClearColor(1,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);glFinish();}
  clock_gettime(CLOCK_MONOTONIC,&b); double s=(b.tv_sec-a.tv_sec)+(b.tv_nsec-a.tv_nsec)/1e9;
  unsigned char px[2][4]; glReadPixels(512,300,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px[0]); glReadPixels(5,1000,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px[1]);
  printf("renderer=%s center=%d,%d,%d corner=%d,%d,%d err=0x%x  %d frames 1024x1024 in %.2fs = %.1f fps\n",
    glGetString(GL_RENDERER),px[0][0],px[0][1],px[0][2],px[1][0],px[1][1],px[1][2],glGetError(),N,s,N/s);
  return 0;}
