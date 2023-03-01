#include "world.cpp"
#include <stdio.h>
#include "png_reader.cpp"
#include "sim_region.cpp"
#include "game.h"

function U32 denormalize_color(F32 R, F32 G, F32 B) {
  U32 Color = ((U8)roundf(R * 255.0f) << 16) | ((U8)roundf(G * 255.0f) << 8) | ((U8)roundf(B * 255.0f) << 0);
  return Color;
}

//TODO: associate PNG(pixel) data to entities, every data should be connected to a entity
global_variable ImageU32 background_png;
global_variable ImageU32 player_right_png;
global_variable ImageU32 player_left_png;

struct MoveSpec {
  B32 unit_max_accel_vector;
  F32 speed;
  F32 drag;
};

inline MoveSpec default_move_spec() {
  MoveSpec result;
  result.unit_max_accel_vector = 0;
  result.speed = 1.0f;
  result.drag = 0.0f;
  return result;
}

function void draw_bitmap(OffscreenBuffer *buffer, ImageU32 *bitmap, F32 RealX, F32 RealY, F32 CAlpha = 1.0f) {
  S32 MinX = (S32)roundf(RealX);
  S32 MinY = (S32)roundf(RealY);
  S32 MaxX = MinX + bitmap->width;
  S32 MaxY = MinY + bitmap->height;

  S32 SourceOffsetX = 0;

  if (MinX < 0) {
    SourceOffsetX = -MinX;
    MinX = 0;
  }

  S32 SourceOffsetY = 0;
  if (MinY < 0) {
    SourceOffsetY = -MinY;
    MinY = 0;
  }

  if (MaxX > buffer->width) {
    MaxX = buffer->width;
  }

  if (MaxY > buffer->height) {
    MaxY = buffer->height;
  }

  // TODO(casey): SourceRow needs to be changed based on clipping.
  U32 *SourceRow = bitmap->pixels + bitmap->width*(bitmap->height - 1);
  SourceRow += -SourceOffsetY * bitmap->width + SourceOffsetX;

  U8 *DestRow = ((U8*)buffer->memory + MinX * buffer->bytes_per_pixel + MinY * buffer->pitch);

  for (int Y = MinY; Y < MaxY; ++Y) {

    U32 *Dest = (U32 *)DestRow;
    U32 *Source = SourceRow;

    for (int X = MinX; X < MaxX; ++X) {
      F32 A = (F32)((*Source >> 24) & 0xFF) / 255.0f;
      A *= CAlpha;
      F32 SR = (F32)((*Source >> 16) & 0xFF);
      F32 SG = (F32)((*Source >> 8) & 0xFF);
      F32 SB = (F32)((*Source >> 0) & 0xFF);
      F32 DR = (F32)((*Dest >> 16) & 0xFF);
      F32 DG = (F32)((*Dest >> 8) & 0xFF);
      F32 DB = (F32)((*Dest >> 0) & 0xFF);

      // TODO(casey): Someday, we need to talk about premultiplied alpha!
      // (this is not premultiplied alpha)
      F32 R = (1.0f - A)*DR + A * SR;
      F32 G = (1.0f - A)*DG + A * SG;
      F32 B = (1.0f - A)*DB + A * SB;

      *Dest = (((U32)(R + 0.5f) << 16) | ((U32)(G + 0.5f) << 8) | ((U32)(B + 0.5f) << 0));
      ++Dest;
      ++Source;
    }
    DestRow += buffer->pitch;
    SourceRow -= bitmap->width;
  }
}



#define MAX(a,b)(((a)>(b))?(a):(b))
function B32 test_wall(F32 wall_x, F32 rel_x, F32 rel_y, F32 p_delta_x, F32 p_delta_y, F32 *t_min, F32 min_y, F32 max_y) {
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

function void move_entity(SimRegion * sim_region, SimEntity* entity, F32 dt, MoveSpec* move_spec, V2 ddP) {
  World* world = sim_region->world;

  if (move_spec->unit_max_accel_vector) {
    F32 ddP_len = length_sq(ddP);
    if (ddP_len > 1.0f) {
      ddP = ddP * (1.0f / sq_root(ddP_len));
    }
  }

  ddP *= move_spec->speed;
  ddP += -move_spec->drag*entity->dP;
  V2 delta = (0.5f* ddP * square(dt) + entity->dP* dt);
  entity->dP = ddP * dt + entity->dP;

  for (S32 j = 0; j < 4; j += 1) {
    F32 t_min = 1.0f;
    V2 wall_normal = {};
    U32 hit_entity_index = -1;
    V2 desired_pos = entity->pos + delta;

    for (S32 i = 0; i < sim_region->entity_count; i += 1) {
      if (i != entity->storage_index) {
       SimEntity * test_entity = sim_region->entities + i;
       F32 diameter_w = test_entity->width + entity->width;
       F32 diameter_h = test_entity->height + entity->height;
       V2 min_corner = -0.5f*V2{ diameter_w, diameter_h };
       V2 max_corner = 0.5f*V2{ diameter_w, diameter_h };

       V2 rel = entity->pos - test_entity->pos;

       if (test_wall(min_corner.x, rel.x, rel.y, delta.x, delta.y, &t_min, min_corner.y, max_corner.y)) {
         wall_normal = V2{ -1, 0 };
         hit_entity_index = i;
       }
       if (test_wall(max_corner.x, rel.x, rel.y, delta.x, delta.y, &t_min, min_corner.y, max_corner.y)) {
         wall_normal = V2{ 1, 0 };
         hit_entity_index = i;
       }
       if (test_wall(max_corner.y, rel.y, rel.x, delta.y, delta.x, &t_min, min_corner.x, max_corner.x)) {
         wall_normal = V2{ 0, 1 };
         hit_entity_index = i;
         clear_flag(entity, entity_flag_falling);
       }
       if (test_wall(min_corner.y, rel.y, rel.x, delta.y, delta.x, &t_min, min_corner.x, max_corner.x)) {
         wall_normal = V2{ 0, -1 };
         hit_entity_index = i;
	  //add_flag(entity, entity_on_ground);
	  //clear_flag(entity, entity_flag_falling);
       }
     }
   }

   entity->pos += t_min * delta;
   if (hit_entity_index != -1) {
    entity->dP = entity->dP - 1 * inner(entity->dP, wall_normal)* wall_normal;
    delta = desired_pos - entity->pos;
    delta = delta - 1 * inner(delta, wall_normal)* wall_normal;
  }
  else {
    clear_flag(entity, entity_on_ground);
    if (!is_flag_set(entity, entity_flag_jumping)) {
     add_flag(entity, entity_flag_falling);
   }
   break;
 }
}
if((entity->dP.x == 0.0f) && (entity->dP.x == 0.0f)) {
}
else if(ABS(entity->dP.x) > ABS(entity->dP.y)) {
  if(entity->dP.x > 0) {
	  entity->face_direction = 0;//right 
  }
  else {
	  entity->face_direction = 2;//left
  }
}
    /**
    else
    {
        if(Entity->dP.Y > 0)
        {
            Entity->FacingDirection = 1;
        }
        else
        {
            Entity->FacingDirection = 3;
        }
    }
    **/
}

function void draw_rectangle(OffscreenBuffer* buffer, S32 min_x, S32 min_y, S32 max_x, S32 max_y, U32 color) {
  if (min_x < 0) min_x = 0;
  if (min_y < 0) min_y = 0;
  if (max_x < 0) max_x = 0;
  if (max_y < 0) max_y = 0;
  if (max_x > buffer->width)  max_x = buffer->width;
  if (max_y > buffer->height) max_y = buffer->height;
  if (min_x > buffer->width)  min_x = buffer->width;
  if (min_y > buffer->height) min_y = buffer->height;

  //pitch has bytePerPixel
  U8* row = (U8*)buffer->memory + min_y * buffer->pitch + min_x * buffer->bytes_per_pixel;
  for (S32 Y = min_y; Y < max_y; Y++) {
    U32* pixels = (U32*)row;
    for (S32 X = min_x; X < max_x; X++)  *pixels++ = color;
      row += buffer->pitch;
  }
}

function void paint_buffer(OffscreenBuffer* buffer, F32 r, F32 g, F32 b) {
  U8* row = (U8*)buffer->memory;
  U32 color = denormalize_color(r, g, b);
  for (U32 y = 0; y < buffer->height; y++) {
    for (U32 x = 0; x < buffer->width; x++) {
      U32* pixel = (U32*)row + x;
      *pixel++ = color;
    }
    row += buffer->pitch;
  }
}

struct AddLowEntityResult {
  LowEntity* low;
  U32 index;
};

inline WorldPosition null_pos() {
  WorldPosition result = {};
  result.chunk_x = TILE_CHUNK_UNINITILIZED;
  return result;
}


function AddLowEntityResult add_low_entity(GameState* game_state, EntityType type, WorldPosition p, U32 color) {
  U32 entity_index = game_state->low_entity_count++;
  LowEntity* low_entity = game_state->low_entities + entity_index;
  *low_entity = {};
  low_entity->pos = null_pos();
  change_entity_location(&game_state->world_arena, game_state->world, entity_index, low_entity, p);
  //low_entity->pos = p;
  low_entity->sim.color = color;
  low_entity->sim.storage_index = entity_index;
  low_entity->sim.type = type;
  AddLowEntityResult result = {};
  result.low = low_entity;
  result.index = entity_index;
  return result;
}

function AddLowEntityResult add_player(GameState* game_state, WorldPosition pos) {
  U32 color = denormalize_color(0.4f, 0.2f, 0.2f);
  AddLowEntityResult entity = add_low_entity(game_state, entity_type_player, pos, color);
  entity.low->sim.height = 6.5f;
  entity.low->sim.width = 5.0f;
  return entity;
}

function AddLowEntityResult add_wall(GameState* game_state, S32 chunk_x, S32 chunk_y, V2 pos, F32 width, F32 height) {
  WorldPosition wall_pos = {};
  wall_pos.chunk_y = chunk_y;
  wall_pos.chunk_x = chunk_x;
  wall_pos.offset = pos;
  U32 color = denormalize_color(0.0f, 0.4f, 0.3f);
  AddLowEntityResult entity = add_low_entity(game_state, entity_type_wall, wall_pos, color);
  entity.low->sim.height = height;
  entity.low->sim.width = width;
  return entity;
}

function void render_entities(Rec2* cam_bounds, WorldPosition* camera_pos, OffscreenBuffer* buffer, GameState* game_state) {

  //First find out the min and max corner so that we can know which chunks are required
  World* world = game_state->world;

  //TODO fix this
  WorldPosition min_chunk = map_into_world_position(game_state->world, camera_pos, get_min_corner(*cam_bounds));
  WorldPosition max_chunk = map_into_world_position(game_state->world, camera_pos, get_max_corner(*cam_bounds));

  for (S32 y = min_chunk.chunk_y; y <= max_chunk.chunk_y; y++) {
    for (S32 x = min_chunk.chunk_x; x <= max_chunk.chunk_x; x++) {
      WorldChunk* chunk = get_world_chunk(world, x, y);
      while (chunk) {
       EntityNode* node = chunk->node;
       while (node) {
         LowEntity* low_entity = game_state->low_entities + node->entity_index;
         V2 entity_cam_space = subtract(game_state->world, &low_entity->pos, camera_pos);
         if (is_in_rectangle(*cam_bounds, entity_cam_space)) {
           SimEntity *entity = &low_entity->sim;
           switch (entity->type) {
            case entity_type_player:{
              F32 mtop = world->meters_to_pixels;
              F32 center_x = (world->chunk_size_in_pixels*0.5f) + (entity->pos.x * mtop);
              F32 center_y = (world->chunk_size_in_pixels*0.5f) - (entity->pos.y * mtop);
              S32 min_x = center_x - (F32)((entity->width  *  mtop) *0.5f);
              S32 min_y = center_y - (F32)((entity->height *  mtop) *0.5f)-1;
              S32 max_x = center_x + (F32)((entity->width  *  mtop) *0.5f);
              S32 max_y = center_y + (F32)((entity->height *  mtop) *0.5f);
	     //draw_rectangle(buffer, min_x, min_y, max_x, max_y, entity->color);
              if(entity->face_direction == 0){
                draw_bitmap(buffer, &player_right_png, min_x+3, min_y);
              }else if(entity->face_direction == 2){
                draw_bitmap(buffer, &player_left_png, min_x+3, min_y);
              }

            }break;
            case entity_type_wall: {
              F32 mtop = world->meters_to_pixels;
              F32 center_x = (world->chunk_size_in_pixels*0.5f) + (entity->pos.x * mtop);
              F32 center_y = (world->chunk_size_in_pixels*0.5f) - (entity->pos.y * mtop);
              S32 min_x = center_x - (F32)((entity->width  *  mtop) *0.5f);
              S32 min_y = center_y - (F32)((entity->height *  mtop) *0.5f)-1;
              S32 max_x = center_x + (F32)((entity->width  *  mtop) *0.5f);
              S32 max_y = center_y + (F32)((entity->height *  mtop) *0.5f);
              draw_rectangle(buffer, min_x, min_y, max_x, max_y, entity->color);
            }break;
            case entity_type_npc: {
            }break;
            case entity_type_null: {
	      //Do nothing
            }break;
          }
        }
        node = node->next;
      }
      chunk = chunk->next;
    }
  }
}
}

#define ended_down(b, C)((C->b.ended_down)? 1 :0)
#define was_down(b, C)((!C->b.ended_down && C->b.half_transition_count)?1:0)

function void render_game(OffscreenBuffer* buffer, PlatformState* platform_state, GameInput* input) {

  GameState* game_state = (GameState*)platform_state->permanent_storage;

  S64 new_config_time = get_file_time("../config.bmf");

  if (is_file_changed(new_config_time, game_state->tokens.last_write_time)) {
    initilize_arena(&game_state->world_arena, platform_state->permanent_storage_size, (U8*)platform_state->permanent_storage + sizeof(GameState));
    game_state->world = push_struct(&game_state->world_arena, World);
    initilize_world(game_state->world, buffer->height, 40.0f); //this might cause null error?
    World *world = game_state->world;

    game_state->camera_p = create_world_pos(0, 0, 0, 0);
    background_png = parse_png("../data/background_biggie.png");

    player_right_png = parse_png("../data/character_smoll.png");
    player_left_png = parse_png("../data/character_smoll_left.png");

    //TODO: put low_entitites in the world instead of game_state
    for (U32 i = 0; i < array_count(game_state->low_entities); i++) {
      game_state->low_entities[i] = {};
    }

    for(U32 i = 0; i < array_count(game_state->controlled_entity_index); i++){
      game_state->controlled_entity_index[i] = 0;
    }
    game_state->player_index = 0;
    game_state->low_entity_count = 0;

    add_low_entity(game_state, entity_type_null, {}, 0);

    //It is all about how big it is
    V2 dim_in_meters = {};
    dim_in_meters.x = (F32)buffer->width*(1.0f / (F32)world->meters_to_pixels);
    dim_in_meters.y = (F32)buffer->height*(1.0f / (F32)world->meters_to_pixels);
    game_state->camera_bounds = rect_center_half_dim(V2{ 0,0 }, dim_in_meters);

    Config * tokens = &game_state->tokens;
    *tokens = {};
    tokens->last_write_time = new_config_time;
    parse_commands("../config.bmf", tokens);
    for (U32 i = 0; i < tokens->count; i++) {
      ConfigToken curr = tokens->tokens[i];
      switch (curr.type) {
        case(command_add_wall): {
          S32 chunk_x = 0;
          S32 chunk_y = 0;
          F32 rel_x = 0.0f;
          F32 rel_y = 0.0f;
          S32 assigned = sscanf_s(curr.args, "{%d,%d,%f,%f}", &chunk_x, &chunk_y, &rel_x, &rel_y);
          if (assigned == 4) {
            add_wall(game_state, chunk_x, chunk_y, V2{ rel_x, rel_y }, 5.0f, 2.0f);
          }
          else {
            assert(0);
          }
        }break;
      }
    }
  }

  //Background color
  paint_buffer(buffer, 0.4f, 0.9f, 0.3f);
  //draw_bitmap(buffer, &background_png, 0, 0);
  World* world = game_state->world;

  V2 player_ddp = {}; //acceleration
  for (S32 i = 0; i < array_count(input->controllers); i += 1) {
    S32 controlled_entity_index = game_state->controlled_entity_index[i];
    ControllerInput* controller = input->controllers + i;

    if (controlled_entity_index == 0) {
      if (controller->start.ended_down) {
        WorldPosition new_player_pos = {};
        new_player_pos.offset = V2{ 0.0f, 0.0f };
        AddLowEntityResult res = add_player(game_state, new_player_pos);
        game_state->controlled_entity_index[i] = res.index;
        game_state->player_index = res.index;
      }
    }
    else {
      LowEntity* low_entity = game_state->low_entities + controlled_entity_index;
      SimEntity* entity = &low_entity->sim;
      if (controller->is_analog) {
        player_ddp = V2{ controller->stick_x, controller->stick_y };
      }
      else {
        //TODO:if the entity is in a ladder then move_up should be activated
        if (entity->type == entity_type_player) {
          //if (ended_down(move_up, controller))  player_ddp.y = 1.0f;
          //if (ended_down(move_down, controller))  player_ddp.y = -1.0f;
          if (ended_down(move_right, controller))  player_ddp.x = 1.0f;
          if (ended_down(move_left, controller))  player_ddp.x = -1.0f;
        }

        if (ended_down(start, controller)) {
          if (entity->jump_time >= 1.0f) {
            add_flag(entity, entity_flag_falling);
            clear_flag(entity, entity_flag_jumping);
            entity->jump_time = 0;
          }
          else {
            entity->jump_time += input->dtForFrame;
            player_ddp.y = 1.0f;
            if (!is_flag_set(entity, entity_flag_jumping)) {
              add_flag(entity, entity_flag_jumping);
            }
          }
        }
        else {
          if (is_flag_set(entity, entity_flag_jumping)) {
            clear_flag(entity, entity_flag_jumping);
            add_flag(entity, entity_flag_falling);
            entity->jump_time = 0;
          }
        }

        if (is_flag_set(entity, entity_flag_falling)) {
          entity->jump_time = 0;
          if (!is_flag_set(entity, entity_on_ground)) {
            player_ddp.y = -1.0f;
          }
          else {
            clear_flag(entity, entity_flag_falling);
          }
        }
      }
    }
  }

  MemoryArena sim_arena;
  initilize_arena(&sim_arena, platform_state->temporary_storage_size, platform_state->temporary_storage);
  SimRegion* sim_region = begin_sim(&sim_arena, game_state, game_state->world, game_state->camera_p, game_state->camera_bounds);

  //Update entities
  SimEntity* entity = sim_region->entities;
  for (S32 i = 0; i < sim_region->entity_count; i += 1, entity++) {
    switch (entity->type) {
      case entity_type_player: {
        if (entity->type == entity_type_player) {
         MoveSpec spec = default_move_spec();
         spec.unit_max_accel_vector = 1;
         spec.speed = 28.61f;
         spec.drag = 2.0f;
         move_entity(sim_region, entity, input->dtForFrame, &spec, player_ddp);
       }
     }
   }
 }
 end_sim(sim_region, game_state);

 if(game_state->player_index){
   update_camera(game_state);
 }
 render_entities(&game_state->camera_bounds, &game_state->camera_p, buffer, game_state);
}


