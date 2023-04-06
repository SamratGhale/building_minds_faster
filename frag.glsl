#version 330

in vec2 t_coord0;
in vec2 v_pos;
in vec4 obj_color;


uniform vec2 xy_pos;
uniform vec2 light_pos;
uniform sampler2D sampler;
uniform bool use_light;
uniform bool tiles= false;
out vec4 color;

void main(void){
  vec4 tex_color = texture2D(sampler, t_coord0);
  if(use_light){
    float dx = light_pos.x - (xy_pos.x + v_pos.x);
    float dy = light_pos.y - (xy_pos.y + v_pos.y);

    float dist = sqrt(dx * dx + dy * dy);
    float max_dist = 1000.0f;
    float percent = clamp(1.0f - dist / max_dist, 0.0, 1.0f);
    percent *= percent;
    vec3 ambient_light = vec3(percent, percent, percent);

    if(tiles){
      color = obj_color * vec4(ambient_light, 1.0);
    }else{
      color = tex_color* vec4(ambient_light, 1.0) * tex_color;
    }
  }else{

    if(tiles){
      color = obj_color ;
      }else{
        color = tex_color* tex_color;
      }
    }
}