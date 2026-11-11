/* GLES2 offscreen GPU stress + correctness check for lima (Mali-450).
 * Usage: gles2_stress SECONDS [ITERS] [CHECK_EVERY]
 * Full-screen 1024x1024 quad, ALU-heavy fragment shader sampling a 256x256 texture.
 * Every frame's framebuffer checksum must equal frame 0's -> detects corruption.
 * Build: gcc -O2 -o gles2_stress gles2_stress.c -lEGL -lGLESv2
 */
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define W 1024
#define H 1024

static const char *vs =
	"attribute vec2 p; varying vec2 uv;"
	"void main(){ uv = p * 0.5 + 0.5; gl_Position = vec4(p, 0.0, 1.0); }";
static char fs[1024];

static GLuint sh(GLenum t, const char *s)
{
	GLint ok; char log[512];
	GLuint x = glCreateShader(t);
	glShaderSource(x, 1, &s, 0); glCompileShader(x);
	glGetShaderiv(x, GL_COMPILE_STATUS, &ok);
	if (!ok) { glGetShaderInfoLog(x, sizeof(log), 0, log); fprintf(stderr, "shader: %s\n", log); exit(2); }
	return x;
}

static double now(void)
{
	struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec / 1e9;
}

static unsigned sum(const unsigned char *b, size_t n)
{
	unsigned h = 2166136261u;
	for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
	return h;
}

int main(int argc, char **argv)
{
	double secs = argc > 1 ? atof(argv[1]) : 10;
	int iters = argc > 2 ? atoi(argv[2]) : 24;
	int every = argc > 3 ? atoi(argv[3]) : 1;
	long checks = 0;

	snprintf(fs, sizeof(fs),
		"precision mediump float; varying vec2 uv; uniform sampler2D t;"
		"void main(){ vec2 c = uv; vec4 a = vec4(0.0);"
		" for (int i = 0; i < %d; i++) {"
		"  a += texture2D(t, c) * 0.05;"
		"  c = fract(c * 1.37 + vec2(0.11, 0.23) + a.xy * 0.1); }"
		" gl_FragColor = fract(a); }", iters);

	if (getenv("FS")) snprintf(fs, sizeof(fs), "%s", getenv("FS"));

	PFNEGLGETPLATFORMDISPLAYEXTPROC gpd = (void *)eglGetProcAddress("eglGetPlatformDisplayEXT");
	EGLDisplay d = gpd(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, 0);
	eglInitialize(d, 0, 0);
	EGLint ca[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE }, n;
	EGLConfig c; eglChooseConfig(d, ca, &c, 1, &n);
	eglBindAPI(EGL_OPENGL_ES_API);
	EGLint xa[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	EGLContext x = eglCreateContext(d, c, EGL_NO_CONTEXT, xa);
	if (!eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, x)) { fprintf(stderr, "no context\n"); return 2; }

	GLuint fb, rb, tex;
	glGenFramebuffers(1, &fb); glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glGenRenderbuffers(1, &rb); glBindRenderbuffer(GL_RENDERBUFFER, rb);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, W, H);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
	glViewport(0, 0, W, H);

	unsigned char *img = malloc(256 * 256 * 4);
	for (int i = 0; i < 256 * 256 * 4; i++) img[i] = (unsigned char)((i * 2654435761u) >> 24);
	glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLuint p = glCreateProgram();
	glAttachShader(p, sh(GL_VERTEX_SHADER, vs)); glAttachShader(p, sh(GL_FRAGMENT_SHADER, fs));
	glBindAttribLocation(p, 0, "p"); glLinkProgram(p);
	{
		GLint ok; char log[512];
		glGetProgramiv(p, GL_LINK_STATUS, &ok);
		if (!ok) { glGetProgramInfoLog(p, sizeof(log), 0, log); fprintf(stderr, "link: %s\n", log); return 2; }
	}
	glUseProgram(p);
	glUniform1i(glGetUniformLocation(p, "t"), 0);
	float v[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
	glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, v); glEnableVertexAttribArray(0);

	unsigned char *buf = malloc(W * H * 4);
	unsigned ref = 0;
	long frames = 0, bad = 0;
	double t0 = now(), tl = t0;

	while (now() - t0 < secs) {
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (frames % every == 0) {
			glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, buf);
			if (frames == 0)
				printf("px(0,0)=%d,%d,%d px(512,512)=%d,%d,%d px(1000,77)=%d,%d,%d err=0x%x\n",
				       buf[0], buf[1], buf[2], buf[(512 * W + 512) * 4], buf[(512 * W + 512) * 4 + 1],
				       buf[(512 * W + 512) * 4 + 2], buf[(77 * W + 1000) * 4], buf[(77 * W + 1000) * 4 + 1],
				       buf[(77 * W + 1000) * 4 + 2], glGetError());
			unsigned s = sum(buf, W * H * 4);
			if (frames == 0) ref = s;
			else if (s != ref) bad++;
			checks++;
		} else {
			glFinish();
		}
		frames++;
		if (now() - tl >= 5) {
			printf("t=%.0fs frames=%ld bad=%ld fps=%.1f\n", now() - t0, frames, bad, frames / (now() - t0));
			fflush(stdout); tl = now();
		}
	}
	double dt = now() - t0;
	printf("RESULT renderer=%s iters=%d frames=%ld checks=%ld bad=%ld fps=%.2f glerr=0x%x ref=%08x\n",
	       glGetString(GL_RENDERER), iters, frames, checks, bad, frames / dt, glGetError(), ref);
	return bad ? 1 : 0;
}
