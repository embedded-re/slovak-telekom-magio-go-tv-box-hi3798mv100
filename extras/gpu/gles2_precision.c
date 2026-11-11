/* Measure real shader float precision on lima (vertex=GP, fragment=PP) and renderable FBO formats. */
#include <stdio.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
static GLuint prog(const char*vs,const char*fs){GLuint p=glCreateProgram();const char*s[2]={vs,fs};GLenum t[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
 for(int i=0;i<2;i++){GLuint h=glCreateShader(t[i]);glShaderSource(h,1,&s[i],0);glCompileShader(h);GLint ok;glGetShaderiv(h,GL_COMPILE_STATUS,&ok);
  if(!ok){char b[512];glGetShaderInfoLog(h,512,0,b);printf("compile err: %s\n",b);}glAttachShader(p,h);}
 glBindAttribLocation(p,0,"pos");glLinkProgram(p);return p;}
static const float quad[]={-1,-1,1,-1,-1,1,1,1};
/* returns pixel red channel (0..255) after drawing with given program and uniform a */
static int run(GLuint p,float a){glUseProgram(p);glUniform1f(glGetUniformLocation(p,"a"),a);
 glVertexAttribPointer(0,2,GL_FLOAT,0,0,quad);glEnableVertexAttribArray(0);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 glDrawArrays(GL_TRIANGLE_STRIP,0,4);unsigned char px[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);return px[0];}
int main(void){
 EGLDisplay d=eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,NULL);eglInitialize(d,0,0);eglBindAPI(EGL_OPENGL_ES_API);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};EGLContext c=eglCreateContext(d,EGL_NO_CONFIG_KHR,EGL_NO_CONTEXT,ca);eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,c);
 GLuint fb,tex;glGenFramebuffers(1,&fb);glBindFramebuffer(GL_FRAMEBUFFER,fb);glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,4,4,0,GL_RGBA,GL_UNSIGNED_BYTE,0);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);glViewport(0,0,4,4);
 /* mod(a+1, 2): exact if mantissa holds a+1. a = 2^k: fp16 (10-bit mantissa) breaks at k=11, fp24 at 17, fp32 at 24 */
 const char*vsP="attribute vec2 pos;void main(){gl_Position=vec4(pos,0.,1.);}";
 const char*fsP="precision highp float;uniform float a;void main(){gl_FragColor=vec4(mod(a+1.0,2.0),0.,0.,1.);}";
 const char*vsV="attribute vec2 pos;uniform float a;varying float v;void main(){v=mod(a+1.0,2.0);gl_Position=vec4(pos,0.,1.);}";
 const char*fsV="precision mediump float;varying float v;void main(){gl_FragColor=vec4(v,0.,0.,1.);}";
 GLuint pf=prog(vsP,fsP),pv=prog(vsV,fsV);
 int lastF=-1,lastV=-1;
 for(int k=8;k<=26;k++){float a=(float)(1<<k);int rf=run(pf,a),rv=run(pv,a);
   printf("a=2^%-2d fragment %s  vertex %s\n",k,rf>127?"exact":"LOST ",rv>127?"exact":"LOST ");
   if(rf>127)lastF=k; if(rv>127)lastV=k;}
 printf("=> fragment mantissa ~%d bits, vertex mantissa ~%d bits\n",lastF,lastV);
 struct{GLenum ifmt,fmt,type;const char*n;}F[]={{GL_RGBA,GL_RGBA,GL_UNSIGNED_BYTE,"RGBA8"},{GL_RGB,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,"RGB565"},
  {GL_RGBA,GL_RGBA,GL_HALF_FLOAT_OES,"RGBA16F (half float)"},{GL_RGBA,GL_RGBA,GL_FLOAT,"RGBA32F (float)"},{GL_RG_EXT,GL_RG_EXT,GL_UNSIGNED_BYTE,"RG8"}};
 for(unsigned i=0;i<sizeof F/sizeof*F;i++){while(glGetError());glTexImage2D(GL_TEXTURE_2D,0,F[i].ifmt,4,4,0,F[i].fmt,F[i].type,0);GLenum te=glGetError();
   GLenum st=glCheckFramebufferStatus(GL_FRAMEBUFFER);printf("render target %-22s %s\n",F[i].n,te?"texture format rejected":(st==GL_FRAMEBUFFER_COMPLETE?"OK":"NOT renderable"));}
 return 0;}
