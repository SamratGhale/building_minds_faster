#include "world.h"
#include "game.h"
#include <stdio.h>

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
  return new_chunk;
}

//This will return the world chunk with chunk_pos.x and chunk_pos.y exactly of p
inline WorldChunk* get_world_chunk(World* world, V2_S32 chunk_pos, MemoryArena* arena = 0) {
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
      break;
    }
    chunk = chunk->next;
  }
  if (!chunk && arena) {
    chunk = add_new_chunk(arena, world, head, chunk_pos);
  }
  return chunk;
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

  world->chunk_size_in_meters = V2_F32{TILE_COUNT_PER_WIDTH , TILE_COUNT_PER_HEIGHT};
  world->meters_to_pixels = min(buffer_height/TILE_COUNT_PER_HEIGHT, buffer_width/TILE_COUNT_PER_WIDTH);

  for (U32 i = 0; i < array_count(world->chunk_hash); i += 1) {
    world->chunk_hash[i] = {};
    world->chunk_hash[i].chunk_pos.x = TILE_CHUNK_UNINITILIZED;
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

function void update_camera(GameState* game_state, V2_F32* camera_ddp){
	if(!game_state->chunk_animation.is_active){
		LowEntity* player = game_state->low_entities + game_state->player_index;
		WorldPosition* camera = &game_state->camera_p;
		V2_F32 entity_cam_space = subtract(game_state->world, &player->pos, &game_state->camera_p);
		World* world = game_state->world;

		if(entity_cam_space.x > 5.0f){
			camera_ddp->x = 1.0f;
		}
		else if(entity_cam_space.x < -5.0f ){
			camera_ddp->x = -1.0f;
		}
		if(entity_cam_space.y > 2.0f){
			camera_ddp->y = 1;
		}
		else if( entity_cam_space.y < -2.0f ){
			camera_ddp->y = -1;
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









