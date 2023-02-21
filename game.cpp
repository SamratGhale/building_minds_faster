#include "world.cpp"
#include "png_reader.cpp"
#include "game.h"

function U32 denormalize_color(F32 R, F32 G, F32 B) {
  U32 Color = ((U8)roundf(R * 255.0f) << 16) | ((U8)roundf(G * 255.0f) << 8) | ((U8)roundf(B * 255.0f) << 0);
  return Color;
}

//TODO: associate PNG(pixel) data to entities, every data should be connected to a entity
global_variable ImageU32 background_png;

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

function void draw_bitmap(OffscreenBuffer *buffer, ImageU32 *bitmap, F32 RealX, F32 RealY, F32 CAlpha = 1.0f) {
  S32 MinX = (S32)roundf(RealX);
  S32 MinY = (S32)roundf(RealY);
  S32 MaxX = MinX + bitmap->width;
  S32 MaxY = MinY + bitmap->height;

  S32 SourceOffsetX = 0;

  if(MinX < 0) {
    SourceOffsetX = -MinX;
    MinX = 0;
  }

  S32 SourceOffsetY = 0;
  if(MinY < 0) {
    SourceOffsetY = -MinY;
    MinY = 0;
  }

  if(MaxX > buffer->width) {
    MaxX = buffer->width;
  }

  if(MaxY > buffer->height) {
    MaxY = buffer->height;
  }

  // TODO(casey): SourceRow needs to be changed based on clipping.
  U32 *SourceRow = bitmap->pixels + bitmap->width*(bitmap->height - 1);
  SourceRow += -SourceOffsetY*bitmap->width + SourceOffsetX;

  U8 *DestRow = ((U8*)buffer->memory + MinX*buffer->bytes_per_pixel+ MinY*buffer->pitch);

  for(int Y = MinY; Y < MaxY; ++Y) {

      U32 *Dest = (U32 *)DestRow;
      U32 *Source = SourceRow;

      for(int X = MinX; X < MaxX; ++X) {
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
	F32 R = (1.0f-A)*DR + A*SR;
	F32 G = (1.0f-A)*DG + A*SG;
	F32 B = (1.0f-A)*DB + A*SB;

	*Dest = (((U32)(R + 0.5f) << 16) | ((U32)(G + 0.5f) << 8) | ((U32)(B + 0.5f) << 0));
	++Dest;
	++Source;
      }
      DestRow += buffer->pitch;
      SourceRow -= bitmap->width;
    }
}



#define MAX(a,b)((a>b)?(a):(b))
function B32 test_wall(F32 wall_x, F32 rel_x, F32 rel_y, F32 p_delta_x, F32 p_delta_y, F32 *t_min, F32 min_y, F32 max_y){
  B32 hit = false;

  F32 t_epsilon = 0.001f;
  if(p_delta_x != 0.0f){
    F32 t_result = (wall_x - rel_x) /p_delta_x;
    F32 y = rel_y + t_result * p_delta_y;
    if((t_result >= 0.0f) && (*t_min > t_result)){
      if((y >= min_y) && (y <= max_y)){
	*t_min = MAX(0.0f, t_result - t_epsilon);
	hit = true;
      }
    }
  }
  return hit;
}

/**
   Problem: the position and dp is on pixels  but speed is on meters
   Soultion: convert meters to pixles
**/

function void move_entity(GameState* game_state, LowEntity* entity, F32 dt, MoveSpec* move_spec, V2 ddP){
  World* world = game_state->world;

  if(move_spec->unit_max_accel_vector){
    F32 ddP_len= length_sq(ddP);
    if(ddP_len > 1.0f){
      ddP = ddP *  (1.0f / sq_root(ddP_len));
    }
  }
  ddP *= move_spec->speed;
  ddP -= move_spec->drag*entity->dP;

  V2 delta = (0.5f* ddP * square(dt) + entity->dP* dt);
  entity->dP = ddP * dt + entity->dP;

  for (S32 j = 0; j <4 ; j += 1){
    F32 t_min = 1.0f;
    V2 wall_normal = {};
    U32 hit_entity_index = 0;
    V2 desired_pos = entity->pos + delta;

    for (S32 i = 1; i < game_state->low_entity_count; i += 1){
      if(i != entity->stored_index){
	LowEntity * test_entity = game_state->low_entities + i;
	F32 diameter_w = test_entity->width + entity->width;
	F32 diameter_h = test_entity->height + entity->height;
	V2 min_corner = -0.5f*V2{diameter_w, diameter_h};
	V2 max_corner = 0.5f*V2{diameter_w, diameter_h};

	V2 rel = entity->pos - test_entity->pos;

	if(test_wall(min_corner.x, rel.x, rel.y, delta.x, delta.y, &t_min, min_corner.y, max_corner.y)){
	  wall_normal = V2{-1, 0};
	  hit_entity_index = i;
	}
	if(test_wall(max_corner.x, rel.x, rel.y, delta.x, delta.y, &t_min, min_corner.y, max_corner.y)){
	  wall_normal = V2{1, 0};
	  hit_entity_index = i;
	}
	if(test_wall(max_corner.y, rel.y, rel.x, delta.y, delta.x, &t_min, min_corner.x, max_corner.x)){
	  wall_normal = V2{0, 1};
	  hit_entity_index = i;
	  clear_flag(entity, entity_flag_falling);
	}
	if(test_wall(min_corner.y, rel.y, rel.x, delta.y, delta.x, &t_min, min_corner.x, max_corner.x)){
	  wall_normal = V2{0, -1};
	  add_flag(entity, entity_on_ground);
	  clear_flag(entity, entity_flag_falling);
	  hit_entity_index = i;
	}
      }
    }
    entity->pos += t_min* delta;
    if(hit_entity_index){
      entity->dP = entity->dP - 1*inner(entity->dP, wall_normal)* wall_normal;
      delta = desired_pos - entity->pos;
      delta = delta -1*inner(delta, wall_normal)* wall_normal;
    }else{
      if(is_flag_set(entity, entity_on_ground)){
	clear_flag(entity, entity_on_ground);
      }else{
	if(!is_flag_set(entity, entity_flag_jumping)){
	  add_flag(entity, entity_flag_falling);
	}
      }
    }
  }
}

function void draw_rectangle(OffscreenBuffer* buffer, S32 min_x, S32 min_y, S32 max_x, S32 max_y, U32 color) {
  if(min_x < 0) min_x = 0;
  if(min_y < 0) min_y = 0;
  if(max_x < 0) max_x = 0;
  if(max_y < 0) max_y = 0;
  if(max_x > buffer->width)  max_x = buffer->width;
  if(max_y > buffer->height) max_y = buffer->height;
  if(min_x > buffer->width)  min_x = buffer->width;
  if(min_y > buffer->height) min_y = buffer->height;

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

struct AddLowEntityResult{
  LowEntity* low;
  U32 index;
};


function AddLowEntityResult add_low_entity(GameState* game_state, EntityType type, V2 p, U32 color){
  U32 entity_index = game_state->low_entity_count++;
  LowEntity* low_entity= game_state->low_entities + entity_index;
  *low_entity = {};
  low_entity->pos = p;
  low_entity->color = color;
  low_entity->stored_index = entity_index;
  low_entity->type = type;
  AddLowEntityResult result = {};
  result.low = low_entity;
  result.index = entity_index;
  return result;
}

function AddLowEntityResult add_player(GameState* game_state, V2 pos){
  U32 color = denormalize_color(0.4f, 0.2f, 0.2f);
  AddLowEntityResult entity = add_low_entity(game_state, entity_type_player, pos, color);
  entity.low->height = 0.9f;
  entity.low->width  = 0.7f;
  return entity;
}

function AddLowEntityResult add_wall(GameState* game_state, V2 pos, F32 width, F32 height){
  U32 color = denormalize_color(0.0f, 0.4f, 0.3f);
  AddLowEntityResult entity = add_low_entity(game_state, entity_type_wall, pos, color);
  entity.low->height = height;
  entity.low->width  = width;
  return entity;
}


function void render_entities(OffscreenBuffer* buffer, GameState* game_state){
  LowEntity* entity = game_state->low_entities;
  World* world = game_state->world;

  for (S32 i = 0; i < game_state->low_entity_count; i += 1, entity++){
    switch(entity->type){
    case entity_type_player:
    case entity_type_wall:{
      S32 center_x = (world->tile_size_in_meters * (F32)((F32)world->tiles_per_width*0.5f))*world->meters_to_pixels + entity->pos.x * world->meters_to_pixels;
      S32 center_y = (world->tile_size_in_meters * (F32)((F32)world->tiles_per_height*0.5f))*world->meters_to_pixels - entity->pos.y* world->meters_to_pixels;
      S32 min_x = center_x - (entity->width *  world->meters_to_pixels)/2;
      S32 min_y = center_y - (entity->height * world->meters_to_pixels)/2;
      S32 max_x = center_x + (entity->width *  world->meters_to_pixels)/2;
      S32 max_y = center_y + (entity->height * world->meters_to_pixels)/2;
      draw_rectangle(buffer, min_x, min_y, max_x, max_y, entity->color);
    }break;
    case entity_type_npc:{
    }break;
    case entity_type_null:{
      //Do nothing
    }break;
    }
  }
}

#define ABS(num)(num>=0?num:(-1*num))
#define ended_down(b, C)((C->b.ended_down)? 1 :0)
#define was_down(b, C)((!C->b.ended_down && C->b.half_transition_count)?1:0)

function void render_game(OffscreenBuffer* buffer, PlatformState* platform_state, GameInput* input) {
  //NOTE(casery): I like to reserve the entity slot 0 for the null entity cause i'm a genius programmer
  GameState* game_state = (GameState*)platform_state->permanent_storage;

  if (!game_state->is_initilized) {

     
    initilize_arena(&game_state->world_arena, platform_state->permanent_storage_size, (U8*)platform_state->permanent_storage + sizeof(GameState));
    game_state->world = push_struct(&game_state->world_arena, World);
    initilize_world(game_state->world, 9 , 17, 1.4f); //this might cause null error?
    World *world = game_state->world;
    S32 tile_size_pixels = 60;
    platform_state->meters_to_pixels = tile_size_pixels / world->tile_size_in_meters;
    U32 camera_tile_x = world->tiles_per_width/2;
    U32 camera_tile_y = world->tiles_per_height/2;
    //game_state->camera_position = chunkp_from_tilep(world, camera_tile_x, camera_tile_y);

    add_low_entity(game_state, entity_type_null,  V2{0, 0}, 0);
    add_wall(game_state, V2{-1.0f, 6.0f}, 25.0f, 0.2f);
    add_wall(game_state, V2{-1.0f, -6.0f}, 25.0f, 0.2f);

    add_wall(game_state, V2{-1.0f, -3.0f}, 10.0f, 0.2f);

    add_wall(game_state, V2{-11.3f, 0.0f}, 0.2f, 13.0f);
    add_wall(game_state, V2{10.0f, 0.0f}, 0.2f, 13.0f);

    //Get the decompressed pixels and it's attributes
    //Render it to the buffer

    background_png = parse_png("../data/background_smoll.png");
    game_state->is_initilized = true;
  }

  //Player movement
  for (S32 i = 0; i < array_count(input->controllers) ; i += 1){
    S32 entity_index = game_state->controlled_entity_index[i];
    ControllerInput* controller = input->controllers + i;

    if(entity_index ==  0){
      if(controller->start.ended_down){
	AddLowEntityResult res = add_player(game_state, V2{1.0f, -5.0f});
	game_state->controlled_entity_index[i] = res.index;
      }
    }else{
      V2 ddP = {}; //acceleration
      LowEntity* entity = game_state->low_entities + entity_index;

      if(controller->is_analog){
	//ddP = V2{controller->stick_x, controller->stick_y};
      } else{
	//if(ended_down(move_up, controller))     ddP.y =  1.0f;
	//if(ended_down(move_down, controller))   ddP.y = -1.0f;
	if(ended_down(move_right, controller))  ddP.x =  1.0f;
	if(ended_down(move_left, controller))  ddP.x =  -1.0f;

	//First step
	if(ended_down(start,controller)){
	  //If we reached jump time then stop jumping and stop falling
	  if(entity->jump_time >= 1.0f){
	    add_flag(entity, entity_flag_falling);
	    clear_flag(entity, entity_flag_jumping);
	    entity->jump_time = 0;
	  }else{
	    entity->jump_time += input->dtForFrame;
	    ddP.y = 1.0f;
	    if(!is_flag_set(entity, entity_flag_jumping)){
	      add_flag(entity, entity_flag_jumping);
	    }
	  }
	}else{
	  //If the entity was jumping and start isn't pressed then we need to stop jumping and start falling
	  if(is_flag_set(entity, entity_flag_jumping)){
	    clear_flag(entity, entity_flag_jumping);
	    add_flag(entity, entity_flag_falling);
	    entity->jump_time = 0;
	  }
	}

	//If entity is falling we need to set the jump_time to 0 and check if it reached ground
	if(is_flag_set(entity, entity_flag_falling)){
	  entity->jump_time = 0;
	  if(!is_flag_set(entity, entity_on_ground)){
	    ddP.y = -1.0f;
	  }else{
	    clear_flag(entity, entity_flag_falling);
	  }
	}

      }
      MoveSpec spec = default_move_spec();
      spec.unit_max_accel_vector = 1;
      spec.speed = 3.61f;
      spec.drag = 4.0f;
      move_entity(game_state, entity, input->dtForFrame, &spec, ddP);
    }
  }

  //Background color
  //paint_buffer(buffer, 0.4f, 0.9f, 0.3f);
  draw_bitmap(buffer, &background_png,0, 0);
  render_entities(buffer, game_state);
}


