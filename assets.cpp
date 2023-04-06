
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
      *bitmap = parse_png("../data/character_smoll.png");
    }break;

    case asset_player_left:{
      *bitmap = parse_png("../data/character_smoll_left.png");
    }break;

    case asset_wall :{
      *bitmap = parse_png("../data/tex_wall_smool.png");
    }break;

    case asset_grass :{
      *bitmap = parse_png("../data/grass.png");
    }break;

    case asset_banner_tile:{
      *bitmap = parse_png("../data/border.png");
    }break;
    case asset_fire_torch:{
      *bitmap = parse_png("../data/torch.png");
    }break;
    }
  }
  return bitmap;
}

function FontAsset load_font_asset(MemoryArena* arena){
  char* file_name = "../data/fonts.dat"; 

  //HANDLE file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

  ReadFileResult dat_file = read_entire_file(file_name);

  assert(dat_file.contents);

  FontAsset asset = {};
  uint64_t offset = 0;

  for (int i = 0; i < (128 - 32); ++i) {
    BitmapHeader * header = (BitmapHeader*)((U8*)dat_file.contents + offset);

    asset.bitmaps[i] = {};
    asset.bitmaps[i].height = header->height;
    asset.bitmaps[i].width = header->width;
    asset.bitmaps[i].pitch = header->pitch;
    asset.bitmaps[i].total_size = header->total_size;


    offset += sizeof(BitmapHeader);

    uint32_t* pixels = (uint32_t*)((uint8_t*)dat_file.contents + offset);

    uint32_t pixel_size = header->total_size;

    asset.bitmaps[i].pixels = (U32*)push_size(arena, pixel_size);

    CopyMemory((PVOID)asset.bitmaps[i].pixels, (void*)pixels, pixel_size);
    offset += pixel_size;
  }

  return asset;
}

function LoadedBitmap* get_font(FontAsset* asset, char c){
  assert(asset);
  int i = (int)c;
  assert(i >= 32);
  return &asset->bitmaps[i - 32];
}



function LoadedBitmap* get_font_number(FontAsset* asset, U32 n){
  //assert(n < 10);
  n += 48;

  assert(asset);
  return &asset->bitmaps[n - 32];
}













