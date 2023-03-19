#ifndef GRID_MATH_H
#include <math.h>

#define ABS(num)(num>=0?num:(-1*num))

union V2_F32{
  struct {
    F32 x, y;
  };
  F32 e[2];
};

union V2_S32{
  struct {
    S32 x, y;
  };
  S32 e[2];
};

union V4 {
  struct {
    F32 w, x, y, z;
  };
  struct {
    F32 r, g, b, a;
  };
  F32 e[4];
};

inline V2_F32 operator*(F32 a, V2_F32 b){
  V2_F32 result;
  result.x = a*b.x;
  result.y = a*b.y;
  return result;
}

inline V2_F32 operator*(V2_F32 b,F32 a){
  V2_F32 result = a*b;
  return result;
}

inline V2_F32 & operator*=(V2_F32 &b, F32 a){
  b = a * b;
  return b;
}

inline V2_F32 operator+(V2_F32 a, V2_F32 b){
  V2_F32 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

inline V2_F32 operator+(V2_F32 a, F32 b){
  V2_F32 result;
  result.x = a.x + b;
  result.y = a.y + b;
  return result;
}

inline V2_F32 & operator+=(V2_F32 &a, V2_F32 b){
  a = a + b;
  return a;
}

inline V2_F32 & operator+=(V2_F32 &a, F32 b){
  a = a + b;
  return a;
}

inline V2_F32 operator-(V2_F32 a, V2_F32 b){
  V2_F32 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

inline V2_F32 &operator-=(V2_F32 &a, V2_F32 b){
  a = a - b;
  return a;
}

inline V2_F32 operator-(V2_F32 a){
  V2_F32 result;
  result.x = -a.x;
  result.y = -a.y;
  return result;
}

inline F32 square(F32 a){
  F32 res = a * a;
  return res;
}

inline F32 inner(V2_F32 a, V2_F32 b){
  F32 res = a.x * b.x + a.y *b.y;
  return res;
}

inline F32 length_sq(V2_F32 a){
  F32 res = inner(a, a);
  return res;
}

inline F32 sq_root(F32 f){
  F32 res = (F32)sqrtf(f);
  return res;
}

inline F32 length(V2_F32 a){
  F32 res = sq_root(length_sq(a));
  return res;
}

struct Rec2{
  V2_F32 min;
  V2_F32 max;
};

inline V2_F32 get_center(Rec2 rect){
  V2_F32 result = 0.5f*(rect.min + rect.max);
  return result;
}

inline V2_F32 get_min_corner(Rec2 rect){
  return rect.min;
}

inline V2_F32 get_max_corner(Rec2 rect){
  return rect.max;
}


//Create rectangle from min vec and max vec
inline Rec2 rect_min_max(V2_F32 min, V2_F32 max){
  Rec2 res;
  res.min = min;
  res.max = max;
  return res;
}

//Create rectangle from min and dimention
inline Rec2 rect_min_dim(V2_F32 min, V2_F32 dim){
  Rec2 res;
  res.min = min;
  res.max = min + dim;
  return res;
}

//Create rectangle from given center and half_dim
inline Rec2 rect_center_half_dim(V2_F32 center, V2_F32 half_dim){
  Rec2 res;
  res.min = center - half_dim;
  res.max = center + half_dim;
  return res;
}

//Create rectangle from given center and full_dim
inline Rec2 rect_center_full_dim(V2_F32 center, V2_F32 full_dim){
  return rect_center_half_dim(center, full_dim * 0.5f);
}

//test is relative to a WorldPosition
inline B32 is_in_rectangle(Rec2 rect, V2_F32 test){
  B32 result = ((test.x <  rect.max.x && test.y < rect.max.y)
	    && ( test.x >= rect.min.x && test.y >= rect.min.y));
  return result;
}
#define GRID_MATH_H
#endif //GUI_MATH_H
