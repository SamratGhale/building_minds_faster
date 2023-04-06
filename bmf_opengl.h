#ifndef BMF_OPENGL

//To be attached to the entity structs
struct OpenglContext{
  U32 tex_handle; //This will be uninitilized for tile
  U32 vbo;
  U32 vao;
  U32 ebo;
};

struct OpenglInfo{
  B32 modern_context;

  char* vendor;
  char* renderer;
  char* version;
  char* shading_version;
  char* extensions;

  B32 GL_EXT_texture_sRGB;
  B32 GL_EXT_framebuffer_sRGB;
  B32 GL_ARB_framebuffer_object;
};

struct OpenglConfig{
  GLuint basic_light_program;
  GLuint default_internal_text_format;
  GLuint texture_sampler_id;
  GLuint transform_id;
  GLuint light_pos_id;
  GLuint use_light;
  bool use_light_local;

  GLuint tile_uniform;
};



#define BMF_OPENGL
#endif