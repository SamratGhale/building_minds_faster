#ifndef GAME_H

#include "world.h"
#include "sim_region.h"
#include "parser.cpp"

struct LowEntity{
  //NOTE: we need everything else while simulating
  WorldPosition pos;
  SimEntity sim;
};

/**
 * Helper function to add and remove flags
 */
inline void add_flag(SimEntity* entity, U32 flag){
  assert(entity);
  entity->flags |= flag;
}

inline B32 is_flag_set(SimEntity* entity, U32 flag){
  assert(entity);
  B32 ret = entity->flags & flag;
  return ret;
}

inline void clear_flag(SimEntity* entity, U32 flag){
  assert(entity);
  entity->flags &= ~flag;
}

enum Asset_Enum{
  asset_background,
  asset_temple,
  asset_player_right,
  asset_player_left,
  asset_wall,
  asset_grass,
  asset_banner_tile,
  asset_fire_torch,
  asset_count
};

struct Asset{
  LoadedBitmap bitmaps[asset_count];
};

struct FontAsset{
  LoadedBitmap bitmaps[96];
};

struct GameState{
  MemoryArena world_arena;

  World* world;
  WorldPosition camera_p;
  Rec2 camera_bounds;
  U32 low_entity_count;
  LowEntity low_entities[1000];
  B32 is_initilized;
  S32 controlled_entity_index[5];
  S32 player_index;
  S32 debug_index;
  Animation chunk_animation; //@debug
  Asset asset;
  FontAsset font_asset;

  Config tokens;

  V2_S32 curr_chunk; //basically the current level
  
  bool show_tiles;
};


struct MoveSpec{
  B32 unit_max_accel_vector;
  F32 speed;
  F32 drag;
};

#define GAME_H
#endif
