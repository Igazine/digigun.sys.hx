#ifndef DIGIGUN_EXPORT_H
#define DIGIGUN_EXPORT_H

#ifdef _WIN32
    #ifdef DIGIGUN_EXPORTS
        #define DIGIGUN_API __declspec(dllexport)
    #elif defined(DIGIGUN_DYNAMIC)
        #define DIGIGUN_API __declspec(dllimport)
    #else
        #define DIGIGUN_API
    #endif
#else
  #define DIGIGUN_API __attribute__((visibility("default")))
#endif

#endif
