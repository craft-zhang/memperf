#ifndef CPY_OPT_H
#define CPY_OPT_H

/* load test (with summing to be fair against non_optimized code */
float cpy_lsopt(float *a, int l, int mx, int it);

/* const store test */
float cpy_vsopt(float *a, int l, int mx, int it);

/* load copy test (strided load, contiguous store) */
float cpy_lcopt(float *a, float *c, int l, int mx, int it);

/* copy store test (contiguous load, strided store) */
float cpy_csopt(float *a, float *c, int l, int mx, int it);

#endif
