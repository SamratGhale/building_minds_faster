#ifndef WIN32_TREE
#define WIN32_TREE

#include <windows.h>
#include "math.h"

struct OffscreenBuffer{
  BITMAPINFO info;
  void* memory;
  U32   pitch;
  S32   width;
  S32   height;
  S32   bytes_per_pixel;
};

#define TILE_DIMENTION 25

struct  ButtonState{
  S32 half_transition_count;
  B32 ended_down;
};

struct ControllerInput{
  B32 is_connected;
  B32 is_analog;
  F32 stick_x;
  F32 stick_y;
  union {
    ButtonState buttons[12];
    struct {
      ButtonState move_up;
      ButtonState move_down;
      ButtonState move_left;
      ButtonState move_right;
      ButtonState action_up;
      ButtonState action_down;
      ButtonState action_left;
      ButtonState action_right;
      ButtonState left_shoulder;
      ButtonState right_shoulder;
      ButtonState back;
      ButtonState start;
    };
  };
};

struct  GameInput{
  ButtonState mouse_buttons[5];
  F32 dtForFrame;
  S32 mouse_x, mouse_y, mouse_z;
  ControllerInput controllers[5];
};

//Arena stuffs
struct MemoryArena{
  S64 size;
  U8* base;
  S64 used;
};

inline void initilize_arena(MemoryArena* arena, S64 size, void* base){
  arena->size = size;
  arena->base = (U8*)base;
  arena->used = 0;
}

#define push_struct(arena, type)(type*)push_size_(arena, sizeof(type))
#define push_array(arena, count, type)(type*)push_size_(arena, (count)* sizeof(type))
inline void* push_size_(MemoryArena* arena, S64 size){
  void* ret  = arena->base + arena->used;
  arena->used += size;
  return ret;
}

struct PlatformState{
  U64 total_size;
  U64 permanent_storage_size;
  U64 temporary_storage_size;

  void* permanent_storage;
  void* temporary_storage;
};

struct ReadFileResult{
  U32 content_size;
  void* contents;
};

function ReadFileResult read_entire_file(char* file_name);
inline U64 get_file_time(char* file_name);
inline B32 is_file_changed(S64 arg1, S64 arg2);

#endif

