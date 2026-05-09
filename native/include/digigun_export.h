#ifndef DIGIGUN_EXPORT_H
#define DIGIGUN_EXPORT_H

#ifdef _WIN32
  #if defined(dll_export) || defined(dll_import) || defined(DIGIGUN_DYNAMIC)
    #ifdef DIGIGUN_EXPORTS
      #define DIGIGUN_API __declspec(dllexport)
    #else
      #define DIGIGUN_API __declspec(dllimport)
    #endif
  #else
    #define DIGIGUN_API
  #endif
#else
  #define DIGIGUN_API __attribute__((visibility("default")))
#endif

#endif
