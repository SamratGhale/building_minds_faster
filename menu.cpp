struct MenuItem{
	char title[20]; 
};

struct MenuState{
	MenuItem items[4];
	B32 initilized;
	OpenglContext gl_context;
	U8 selected;
};

function void openg_draw_rect(F32 min_x, F32 min_y, F32 max_x, F32 max_y, MenuState* state) {

	glUseProgram(opengl_config.basic_light_program);
	glUniform1i(opengl_config.tile_uniform, true);

	F32 mtop = 50;

	if(!state->initilized){
		opengl_init_texture(&state->gl_context, V2_F32{min_x, min_y}, V2_F32{max_x, max_y}, V4{1,0,0,1} , mtop, mtop);
		state->initilized = true;
	}else{
		opengl_update_vertex_data(&state->gl_context, V2_F32{min_x, min_y}, V2_F32{max_x, max_y}, V4{1,0,0,1});
	}

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glUniform1i(opengl_config.tile_uniform, false);

	glUseProgram(0);
}


function void show_menu(OffscreenBuffer* buffer, MenuState* state){
	S32 mtop = 50;
	F32 min_x, min_y;
	F32 center_x = buffer->width  * 0.5;
	F32 center_y = buffer->height * 0.5;

	for (int i = 0; i < array_count(state->items); ++i) {
		MenuItem *item = &state->items[i];


		if(i == state->selected){
			min_x = (center_x + (-10) * mtop) - (mtop * 0.5f);
			min_y = (center_y - i * (mtop + 20))- (mtop * 0.5f);
			F32 width = min_x + mtop * 20;
			F32 height = min_y + mtop;
			openg_draw_rect(min_x, min_y, width, height, state);
		} 

		for (int j = 0; j < 20; ++j) {

			char c = item->title[j];

			if(c == '\n'){
				break;
			}
			LoadedBitmap* bmp = get_font(&platform.font_asset, c);

			S32 tex_width  = bmp->width  * mtop;
			S32 tex_height = bmp->height * mtop;

			min_x = (center_x + (-10+j) * mtop) - tex_width*0.5f;
			min_y = (center_y - i * (mtop + 20)) - tex_height*0.5f;
			opengl_bitmap(bmp, min_x, min_y, tex_width, tex_height);
		}
	}
}

function void render_menu(OffscreenBuffer* buffer, GameInput* input) {

	MenuState* state = (MenuState*)((U8*)platform.permanent_storage + sizeof(GameState));

  LoadedBitmap* background_png = get_asset(&platform.asset, asset_background);
  opengl_bitmap(background_png, 0,0, background_png->width, background_png->height);

	if(!state->initilized){

		strncpy(state->items[0].title, "SETTINGS\n", 20);
		strncpy(state->items[1].title, "PLAY\n", 20);
		strncpy(state->items[2].title, "QUIT\n", 20);
		strncpy(state->items[3].title, "RESUME\n", 20);

		state->selected = 2;

		//state->initilized = true;
		F32 a = 2.0f/(F32)buffer->width;
		F32 b = 2.0f/(F32)buffer->height;
		F32 proj[]={
			a, 0, 0, 0,
			0, b, 0, 0,
			0, 0, 1, 0,
			-1,-1, 0, 1,
		};
		glUseProgram(opengl_config.basic_light_program);
		glUniformMatrix4fv(opengl_config.transform_id, 1, GL_FALSE, &proj[0]);
		glUseProgram(0);
	}

	show_menu(buffer, state);

	for (S32 i = 0; i < array_count(input->controllers); i += 1) {
		ControllerInput* controller = input->controllers + i;

		if (was_down(move_down, controller)){
			if(state->selected < 3) state->selected++;
		}
		if (was_down(move_up, controller)){
			if(state->selected > 0) state->selected--;
		}
		if (was_down(enter, controller)){
			switch(state->selected){
				case 0:{

				}break;
				case 1:{
					platform.game_mode = game_mode_play;
				}break;
				case 2:{
					platform.game_mode = game_mode_exit;
				}break;
				case 3:{
					platform.game_mode = game_mode_play;
				}break;
			}
		}
	}
}










