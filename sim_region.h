#ifndef SIM_REGION

//TODO: we need this for player
struct Animation{
  B32 is_active;
  V2_F32 dest;
  V2_F32 source;
  V2_S32 ddp;
  B32 forced;
  S32 completed; //In %
};

enum EntityType{
  entity_type_null,
  entity_type_player,
  entity_type_npc,
  entity_type_wall,
  entity_type_temple,
  entity_type_grass,
  entity_type_fire_torch,
  entity_type_tile, //NOTE: based
  entity_type_number,
  entity_type_letter,
};

enum EntityFlags{
  entity_flag_simming = (1 << 1),
  entity_undo = (1<<2),
  entity_flag_dead = (1<<8)
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

  union { //could be use for storing other informations for other kinds of entities
    S64 value_s64;
    F64 value_f64;
  };

  LoadedBitmap* texture;
  Animation* animation; //Curently used for player
  TileNode *tile_path;

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
