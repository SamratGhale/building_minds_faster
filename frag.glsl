#version 330

in vec2 t_coord0;
in vec2 v_pos;

uniform vec3 obj_color;
uniform vec2 xy_pos;
uniform vec2 light_pos;
uniform sampler2D sampler;

out vec4 color;

void main(void){
  vec4 tex_color = texture2D(sampler, t_coord0);

  //tex_color = vec4(mix(tex_color.rgb, tex_color.rgb, tex_color.a), 1.0f);

  if(light_pos.x == 0.0f  && light_pos.y == 0.0f){
    color = vec4(0, 0, 0, 0);
    return;
  }

  float dx = light_pos.x - (xy_pos.x + v_pos.x);
  float dy = light_pos.y - (xy_pos.y + v_pos.y);

  float dist = sqrt(dx * dx + dy * dy);
  float max_dist = 1220.0f;
  float percent = clamp(1.0f - dist / max_dist, 0.0, 1.0f);
  percent *= percent;
  vec3 ambient_light = vec3(percent, percent, percent);

  color = tex_color* tex_color * vec4(ambient_light, 1.0);// * vec4(obj_color, 0.5f) ;
  /*
  if((tex_color.x == 0 && tex_color.y == 0 && tex_color.z == 0) || tex_color.a <= 0){
    color = vec4(obj_color, 1.0) * vec4(ambient_light, 1.0);
  }else{
  }
  */
}