#ifndef _WG_SORT_H
#define _WG_SORT_H

WG_PUBLIC void
wg_sort_double(wg_double *data, wg_uint num);

WG_PUBLIC void
wg_sort_uint(wg_uint *data, wg_uint num);

WG_PUBLIC void
wg_sort_uint_insert(wg_uint *data, wg_uint num);

WG_INLINE void
wg_sort_uint_inline(wg_uint *data, wg_uint num)
{
    WG_STATIC const wg_uint h[] = {19, 5, 1, 0};

    __asm__ __volatile__("6:\n\t"
             "cmpl $0x0, (%0)\n\t"
             /* exit 1st loop */
             "jle 1f\n\t"
                /* i = increment */
               "movl (%0), %%ecx\n\t"
               ".align 4, 0x90\n\t"
               "3:\n\t"
               /* while (i < num) */
               "cmpl %2, %%ecx\n\t"
               "jge 5f\n\t"
                 /* edx = temp */
                 "movl (%1, %%ecx, 0x4), %%edx\n\t"
                 "pushl %%ecx\n\t"
                 ".align 4, 0x90\n\t"
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
                     ".align 4, 0x90\n\t"
                    "8:\n\t"
                    "movl %%ebx, %%ecx\n\t"
                  ".align 4, 0x90\n\t"
                  "4:\n\t"
                  "movl %%edx, (%1, %%ecx, 0x4)\n\t"
                ".align 4, 0x90\n\t"
               "2:\n\t"
               "popl %%ecx\n\t"
               "incl %%ecx\n\t"
             "jmp 3b\n\t"
             ".align 4, 0x90\n\t"
             "5:\n\t"
             "addl $0x4, %0\n\t"
             "jmp 6b\n\t"
             ".align 4, 0x90\n\t"
             "1:\n\t"
            :
            : "S"(h), "D"(data), "m"(num)
            :"eax", "ebx", "ecx", "edx"
    );
}

#endif
