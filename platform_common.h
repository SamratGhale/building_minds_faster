#ifndef PLATFORM_COMMON
#include <stdint.h>

#ifndef UNICODE
#undef UNICODE 
#endif

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


function B32 strings_are_equal(char* a, char* b){
  B32 result = ( a == b);
  if(a && b){
    while(*a && *b &&(*a == *b)){
      ++a;
      ++b;
    }
    result = ((*a == 0) && (*b == 0));
  }
  return result;
}

function B32 strings_are_equal(U32 a_len, char* a, char* b){
  char* at = b;
  for(U32 i = 0; i < a_len; ++i, ++at){
    if((at == 0) || (a[i] != *at)){
      return false;
    }
  }
  B32 result = (*at == 0);
  return result;
}

function B32 is_eol(char c){
  B32 result = ((c == '\n') || (c == '\r'));
  return result;
}


function B32 is_whitespace(char c){
  B32 result = ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f') || is_eol(c));
  return result;
}


#define PLATFORM_COMMON
#endif //GUI_PLATFORM_H
