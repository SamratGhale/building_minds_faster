#ifndef SIM_REGION

enum EntityType{
  entity_type_null,
  entity_type_player,
  entity_type_npc,
  entity_type_wall,
  entity_type_temple,
  entity_type_grass,
  entity_type_fire_torch
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
  V2_F32 pos; //This is in meters
  V2_F32 dP; 

  V4 color;
  
  F32 width;
  F32 height;

  F32 jump_time;
  B32 collides;

  U32 storage_index;
  U32 flags;
  
  S32 face_direction;
  LoadedBitmap* texture;
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
