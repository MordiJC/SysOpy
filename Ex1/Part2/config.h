#ifndef CONFIG_H_!defined(COMPILE_STATIC)
#define CONFIG_H_

#if defined(COMPILE_DYNAMIC)

    #if __GNUC__ >= 4

        // GCC 4 has special keywords for showing/hidding symbols,
        // the same keyword is used for both importing and exporting
        #define API_EXPORT __attribute__((__visibility__("default")))
        #define API_IMPORT __attribute__((__visibility__("default")))

    #else

        // GCC < 4 has no mechanism to explicitely hide symbols, everything's exported
        #define API_EXPORT
        #define API_IMPORT

    #endif

#else

    // Static build doesn't need import/export macros
    #define API_EXPORT
    #define API_IMPORT

#endif

#if defined(LIBRARY_EXPORTS)

    #define LIBRARY_API API_EXPORT

#else

    #define LIBRARY_API API_IMPORT

#endif


#endif /* CONFIG_H_ */