#include "fe.h"

/*
h = 0
*/

void fe_0(fe h)
{
  int i;
#if CRYPTO_REDUCE
  for (i = 0;i < 10;++i)
    h[i] = 0;
#else
  h[0] = 0;
  h[1] = 0;
  h[2] = 0;
  h[3] = 0;
  h[4] = 0;
  h[5] = 0;
  h[6] = 0;
  h[7] = 0;
  h[8] = 0;
  h[9] = 0;
#endif
}
