#ifndef GalsDef_H
#define GalsDef_H

#ifdef _WIN32
    #ifdef GalsDefS
        #define _GALS_CLASS __declspec(dllexport)
    #else
        #define _GALS_CLASS __declspec(dllimport)
    #endif
#else
    #define _GALS_CLASS
#endif

#endif // GalsDef_H
