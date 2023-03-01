#ifndef GRID_MATH_H
#include <math.h>

#define ABS(num)(num>=0?num:(-1*num))

union V2 {
  struct {
    F32 x, y;
  };
  F32 e[2];
};

inline V2 operator*(F32 a, V2 b){
  V2 result;
  result.x = a*b.x;
  result.y = a*b.y;
  return result;
}

inline V2 operator*(V2 b,F32 a){
  V2 result = a*b;
  return result;
}
inline V2 & operator*=(V2 &b, F32 a){
  b = a * b;
  return b;
}

inline V2 operator+(V2 a, V2 b){
  V2 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

inline V2 & operator+=(V2 &a, V2 b){
  a = a + b;
  return a;
}

inline V2 operator-(V2 a, V2 b){
  V2 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

inline V2 &operator-=(V2 &a, V2 b){
  a = a - b;
  return a;
}

inline V2 operator-(V2 a){
  V2 result;
  result.x = -a.x;
  result.y = -a.y;
  return result;
}




inline F32 square(F32 a){
  F32 res = a * a;
  return res;
}

inline F32 inner(V2 a, V2 b){
  F32 res = a.x * b.x + a.y *b.y;
  return res;
}

inline F32 length_sq(V2 a){
  F32 res = inner(a, a);
  return res;
}

inline F32 sq_root(F32 f){
  F32 res = (F32)sqrtf(f);
  return res;
}

inline F32 length(V2 a){
  F32 res = sq_root(length_sq(a));
  return res;
}

struct Rec2{
  V2 min;
  V2 max;
};

inline V2 get_center(Rec2 rect){
  V2 result = 0.5f*(rect.min + rect.max);
  return result;
}

inline V2 get_min_corner(Rec2 rect){
  return rect.min;
}

inline V2 get_max_corner(Rec2 rect){
  return rect.max;
}


//Create rectangle from min vec and max vec
inline Rec2 rect_min_max(V2 min, V2 max){
  Rec2 res;
  res.min = min;
  res.max = max;
  return res;
}

//Create rectangle from min and dimention
inline Rec2 rect_min_dim(V2 min, V2 dim){
  Rec2 res;
  res.min = min;
  res.max = min + dim;
  return res;
}

//Create rectangle from given center and half_dim
inline Rec2 rect_center_half_dim(V2 center, V2 half_dim){
  Rec2 res;
  res.min = center - half_dim;
  res.max = center + half_dim;
  return res;
}

//Create rectangle from given center and full_dim
inline Rec2 rect_center_full_dim(V2 center, V2 full_dim){
  return rect_center_half_dim(center, full_dim * 0.5f);
}

//test is relative to a WorldPosition
inline B32 is_in_rectangle(Rec2 rect, V2 test){
  B32 result = ((test.x <  rect.max.x && test.y < rect.max.y)
	    && ( test.x >= rect.min.x && test.y >= rect.min.y));
  return result;
}
#define GRID_MATH_H
#endif //GUI_MATH_H
