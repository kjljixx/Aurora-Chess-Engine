#ifdef _MSC_VER
#define SP_MSVC
#pragma push_macro("_MSC_VER")
#undef _MSC_VER
#endif

#include "external/incbin.h"

#ifdef SP_MSVC
#pragma pop_macro("_MSC_VER")
#undef SP_MSVC
#endif

extern "C" {
  INCBIN(networkData, "alcaeus-2.nnue");
}
