enum CommandEnum{
  command_add_ladder,
  command_add_wall,
  command_set_camera_pos,
  command_set_player_pos,
  command_set_background_png,
  command_add_temple
};

struct ConfigToken{
  S32 total_len;
  S32 command_len;
  CommandEnum type;
  char* base;
  char* args;
};

struct Config{
  S32 count;
  S64 last_write_time;
  ConfigToken tokens[128];
};
