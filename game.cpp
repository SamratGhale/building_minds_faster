#include <stdio.h>
#include "world.cpp"
#include "png_reader.cpp"
#include "sim_region.cpp"
#include "game.h"

function U32 denormalize_color(F32 R, F32 G, F32 B) {
  U32 Color = ((U8)roundf(R * 255.0f) << 16) | ((U8)roundf(G * 255.0f) << 8) | ((U8)roundf(B * 255.0f) << 0);
  return Color;
}

//TODO: associate PNG(pixel) data to entities, every data should be connected to a entity
global_variable ImageU32 background_png;
global_variable ImageU32 temple_png;
global_variable ImageU32 player_right_png;
global_variable ImageU32 player_left_png;
global_variable ImageU32 tex_wall;
global_variable ImageU32 grass_png;

struct MoveSpec{
  B32 unit_max_accel_vector;
  F32 speed;
  F32 drag;
};

inline MoveSpec default_move_spec(){
  MoveSpec result;
  result.unit_max_accel_vector = 0;
  result.speed = 1.0f;
  result.drag = 0.0f;
  return result;
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

function void move_entity(SimRegion * sim_region, SimEntity* entity, F32 dt, MoveSpec* move_spec, V2_F32 ddP) {
  World* world = sim_region->world;

  if (move_spec->unit_max_accel_vector) {
    F32 ddP_len = length_sq(ddP);
    if (ddP_len > 1.0f) {
      ddP = ddP * (1.0f / sq_root(ddP_len));
    }
  }

  ddP *=  move_spec->speed;
  ddP += -move_spec->drag * entity->dP;

  V2_F32 delta = (0.5f* ddP * square(dt) + entity->dP* dt);
  entity->dP = ddP * dt + entity->dP;

  for (S32 j = 0; j < 4; j += 1) {
    F32 t_min = 1.0f;
    V2_F32 wall_normal = {};
    U32 hit_entity_index = -1;
    V2_F32 desired_pos = entity->pos + delta;

    for (S32 i = 0; i < sim_region->entity_count; i += 1) {
      if (i != entity->storage_index) {
        SimEntity * test_entity = sim_region->entities + i;
        if(test_entity->collides){

          F32 diameter_w = test_entity->width + entity->width;
          F32 diameter_h = test_entity->height + entity->height;
          V2_F32 min_corner = -0.5f*V2_F32{ diameter_w, diameter_h };
          V2_F32 max_corner = 0.5f*V2_F32{ diameter_w, diameter_h };

          V2_F32 rel = entity->pos - test_entity->pos;

          if (test_wall(min_corner.x, rel.x, rel.y, delta.x, delta.y, &t_min, min_corner.y, max_corner.y)) {
            wall_normal = V2_F32{ -1, 0 };
            hit_entity_index = i;
          }
          if (test_wall(max_corner.x, rel.x, rel.y, delta.x, delta.y, &t_min, min_corner.y, max_corner.y)) {
            wall_normal = V2_F32{ 1, 0 };
            hit_entity_index = i;
          }
          if (test_wall(max_corner.y, rel.y, rel.x, delta.y, delta.x, &t_min, min_corner.x, max_corner.x)) {
            wall_normal = V2_F32{ 0, 1 };
            hit_entity_index = i;
            clear_flag(entity, entity_flag_falling);
          }
          if (test_wall(min_corner.y, rel.y, rel.x, delta.y, delta.x, &t_min, min_corner.x, max_corner.x)) {
            wall_normal = V2_F32{ 0, -1 };
            hit_entity_index = i;
          //add_flag(entity, entity_on_ground);
          //clear_flag(entity, entity_flag_falling);
          }
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
}

function void paint_buffer(OffscreenBuffer* buffer, F32 r, F32 g, F32 b) {
  //glViewport(0, 0, buffer->width, buffer->height);
  //glBegin(GL_TRIANGLES);
  glClearColor(r, g, b, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  //glEnd();
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


function AddLowEntityResult add_low_entity(GameState* game_state, EntityType type, WorldPosition p, V4 color = V4{0,0,0,0}) {
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

function AddLowEntityResult add_entity(GameState* game_state, V2_S32 chunk_pos, V2_F32 offset, EntityType type, ImageU32* image, V4 color, V2_F32 size, B32 collides = true){

  S32 mtop = game_state->world->meters_to_pixels;

  WorldPosition pos = {};
  pos.chunk_pos = chunk_pos;
  pos.offset    = offset;
  AddLowEntityResult entity = add_low_entity(game_state, entity_type_wall, pos, color);
  entity.low->sim.width  = size.x;
  entity.low->sim.height = size.y;
  entity.low->sim.color  = color;
  entity.low->sim.type   = type;
  entity.low->sim.face_direction = -1;
  entity.low->sim.texture = image;
  entity.low->sim.collides = collides;
  return entity;

}

function void add_walls_around_chunk_pos(GameState* game_state, V2_S32 chunk_pos, V2_F32 door_pos){

  S32 mtop = game_state->world->meters_to_pixels ;

  F32 height = ((F32)tex_wall.height / (F32)mtop);
  F32 width  = ((F32)tex_wall.width  / (F32)mtop);
  V4 color   = V4{1,1 ,1 ,1};

  for(F32 x = -8; x <= 8; x++){
    if(!(x == door_pos.x && door_pos.y == -4)){
      add_entity(game_state, chunk_pos, V2_F32{x, -4}, entity_type_wall, &tex_wall, color, V2_F32{height, width});
    }
    if(!(x == door_pos.x && door_pos.y == 4)){
      add_entity(game_state, chunk_pos, V2_F32{x, 4}, entity_type_wall, &tex_wall, color, V2_F32{height, width});
    }
  }
  for(F32 y = -4; y <= 4; y++){
    if(!(y == door_pos.y && door_pos.x == -8)){
      add_entity(game_state, chunk_pos, V2_F32{-8, y}, entity_type_wall, &tex_wall, color, V2_F32{height, width});
    }
    if(!(y == door_pos.y && door_pos.x == 8)){
      add_entity(game_state, chunk_pos, V2_F32{8, y}, entity_type_wall, &tex_wall, color, V2_F32{height, width});
    }
  }
  for(F32 x = -7; x <= 7; x++){
    for(F32 y = -3; y <= 3; y++){
      add_entity(game_state, chunk_pos, V2_F32{x, y}, entity_type_grass, &grass_png, color, V2_F32{height, width}, false);
    }
  }
}

//Just a thing
//Height and width will be image height and width
function AddLowEntityResult add_temple(GameState* game_state, V2_S32 chunk_pos, V2_F32 pos, ImageU32 * image){

  S32 mtop = game_state->world->meters_to_pixels;

  WorldPosition temple_pos = {};
  temple_pos.chunk_pos.y = chunk_pos.y;
  temple_pos.chunk_pos.x = chunk_pos.x;
  temple_pos.offset = pos;
  //U32 color = denormalize_color(0.0f, 0.4f, 0.3f);
  AddLowEntityResult entity = add_low_entity(game_state, entity_type_temple, temple_pos, V4{1.0f, 0.0f, 0.0f, 1.0f});

  entity.low->sim.height = ((F32)image->height/(F32)mtop);
  entity.low->sim.width  = ((F32)image->width/(F32)mtop);
  entity.low->sim.color  = V4{0.7f,0.4f,.9f,1};
  entity.low->sim.face_direction = -1;
  entity.low->sim.texture = image;
  return entity;
}

function void render_entities(Rec2* cam_bounds, WorldPosition* camera_pos, OffscreenBuffer* buffer, GameState* game_state) {
  OutputDebugStringA("Apple banana");

  //First find out the min and max corner so that we can know which chunk_poss are required
  World* world = game_state->world;

  //TODO fix this
  WorldPosition min_chunk_pos = map_into_world_position(game_state->world, camera_pos, get_min_corner(*cam_bounds));
  WorldPosition max_chunk_pos = map_into_world_position(game_state->world, camera_pos, get_max_corner(*cam_bounds));
  //glUseProgram(opengl_config.basic_light_program);

  for (S32 y = min_chunk_pos.chunk_pos.y; y <= max_chunk_pos.chunk_pos.y; y++) {
    for (S32 x = min_chunk_pos.chunk_pos.x; x <= max_chunk_pos.chunk_pos.x; x++) {
      WorldChunk* chunk_pos = get_world_chunk(world, V2_S32{x, y});
      while (chunk_pos) {
        EntityNode* node = chunk_pos->node;
        while (node) {
          LowEntity* low_entity = game_state->low_entities + node->entity_index;
          V2_F32 entity_cam_space = subtract(game_state->world, &low_entity->pos, camera_pos);
          if (is_in_rectangle(*cam_bounds, entity_cam_space)) {
            SimEntity *entity = &low_entity->sim;
            switch (entity->type) {
              case entity_type_player:
              case entity_type_wall:
              case entity_type_temple:
              case entity_type_grass:{
                F32 mtop = world->meters_to_pixels;

                S32 height_pixels = entity->height * mtop;
                S32 width_pixels  = entity->width  * mtop;

                F32 center_x = (world->chunk_size_in_meters.x * mtop) * 0.5;
                F32 center_y = (world->chunk_size_in_meters.y * mtop) * 0.5 ;

                F32 min_x = (center_x + entity->pos.x * mtop) - ((F32)width_pixels  *0.5f) ;

                F32 min_y = (center_y + entity->pos.y * mtop) - ((F32)height_pixels *0.5f);
                F32 max_x = min_x + (F32)width_pixels;
                F32 max_y = min_y + (F32)height_pixels;

                V2_F32 max_p = V2_F32{max_x, max_y};
                V2_F32 min_p = V2_F32{min_x, min_y};

                if(entity->type == entity_type_player){
                  if(entity->face_direction == 0){
                    opengl_bitmap( &player_left_png,min_x, min_y, width_pixels, height_pixels);
                  }else{
                    opengl_bitmap(&player_right_png, min_x, min_y,  width_pixels, height_pixels);
                  }

                }else{
                  opengl_bitmap(entity->texture, min_x, min_y,  width_pixels, height_pixels);
                }
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
        chunk_pos = chunk_pos->next;
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

    tex_wall = parse_png("../data/tex_wall_smool.png");
    grass_png = parse_png("../data/torch.png");

    assert(tex_wall.height == tex_wall.width);

    initilize_world(game_state->world, buffer->width, buffer->height);
    World *world = game_state->world;

    game_state->camera_p = create_world_pos(V2_S32{0, 0}, 0, 0);
    background_png = parse_png("../data/green_background.png");
    temple_png = parse_png("../data/temple.png");
    player_right_png = parse_png("../data/character_smoll.png");
    player_left_png = parse_png("../data/character_smoll_left.png");

    //TODO: put low_entitites in the world instead of game_state
    for (U32 i = 0; i < array_count(game_state->low_entities); i++) {
      game_state->low_entities[i] = {};
    }

    for(U32 i = 0; i < array_count(game_state->controlled_entity_index); i++){
      game_state->controlled_entity_index[i] = 0;
    }
    game_state->player_index     = 0;
    game_state->low_entity_count = 0;

    add_low_entity(game_state, entity_type_null, {});

    //It is all about how big it is
    V2_F32 dim_in_meters = {};
    dim_in_meters.x = (F32)buffer->width*(1.0f / (F32)world->meters_to_pixels);
    dim_in_meters.y = (F32)buffer->height*(1.0f / (F32)world->meters_to_pixels);
    game_state->camera_bounds = rect_center_half_dim(V2_F32{ 0,0 }, dim_in_meters);

    Config * tokens = &game_state->tokens;
    *tokens = {};
    tokens->last_write_time = new_config_time;
    parse_commands("../config.bmf", tokens);

    for (U32 i = 0; i < tokens->count; i++) {
      ConfigToken curr = tokens->tokens[i];
      switch (curr.type) {

        case(command_add_wall): {
          V2_S32 chunk_pos  = {};
          F32 rel_x = 0.0f;
          F32 rel_y = 0.0f;
          S32 assigned = sscanf_s(curr.args, "{%d,%d,%f,%f}", &chunk_pos.x, &chunk_pos.y, &rel_x, &rel_y);
          if (assigned == 4) {
            //add_wall(game_state, chunk_pos, V2_F32{ rel_x, rel_y });
          } else {
            assert(0);
          }
        }break;

        case(command_add_walls_around_chunk_pos): {
          V2_S32 chunk_pos = {};
          F32 rel_x = 0.0f;
          F32 rel_y = 0.0f;
          S32 assigned = sscanf_s(curr.args, "{%d,%d,%f,%f}", &chunk_pos.x, &chunk_pos.y, &rel_x, &rel_y);
          if (assigned == 4) {
            add_walls_around_chunk_pos(game_state, chunk_pos, V2_F32{ rel_x, rel_y });
          } else {
            assert(0);
          }
        }break;

        case(command_add_temple):{
          V2_S32 chunk_pos = {};
          F32 rel_x = 0.0f;
          F32 rel_y = 0.0f;
          S32 assigned = sscanf_s(curr.args, "{%d,%d,%f,%f}", &chunk_pos.x, &chunk_pos.y, &rel_x, &rel_y);
          if (assigned == 4) {
            add_temple(game_state, chunk_pos, V2_F32{ rel_x, rel_y },&temple_png);
          } else {
            assert(0);
          }
        }
      }
    }

    V4 color = {0,1,1,0};
    GLuint obj_color = glGetUniformLocation(opengl_config.basic_light_program, "obj_color"); 
    GLuint sampler = glGetUniformLocation(opengl_config.basic_light_program, "sampler");
    GLuint mat = glGetUniformLocation(opengl_config.basic_light_program, "mat");
    glUniform4f(obj_color, color.r, color.g, color.b, color.a);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    opengl_config.light_pos_id = glGetUniformLocation(opengl_config.basic_light_program, "light_pos");

    //Bind stuffs
    F32 a = 2.0f/(F32)buffer->width;
    F32 b = 2.0f/(F32)buffer->height;

    F32 proj[]={
      a, 0, 0, 0,
      0, b, 0, 0,
      0, 0, 1, 0,
      -1,-1, 0, 1,
    };
    F32 mtop = world->meters_to_pixels;
    F32 center_x = (world->chunk_size_in_meters.x * mtop) *0.5f;
    F32 center_y = (world->chunk_size_in_meters.y * mtop) * 0.5f;
    glUseProgram(opengl_config.basic_light_program);
    glUniformMatrix4fv(opengl_config.transform_id, 1, GL_FALSE, &proj[0]);
    glUniform2f(opengl_config.light_pos_id, center_x, center_y);
    glUseProgram(0);
  }

  World* world = game_state->world;

  V2_F32 player_ddp = {}; //acceleration
  V2_F32 camera_ddp = {};
  for (S32 i = 0; i < array_count(input->controllers); i += 1) {
    S32 controlled_entity_index = game_state->controlled_entity_index[i];
    ControllerInput* controller = input->controllers + i;

    if (was_down(action_left, controller))  camera_ddp.x = -1;
    if (was_down(action_right, controller)) camera_ddp.x = 1;
    if (was_down(action_down, controller)) {
      camera_ddp.y = -1;

        //TODO: implement mouse here
        #if 0
        world->meters_to_pixels -= 3;
        S32 mtop = world->meters_to_pixels;
        V2_F32 dim_in_meters = {};
        dim_in_meters.x = (F32)buffer->width*(1.0f / (F32)mtop);
        dim_in_meters.y = (F32)buffer->height*(1.0f / (F32)mtop);
        //game_state->camera_bounds = rect_center_half_dim(V2_F32{ 0,0 }, dim_in_meters);
        #endif

    } 
    if (was_down(action_up, controller)) {
      camera_ddp.y = 1;
        #if 0
        world->meters_to_pixels += 3;
        V2_F32 dim_in_meters = {};
        dim_in_meters.x = (F32)buffer->width*(1.0f / (F32)world->meters_to_pixels);
        dim_in_meters.y = (F32)buffer->height*(1.0f / (F32)world->meters_to_pixels);
        //game_state->camera_bounds = rect_center_half_dim(V2_F32{ 0,0 }, dim_in_meters);
        #endif
    } 

    if (controlled_entity_index == 0) {
      if (controller->start.ended_down) {
        WorldPosition new_player_pos = {};
        new_player_pos.offset = V2_F32{ 0.0f, 0.0f };
        //AddLowEntityResult res = add_entity(game_state, new_player_pos);
        AddLowEntityResult res = add_entity(game_state, new_player_pos.chunk_pos, new_player_pos.offset, entity_type_player, &player_left_png, V4{1,1,1,1}, V2_F32{1.0f,1.0f});
        game_state->controlled_entity_index[i] = res.index;
        game_state->player_index = res.index;
      }
    }
    else {
      LowEntity* low_entity = game_state->low_entities + controlled_entity_index;
      SimEntity* entity = &low_entity->sim;
      if (controller->is_analog) {
        player_ddp = V2_F32{ controller->stick_x, controller->stick_y };
      }
      else {
        //TODO:if the entity is in a ladder then move_up should be activated
        if (entity->type == entity_type_player) {
          if (ended_down(move_up, controller))  player_ddp.y = 1.0f;
          if (ended_down(move_down, controller))  player_ddp.y = -1.0f;
          if (ended_down(move_right, controller))  player_ddp.x = 1.0f;
          if (ended_down(move_left, controller))  player_ddp.x = -1.0f;
        }

#if JUMPING_ENABLED
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
#endif
      }
    }
  }

  //Smooth chunk_pos start
  if(camera_ddp.x != 0 || camera_ddp.y != 0){
    if(!game_state->chunk_animation.is_active){
      game_state->chunk_animation.is_active = 1;
      game_state->chunk_animation.source = game_state->camera_p;
      game_state->chunk_animation.dest   = game_state->camera_p;
      game_state->chunk_animation.dest.chunk_pos.x += camera_ddp.x;
      game_state->chunk_animation.dest.chunk_pos.y += camera_ddp.y;
      game_state->chunk_animation.completed = 0;
    }
  }
  if(game_state->chunk_animation.is_active){
    ChunkAnimation* anim = &game_state->chunk_animation;
    if(anim->completed >= 100){
      anim->is_active = false;
    }else{
      V2_F32 chunk_diff = subtract(game_state->world, &anim->dest, &anim->source);
      //chunk_pos_diff = chunk_pos_diff * game_state->world->meters_to_pixels;
      F32 add_x = chunk_diff.x / 20.0f; 
      F32 add_y = chunk_diff.y / 20.0f; 

      game_state->camera_p = map_into_world_position(game_state->world, &game_state->camera_p, V2_F32{add_x, add_y});
      anim->completed += 5;
    }
  }
  //Smooth chunk_pos end

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
          spec.drag = 4.0f;
          move_entity(sim_region, entity, input->dtForFrame, &spec, player_ddp);
        }
      }
    }
  }
  end_sim(sim_region, game_state);
  opengl_bitmap(&background_png,game_state->camera_p.offset.x, game_state->camera_p.offset.y,   background_png.width, background_png.height);
  render_entities(&game_state->camera_bounds, &game_state->camera_p, buffer, game_state);
}
