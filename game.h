#ifndef GAME_H

#include "world.h"

enum EntityType{
  entity_type_null,
  entity_type_player,
  entity_type_npc,
  entity_type_wall
};

enum EntityFlags{
  //This are all used for the player only
  entity_flag_jumping = (1 << 30),
  entity_flag_falling = (1 << 29),
  entity_on_ground    = (1 << 28),
};

struct LowEntity{
  EntityType type;

  WorldPosition p; /** NOTE: we don't use this now because our camera isn't dynamic **/
  V2 pos; //This is in meters

  V2 dP; 
  F32 width, height;
  B32 collides;

  U32 color;
  U32 stored_index;

  U32 flags;
  F32 jump_time;
  //No showord stuff
};

inline void add_flag(LowEntity* entity, U32 flag){
  assert(entity);
  entity->flags |= flag;
}

inline B32 is_flag_set(LowEntity* entity, U32 flag){
  assert(entity);
  B32 ret = entity->flags & flag;
  return ret;
}

inline void clear_flag(LowEntity* entity, U32 flag){
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
  
  Rec2 camera_p;


  U32 low_entity_count;
  LowEntity low_entities[1000];
  B32 is_initilized;
  S32 controlled_entity_index[5];



};

#define GAME_H
#endif
