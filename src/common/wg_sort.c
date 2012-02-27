#include <stdlib.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_sort.h>


void
wg_sort_double(wg_double *data, wg_uint num)
{
    register wg_double *d = NULL;
    wg_int increment = 0;
    wg_int i = 0;
    wg_int j = 0;
    wg_double tmp = 0.0;

    d = data;

    for (increment = num >> 1; increment > 0; increment >>= 1){
        for (i = increment; i < num; ++i){
            tmp = d[i];
            for (j = i; j >= increment; j -= increment){
                if (tmp < d[j - increment]){
                    d[j] = d[j - increment];
                }else{
                    break;
                }
            }
            d[j] = tmp;
        }
    }

    return;
}

WG_PRIVATE int 
comp_func(const void *e1, const void *e2)
{
    register wg_uint val1;
    register wg_uint val2;

    val1 = *(wg_uint*)e1;
    val2 = *(wg_uint*)e2;

    return val1 - val2;
}

void
wg_sort_uint_qsort(wg_uint *data, wg_uint num)
{
    qsort(data, num, sizeof (*data), comp_func);
}

void
wg_sort_uint(wg_uint *data, wg_uint num)
{
    WG_STATIC const wg_uint h[] = {19, 5, 1, 0};

    __asm__ __volatile__("6:\n\t"
             "cmpl $0x0, (%0)\n\t"
             /* exit 1st loop */
             "jle 1f\n\t"
                /* i = increment */
               "movl (%0), %%ecx\n\t"
               "3:\n\t"
               /* while (i < num) */
               "cmpl %2, %%ecx\n\t"
               "jge 5f\n\t"
                 /* edx = temp */
                 "movl (%1, %%ecx, 0x4), %%edx\n\t"
                 "pushl %%ecx\n\t"
                 "7:\n\t"
                 /* while (j >= increment) */
                 "cmpl (%0), %%ecx\n\t"
                 "jl 4f\n\t"
                   "movl %%ecx, %%ebx\n\t"
                   "subl (%0), %%ecx\n\t"
                   "cmpl %%edx, (%1, %%ecx, 0x4)\n\t"
                   "jb 8f\n\t"
                     "movl (%1, %%ecx, 0x4), %%eax\n\t"
                     "movl %%eax, (%1, %%ebx, 0x4)\n\t"
                     "jmp 7b\n\t" 
                    "8:\n\t"
                    "movl %%ebx, %%ecx\n\t"
                  "4:\n\t"
                  "movl %%edx, (%1, %%ecx, 0x4)\n\t"
               "2:\n\t"
               "popl %%ecx\n\t"
               "incl %%ecx\n\t"
             "jmp 3b\n\t"
             "5:\n\t"
             "addl $0x4, %0\n\t"
             "jmp 6b\n\t"
             "1:\n\t"
            :
            : "S"(h), "D"(data), "m"(num)
            :"eax", "ebx", "ecx", "edx"
    );
}

void
wg_sort_uint_(wg_uint *data, wg_uint num)
{
    register wg_uint *d = NULL;
    register wg_int increment = 0;
    register wg_int i = 0;
    register wg_int j = 0;
    wg_uint tmp = 0.0;
    wg_uint const *hh = NULL;
    WG_STATIC const wg_uint h[] = {19, 5, 1, 0};

    d = data;
    hh = h;

    for (increment = *hh++; increment > 0; increment = *hh++){
        for (i = increment; i < num; ++i){
            tmp = d[i];
            for (j = i; j >= increment; j -= increment){
                if (tmp < d[j - increment]){
                    d[j] = d[j - increment];
                }else{
                    break;
                }
            }
            d[j] = tmp;
        }
    }

    return;
}
