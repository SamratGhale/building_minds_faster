#ifndef GAME_H

#include "world.h"
#include "sim_region.h"
#include "parser.cpp"


struct LowEntity{
  //NOTE: we need everything else while simulating
  WorldPosition pos;
  SimEntity sim;
};

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

struct ImageU32{
  U32 width;
  U32 height;
  U32 *pixels;
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
  Config tokens;
};

#define GAME_H
#endif
