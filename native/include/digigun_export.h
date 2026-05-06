#ifndef DIGIGUN_EXPORT_H
#define DIGIGUN_EXPORT_H

#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
  #if defined(DIGIGUN_EXPORTS)
    #define DIGIGUN_API __declspec(dllexport)
  #else
    #define DIGIGUN_API __declspec(dllimport)
  #endif
#else
  #if defined(__GNUC__) && __GNUC__ >= 4
    #define DIGIGUN_API __attribute__ ((visibility ("default")))
  #else
    #define DIGIGUN_API
  #endif
#endif

#endif // DIGIGUN_EXPORT_H
