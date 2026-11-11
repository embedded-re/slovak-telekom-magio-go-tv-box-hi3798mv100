/* Which GLSL ES 1.00 constructs does lima compile+link+draw? (vertex = GP, fragment = PP) */
#include <stdio.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
static const char*VS0="attribute vec2 pos;void main(){gl_Position=vec4(pos,0.,1.);}";
static const char*FS0="precision mediump float;void main(){gl_FragColor=vec4(1.);}";
static const char*VSV="attribute vec2 pos;varying float v;uniform float n;void main(){";
static int test(const char*name,const char*vs,const char*fs){
 const char*s[2]={vs,fs};GLenum t[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};GLuint p=glCreateProgram();char log[400]="";int ok=1;
 for(int i=0;i<2;i++){GLuint h=glCreateShader(t[i]);glShaderSource(h,1,&s[i],0);glCompileShader(h);GLint c;glGetShaderiv(h,GL_COMPILE_STATUS,&c);
  if(!c){glGetShaderInfoLog(h,400,0,log);ok=0;}glAttachShader(p,h);}
 glBindAttribLocation(p,0,"pos");glLinkProgram(p);GLint l;glGetProgramiv(p,GL_LINK_STATUS,&l);if(ok&&!l){glGetProgramInfoLog(p,400,0,log);ok=0;}
 if(ok){static const float q[]={-1,-1,1,-1,-1,1,1,1};glUseProgram(p);glUniform1f(glGetUniformLocation(p,"n"),7.0);
  glVertexAttribPointer(0,2,GL_FLOAT,0,0,q);glEnableVertexAttribArray(0);glDrawArrays(GL_TRIANGLE_STRIP,0,4);glFinish();if(glGetError())ok=0,snprintf(log,400,"GL error on draw");}
 for(char*c=log;*c;c++)if(*c=='\n')*c=' ';
 printf("%-44s %s %s\n",name,ok?"OK  ":"FAIL",ok?"":log);return ok;}
int main(void){
 EGLDisplay d=eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,NULL);eglInitialize(d,0,0);eglBindAPI(EGL_OPENGL_ES_API);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};EGLContext c=eglCreateContext(d,EGL_NO_CONFIG_KHR,EGL_NO_CONTEXT,ca);eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,c);
 GLuint fb,tex;glGenFramebuffers(1,&fb);glBindFramebuffer(GL_FRAMEBUFFER,fb);glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);glViewport(0,0,64,64);
 char vs[600],fs[600];
 test("baseline",VS0,FS0);
 /* fragment side */
 test("FS: if/else on uniform",VS0,"precision mediump float;uniform float n;void main(){if(n>3.)gl_FragColor=vec4(1.);else gl_FragColor=vec4(0.);}");
 test("FS: if on gl_FragCoord (divergent)",VS0,"precision mediump float;void main(){if(gl_FragCoord.x>32.)gl_FragColor=vec4(1.);else gl_FragColor=vec4(0.);}");
 test("FS: constant loop (16)",VS0,"precision mediump float;void main(){float s=0.;for(int i=0;i<16;i++)s+=0.01;gl_FragColor=vec4(s);}");
 test("FS: loop with uniform break",VS0,"precision mediump float;uniform float n;void main(){float s=0.;for(int i=0;i<64;i++){if(float(i)>=n)break;s+=0.01;}gl_FragColor=vec4(s);}");
 test("FS: discard",VS0,"precision mediump float;void main(){if(gl_FragCoord.x<8.)discard;gl_FragColor=vec4(1.);}");
 test("FS: dFdx/fwidth (OES_standard_derivatives)",VS0,"#extension GL_OES_standard_derivatives:enable\nprecision mediump float;void main(){gl_FragColor=vec4(fwidth(gl_FragCoord.x));}");
 test("FS: texture2D + dependent read",VS0,"precision mediump float;uniform sampler2D t;void main(){vec4 a=texture2D(t,gl_FragCoord.xy/64.);gl_FragColor=texture2D(t,a.xy);}");
 test("FS: uniform array, dynamic index",VS0,"precision mediump float;uniform vec4 u[8];uniform float n;void main(){gl_FragColor=u[int(n)];}");
 test("FS: sin/exp/pow/sqrt",VS0,"precision mediump float;void main(){float x=gl_FragCoord.x;gl_FragColor=vec4(sin(x),exp(-x),pow(x,.5),sqrt(x));}");
 test("FS: function call",VS0,"precision mediump float;float f(float x){return x*x+1.;}void main(){gl_FragColor=vec4(f(gl_FragCoord.x*.01));}");
 test("FS: gl_FragData[0] (MRT syntax)",VS0,"precision mediump float;void main(){gl_FragData[0]=vec4(1.);}");
 /* vertex side */
 snprintf(vs,600,"%sif(n>3.)v=1.;else v=0.;gl_Position=vec4(pos,0.,1.);}",VSV);
 test("VS: if/else on uniform",vs,"precision mediump float;varying float v;void main(){gl_FragColor=vec4(v);}");
 snprintf(vs,600,"%sfloat s=0.;for(int i=0;i<8;i++)s+=pos.x;v=s;gl_Position=vec4(pos,0.,1.);}",VSV);
 test("VS: constant loop (8)",vs,"precision mediump float;varying float v;void main(){gl_FragColor=vec4(v);}");
 snprintf(vs,600,"%sfloat s=0.;for(int i=0;i<64;i++){if(float(i)>=n)break;s+=pos.x;}v=s;gl_Position=vec4(pos,0.,1.);}",VSV);
 test("VS: loop with uniform break",vs,"precision mediump float;varying float v;void main(){gl_FragColor=vec4(v);}");
 snprintf(vs,600,"%sif(pos.x>0.)v=1.;else v=0.;gl_Position=vec4(pos,0.,1.);}",VSV);
 test("VS: if on attribute (divergent)",vs,"precision mediump float;varying float v;void main(){gl_FragColor=vec4(v);}");
 test("VS: texture2D in vertex shader","attribute vec2 pos;uniform sampler2D t;varying vec4 v;void main(){v=texture2D(t,pos);gl_Position=vec4(pos,0.,1.);}",
      "precision mediump float;varying vec4 v;void main(){gl_FragColor=v;}");
 snprintf(vs,600,"attribute vec2 pos;uniform vec4 u[64];uniform float n;varying vec4 v;void main(){v=u[int(n)];gl_Position=vec4(pos,0.,1.);}");
 test("VS: uniform array, dynamic index",vs,"precision mediump float;varying vec4 v;void main(){gl_FragColor=v;}");
 return 0;}
