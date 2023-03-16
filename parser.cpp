#include "parser.h"

function void parse_commands(const char* file_name, Config* tokens) {
  ReadFileResult file = read_entire_file((char*)file_name);
  char* curr = (char*)file.contents;
  for (S32 i = 0; i < file.content_size; i++) {
    if (curr[0] == '\n' || i == 0) {
      if (curr[0] == '\n' || curr[0] == '\r') {
        curr++;
      }
      tokens->tokens[tokens->count].base = curr;
      if (strncmp("add_wall", tokens->tokens[tokens->count].base, 8) == 0) {
        tokens->tokens[tokens->count].type = command_add_wall;
        tokens->tokens[tokens->count].args = tokens->tokens[tokens->count].base + 9;
        tokens->tokens[tokens->count].command_len = 8;
      } else if (strncmp("camera_pos", tokens->tokens[tokens->count].base, 10) == 0) {
        tokens->tokens[tokens->count].type = command_set_camera_pos;
        tokens->tokens[tokens->count].args = tokens->tokens[tokens->count].base + 11;
        tokens->tokens[tokens->count].command_len = 10;
      }else if(strncmp("add_temple", tokens->tokens[tokens->count].base, 10) == 0){
        tokens->tokens[tokens->count].type = command_add_temple;
        tokens->tokens[tokens->count].args = tokens->tokens[tokens->count].base + 11;
        tokens->tokens[tokens->count].command_len = 10;
      } else{
        //assert(0);
      }
      tokens->count++;
    }
    tokens->tokens[tokens->count - 1].total_len++;
    curr++;
  }
}






