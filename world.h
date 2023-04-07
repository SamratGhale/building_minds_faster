#ifndef WORLD
#define TILE_CHUNK_UNINITILIZED INT32_MAX

#define TILE_COUNT_PER_WIDTH  16
#define TILE_COUNT_PER_HEIGHT 10

struct WorldPosition{
  V2_S32 chunk_pos; 
  V2_F32 offset;
};

struct EntityNode{
  U32 entity_index;
  EntityNode* next;
};

//Should this be flags?
enum TileFlags{
  tile_occoupied = (1<<1),
  tile_path   = (1<<2),
  tile_empty  = (1<<3),
  tile_entity = (1<<4),
  tile_end    = (1<<5)
};

struct Tile{
  V2_S32 tile_pos;
  V4 color;
  B32 initlized;
  OpenglContext gl_context;

  U32 entity_count;
  EntityNode entities;
  U32 flags;
};

struct TileNode{
 Tile* tile; 
 TileNode *next;
};
/**
 * Helper function to add and remove flags
*/
inline void add_flag(Tile* tile, U32 flag){
  assert(tile);
  tile->flags |= flag;
}

inline B32 is_flag_set(Tile* tile, U32 flag){
  assert(tile);
  B32 ret = tile->flags & flag;
  return ret;
}

inline void clear_flag(Tile* tile, U32 flag){
  assert(tile);
  tile->flags &= ~flag;
}

//TODO: new  file for banner code?

enum BannerStatus{
  banner_status_empty,
  banner_status_number,
  banner_status_letter,
};

struct BannerTile{
  S32 low_index; //Don't use this pointer when status is banner_status_empty
  BannerStatus status;
};

struct Banner{
  BannerTile banners[5];
};

struct WorldChunk{
  V2_S32 chunk_pos;
  V2_F32 player_offset;
  
  U32 entity_count;
  U32 player_index; //this is storage index because 
  Tile tiles[TILE_COUNT_PER_HEIGHT * TILE_COUNT_PER_WIDTH];

  Banner top_banner;
  Banner bottom_banner;

  
  TileNode *tile_path;
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
