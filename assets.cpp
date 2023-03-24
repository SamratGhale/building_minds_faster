
function LoadedBitmap* get_asset(Asset* asset, Asset_Enum id){
	assert(asset);
	LoadedBitmap* bitmap = &asset->bitmaps[id];

	if(!bitmap->pixels){
		switch(id){
		  case asset_background :{
				*bitmap = parse_png("../data/green_background.png");
			}break;

			case asset_temple :{
				*bitmap = parse_png("../data/temple.png");
			}break;

			case asset_player_right:{
				*bitmap = parse_png("../data/player_anim/player_standing.png");
			}break;

			case asset_player_left:{
				*bitmap = parse_png("../data/player_anim/player_standing_left.png");
			}break;

			case asset_wall :{
				*bitmap = parse_png("../data/tex_wall_smool.png");
			}break;

			case asset_grass :{
				*bitmap = parse_png("../data/grass.png");
			}break;
		}
	}
	return bitmap;
}