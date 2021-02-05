
#if !defined(_DLL_H_)
#define _DLL_H_

#ifdef _WINDLL
#endif

#include "search/search.h"
#include "board.h"

DLLAPI void DllInit();
DLLAPI void DllConfigureSearch(unsigned char midDepth, unsigned char endDepth);
DLLAPI uint8 DllSearch(double *value);

#endif // _WINDLL

#endif // _DLL_H_
