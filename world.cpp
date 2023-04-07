#include "world.h"
#include "game.h"
#include <stdio.h>

function void append_tile(WorldChunk* chunk, Tile* new_tile, MemoryArena* arena){

  add_flag(new_tile, tile_path);
  if(chunk->tile_path == NULL){
    chunk->tile_path = push_struct(arena, TileNode); 
    chunk->tile_path->tile = new_tile;
    chunk->tile_path->next = NULL;
  }
  else{
    TileNode* curr = chunk->tile_path;
    while(curr->next){
      curr = curr->next;
    }
    curr->next = push_struct(arena, TileNode);
    curr->next->tile = new_tile;
    curr->next->next = NULL;
  }
}

inline Tile* get_last_tile(TileNode* head){
  TileNode* curr = head; 
  while(curr->next){
    curr = curr->next;
  }
  return curr->tile;
}

inline void remove_last_tile(TileNode* head){
  TileNode* curr = head; 
  if(head->next != NULL){

    if(head->next->next == NULL){
      clear_flag(head->next->tile, tile_path);
      head->next = NULL;
    }else{
      while(curr->next->next){
        curr = curr->next;
      }
      clear_flag(curr->next->tile, tile_path);
      curr->next = NULL;
    }
  }
}

inline S32 get_path_length(TileNode* head){
  S32 count = 1;
  TileNode* curr = head; 
  if(head->next == NULL){
    return 1;
  }else{
    while(curr->next){
      count++;
      curr = curr->next;
    }
  }
  return count;
}

inline Tile* get_index_path(TileNode* head, S32 index){
  if(get_path_length(head) < index){
    return NULL; 
  }

  TileNode* curr = head;

  for (S32 i = 1; i < index; ++i) {
    curr = curr->next;
  }
  return curr->tile;
}



inline void print_world_pos(WorldPosition pos){
  char buff[255];
  sprintf(buff, "chunk_pos.x = %d, chunk_pos.y = %d, x = %.2f, y = %.2f\n",pos.chunk_pos.x, pos.chunk_pos.y, pos.offset.x, pos.offset.y);
  OutputDebugStringA(buff);
}

inline void print_V2_F32(V2_F32 pos){
  char buff[128];
  sprintf(buff, "x = %.2f, y = %.2f\n", pos.x, pos.y);
  OutputDebugStringA(buff);
}

inline B32 is_valid(WorldPosition p) {
  B32 result = (p.chunk_pos.x != TILE_CHUNK_UNINITILIZED);
  return result;
}

inline V2_F32 null_position() {
  V2_F32 result = {};
  result.x = TILE_CHUNK_UNINITILIZED;
  return result;
}

inline B32 are_in_same_chunk(V2_S32 chunk_pos, WorldPosition p) {
  B32 result = ((chunk_pos.x == p.chunk_pos.x) && (chunk_pos.y == p.chunk_pos.y));
  return result;
}

function void initilize_chunk_tiles(World* world, WorldChunk* chunk){
  for (S32 x = -8; x <= 8; x++) {
    for (S32 y = -4; y <= 4; y++) {
      Tile* tile = &chunk->tiles[(x+8) + ((y+4) * TILE_COUNT_PER_WIDTH)];
      tile->color = V4{1.0f, 1.0f, 1.0f, 0.2f};
      tile->tile_pos = V2_S32{x, y};
    }
  }
}

function WorldChunk* add_new_chunk(MemoryArena* arena, World* world, WorldChunk* head, V2_S32 chunk_pos) {
  assert(head);
  WorldChunk* new_chunk = push_struct(arena, WorldChunk);
  new_chunk->chunk_pos = chunk_pos;
  new_chunk->next = NULL;
  new_chunk->node = NULL;
  new_chunk->entity_count = 0;

  WorldChunk* curr = head;

  while (curr->next) {
    curr = curr->next;
  }
  curr->next = new_chunk;
  initilize_chunk_tiles(world, new_chunk);
  return new_chunk;
}

//This will return the world chunk with chunk_pos.x and chunk_pos.y exactly of p
function WorldChunk* get_world_chunk(World* world, V2_S32 chunk_pos, MemoryArena* arena = 0) {
  U32 hash = 19 * chunk_pos.x + 7 * chunk_pos.y;
  U32 hash_slot = hash % (array_count(world->chunk_hash) - 1);

  WorldChunk* head = world->chunk_hash + hash_slot;
  WorldChunk* chunk = head;

  while (chunk) {
    if ((chunk_pos.x == chunk->chunk_pos.x) && (chunk_pos.y == chunk->chunk_pos.y)) {
      break;
    }

    if (chunk->chunk_pos.x == TILE_CHUNK_UNINITILIZED) {
      chunk->chunk_pos = chunk_pos;
      chunk->entity_count = 0;
      initilize_chunk_tiles(world, chunk);
      break;
    }
    chunk = chunk->next;
  }
  if (!chunk && arena) {
    chunk = add_new_chunk(arena, world, head, chunk_pos);
  }
  return chunk;
}

function Tile* get_tile(World* world, WorldPosition pos){
  WorldChunk* chunk = get_world_chunk(world, pos.chunk_pos);
  S32 x = ((S32)roundf(pos.offset.x) + 8);
  S32 y = ((S32)roundf(pos.offset.y) + 4);
  Tile* tile = &chunk->tiles[x + y * TILE_COUNT_PER_WIDTH];
  return tile; 
}


//TODO: make sure the entity was the removed and the new_p was added
inline void change_entity_location_raw(MemoryArena* arena, World* world, U32 entity_index, WorldPosition old_p, WorldPosition new_p) {
  if (is_valid(old_p)) {
    //Old_p is valid now we need to remove the old p from it's previous location in the world
    WorldChunk* chunk = get_world_chunk(world, old_p.chunk_pos, arena);

    assert(chunk);
    assert(chunk->node); //?

    EntityNode* node = chunk->node;
    if (node->entity_index == entity_index) {
      chunk->node = node->next;
    }
    else {
      EntityNode* curr = node;
      while (curr->next) {
        if (curr->next->entity_index == entity_index) {
          node = curr->next;
          curr->next = curr->next->next;
        }
        else {
          curr = curr->next;
        }
      }
    }
  }
  //Add this to the world chunk_pos
  WorldChunk* chunk = get_world_chunk(world, new_p.chunk_pos, arena);
  //Just add it to the end/beggining does it matter?
  EntityNode *node = push_struct(arena, EntityNode);
  node->entity_index = entity_index;
  node->next = chunk->node;
  chunk->node = node;
}

inline void change_entity_location(MemoryArena* arena, World* world, U32 entity_index, LowEntity* entity, WorldPosition new_p_init) {

  if (is_valid(new_p_init)) {
    change_entity_location_raw(arena, world, entity_index, entity->pos, new_p_init);
    entity->pos = new_p_init;
    //NOTE: convert entity->Sim.p to entity->p too
  }
  else {
    //Faulty new position
    assert(0);
  }
}

function void initilize_world(World* world, S32 buffer_width, S32 buffer_height) {

  world->meters_to_pixels = min(buffer_height/TILE_COUNT_PER_HEIGHT, buffer_width/TILE_COUNT_PER_WIDTH);

  world->chunk_size_in_meters = V2_F32{TILE_COUNT_PER_WIDTH , TILE_COUNT_PER_HEIGHT};

  for (U32 i = 0; i < array_count(world->chunk_hash); i += 1) {
    WorldChunk* chunk = &world->chunk_hash[i]; 
    *chunk = {};
    chunk->chunk_pos.x = TILE_CHUNK_UNINITILIZED;

#if 0
#endif
  }
}

inline WorldPosition map_into_chunk_pos_space(World* world, WorldPosition base_pos, V2_F32 offset) {
  WorldPosition result = base_pos;
  result.offset += offset;
}

inline void adjust_world_positon(World* world, S32* chunk_pos, F32 * offset, F32 chunk_size_in_meters) {
  S32 extra_offset = (S32)roundf(*offset / chunk_size_in_meters);
  *chunk_pos += extra_offset;
  *offset -= extra_offset * chunk_size_in_meters;
  //TODO: check if the new values are valid
}

inline WorldPosition map_into_world_position(World* world, WorldPosition* origin, V2_F32 offset) {
  WorldPosition result = *origin;
  result.offset += offset;

  adjust_world_positon(world, &result.chunk_pos.y, &result.offset.y, world->chunk_size_in_meters.y);
  adjust_world_positon(world, &result.chunk_pos.x, &result.offset.x, world->chunk_size_in_meters.x);
  return result;
}

inline V2_F32 subtract(World* world, WorldPosition* a, WorldPosition* b) {
  V2_F32 result = {};
  result.y = a->chunk_pos.y - b->chunk_pos.y;
  result.x = a->chunk_pos.x - b->chunk_pos.x;
  result = V2_F32{result.x  * world->chunk_size_in_meters.x, result.y  * world->chunk_size_in_meters.y};
  result = result + (a->offset - b->offset);
  return result;
}

function WorldPosition create_world_pos(V2_S32 chunk_pos, F32 off_x, F32 off_y) {
  WorldPosition result = {};
  result.chunk_pos.x = chunk_pos.x;
  result.chunk_pos.y = chunk_pos.y;
  result.offset = V2_F32{ off_x, off_y };
  return result;
}

function void update_camera(GameState* game_state, V2_F32 camera_ddp){

  Animation* animation = &game_state->chunk_animation;

  if(!animation->is_active){
    if(camera_ddp.x != 0 || camera_ddp.y != 0){
      animation->is_active = 1;

      //Here the source and dest is in chunk position
      animation->source = game_state->camera_p.offset;
      animation->dest.x += (camera_ddp.x * game_state->world->chunk_size_in_meters.x);
      animation->dest.y += (camera_ddp.y * game_state->world->chunk_size_in_meters.y);
      animation->ddp.x = camera_ddp.x * game_state->world->chunk_size_in_meters.x;
      animation->ddp.y = camera_ddp.y * game_state->world->chunk_size_in_meters.y;
      animation->completed = 0;
    }
  }else{
    if(animation->completed > 100){
      animation->is_active = false;
      game_state->camera_p.offset = {};
      game_state->curr_chunk = game_state->camera_p.chunk_pos;
    }else{
      V2_S32 chunk_diff = animation->ddp;
      F32 add_x = (F32)chunk_diff.x /20.0f;
      F32 add_y = (F32)chunk_diff.y /20.0f;
      game_state->camera_p = map_into_world_position(game_state->world, &game_state->camera_p, V2_F32{add_x, add_y});
      //game_state->curr_chunk = game_state->camera_p.chunk_pos;
      animation->completed += 5;
    }
  }
}


//This is for linked list
function void bubble_sort_entities(GameState* game_state, EntityNode* head){
  World* world = game_state->world;
  EntityNode* curr = head, *index = NULL;
  S32 temp;

  if(head == NULL){
    return;
  }

  while(curr != NULL){

    index = curr->next;
    while(index != NULL){

      LowEntity* curr_entity = &game_state->low_entities[curr->entity_index];
      LowEntity* index_entity = &game_state->low_entities[index->entity_index];

      if(!(curr_entity->sim.type == entity_type_null || index_entity->sim.type == entity_type_null)){


        if(curr_entity->pos.offset.y <= index_entity->pos.offset.y){
          temp = curr->entity_index;
          curr->entity_index = index->entity_index;
          index->entity_index = temp;
        }
      }
      index = index->next;
    }
    curr = curr->next;
  }
}




