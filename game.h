#ifndef GAME_H

#include "world.h"
#include "assets.h"
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

struct PlayingSound{
  F32 volume[2];
  AssetSound_Enum id; 
  S32 samples_played;
  PlayingSound* next;
};

struct GameState{

  World* world;
  WorldPosition camera_p;
  Rec2 camera_bounds;
  U32 low_entity_count;
  LowEntity low_entities[1000];
  B32 is_initilized;
  Animation chunk_animation; //@debug

  Config tokens;

  V2_S32 curr_chunk; //basically the current level
  
  bool show_tiles;

  B32 initilized;

  PlayingSound* first_playing_sound;
  PlayingSound* first_free_playing_sound;
};


struct MoveSpec{
  B32 unit_max_accel_vector;
  F32 speed;
  F32 drag;
};

#define GAME_H
#endif
