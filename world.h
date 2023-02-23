#ifndef WORLD
#define TILE_CHUNK_UNINITILIZED INT32_MAX

struct WorldPosition{
  S32 chunk_x; 
  S32 chunk_y;
  V2 offset;
};

struct EntityNode{
  U32 entity_index;
  EntityNode* next;
};

struct WorldChunk{
  S32 chunk_x;
  S32 chunk_y;

  U32 entity_count;
  EntityNode* node;
  WorldChunk* next;
};

struct World{
  U32 chunk_size_in_pixels;
  F32 chunk_size_in_meters;
  WorldChunk chunk_hash[128];
  F32 meters_to_pixels;
};

#define WORLD
#endif
