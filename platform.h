#ifndef WIN32_TREE
#define WIN32_TREE

#include <dsound.h>
#include <windows.h>

// TODO: this should not be here
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "Dsound.lib")
#pragma comment(lib, "winmm.lib")

#include "bmf_opengl.h"
#include "math.h"
#include "assets.h"

#define BITMAP_BYTES_PER_PIXEL 4

struct OffscreenBuffer {
  BITMAPINFO info;
  void* memory;
  U32 pitch;
  S32 width;
  S32 height;
  S32 bytes_per_pixel;
};

#define TILE_DIMENTION 25

struct ButtonState {
  S32 half_transition_count;
  B32 ended_down;
};

struct ControllerInput {
  B32 is_connected;
  B32 is_analog;
  F32 stick_x;
  F32 stick_y;
  union {
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
      ButtonState Key_u;
      ButtonState Key_l;  // TODO: put this only on debug build?
      ButtonState Key_t;  // TODO: put this only on debug build?
      ButtonState enter;
      ButtonState escape;
    };
    ButtonState buttons[17];
  };
};

struct GameInput {
  ButtonState mouse_buttons[5];
  F32 dtForFrame;
  S32 mouse_x, mouse_y, mouse_z;
  ControllerInput controllers[5];
};

struct SoundOutput {
  S32 samples_per_second;
  U32 running_sample_index;
  S32 bytes_per_sample;
  DWORD secondary_buffer_size;
  DWORD safety_bytes;
};

struct GameSoundOutputBuffer {
  S32 samples_per_second;
  S32 sample_count;
  S16* samples;
};

struct GameOffscreenBuffer {
  void* memory;
  S32 width;
  S32 height;
  S32 pitch;
};

#define ended_down(b, C) ((C->b.ended_down) ? 1 : 0)
#define was_down(b, C) \
  ((!C->b.ended_down && C->b.half_transition_count) ? 1 : 0)

// Arena stuffs
struct MemoryArena {
  S64 size;
  U8* base;
  S64 used;

  S32 temp_count;
};

struct TransientState{
  B32 is_initilized;
  MemoryArena trans_arena;
};

inline S64 get_alignment_offset(MemoryArena* arena, S64 alignment) {
  S64 align_offset = 0;
  S64 result_ptr = (S64)arena->base + arena->used;

  S64 align_mask = alignment - 1;
  if (result_ptr & align_mask) {
    align_offset = alignment - (result_ptr & align_mask);
  }
  return align_offset;
}

inline void initilize_arena(MemoryArena* arena, S64 size, void* base) {
  arena->size = size;
  arena->base = (U8*)base;
  arena->used = 0;
}

#define push_struct(arena, type, ...) \
  (type*)push_size_(arena, sizeof(type), ##__VA_ARGS__)
#define push_array(arena, count, type, ...) \
  (type*)push_size_(arena, (count) * sizeof(type), ##__VA_ARGS__)

#define push_size(arena, size, ...) push_size_(arena, size, ##__VA_ARGS__)

inline void* push_size_(MemoryArena* arena, S64 size_init, S64 alignment = 4) {
  S64 size = size_init;

  S64 align_offset = get_alignment_offset(arena, alignment);
  size += align_offset;
  assert((arena->used + size) <= arena->size);

  void* ret = arena->base + arena->used + align_offset;
  arena->used += size;
  assert(size >= size_init);

  return ret;
}

enum GameMode {
  game_mode_menu,
  game_mode_view,
  game_mode_play,
  game_mode_exit
};

struct ReadFileResult {
  U32 content_size;
  void* contents;
};

#pragma pack(push, 1)  // Using pragma pack(push, 1) so that we can cast
                       // directly from the file
struct BitmapHeader {  // This is used only when reading the files
  float width;
  float height;
  uint32_t pitch;
  U64 total_size;
};
struct LoadedBitmap {
  float width;
  float height;
  U32 pitch;
  U64 total_size;
  U32* pixels;
  V2_F32 align_percent;
  F32 width_over_height;

  // For texture stuffs
  OpenglContext gl_context;
};

#pragma pack(pop)

enum Asset_Enum {
  asset_background,
  asset_temple,
  asset_player_right,
  asset_player_left,
  asset_player_back,
  asset_wall,
  asset_grass,
  asset_banner_tile,
  asset_fire_torch,
  asset_count
};

struct Asset {
  LoadedBitmap bitmaps[asset_count];
};

struct FontAsset {
  LoadedBitmap bitmaps[96];
};

enum AssetSound_Enum{
  asset_sound_background,
  asset_sound_jump
};

struct SoundAsset {
  LoadedSound sounds[10];
};



struct TempMemory{
  MemoryArena *arena;
  S64 used; 
};

inline TempMemory begin_temp_memory(MemoryArena* arena){
  TempMemory result;
  result.arena = arena;
  result.used = arena->used;
  ++arena->temp_count;

  return result;
}

inline void end_temp_memory(TempMemory temp_mem){
  MemoryArena* arena =  temp_mem.arena;
  arena->used = temp_mem.used;
  --arena->temp_count;
}

struct PlatformState {
  GameMode game_mode;

  Asset asset;
  U64 total_size;
  U64 permanent_storage_size;
  U64 temporary_storage_size;
  MemoryArena arena;
  FontAsset font_asset;
  SoundAsset sound_asset;

  void* permanent_storage;
  void* temporary_storage;
};

function ReadFileResult read_entire_file(char* file_name);
inline U64 get_file_time(char* file_name);
inline B32 is_file_changed(S64 arg1, S64 arg2);

#endif
