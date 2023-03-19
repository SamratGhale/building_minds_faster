#ifndef WORLD
#define TILE_CHUNK_UNINITILIZED INT32_MAX

#define TILE_COUNT_PER_WIDTH  17
#define TILE_COUNT_PER_HEIGHT 9

struct WorldPosition{
  V2_S32 chunk_pos; 
  V2_F32 offset;
};

struct EntityNode{
  U32 entity_index;
  EntityNode* next;
};

struct WorldChunk{
  V2_S32 chunk_pos;
  U32 entity_count;
  EntityNode* node;
  WorldChunk* next;
};

struct World{
  V2_F32 chunk_size_in_meters;
  WorldChunk chunk_hash[128];
  S32 meters_to_pixels;
};

#define WORLD
#endif
