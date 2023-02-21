#ifndef PLATFORM_COMMON
#include <stdint.h>

#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Gdi32.lib")
#define Pi32 3.14159265359f

#define local_persist static
#define global_variable static
#define function static

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef S32 B32;

typedef float F32;
typedef double F64;

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define array_count(Array)(sizeof(Array)/sizeof((Array)[0]))

#define assert(expression)if(!(expression)){*(int*)0 =0;}
#define PLATFORM_COMMON

#endif //GUI_PLATFORM_H
