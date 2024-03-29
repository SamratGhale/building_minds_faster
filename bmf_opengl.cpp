#include "game.h"

#define GL_FRAMEBUFFER_SRGB                     0x8DB9
#define GL_SRGB8_ALPHA8                         0x8C43
#define GL_LINK_STATUS                          0x8B82
#define GL_SHADING_LANGUAGE_VERSION             0x8B8C

// NOTE: Windows-specific
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_MULTISAMPLE                    0x809D
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4
#define GL_TEXTURE0                       0x84C0
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_RGBA                           0x1908
#define GL_BGRA                           0x80E1
#define GL_DYNAMIC_DRAW                   0x88E8


function OpenglInfo opengl_get_info(B32 modern_context){
  OpenglInfo result = {};
  result.modern_context = modern_context;
  result.vendor   = (char*)glGetString(GL_VENDOR);
  result.renderer = (char*)glGetString(GL_RENDERER);
  result.version  = (char*)glGetString(GL_VERSION);
  if(result.modern_context){
    result.shading_version = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  }else{
    result.shading_version = "(none)";
  }
  result.extensions = (char*)glGetString(GL_EXTENSIONS);

  char* at = result.extensions;
  while(*at){
    while(is_whitespace(*at)) {++at;}
    char* end = at;
    //NOTE(samrat) Chopping up the string
    while(*end && !is_whitespace(*end)){++end;}
    U32 count = end - at;

    if(0){}
    else if(strings_are_equal(count, at, "GL_EXT_texture_sRGB")){
      result.GL_EXT_texture_sRGB=true;
    }else if(strings_are_equal(count, at, "GL_EXT_framebuffer_sRGB")){
      result.GL_EXT_framebuffer_sRGB=true;
    }
    at = end;
  }
  return result;
}

function GLuint opengl_create_program(char* vertex_code, char* fragment_code){

  GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLchar* vertex_shader_code[] = {
    vertex_code
  };
  glShaderSource(vertex_shader_id, array_count(vertex_shader_code), vertex_shader_code, 0);
  glCompileShader(vertex_shader_id);


  GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  GLchar* fragment_shader_code[] = {
    fragment_code
  };
  glShaderSource(fragment_shader_id, array_count(fragment_shader_code), fragment_shader_code, 0);
  glCompileShader(fragment_shader_id);

  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);
  glLinkProgram(program_id);
  glValidateProgram(program_id);

  GLint linked = 0;
  glGetProgramiv(program_id, GL_LINK_STATUS, &linked);
  if(!linked){
    GLsizei ignored;
    char vertex_errors[4096];
    char fragment_errors[4096];
    char program_errors[4096];
    glGetShaderInfoLog(vertex_shader_id, sizeof(vertex_errors), &ignored, vertex_errors);
    glGetShaderInfoLog(fragment_shader_id, sizeof(fragment_errors), &ignored, fragment_errors);
    glGetProgramInfoLog(program_id, sizeof(program_errors), &ignored, program_errors);
    assert(0);
  }
  return program_id;
}

function OpenglInfo opengl_init(B32 modern_context){
  OpenglInfo info = opengl_get_info(modern_context);
  opengl_config.default_internal_text_format = GL_RGBA8;
  if(info.GL_EXT_texture_sRGB){
    opengl_config.default_internal_text_format = GL_SRGB8_ALPHA8;
  }
  if(info.GL_EXT_framebuffer_sRGB){
    glEnable(GL_FRAMEBUFFER_SRGB);
  }

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_MODULATE);

  char* vertex_code = (char* )read_entire_file("../vert.glsl").contents;
  char* fragment_code = (char*)read_entire_file("../frag.glsl").contents;

  //might need the header code later
  opengl_config.basic_light_program = opengl_create_program(vertex_code, fragment_code);
  opengl_config.transform_id = glGetUniformLocation(opengl_config.basic_light_program, "mat");
  opengl_config.tile_uniform = glGetUniformLocation(opengl_config.basic_light_program, "tiles");

  opengl_config.light_pos_id =
    glGetUniformLocation(opengl_config.basic_light_program, "light_pos");
  opengl_config.use_light =
    glGetUniformLocation(opengl_config.basic_light_program, "use_light");

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(1);

  return info;
}

function void opengl_init_texture(OpenglContext* context, V2_F32 min_p, V2_F32 max_p, V4 color,  S32 width, S32 height, LoadedBitmap* image = NULL){

  F32 vertices[] = {
    max_p.x,  max_p.y, 0.0f,   0.0f, 0.0f, color.r, color.g, color.b, color.a, // top right
    max_p.x,  min_p.y, 0.0f,   0.0f, 1.0f, color.r, color.g, color.b, color.a, // bottom right
    min_p.x,  min_p.y, 0.0f,   1.0f, 1.0f, color.r, color.g, color.b, color.a, // bottom left
    min_p.x,  max_p.y, 0.0f,   1.0f, 0.0f, color.r, color.g, color.b, color.a // top left 
  };

  glGenVertexArrays(1, &context->vao);
  glGenBuffers(1, &context->vbo);
  glGenBuffers(1, &context->ebo);

  if(image){
    glGenTextures(1, &context->tex_handle);
    glBindTexture(GL_TEXTURE_2D, context->tex_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, image->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  }

  U32 indices[] = {
    0, 1, 3, //First triangle
    1, 2, 3  //Second triangle
  };
  glBindVertexArray(context->vao);
  glBindBuffer(GL_ARRAY_BUFFER, context->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 9 * sizeof(F32), (void*)(3 * sizeof(F32)));
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 9 * sizeof(F32), (void*)0);
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, 9 * sizeof(F32), (void*)(5 * sizeof(F32))); 

}

function void opengl_update_vertex_data(OpenglContext* context, V2_F32 min_p, V2_F32 max_p, V4 color = V4{-1,-1,-1,-1}){

  glBindVertexArray(context->vao);
  glBindBuffer(GL_ARRAY_BUFFER, context->vbo);

  V2_F32 top_right    = V2_F32{max_p.x, max_p.y};
  V2_F32 bottom_right = V2_F32{max_p.x, min_p.y};
  V2_F32 bottom_left  = V2_F32{min_p.x, min_p.y};
  V2_F32 top_left     = V2_F32{min_p.x, max_p.y};

  glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(float), (const GLvoid*)top_right.e);
  glBufferSubData(GL_ARRAY_BUFFER, 1 * 9 * sizeof(float), 2 * sizeof(float), (const GLvoid*)bottom_right.e);
  glBufferSubData(GL_ARRAY_BUFFER, 2 * 9 * sizeof(float), 2 * sizeof(float), (const GLvoid*)bottom_left.e);
  glBufferSubData(GL_ARRAY_BUFFER, 3 * 9 * sizeof(float), 2 * sizeof(float), (const GLvoid*)top_left.e);

  if(color.r == -1){
    glBindTexture(GL_TEXTURE_2D, context->tex_handle);
    glBufferSubData(GL_ARRAY_BUFFER, 0 * 9 * sizeof(float) + 5*sizeof(float), 4 * sizeof(float), (const GLvoid*)color.e);
    glBufferSubData(GL_ARRAY_BUFFER, 1 * 9 * sizeof(float) + 5*sizeof(float), 4 * sizeof(float), (const GLvoid*)color.e);
    glBufferSubData(GL_ARRAY_BUFFER, 2 * 9 * sizeof(float) + 5*sizeof(float), 4 * sizeof(float), (const GLvoid*)color.e);
    glBufferSubData(GL_ARRAY_BUFFER, 3 * 9 * sizeof(float) + 5*sizeof(float), 4 * sizeof(float), (const GLvoid*)color.e);
  }
}


function void opengl_tile(V2_F32 cam_offset, World *world, Tile *tile, WorldChunk* chunk) {
  F32 mtop = world->meters_to_pixels;
  V2_F32 csim = world->chunk_size_in_meters;

  F32 center_x = (csim.x * 0.5f - cam_offset.x ) * mtop ;
  F32 center_y = (csim.y * 0.5f - cam_offset.y ) * mtop ;

  F32 min_x = (center_x + tile->tile_pos.x * mtop) - (mtop * 0.5f);
  F32 min_y = (center_y + tile->tile_pos.y * mtop) - (mtop * 0.5f);
  F32 max_x = min_x + mtop;
  F32 max_y = min_y + mtop;


  if(!tile->initlized){
    opengl_init_texture(&tile->gl_context, V2_F32{min_x, min_y}, V2_F32{max_x, max_y}, tile->color, mtop, mtop);
    tile->initlized = true;
  }else{
    opengl_update_vertex_data(&tile->gl_context, V2_F32{min_x, min_y}, V2_F32{max_x, max_y}, tile->color);
  }

  glUseProgram(opengl_config.basic_light_program);
  if(is_flag_set(tile, tile_path)){
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }
}

//All of the int arguements is in pixels
function void opengl_bitmap(LoadedBitmap *image, F32 min_x, F32 min_y, S32 width, S32 height){
  F32 max_x  = min_x + width;
  F32 max_y  = min_y + height;

  //The coordinates are always the same just change the position
  if(image->gl_context.tex_handle == 0){

    if(image->width <=1.0f || image->height <= 1.0f){

      //Calculate width and height

      S32 width = image->pitch / BITMAP_BYTES_PER_PIXEL;
      S32 height = image->total_size/image->pitch;

      opengl_init_texture(&image->gl_context, V2_F32{min_x, min_y}, V2_F32{max_x, max_y}, V4{0,0,0,0}, width, height, image);
    }else{


      opengl_init_texture(&image->gl_context, V2_F32{min_x, min_y}, V2_F32{max_x, max_y}, V4{0,0,0,0}, image->width, image->height, image);
    }
      
  }else{
    opengl_update_vertex_data(&image->gl_context, V2_F32{min_x, min_y}, V2_F32{max_x, max_y});
  }
  glUseProgram(opengl_config.basic_light_program);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}










