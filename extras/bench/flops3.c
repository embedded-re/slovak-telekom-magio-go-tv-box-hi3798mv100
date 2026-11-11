/* FP32 peak on Cortex-A7 with inline asm: the compiler cannot merge or drop chains.
   gcc -O2 -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -fopenmp flops3.c */
#include <stdio.h>
#include <omp.h>
#include <arm_neon.h>
#define IT 10000000L

#define Q12 "+w"(a0),"+w"(a1),"+w"(a2),"+w"(a3),"+w"(a4),"+w"(a5),"+w"(a6),"+w"(a7),"+w"(a8),"+w"(a9),"+w"(a10),"+w"(a11)
#define REPORT(name, flops) do { t = omp_get_wtime() - t; \
	printf("%-28s %6.2f GFLOPS  (%.2f flop/cycle/core @1.2GHz)\n", name, \
	       (double)IT * (flops) * omp_get_max_threads() / t / 1e9, (double)IT * (flops) / t / 1.2e9); } while (0)

int main(void) {
	double t; volatile float sink;
	printf("threads=%d\n", omp_get_max_threads());

	t = omp_get_wtime();
#pragma omp parallel
	{	float32x4_t a0 = vdupq_n_f32(1), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0, a8 = a0, a9 = a0, a10 = a0, a11 = a0;
		const float32x4_t b = vdupq_n_f32(0.999999f), c = vdupq_n_f32(1e-7f);
		for (long i = 0; i < IT; i++)
			__asm__ volatile(
			"vfma.f32 %q0,%q12,%q13\n vfma.f32 %q1,%q12,%q13\n vfma.f32 %q2,%q12,%q13\n vfma.f32 %q3,%q12,%q13\n"
			"vfma.f32 %q4,%q12,%q13\n vfma.f32 %q5,%q12,%q13\n vfma.f32 %q6,%q12,%q13\n vfma.f32 %q7,%q12,%q13\n"
			"vfma.f32 %q8,%q12,%q13\n vfma.f32 %q9,%q12,%q13\n vfma.f32 %q10,%q12,%q13\n vfma.f32 %q11,%q12,%q13\n"
			: Q12 : "w"(b), "w"(c));
		sink = vgetq_lane_f32(a0, 0) + vgetq_lane_f32(a11, 0); }
	REPORT("vfma.f32 q x12 (128-bit FMA)", 12 * 4 * 2);

	t = omp_get_wtime();
#pragma omp parallel
	{	float32x2_t a0 = vdup_n_f32(1), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0, a8 = a0, a9 = a0, a10 = a0, a11 = a0;
		const float32x2_t b = vdup_n_f32(0.999999f), c = vdup_n_f32(1e-7f);
		for (long i = 0; i < IT; i++)
			__asm__ volatile(
			"vfma.f32 %P0,%P12,%P13\n vfma.f32 %P1,%P12,%P13\n vfma.f32 %P2,%P12,%P13\n vfma.f32 %P3,%P12,%P13\n"
			"vfma.f32 %P4,%P12,%P13\n vfma.f32 %P5,%P12,%P13\n vfma.f32 %P6,%P12,%P13\n vfma.f32 %P7,%P12,%P13\n"
			"vfma.f32 %P8,%P12,%P13\n vfma.f32 %P9,%P12,%P13\n vfma.f32 %P10,%P12,%P13\n vfma.f32 %P11,%P12,%P13\n"
			: Q12 : "w"(b), "w"(c));
		sink = vget_lane_f32(a0, 0) + vget_lane_f32(a11, 0); }
	REPORT("vfma.f32 d x12 (64-bit FMA)", 12 * 2 * 2);

	t = omp_get_wtime();
#pragma omp parallel
	{	float32x4_t a0 = vdupq_n_f32(1), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0, a8 = a0, a9 = a0, a10 = a0, a11 = a0;
		const float32x4_t b = vdupq_n_f32(0.999999f), c = vdupq_n_f32(1e-7f);
		for (long i = 0; i < IT; i++)
			__asm__ volatile(
			"vmul.f32 %q0,%q0,%q12\n vadd.f32 %q1,%q1,%q13\n vmul.f32 %q2,%q2,%q12\n vadd.f32 %q3,%q3,%q13\n"
			"vmul.f32 %q4,%q4,%q12\n vadd.f32 %q5,%q5,%q13\n vmul.f32 %q6,%q6,%q12\n vadd.f32 %q7,%q7,%q13\n"
			"vmul.f32 %q8,%q8,%q12\n vadd.f32 %q9,%q9,%q13\n vmul.f32 %q10,%q10,%q12\n vadd.f32 %q11,%q11,%q13\n"
			: Q12 : "w"(b), "w"(c));
		sink = vgetq_lane_f32(a0, 0) + vgetq_lane_f32(a11, 0); }
	REPORT("vmul+vadd q x12 (128-bit)", 12 * 4);

	t = omp_get_wtime();
#pragma omp parallel
	{	float a0 = 1, a1 = 1, a2 = 1, a3 = 1, a4 = 1, a5 = 1, a6 = 1, a7 = 1, a8 = 1, a9 = 1, a10 = 1, a11 = 1;
		const float b = 0.999999f, c = 1e-7f;
		for (long i = 0; i < IT; i++)
			__asm__ volatile(
			"vfma.f32 %0,%12,%13\n vfma.f32 %1,%12,%13\n vfma.f32 %2,%12,%13\n vfma.f32 %3,%12,%13\n"
			"vfma.f32 %4,%12,%13\n vfma.f32 %5,%12,%13\n vfma.f32 %6,%12,%13\n vfma.f32 %7,%12,%13\n"
			"vfma.f32 %8,%12,%13\n vfma.f32 %9,%12,%13\n vfma.f32 %10,%12,%13\n vfma.f32 %11,%12,%13\n"
			: "+t"(a0),"+t"(a1),"+t"(a2),"+t"(a3),"+t"(a4),"+t"(a5),"+t"(a6),"+t"(a7),"+t"(a8),"+t"(a9),"+t"(a10),"+t"(a11)
			: "t"(b), "t"(c));
		sink = a0 + a11; }
	REPORT("vfma.f32 s x12 (scalar VFP)", 12 * 2);

	t = omp_get_wtime();
#pragma omp parallel
	{	double a0 = 1, a1 = 1, a2 = 1, a3 = 1, a4 = 1, a5 = 1, a6 = 1, a7 = 1, a8 = 1, a9 = 1, a10 = 1, a11 = 1;
		const double b = 0.9999999, c = 1e-8;
		for (long i = 0; i < IT; i++)
			__asm__ volatile(
			"vfma.f64 %P0,%P12,%P13\n vfma.f64 %P1,%P12,%P13\n vfma.f64 %P2,%P12,%P13\n vfma.f64 %P3,%P12,%P13\n"
			"vfma.f64 %P4,%P12,%P13\n vfma.f64 %P5,%P12,%P13\n vfma.f64 %P6,%P12,%P13\n vfma.f64 %P7,%P12,%P13\n"
			"vfma.f64 %P8,%P12,%P13\n vfma.f64 %P9,%P12,%P13\n vfma.f64 %P10,%P12,%P13\n vfma.f64 %P11,%P12,%P13\n"
			: "+w"(a0),"+w"(a1),"+w"(a2),"+w"(a3),"+w"(a4),"+w"(a5),"+w"(a6),"+w"(a7),"+w"(a8),"+w"(a9),"+w"(a10),"+w"(a11)
			: "w"(b), "w"(c));
		sink = a0 + a11; }
	REPORT("vfma.f64 d x12 (scalar FP64)", 12 * 2);
	(void)sink;
	return 0;
}
