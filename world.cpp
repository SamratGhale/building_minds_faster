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
inline WorldChunk* get_world_chunk(World* world, WorldPosition p, MemoryArena* arena =0){
    assert(is_valid(p));
    U32 hash = 19* p.chunk_x + 7 * p.chunk_y;
    U32 hash_slot = hash % (array_count(world->chunk_hash) -1);

    WorldChunk* head = world->chunk_hash + hash_slot;
    WorldChunk* chunk = head;

    while(chunk){
        if(are_in_same_chunk(chunk->chunk_x, chunk->chunk_y, p)){
            break;
        }

        if(chunk->chunk_x == TILE_CHUNK_UNINITILIZED) {
            chunk->chunk_x = p.chunk_x;
            chunk->chunk_y = p.chunk_y;
            chunk->entity_count = 0;
            break;
        }
        chunk = chunk->next;
    }
    if(!chunk){
        chunk = add_new_chunk(arena, world, head, p.chunk_x, p.chunk_y);
    }
    return chunk;
}

inline void change_entity_location_raw(MemoryArena* arena,World* world,  U32 entity_index, WorldPosition old_p, WorldPosition new_p){
    if(is_valid(old_p)){
        //Old_p is valid now we need to remove the old p from it's previous location in the world
        WorldChunk* chunk = get_world_chunk(world, old_p, arena);
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
    WorldChunk* chunk = get_world_chunk(world, new_p, arena);
    //Just add it to the end/beggining does it matter?
    EntityNode *node = push_struct(arena, EntityNode);
    node->entity_index = entity_index;
    node->next = chunk->node;
    chunk->node = node;
}

inline void change_entity_location(MemoryArena* arena,World* world,  U32 entity_index, LowEntity* entity, WorldPosition new_p_init){
    if(is_valid(new_p_init)){
        change_entity_location_raw(arena, world, entity_index, entity->p, new_p_init);
        entity->p = new_p_init;
    }else{
        //Faulty new position
        assert(0);
    }
}

function void initilize_world(World* world, S32 tiles_per_height, S32 tiles_per_width,  F32 tile_size_in_meters){
    world->tiles_per_height = tiles_per_height;
    world->tiles_per_width  = tiles_per_width;

    world->tile_size_in_meters = tile_size_in_meters;
    world->chunk_size_in_meters = tiles_per_width * tile_size_in_meters;

    world->tile_size_in_pixels = 60;
    world->meters_to_pixels = (F32)world->tile_size_in_pixels / (F32)world->tile_size_in_meters;

    for (U32 i = 0; i < array_count(world->chunk_hash); i += 1){
        world->chunk_hash[i].chunk_x = TILE_CHUNK_UNINITILIZED;
    }
}

inline WorldPosition map_into_chunk_space(World* world, WorldPosition base_pos, V2 offset){
    WorldPosition result = base_pos;
    result.offset += offset;
}

inline WorldPosition chunkp_from_tilep(World *world, S32 tile_x, S32 tile_y){
    WorldPosition result = {};
    result.chunk_x = tile_x / world->tiles_per_width;
    result.chunk_y = tile_y / world->tiles_per_height;
    result.offset.x = (F32)((tile_x - (world->tiles_per_width/2)) -  (result.chunk_x*world->tiles_per_width)) * world->tile_size_in_meters;
    result.offset.y = (F32)((tile_y - (world->tiles_per_height/2)) - (result.chunk_y*world->tiles_per_height)) * world->tile_size_in_meters;
    return result;
}

