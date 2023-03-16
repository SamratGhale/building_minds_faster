#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 t_coord;

uniform mat4 mat;
out vec2 t_coord0;
out vec2 v_pos;

void main(){
  t_coord0 = vec2(t_coord.x, 1 - t_coord.y);
  gl_Position = mat * vec4(pos, 1.0);
  v_pos = vec2(pos.x, pos.y);
}