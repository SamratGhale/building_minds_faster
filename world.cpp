#include "world.h"
#include "game.h"

inline B32 is_valid(WorldPosition p){
  B32 result = (p.chunk_x != TILE_CHUNK_UNINITILIZED);
  return result;
}

inline V2 null_position(){
  V2 result = {};
  result.x = TILE_CHUNK_UNINITILIZED;
  return result;
}

inline B32 are_in_same_chunk(S32 chunk_x, S32 chunk_y, WorldPosition p){
  B32 result = ((chunk_x == p.chunk_x) && (chunk_y == p.chunk_y));
  return result;
}

function WorldChunk* add_new_chunk(MemoryArena* arena, World* world, WorldChunk* head, S32 chunk_x, S32 chunk_y){
  assert(head);
  WorldChunk* new_chunk = push_struct(arena, WorldChunk);
  new_chunk->chunk_x = chunk_x;
  new_chunk->chunk_y = chunk_y;
  new_chunk->next= NULL;
  new_chunk->node= NULL;
  new_chunk->entity_count = 0;
  WorldChunk* curr = head;

  while(curr->next){
    curr = curr->next;
  }
  curr->next = new_chunk;
  return new_chunk;
}

//This will return the world chunk with chunk_x and chunk_y exactly of p
inline WorldChunk* get_world_chunk(World* world, S32 chunk_x, S32 chunk_y, MemoryArena* arena =0){
  U32 hash = 19* chunk_x + 7 * chunk_y;
  U32 hash_slot = hash % (array_count(world->chunk_hash) -1);

  WorldChunk* head = world->chunk_hash + hash_slot;
  WorldChunk* chunk = head;

  while(chunk){
    if((chunk_x == chunk->chunk_x) && (chunk_y == chunk->chunk_y)){
      break;
    }

    if(chunk->chunk_x == TILE_CHUNK_UNINITILIZED) {
      chunk->chunk_x = chunk_x;
      chunk->chunk_y = chunk_y;
      chunk->entity_count = 0;
      break;
    }
    chunk = chunk->next;
  }
  if(!chunk && arena){
    chunk = add_new_chunk(arena, world, head, chunk_x, chunk_y);
  }
  return chunk;
}

inline void change_entity_location_raw(MemoryArena* arena,World* world,  U32 entity_index, WorldPosition old_p, WorldPosition new_p){
  if(is_valid(old_p)){
    //Old_p is valid now we need to remove the old p from it's previous location in the world
    WorldChunk* chunk = get_world_chunk(world, old_p.chunk_x, old_p.chunk_y, arena);
    EntityNode* node = chunk->node;
    if(node->entity_index == entity_index){
      chunk->node = node->next;
    }else{
      while(node){
	EntityNode* prev = node;
	node = node->next;
	if(node->entity_index == entity_index){
	  //Job done
	  prev->next = node->next;
	  break;
	}
	node = node->next;
      }
    }
  }
  //Add this to the world chunk
  WorldChunk* chunk = get_world_chunk(world, new_p.chunk_x, new_p.chunk_y, arena);
  //Just add it to the end/beggining does it matter?
  EntityNode *node = push_struct(arena, EntityNode);
  node->entity_index = entity_index;
  node->next = chunk->node;
  chunk->node = node;
}

inline void change_entity_location(MemoryArena* arena,World* world,  U32 entity_index, LowEntity* entity, WorldPosition new_p_init){

  if(is_valid(new_p_init)){
    change_entity_location_raw(arena, world, entity_index, entity->pos, new_p_init);
    entity->pos = new_p_init;
    //NOTE: convert entity->Sim.p to entity->p too
  }else{
    //Faulty new position
    assert(0);
  }
}

function void initilize_world(World* world, S32 chunk_size_in_pixels,F32 chunk_size_in_meters){

  world->chunk_size_in_pixels = chunk_size_in_pixels;
  world->chunk_size_in_meters = chunk_size_in_meters;

  world->meters_to_pixels = (F32)world->chunk_size_in_pixels / (F32)world->chunk_size_in_meters;

  for (U32 i = 0; i < array_count(world->chunk_hash); i += 1){
    world->chunk_hash[i].chunk_x = TILE_CHUNK_UNINITILIZED;
  }
}

inline WorldPosition map_into_chunk_space(World* world, WorldPosition base_pos, V2 offset){
  WorldPosition result = base_pos;
  result.offset += offset;
}

inline void adjust_world_positon(World* world, S32* chunk, F32 * offset){
  S32 extra_offset = (S32)roundf(*offset / world->chunk_size_in_meters);
  *chunk += extra_offset;
  *offset -= *offset * world->chunk_size_in_meters;

  //TODO: check if the new values are valid
}

inline WorldPosition map_into_world_position(World* world, WorldPosition* origin, V2 offset){
  WorldPosition result = *origin; 
  result.offset += offset;

  adjust_world_positon(world, &result.chunk_y, &offset.y);
  adjust_world_positon(world, &result.chunk_x, &offset.x);
  return result;
}

inline V2 subtract(World* world, WorldPosition* a, WorldPosition* b){
  V2 result = {};
  //This is chunk part
  result.y = a->chunk_y - b->chunk_y;
  result.x = a->chunk_x - b->chunk_x;
  result   = result * world->chunk_size_in_meters;
  result  += (a->offset - b->offset);
  return result;
}