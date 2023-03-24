#ifndef WIN32_TREE
#define WIN32_TREE

#include <windows.h>
//TODO: this should not be here
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "opengl32.lib")

#include "math.h"

#define BITMAP_BYTES_PER_PIXEL 4

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
    ButtonState buttons[13];
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
      ButtonState Key_l; //TODO: put this only on debug build?
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

inline S64 get_alignment_offset(MemoryArena* arena, S64 alignment){
  S64 align_offset = 0;
  S64 result_ptr = (S64)arena->base + arena->used;

  S64 align_mask = alignment - 1;
  if(result_ptr & align_mask){
    align_offset = alignment - (result_ptr & align_mask);
  }
  return align_offset;
}

inline void initilize_arena(MemoryArena* arena, S64 size, void* base){
  arena->size = size;
  arena->base = (U8*)base;
  arena->used = 0;
}

#define push_struct(arena, type, ...)(type*)push_size_(arena, sizeof(type), ## __VA_ARGS__)
#define push_array(arena, count, type, ...)(type*)push_size_(arena, (count)* sizeof(type), ## __VA_ARGS__)

#define push_size(arena, size, ...)push_size_(arena, size, ## __VA_ARGS__)


inline void* push_size_(MemoryArena* arena, S64 size_init, S64 alignment = 4){
  S64 size = size_init;

  S64 align_offset = get_alignment_offset(arena, alignment);
  size += align_offset;
  assert((arena->used + size) <= arena->size);


  void* ret  = arena->base + arena->used + align_offset;
  arena->used += size;
  assert(size >= size_init);
  
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

struct LoadedBitmap{
  U32 width;
  U32 height;
  U32 *pixels;
  U32 pitch;
  V2_F32  align_percent;
  F32 width_over_height;

  //For texture stuffs
  U32 tex_handle;
  U32 vbo;
  U32 vao;
  U32 ebo;
};

function ReadFileResult read_entire_file(char* file_name);
inline U64 get_file_time(char* file_name);
inline B32 is_file_changed(S64 arg1, S64 arg2);

#endif

