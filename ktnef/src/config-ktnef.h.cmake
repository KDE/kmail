#ifdef __APPLE__
# ifdef __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# else
#  undef WORDS_BIGENDIAN
# endif
#else
#cmakedefine WORDS_BIGENDIAN ${CMAKE_WORDS_BIGENDIAN}
#endif
