#ifndef _WG_FIXARITH_H
#define  _WG_FIXARITH_H

#define FPPOS   8

#define FPPOS_M ((FPPOS) >> 1) 
#define FPPOS_MAX (1 << FPPOS)

#define FPPOS_INC(val)  ((val) += (FPPOS_VAL(1)))
#define FPPOS_VAL(val)  ((wg_int32)(((wg_uint32)(val)) << FPPOS))
#define FPPOS_MUL(val1, val2)                  \
                    (FPPOS_INT((val1) * (val2)))

#define FPPOS_DIV(num, dnum)                  \
                    ((num / ((dnum) >> (FPPOS_M))) << (FPPOS_M))

/*! @todo handle negative numbers properly */
#define FPPOS_INT(val)  ((wg_int32)(((wg_int32)(val)) >> FPPOS))

#endif 
