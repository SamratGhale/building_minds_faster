#ifndef SIM_REGION

enum EntityType{
  entity_type_null,
  entity_type_player,
  entity_type_npc,
  entity_type_wall,
  entity_type_temple
};

struct ImageU32{
  U32 width;
  U32 height;
  U32 *pixels;

  U32 tex_handle;
  U32 vbo;
  U32 vao;
  U32 ebo;
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

  V4 color;
  
  F32 width;
  F32 height;

  F32 jump_time;

  B32 collides;
  U32 storage_index;
  U32 flags;
  S32 face_direction;
  ImageU32* texture;
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
