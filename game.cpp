#include "menu.h"
#include "game.h"

#include <stdio.h>

#include "png_reader.cpp"
#include "assets.cpp"
#include "world.cpp"
#include "sim_region.cpp"
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

inline MoveSpec default_move_spec() {
  MoveSpec result;
  result.unit_max_accel_vector = 0;
  result.speed = 1.0f;
  result.drag = 0.0f;
  return result;
}

function B32 test_wall(F32 wall_x, F32 rel_x, F32 rel_y, F32 p_delta_x,
                       F32 p_delta_y, F32* t_min, F32 min_y, F32 max_y) {
  B32 hit = false;
  F32 t_epsilon = 0.001f;
  if (p_delta_x != 0.0f) {
    F32 t_result = (wall_x - rel_x) / p_delta_x;
    F32 y = rel_y + t_result * p_delta_y;
    if ((t_result >= 0.0f) && (*t_min > t_result)) {
      if ((y >= min_y) && (y <= max_y)) {
        *t_min = MAX(0.0f, t_result - t_epsilon);
        hit = true;
      }
    }
  }
  return hit;
}

function PlayingSound* game_play_sound(GameState* game_state, AssetSound_Enum sound_id){
  if(!game_state->first_free_playing_sound)
  {
    game_state->first_free_playing_sound = push_struct(&platform.arena, PlayingSound); 
    game_state->first_free_playing_sound->next = 0;
  } 

  PlayingSound* playing_sound = game_state->first_free_playing_sound;
  game_state->first_free_playing_sound = playing_sound->next;

  playing_sound->samples_played = 0;
  playing_sound->volume[0] = 1.0f;
  playing_sound->volume[1] = 1.0f;
  playing_sound->id = sound_id;
  playing_sound->next = game_state->first_playing_sound;
  game_state->first_playing_sound = playing_sound;
  return playing_sound;
}

function void update_animation_player(GameState* game_state, V2_S32 ddp,
                                      SimEntity* entity, LowEntity* low,
                                      B32 force = false) {
  // Smooth chunk_pos start
  Animation* animation = entity->animation;
  V2_F32* start_pos = &entity->pos;

  World* world = game_state->world;
  if (ddp.x != 0 || ddp.y != 0) {
    WorldPosition dest_pos = low->pos;
    dest_pos.offset.x += ddp.x;
    dest_pos.offset.y += ddp.y;

    Tile* tile = get_tile(world, dest_pos);
    if (!(is_flag_set(tile, tile_occoupied) || is_flag_set(tile, tile_path)) ||
        force) {
      if (!animation->is_active) {
        animation->is_active = true;
        animation->source = *start_pos;
        animation->dest = *start_pos;
        animation->completed = 0;
        animation->dest.x += ddp.x;
        animation->dest.y += ddp.y;
        animation->ddp = ddp;

        if (ddp.x == -1) {
          entity->face_direction = 2;
        }
        if (ddp.x == 1) {
          entity->face_direction = 0;
        }
        if (ddp.y == 1) {
          entity->face_direction = 1;
        }

        animation->forced = force;
        game_play_sound(game_state,  asset_sound_jump);
      }
    }
  }

  if (animation->is_active) {
    if (animation->completed >= 100) {
      animation->is_active = false;
      start_pos->x = roundf(start_pos->x);
      start_pos->y = roundf(start_pos->y);

      Tile* tile = get_tile(world, low->pos);
      WorldChunk* chunk = get_world_chunk(world, low->pos.chunk_pos);
      assert(chunk);

      if (!animation->forced) {
        append_tile(chunk, tile, &platform.arena);

        if (is_flag_set(tile, tile_end)) {
          Banner* top_banner = &chunk->top_banner;
          for (int i = 0; i < array_count(top_banner->banners); i++) {
            BannerTile* btile = &top_banner->banners[i];

            if (btile->status == banner_status_empty) {
              if (low->sim.type == entity_type_letter) {
                btile->status = banner_status_letter;
              } else {
                btile->status = banner_status_number;
              }
              btile->low_index = low->sim.storage_index;
              break;
            }
          }

        } else if (is_flag_set(tile, tile_entity)) {
          EntityNode* node = &tile->entities;
          for (int i = 0; i < tile->entity_count; i++) {
            LowEntity* low = &game_state->low_entities[node->entity_index];
            add_flag(&low->sim, entity_flag_dead);

            Banner* top_banner = &chunk->top_banner;

            for (int i = 0; i < array_count(top_banner->banners); i++) {
              BannerTile* btile = &top_banner->banners[i];

              if (btile->status == banner_status_empty) {
                if (low->sim.type == entity_type_letter) {
                  btile->status = banner_status_letter;
                } else {
                  btile->status = banner_status_number;
                }
                btile->low_index = low->sim.storage_index;
                break;
              }
            }
            node = node->next;
          }
        }
      } else {
        WorldPosition old_pos = {low->pos.chunk_pos, animation->source};

        Tile* source_tile = get_tile(world, old_pos);

        if (is_flag_set(source_tile, tile_entity)) {
          Banner* top = &chunk->top_banner;
          for (int i = 0; i < array_count(top->banners); i++) {
            BannerTile* btile = &top->banners[i];

            if (btile->status != banner_status_empty) {
              assert(btile->low_index);
              LowEntity* banner_entity =
                  &game_state->low_entities[btile->low_index];
              Tile* entity_tile =
                  get_tile(game_state->world, banner_entity->pos);

              if (entity_tile == source_tile) {
                clear_flag(&banner_entity->sim, entity_flag_dead);
                top->banners[i].low_index = 0;
                top->banners[i].status = banner_status_empty;
              }
            }
          }
        }
      }
    } else {
      V2_S32 chunk_diff = animation->ddp;
      F32 add_x = (F32)chunk_diff.x / 10.0f;
      F32 add_y = (F32)chunk_diff.y / 10.0f;
      *start_pos += V2_F32{add_x, add_y};
      animation->completed += 10;
    }
  }
  // Smooth chunk_pos end
}

function void undo_player(GameState* game_state, LowEntity* low_entity,
                          V2_S32* ddp) {
  SimEntity* sim = &low_entity->sim;
  Animation* animation = low_entity->sim.animation;

  WorldChunk* chunk =
      get_world_chunk(game_state->world, game_state->curr_chunk);
  S32 len = get_path_length(chunk->tile_path);

  if ((len > 1) && !animation->is_active) {
    V2_S32 diff = {};
    *ddp = diff;
    Tile* curr_tile = get_last_tile(chunk->tile_path);
    remove_last_tile(chunk->tile_path);
    Tile* prev_tile = get_last_tile(chunk->tile_path);
    assert(prev_tile);  // because we are checking this on the if block
    ddp->x = roundf(prev_tile->tile_pos.x) - roundf(curr_tile->tile_pos.x);
    ddp->y = roundf(prev_tile->tile_pos.y) - roundf(curr_tile->tile_pos.y);
  }
}

function void move_entity(SimRegion* sim_region, SimEntity* entity, F32 dt,
                          MoveSpec* move_spec, V2_F32 ddP) {
  World* world = sim_region->world;

  if (move_spec->unit_max_accel_vector) {
    F32 ddP_len = length_sq(ddP);
    if (ddP_len > 1.0f) {
      ddP = ddP * (1.0f / sq_root(ddP_len));
    }
  }
  ddP *= move_spec->speed;
  ddP += -move_spec->drag * entity->dP;
  V2_F32 delta = (0.5f * ddP * square(dt) + entity->dP * dt);
  entity->dP = ddP * dt + entity->dP;

  for (S32 j = 0; j < 4; j += 1) {
    F32 t_min = 1.0f;
    V2_F32 wall_normal = {};
    U32 hit_entity_index = -1;
    V2_F32 desired_pos = entity->pos + delta;

    for (S32 i = 0; i < sim_region->entity_count; i += 1) {
      if (i != entity->storage_index) {
        SimEntity* test_entity = sim_region->entities + i;
        if (test_entity->collides) {
          F32 diameter_w = test_entity->width + entity->width;
          F32 diameter_h = test_entity->height + entity->height;
          V2_F32 min_corner = -0.5f * V2_F32{diameter_w, diameter_h};
          V2_F32 max_corner = 0.5f * V2_F32{diameter_w, diameter_h};

          V2_F32 rel = entity->pos - test_entity->pos;

          if (test_wall(min_corner.x, rel.x, rel.y, delta.x, delta.y, &t_min,
                        min_corner.y, max_corner.y)) {
            wall_normal = V2_F32{-1, 0};
            hit_entity_index = i;
          }
          if (test_wall(max_corner.x, rel.x, rel.y, delta.x, delta.y, &t_min,
                        min_corner.y, max_corner.y)) {
            wall_normal = V2_F32{1, 0};
            hit_entity_index = i;
          }
          if (test_wall(max_corner.y, rel.y, rel.x, delta.y, delta.x, &t_min,
                        min_corner.x, max_corner.x)) {
            wall_normal = V2_F32{0, 1};
            hit_entity_index = i;
          }
          if (test_wall(min_corner.y, rel.y, rel.x, delta.y, delta.x, &t_min,
                        min_corner.x, max_corner.x)) {
            wall_normal = V2_F32{0, -1};
            hit_entity_index = i;
          }
        }
      }
    }

    entity->pos += t_min * delta;
    if (hit_entity_index != -1) {
      entity->dP =
          entity->dP - 1 * inner(entity->dP, wall_normal) * wall_normal;
      delta = desired_pos - entity->pos;
      delta = delta - 1 * inner(delta, wall_normal) * wall_normal;
    } else {
      break;
    }
  }
  if ((entity->dP.x == 0.0f) && (entity->dP.x == 0.0f)) {
  } else if (ABS(entity->dP.x) > ABS(entity->dP.y)) {
    if (entity->dP.x > 0) {
      entity->face_direction = 0;  // right
    } else {
      entity->face_direction = 2;  // left
    }
  }
}

function void paint_buffer(OffscreenBuffer* buffer, F32 r, F32 g, F32 b) {
  glPushAttrib(GL_VIEWPORT);
  glViewport(0, 0, buffer->width, buffer->height);
  glClearColor(r, g, b, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glPopAttrib();
}

struct AddLowEntityResult {
  LowEntity* low;
  U32 index;
};

inline WorldPosition null_pos() {
  WorldPosition result = {};
  result.chunk_pos.x = TILE_CHUNK_UNINITILIZED;
  return result;
}

function AddLowEntityResult add_low_entity(GameState* game_state,
                                           EntityType type, WorldPosition p,
                                           V4 color = V4{0, 0, 0, 0}) {
  U32 entity_index = game_state->low_entity_count++;
  LowEntity* low_entity = game_state->low_entities + entity_index;
  *low_entity = {};
  low_entity->pos = null_pos();
  change_entity_location(&platform.arena, game_state->world, entity_index,
                         low_entity, p);
  // low_entity->pos = p;
  low_entity->sim.color = color;
  low_entity->sim.storage_index = entity_index;
  low_entity->sim.type = type;
  AddLowEntityResult result = {};
  result.low = low_entity;
  result.index = entity_index;
  return result;
}

function AddLowEntityResult add_entity(GameState* game_state, V2_S32 chunk_pos,
                                       V2_F32 offset, EntityType type,
                                       LoadedBitmap* image, V4 color,
                                       V2_F32 size, B32 collides = true) {
  S32 mtop = game_state->world->meters_to_pixels;

  WorldChunk* chunk = get_world_chunk(game_state->world, chunk_pos);

  WorldPosition pos = {};
  pos.chunk_pos = chunk_pos;
  pos.offset = offset;
  AddLowEntityResult entity =
      add_low_entity(game_state, entity_type_wall, pos, color);
  entity.low->sim.width = size.x;
  entity.low->sim.height = size.y;
  entity.low->sim.color = color;
  entity.low->sim.type = type;
  entity.low->sim.face_direction = -1;
  entity.low->sim.texture = image;
  entity.low->sim.collides = collides;

  Tile* tile = get_tile(game_state->world, entity.low->pos);
  if (collides) {
    add_flag(tile, tile_occoupied);
  }

  if (type == entity_type_player) {
    entity.low->sim.animation = push_struct(&platform.arena, Animation);
    append_tile(chunk, tile, &platform.arena);
  } else if ((type == entity_type_number) || (type == entity_type_letter)) {
    add_flag(tile, tile_entity);

    EntityNode* node = &tile->entities;

    for (int i = 0; i < tile->entity_count; i++) {
      node = node->next;
    }
    tile->entity_count++;
    node->entity_index = entity.index;
  } else if (type == entity_type_fire_torch) {
    add_flag(tile, tile_end);

    EntityNode* node = &tile->entities;

    for (int i = 0; i < tile->entity_count; i++) {
      node = node->next;
    }
    tile->entity_count++;
    node->entity_index = entity.index;
  }
  return entity;
}

function void add_entity_number(GameState* game_state, V2_S32 chunk_pos,
                                V2_F32 offset, S32 val) {
  LoadedBitmap* texture = get_font_number(&platform.font_asset, val);

  AddLowEntityResult entity =
      add_entity(game_state, chunk_pos, offset, entity_type_number, texture,
                 V4{1, 1, 1, 1}, V2_F32{1, 1}, false);

  entity.low->sim.value_s64 = val;
}

function void add_entity_letter(GameState* game_state, V2_S32 chunk_pos,
                                V2_F32 offset, char val) {
  LoadedBitmap* texture = get_font(&platform.font_asset, val);

  AddLowEntityResult entity =
      add_entity(game_state, chunk_pos, offset, entity_type_letter, texture,
                 V4{1, 1, 1, 1}, V2_F32{1, 1}, false);

  entity.low->sim.value_s64 = val;
}

function void add_walls_around_chunk_pos(GameState* game_state,
                                         V2_S32 chunk_pos) {
  S32 mtop = game_state->world->meters_to_pixels;

  F32 height = 1;
  F32 width = 1;
  V4 color = V4{1, 1, 1, 1};

  LoadedBitmap* one = get_font(&platform.font_asset, '4');

  LoadedBitmap* tex_wall = get_asset(&platform.asset, asset_wall);
  LoadedBitmap* tex_grass = get_asset(&platform.asset, asset_grass);

  for (F32 x = -7; x <= 7; x++) {
    add_entity(game_state, chunk_pos, V2_F32{x, -4}, entity_type_wall, tex_wall,
               color, V2_F32{height, width}, true);
    add_entity(game_state, chunk_pos, V2_F32{x, 4}, entity_type_wall, tex_wall,
               color, V2_F32{1, 1}, true);
  }
  for (F32 y = -3; y <= 3; y++) {
    add_entity(game_state, chunk_pos, V2_F32{-7, y}, entity_type_wall, tex_wall,
               color, V2_F32{height, width}, true);
    add_entity(game_state, chunk_pos, V2_F32{7, y}, entity_type_wall, tex_wall,
               color, V2_F32{height, width}, true);
  }
}

// Just a thing
// Height and width will be image height and width
function AddLowEntityResult add_temple(GameState* game_state, V2_S32 chunk_pos,
                                       V2_F32 pos, LoadedBitmap* image) {
  S32 mtop = game_state->world->meters_to_pixels;

  WorldPosition temple_pos = {};
  temple_pos.chunk_pos.y = chunk_pos.y;
  temple_pos.chunk_pos.x = chunk_pos.x;
  temple_pos.offset = pos;
  // U32 color = denormalize_color(0.0f, 0.4f, 0.3f);
  AddLowEntityResult entity = add_low_entity(
      game_state, entity_type_temple, temple_pos, V4{1.0f, 0.0f, 0.0f, 1.0f});

  entity.low->sim.height = ((F32)image->height / (F32)mtop);
  entity.low->sim.width = ((F32)image->width / (F32)mtop);
  entity.low->sim.color = V4{0.7f, 0.4f, .9f, 1};
  entity.low->sim.face_direction = -1;
  entity.low->sim.texture = image;
  return entity;
}

function void render_entities(Rec2* cam_bounds, WorldPosition* camera_pos,
                              OffscreenBuffer* buffer, GameState* game_state) {
  // First find out the min and max corner so that we can know which chunk_poss
  // are required
  World* world = game_state->world;

  // TODO fix this
  WorldPosition min_chunk =
      map_into_world_position(world, camera_pos, get_min_corner(*cam_bounds));
  WorldPosition max_chunk =
      map_into_world_position(world, camera_pos, get_max_corner(*cam_bounds));

  if (game_state->show_tiles && !game_state->chunk_animation.is_active) {
    glUseProgram(opengl_config.basic_light_program);
    glUniform1i(opengl_config.tile_uniform, true);
    glUseProgram(0);

    WorldChunk* chunk = get_world_chunk(world, game_state->curr_chunk);
    for (S32 x = 0; x < TILE_COUNT_PER_WIDTH; x++) {
      for (S32 y = 0; y < TILE_COUNT_PER_HEIGHT; y++) {
        Tile* tile = &chunk->tiles[x + y * TILE_COUNT_PER_WIDTH];
        if (is_flag_set(tile, tile_path)) {
          opengl_tile(camera_pos->offset, world, tile, chunk);
        }
      }
    }
    glUseProgram(opengl_config.basic_light_program);
    glUniform1i(opengl_config.tile_uniform, false);
    glUseProgram(0);
  }

  for (S32 y = min_chunk.chunk_pos.y; y <= max_chunk.chunk_pos.y; y++) {
    for (S32 x = min_chunk.chunk_pos.x; x <= max_chunk.chunk_pos.x; x++) {
      WorldChunk* chunk = get_world_chunk(world, V2_S32{x, y});
      while (chunk) {
        EntityNode* node = chunk->node;
        bubble_sort_entities(game_state, node);

        while (node) {
          LowEntity* low_entity = game_state->low_entities + node->entity_index;
          V2_F32 entity_cam_space =
              subtract(game_state->world, &low_entity->pos, camera_pos);
          SimEntity* entity = &low_entity->sim;

          if (is_flag_set(entity, entity_flag_dead)) {
            int a = 100;
          }

          if ((is_in_rectangle(*cam_bounds, entity_cam_space)) &&
              !is_flag_set(entity, entity_flag_dead)) {
            switch (entity->type) {
              case entity_type_player:
              case entity_type_wall:
              case entity_type_temple:
              case entity_type_number:
              case entity_type_letter:
              case entity_type_fire_torch:
              case entity_type_grass: {
                F32 mtop = world->meters_to_pixels;
                S32 height_pixels = entity->height * mtop;
                S32 width_pixels = entity->width * mtop;

                F32 center_x = (world->chunk_size_in_meters.x * mtop) * 0.5;
                F32 center_y = (world->chunk_size_in_meters.y * mtop) * 0.5;

                F32 min_x = (center_x + entity->pos.x * mtop) -
                            ((F32)width_pixels * 0.5f);
                F32 min_y = (center_y + entity->pos.y * mtop) -
                            ((F32)height_pixels * 0.5f);

                LoadedBitmap* player_left =
                    get_asset(&platform.asset, asset_player_left);
                LoadedBitmap* player_right =
                    get_asset(&platform.asset, asset_player_right);
                LoadedBitmap* player_back =
                    get_asset(&platform.asset, asset_player_back);

                if (entity->type == entity_type_player) {
                  if (entity->face_direction == 0) {
                    opengl_bitmap(player_left, min_x, min_y, width_pixels,
                                  height_pixels);
                  } else if (entity->face_direction == 1) {
                    opengl_bitmap(player_back, min_x, min_y, width_pixels,
                                  height_pixels);
                  } else {
                    opengl_bitmap(player_right, min_x, min_y, width_pixels,
                                  height_pixels);
                  }

                  glUseProgram(opengl_config.basic_light_program);
                  glUniform2f(opengl_config.light_pos_id, min_x, min_y);
                  glUseProgram(0);

                } else if ((entity->type == entity_type_number) ||
                           (entity->type == entity_type_letter)) {
                  S32 tex_width = entity->texture->width * mtop;
                  S32 tex_height = entity->texture->height * mtop;

                  min_x =
                      (center_x + (entity->pos.x * mtop) - tex_width * 0.5f);
                  min_y =
                      (center_y + (entity->pos.y * mtop) - tex_height * 0.5f);

                  opengl_bitmap(entity->texture, min_x, min_y, tex_width,
                                tex_height);

                  /*
                    if(game_state->player_index > 0){
                    LowEntity* player =
                    &game_state->low_entities[game_state->player_index]; S32 len
                    = get_path_length(player->sim.tile_path);

                    if(len >= 10){

                    S32 i = 1;
                    while(len != 0){
                    S32 a = len % 10;

                    LoadedBitmap* bmp = get_font_number(&game_state->font_asset,
                    a); opengl_bitmap(bmp, min_x - (i-1) * mtop, min_y,
                    width_pixels, height_pixels); len = len /10; i+=1;
                    }

                    }else{
                    LoadedBitmap* bmp= get_font_number(&game_state->font_asset,
                    len); opengl_bitmap(bmp, min_x, min_y,  width_pixels,
                    height_pixels);
                    }
                    }
                  */
                } else {
                  opengl_bitmap(entity->texture, min_x, min_y, width_pixels,
                                height_pixels);
                }
              } break;
              case entity_type_npc: {
              } break;
              case entity_type_null: {
                // Do nothing
              } break;
            }
          }
          node = node->next;
        }

        // Render the banner

        chunk = chunk->next;
      }
      if (!game_state->chunk_animation.is_active) {
        WorldChunk* chunk = get_world_chunk(world, game_state->curr_chunk);
        F32 mtop = world->meters_to_pixels;

        F32 center_x = (world->chunk_size_in_meters.x * mtop) * 0.5;
        F32 center_y = (world->chunk_size_in_meters.y * mtop) * 0.5;

        Banner top = chunk->top_banner;

        LoadedBitmap* banner_tile =
            get_asset(&platform.asset, asset_banner_tile);

        S32 tex_width, tex_height;
        F32 min_x, min_y;

        for (int i = 0; i < array_count(top.banners); ++i) {
          min_x = center_x + ((-2.0f + i) * mtop) - (F32)mtop * .5f;
          min_y = center_y + 4 * mtop - (F32)mtop * .5f;

          opengl_bitmap(banner_tile, min_x, min_y, mtop, mtop);
          BannerTile tile = top.banners[i];
          if (tile.status != banner_status_empty) {
            LowEntity low = game_state->low_entities[tile.low_index];
            LoadedBitmap* bmp;

            if (tile.status == banner_status_letter) {
              bmp = get_font(&platform.font_asset, low.sim.value_s64);
            } else {
              bmp = get_font_number(&platform.font_asset, low.sim.value_s64);
            }

            tex_width = bmp->width * mtop;
            tex_height = bmp->height * mtop;
            min_x += (F32)mtop * .5f - ((F32)tex_width * 0.5f);
            // min_x = (center_x + (-2.0f * mtop)) - ((F32)tex_width*0.5f) ;
            min_y = (center_y + 4 * mtop) - ((F32)tex_height * 0.5f);
            opengl_bitmap(bmp, min_x, min_y, tex_width, tex_height);
          }
        }

        Banner bottom = chunk->bottom_banner;
        for (int i = 0; i < array_count(bottom.banners); ++i) {
          min_x = center_x + ((-2.0f + i) * mtop) - (F32)mtop * .5f;
          min_y = center_y - 4 * mtop - (F32)mtop * .5f;

          opengl_bitmap(banner_tile, min_x, min_y, mtop, mtop);
          BannerTile tile = bottom.banners[i];
          if (tile.status != banner_status_empty) {
            LowEntity low = game_state->low_entities[tile.low_index];
            LoadedBitmap* bmp;

            if (tile.status == banner_status_letter) {
              bmp = get_font(&platform.font_asset, low.sim.value_s64);
            } else {
              bmp = get_font_number(&platform.font_asset, low.sim.value_s64);
            }

            tex_width = bmp->width * mtop;
            tex_height = bmp->height * mtop;
            min_x += (F32)mtop * .5f - ((F32)tex_width * 0.5f);
            // min_x = (center_x + (-2.0f * mtop)) - ((F32)tex_width*0.5f) ;
            min_y = (center_y + 4 * mtop) - ((F32)tex_height * 0.5f);
            opengl_bitmap(bmp, min_x, min_y, tex_width, tex_height);
          }
        }
      }
    }
  }
}


function LoadedBitmap make_empty_bitmap(MemoryArena* arena, S32 width,
                                        S32 height, B32 clear_to_zero = true) {
  LoadedBitmap result = {};

  result.align_percent = V2_F32{0.5f, 0.5f};

  if (width != 0 && height != 0) {
    result.width_over_height = (F32)width / (F32)height;
  } else {
    result.width_over_height = 0;
  }

  result.width = width;
  result.height = height;
  result.pitch = result.width * BITMAP_BYTES_PER_PIXEL;
  S32 total_bitmap_size = width * height * BITMAP_BYTES_PER_PIXEL;
  result.pixels = (U32*)push_size(arena, total_bitmap_size, 16);
  return (result);
}

function void toggle_light() {
  glUseProgram(opengl_config.basic_light_program);
  glUniform1i(opengl_config.use_light, opengl_config.use_light_local);

  if (opengl_config.use_light_local) {
    opengl_config.use_light_local = false;
  } else {
    opengl_config.use_light_local = true;
  }
  glUseProgram(0);
}

/*
  function void add_string_to_bitmap(GameState* game_state, const char* string){
  S32 len = lstrlenA((LPSTR)string);
  game_state->debug_fonts = push_struct(&game_state->world_arena, BitmapArray);

  for (int i = 0; i < len; ++i) {
  char c = string[i];
  push_bitmap(game_state->debug_fonts, test_font);
  game_state->debug_fonts->count++;
  }
  }
*/

function void level_one_init(GameState* game_state) {
  S32 mtop = game_state->world->meters_to_pixels;
  add_entity_number(game_state, V2_S32{0, 0}, V2_F32{0, 0}, 2);
  add_entity_number(game_state, V2_S32{0, 0}, V2_F32{1, 1}, 4);
  add_entity_number(game_state, V2_S32{0, 0}, V2_F32{-5, 3}, 2);
  add_entity_letter(game_state, V2_S32{0, 0}, V2_F32{4, 1}, '%');
  // add_entity_letter(game_state, V2_S32{0,0}, V2_F32{-3,-1}, '+');
  add_entity_letter(game_state, V2_S32{0, 0}, V2_F32{-3, -1}, '=');

  LoadedBitmap* torch = get_asset(&platform.asset, asset_fire_torch);
  add_entity(game_state, V2_S32{0, 0}, V2_F32{-6, 3}, entity_type_fire_torch,
             torch, V4{1, 1, 1, 1}, V2_F32{1, 1}, false);
}

function void level_two_init(GameState* game_state) {
  S32 mtop = game_state->world->meters_to_pixels;
  add_entity_number(game_state, V2_S32{1, 0}, V2_F32{0, 0}, 4);
  add_entity_number(game_state, V2_S32{1, 0}, V2_F32{1, 0}, 8);
  add_entity_number(game_state, V2_S32{1, 0}, V2_F32{-5, 2}, 2);
  add_entity_letter(game_state, V2_S32{1, 0}, V2_F32{4, 1}, '%');
  // add_entity_letter(game_state, V2_S32{0,0}, V2_F32{-3,-1}, '+');
  add_entity_letter(game_state, V2_S32{1, 0}, V2_F32{-3, -1}, '=');

  LoadedBitmap* torch = get_asset(&platform.asset, asset_fire_torch);
  add_entity(game_state, V2_S32{1, 0}, V2_F32{-6, 3}, entity_type_fire_torch,
             torch, V4{1, 1, 1, 1}, V2_F32{1, 1}, false);
}

function void render_game(OffscreenBuffer* buffer,
                          PlatformState* platform_state, GameInput* input) {

  GameState* game_state = (GameState*)platform_state->permanent_storage;

  S64 new_config_time = get_file_time("../config.bmf");

  if (is_file_changed(new_config_time, game_state->tokens.last_write_time)) {
    // TODO: add to platform

    game_state->world = push_struct(&platform.arena, World);

    LoadedBitmap* tex_wall = get_asset(&platform.asset, asset_wall);

    assert(tex_wall->height == tex_wall->width);

    initilize_world(game_state->world, buffer->width, buffer->height);
    World* world = game_state->world;

    game_state->camera_p = create_world_pos(V2_S32{0, 0}, 0, 0);

    // TODO: add to platform

    // TODO: put low_entitites in the world instead of game_state
    for (U32 i = 0; i < array_count(game_state->low_entities); i++) {
      game_state->low_entities[i] = {};
    }

    game_state->low_entity_count = 0;

    add_low_entity(game_state, entity_type_null, {});

    // It is all about how big it is
    V2_F32 dim_in_meters = {};
    dim_in_meters.x = world->chunk_size_in_meters.x;
    dim_in_meters.y = world->chunk_size_in_meters.y;
    game_state->camera_bounds =
        rect_center_half_dim(V2_F32{0, 0}, dim_in_meters);

    Config* tokens = &game_state->tokens;
    *tokens = {};
    tokens->last_write_time = new_config_time;
    parse_commands("../config.bmf", tokens);

    game_play_sound(game_state,  asset_sound_background);

    for (U32 i = 0; i < tokens->count; i++) {
      ConfigToken curr = tokens->tokens[i];
      switch (curr.type) {
        case (command_add_wall): {
          V2_S32 chunk_pos = {};
          S32 assigned =
              sscanf_s(curr.args, "{%d,%d}", &chunk_pos.x, &chunk_pos.y);
          if (assigned == 2) {
            // add_wall(game_state, chunk_pos, V2_F32{ rel_x, rel_y });
          } else {
            assert(0);
          }
        } break;

        case (command_add_walls_around_chunk_pos): {
          V2_S32 chunk_pos = {};
          S32 assigned =
              sscanf_s(curr.args, "{%d,%d}", &chunk_pos.x, &chunk_pos.y);
          if (assigned == 2) {
            add_walls_around_chunk_pos(game_state, chunk_pos);
          } else {
            assert(0);
          }
        } break;

        case (command_add_temple): {
          V2_S32 chunk_pos = {};
          F32 rel_x = 0.0f;
          F32 rel_y = 0.0f;
          S32 assigned = sscanf_s(curr.args, "{%d,%d,%f,%f}", &chunk_pos.x,
                                  &chunk_pos.y, &rel_x, &rel_y);
          LoadedBitmap* temple = get_asset(&platform.asset, asset_temple);
          if (assigned == 4) {
            add_temple(game_state, chunk_pos, V2_F32{rel_x, rel_y}, temple);
          } else {
            assert(0);
          }
        }
      }
    }

    level_one_init(game_state);
    level_two_init(game_state);

    F32 a = 2.0f / (F32)buffer->width;
    F32 b = 2.0f / (F32)buffer->height;
    F32 proj[] = {
        a, 0, 0, 0, 0, b, 0, 0, 0, 0, 1, 0, -1, -1, 0, 1,
    };
    glUseProgram(opengl_config.basic_light_program);
    glUniformMatrix4fv(opengl_config.transform_id, 1, GL_FALSE, &proj[0]);
    glUseProgram(0);
    game_state->initilized= true;
  }

  World* world = game_state->world;
  V2_S32 player_ddp = {};  // acceleration
  V2_F32 camera_ddp = {};  // camera movement

  B32 undo = false;

  WorldChunk* chunk =
      get_world_chunk(game_state->world, game_state->curr_chunk);
  for (S32 i = 0; i < array_count(input->controllers); i += 1) {
    ControllerInput* controller = input->controllers + i;
    if (was_down(Key_l, controller)) toggle_light();
    if (was_down(Key_t, controller)) {
      game_state->show_tiles = !game_state->show_tiles;
    }

    // Camera movement
    if (was_down(action_left, controller)) camera_ddp.x = -1;
    if (was_down(action_right, controller)) camera_ddp.x = 1;
    if (was_down(action_down, controller)) camera_ddp.y = -1;
    if (was_down(action_up, controller)) camera_ddp.y = 1;

    if (was_down(escape, controller)) platform.game_mode = game_mode_menu;

    if (chunk->player_index == 0) {
      // Adding new player
      if (controller->start.ended_down) {
        WorldPosition new_player_pos = {};
        new_player_pos.offset = V2_F32{6.0f, 0.0f};
        LoadedBitmap* player_left =
            get_asset(&platform.asset, asset_player_left);
        AddLowEntityResult res =
            add_entity(game_state, game_state->curr_chunk,
                       new_player_pos.offset, entity_type_player, player_left,
                       V4{1, 1, 1, 1}, V2_F32{1.0f, 1.0f});
        chunk->player_index = res.index;
      }
    } else {
      LowEntity* low_entity = game_state->low_entities + chunk->player_index;
      SimEntity* entity = &low_entity->sim;
      if (controller->is_analog) {
        // player_ddp = V2_S32{ controller->stick_x, controller->stick_y };
      } else {
        if (entity->type == entity_type_player) {
          // Player movement
          if (ended_down(move_up, controller)) {
            player_ddp.y = 1;
          }
          if (ended_down(move_down, controller)) player_ddp.y = -1;
          if (ended_down(move_right, controller)) player_ddp.x = 1;
          if (ended_down(move_left, controller)) player_ddp.x = -1;

          if (ended_down(Key_u, controller)) {
            undo = true;
          }
        }
      }
    }
  }

  update_camera(game_state, camera_ddp);
  // update_animation_chunk(game_state, &game_state->chunk_animation,
  // camera_ddp, &game_state->camera_p);

  // Sim start

  MemoryArena sim_arena;
  TransientState* trans_state = (TransientState*)platform.temporary_storage;
  TempMemory sim_memory = begin_temp_memory(&trans_state->trans_arena);

  SimRegion* sim_region =
      begin_sim(sim_memory.arena, game_state, game_state->world, game_state->camera_p,
                game_state->camera_bounds);

  // Update entities
  SimEntity* entity = sim_region->entities;
  for (S32 i = 0; i < sim_region->entity_count; i += 1, entity++) {
    switch (entity->type) {
      case entity_type_player: {
        if (entity->type == entity_type_player) {
          LowEntity* low = &game_state->low_entities[entity->storage_index];
          if (undo) {
            undo_player(game_state, low, &player_ddp);
          }
          update_animation_player(game_state, player_ddp, entity, low, undo);

          MoveSpec spec = default_move_spec();
          spec.unit_max_accel_vector = 1;
          spec.speed = 18.61f;
          spec.drag = 8.0f;
          // move_entity(sim_region, entity, input->dtForFrame, &spec,
          // player_ddp);
        }
      }
    }
  }

  end_sim(sim_region, game_state);
  end_temp_memory(sim_memory);

  LoadedBitmap* background_png = get_asset(&platform.asset, asset_background);
  opengl_bitmap(background_png, 0, 0, background_png->width, background_png->height);
  render_entities(&game_state->camera_bounds, &game_state->camera_p, buffer, game_state);
}

function void game_get_sound_samples(GameSoundOutputBuffer* sound_buffer) {
  GameState* game_state = (GameState*)platform.permanent_storage;


  TransientState* trans_state = (TransientState*)platform.temporary_storage;

  TempMemory mixer_memory = begin_temp_memory(&trans_state->trans_arena);

  F32* real_channel_0 = push_array(mixer_memory.arena, sound_buffer->sample_count, F32);
  F32* real_channel_1 = push_array(mixer_memory.arena, sound_buffer->sample_count, F32);

  {
    F32* dest_0 = real_channel_0;
    F32* dest_1 = real_channel_1;

    for (int i = 0; i < sound_buffer->sample_count; ++i)
    {
      *dest_0++ = 0.0f;
      *dest_1++ = 0.0f;
    }
  }

  for (PlayingSound **playing_sound_ptr = &game_state->first_playing_sound; *playing_sound_ptr;) {
    PlayingSound* playing_sound = *playing_sound_ptr;

    B32 sound_finished = false;

    LoadedSound* loaded_sound = get_sound_asset(&platform.sound_asset, playing_sound->id);
    if(loaded_sound){
      F32 volume_0 = playing_sound->volume[0]; 
      F32 volume_1 = playing_sound->volume[1]; 
      F32* dest_0 = real_channel_0;
      F32* dest_1 = real_channel_1;

      assert(playing_sound->samples_played>=0);

      U32 samples_to_mix = sound_buffer->sample_count;
      U32 samples_remaining_in_sound = loaded_sound->sample_count - playing_sound->samples_played;

      if(samples_to_mix > samples_remaining_in_sound){
        samples_to_mix = samples_remaining_in_sound;
      }

      for (U32 sample_index = playing_sound->samples_played; sample_index < (playing_sound->samples_played + samples_to_mix); ++sample_index) {
        F32 sample_value = loaded_sound->samples[0][sample_index];
        *dest_0++ += volume_0* sample_value;
        *dest_1++ += volume_1* sample_value;
      }

      sound_finished = ((U32)playing_sound->samples_played == loaded_sound->sample_count);

      playing_sound->samples_played += samples_to_mix;
    }else{
      //Invalid code path
      assert(1 == 0);
    }

    if(sound_finished){
      *playing_sound_ptr = playing_sound->next;
      playing_sound->next = game_state->first_free_playing_sound;
      game_state->first_free_playing_sound = playing_sound;
    }else{
      playing_sound_ptr = &playing_sound->next;
    }
  }

  {
    F32* source_0 = real_channel_0;
    F32* source_1 = real_channel_1;

    S16* sample_out = sound_buffer->samples;

    for (int sample_index  = 0; sample_index < sound_buffer->sample_count; ++sample_index) {
      *sample_out++ = (S16)(*source_0++ + 0.5f);
      *sample_out++ = (S16)(*source_1++ + 0.5f);
    }
  }
  end_temp_memory(mixer_memory);
}









