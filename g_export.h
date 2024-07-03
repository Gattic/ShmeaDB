#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef BUILDING_MY_LIBRARY
        #define G_EXPORT __declspec(dllexport)
    #else
        #define G_EXPORT __declspec(dllimport)
    #endif
#else
    #if __GNUC__ >= 4
        #define G_EXPORT __attribute__((visibility("default")))
    #else
        #define G_EXPORT
    #endif
#endif

#ifndef GEXPORT_TEST
#define GEXPORT_TEST

extern "C" G_EXPORT int gexp_test_function(int a, int b)
{
    return a + b;
}

#endif
