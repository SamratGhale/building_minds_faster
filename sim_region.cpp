function SimEntity* add_entity(GameState* game_state, SimRegion* region, U32 low_index, LowEntity* low_entity, V2_F32* entity_rel_pos) {
  assert(low_index);
  SimEntity* entity = 0;
  if (region->entity_count < region->max_count) {
    entity = region->entities + region->entity_count++;

    if (low_entity) {
      *entity = low_entity->sim;
      add_flag(&low_entity->sim, entity_flag_simming);
    }

    if (entity) {
      entity->storage_index = low_index;
      if (entity_rel_pos) {
        entity->pos = *entity_rel_pos;
      }
      else {
        entity->pos = subtract(region->world, &low_entity->pos, &region->center);
      }
    }
  }
  return entity;
}

function SimRegion* begin_sim(MemoryArena* sim_arena, GameState* game_state, World* world, WorldPosition center, Rec2 bounds) {

  //Initilize sim
  SimRegion* sim_region = push_struct(sim_arena, SimRegion);
  sim_region->world = world;
  sim_region->sim_arena = sim_arena;
  sim_region->center = center;
  sim_region->bounds = bounds;

  //NOTE: good programming
  sim_region->max_count = 1024;
  sim_region->entity_count = 0;
  sim_region->entities = push_array(sim_arena, sim_region->max_count, SimEntity);

  //TODO: take the world_position and get the camera_relative positon of
  //Min corner and max corner

  WorldPosition min_chunk_pos = map_into_world_position(world, &sim_region->center, get_min_corner(sim_region->bounds));
  WorldPosition max_chunk_pos = map_into_world_position(world, &sim_region->center, get_max_corner(sim_region->bounds));

  for (S32 y = min_chunk_pos.chunk_pos.y; y <= max_chunk_pos.chunk_pos.y; y++) {
    for (S32 x = min_chunk_pos.chunk_pos.x; x <= max_chunk_pos.chunk_pos.x; x++) {
      WorldChunk*chunk_pos = get_world_chunk(world, V2_S32{x, y});
      if (chunk_pos) {
          //assert(0);
        while (chunk_pos) {
          EntityNode* node = chunk_pos->node;
          while (node && node->entity_index) {

            LowEntity* entity = game_state->low_entities + node->entity_index;
            V2_F32 entity_sim_space = subtract(sim_region->world, &entity->pos, &sim_region->center);

            if (is_in_rectangle(sim_region->bounds, entity_sim_space))
              add_entity(game_state, sim_region, node->entity_index, entity, &entity_sim_space);

            node = node->next;
          }
          chunk_pos = chunk_pos->next;
        }
      }

    }
  }
  return sim_region;
}

function void end_sim(SimRegion* region, GameState* game_state) {
  SimEntity* entity = region->entities;
  for (U32 i = 0; i < region->entity_count; i += 1, ++entity) {
    LowEntity* stored = game_state->low_entities + entity->storage_index;

    entity->flags = stored->sim.flags; //FIXME:
    stored->sim = *entity;
    WorldPosition new_world_p = map_into_world_position(region->world, &region->center, entity->pos);
    WorldPosition old_pos = stored->pos;

    change_entity_location(&platform.arena, region->world, entity->storage_index, stored, new_world_p);

    //Change tile's entity
    Tile* old_tile = get_tile(game_state->world, old_pos);
  }
}


