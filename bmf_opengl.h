#ifndef BMF_OPENGL

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
};


#define BMF_OPENGL
#endif