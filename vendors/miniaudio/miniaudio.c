#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "extras/stb_vorbis.c"

#ifdef L
#undef L
#endif

#ifdef C
#undef C
#endif

#ifdef R
#undef R
#endif