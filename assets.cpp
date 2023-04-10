#include "assets.h"

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
      *bitmap = parse_png("../data/player/right.png");
    }break;

    case asset_player_left:{
      *bitmap = parse_png("../data/player/left.png");
    }break;

    case asset_player_back:{
      *bitmap = parse_png("../data/player/back.png");
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

struct RiffIterator{
  U8* at;
  U8* stop;
};

inline RiffIterator parse_chunk_at(void* at, void* stop){
  RiffIterator iter;
  iter.at   = (U8*)at;
  iter.stop = (U8*)stop;
  return iter;
}

inline RiffIterator next_chunk(RiffIterator iter){
  WAVE_chunk* chunk = (WAVE_chunk*) iter.at;
  U32 size = (chunk->size + 1) & ~1;
  iter.at += sizeof(WAVE_chunk) + size;
  return iter;
}

inline B32 is_valid(RiffIterator iter){
  B32 result = (iter.at < iter.stop);
  return result;
}

inline void* get_chunk_data(RiffIterator iter){
  void* result = (iter.at + sizeof(WAVE_chunk));
  return result;
}

inline U32 get_type(RiffIterator iter){
  WAVE_chunk* chunk = (WAVE_chunk*)iter.at;
  U32 result = chunk->ID;
  return result;
}

inline U32 get_chunk_data_size(RiffIterator iter){
  WAVE_chunk* chunk = (WAVE_chunk*)iter.at;
  U32 result = chunk->size;
  return result;
}

function LoadedSound load_wav(char* file_name){
  LoadedSound result = {};
  ReadFileResult read_result = read_entire_file(file_name);

  if(read_result.content_size != 0){
    WAVE_header* header = (WAVE_header*)read_result.contents;
    assert(header->RIFFID == WAVE_ChunkID_RIFF);
    assert(header->WAVED == WAVE_ChunkID_WAVE);

    U32 channel_count = 0;
    U32 sample_data_size = 0;
    S16 *sample_data = 0;

    for(RiffIterator iter = parse_chunk_at(header + 1, (U8*)(header + 1) + header->size -4); is_valid(iter); iter = next_chunk(iter)){
      switch(get_type(iter)){
      case WAVE_ChunkID_fmt:{
        WAVE_fmt * fmt = (WAVE_fmt*)get_chunk_data(iter); 
        assert(fmt->wFormatTag == 1);
        assert(fmt->nSamplesPerSec == 48000);
        assert(fmt->wBitsPerSample == 16);
        assert(fmt->nBlockAlign == (sizeof(S16)*fmt->nChannels));
        channel_count = fmt->nChannels;
      }break;
      case WAVE_ChunkID_data:{
        sample_data = (S16*)get_chunk_data(iter);
        sample_data_size = get_chunk_data_size(iter);
      }break;
      }
    }
    assert(channel_count && sample_data);

    result.channel_count = channel_count;
    result.sample_count = sample_data_size / (channel_count* sizeof(S16));

    if(channel_count == 1){
      result.samples[0] = sample_data;
      result.samples[1] = 0;
    }else if(channel_count == 2){
      result.samples[0] = sample_data;
      result.samples[1] = sample_data + result.sample_count;

      for(U32 sample_index = 0; sample_index < result.sample_count; ++sample_index){
        S16 source = sample_data[2*sample_index];
        sample_data[2*sample_index] = sample_data[sample_index];
        sample_data[sample_index] = source;
      }
    }else{
      assert(1==0);
    }
    result.channel_count = 1;
  }
  return result;
}


function LoadedSound * get_sound_asset(SoundAsset* asset, AssetSound_Enum id){

  assert(asset);
  LoadedSound* sound = &asset->sounds[id];
  if(!sound->sample_count){
    switch(id){
    case asset_sound_background :{
      *sound = load_wav("../data/sounds/music_test.wav");
    }break;

    case asset_sound_jump:{
      *sound = load_wav("../data/sounds/bloop_00.wav");
    }break;
    }
  }
  return sound;
}











