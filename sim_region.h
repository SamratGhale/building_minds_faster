#ifndef SIM_REGION

enum EntityType{
  entity_type_null,
  entity_type_player,
  entity_type_npc,
  entity_type_wall
};

enum EntityFlags{
  //This are all used for the player only
  entity_flag_jumping = (1 << 1),
  entity_flag_falling = (1 << 2),
  entity_flag_simming = (1 << 3),
  entity_on_ground    = (1 << 4),
};

struct SimEntity{

  EntityType type;
  V2 pos; //This is in meters

  V2 dP; 
  F32 width, height;
  B32 collides;

  U32 color;
  U32 storage_index;

  U32 flags;
  F32 jump_time;
  //No showord stuff
};

struct SimRegion{
  U32 max_count;
  U32 entity_count;
  Rec2 bounds;
  WorldPosition center;
  World* world;
  SimEntity* entities;
  MemoryArena* sim_arena;
};

#define SIM_REGION
#endif
