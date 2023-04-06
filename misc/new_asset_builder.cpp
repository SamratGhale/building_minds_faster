#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>

#define BITMAP_BYTES_PER_PIXEL 4

#pragma pack(push, 1)
struct LoadedBitmap{
  float width;
  float height;
  uint32_t pitch;
  uint64_t total_size;
  uint32_t *pixels;
};
struct BitmapHeader{
  float width;
  float height;
  uint32_t pitch;
  uint64_t total_size;
};

#pragma pack(pop)

struct ReadFileResult{
  uint32_t content_size;
  void* contents;
};

ReadFileResult read_entire_file(char* file_name){
  ReadFileResult result = {};
  HANDLE file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if(file_handle != INVALID_HANDLE_VALUE){
    uint64_t file_size;
    if(GetFileSizeEx(file_handle, (PLARGE_INTEGER)&file_size)){
      result.contents = VirtualAlloc(0, file_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if(result.contents){
        uint32_t bytes_read;
        if(ReadFile(file_handle, result.contents, file_size, (LPDWORD)&bytes_read, 0)) {
          result.content_size = file_size;
        }else{
          VirtualFree(result.contents, 0, MEM_RELEASE);
          result.contents = 0;
        }
      }
    }
    CloseHandle(file_handle);
  }
  return result;
}

LoadedBitmap make_empty_bitmap(int width, int height) {
  LoadedBitmap result = {};
  if(width > height){
    float ratio = (float)height / (float)width;
    result.width = 1;
    result.height = ratio;
  }else{
    float ratio = (float)width / (float)height;
    result.width = ratio;
    result.height = 1;
  }
  result.pitch = width * BITMAP_BYTES_PER_PIXEL;
  result.total_size = width *height * BITMAP_BYTES_PER_PIXEL;
  result.pixels = (uint32_t*)malloc(result.total_size);
  return(result);
}

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

void write_append_file(LoadedBitmap bitmap, HANDLE file_handle){

  uint64_t written = 0;

  WriteFile(file_handle, (LPCVOID)&bitmap, sizeof(BitmapHeader), (LPDWORD)&written, 0);

  assert(written == sizeof(BitmapHeader));

  printf("Total bytes written %I64d\n", written);

  uint64_t pixels_size = bitmap.total_size;
  WriteFile(file_handle, (LPCVOID)bitmap.pixels, pixels_size, (LPDWORD)&written, 0);

  printf("Total bytes written %I64d\n", written);
  printf("Height %f, width %f pitch %d\n", bitmap.height, bitmap.width, bitmap.pitch);
}

int main(){
  ReadFileResult font_file = read_entire_file("c:/Windows/Fonts/FiraCode-SemiBold.ttf");


  stbtt_fontinfo info;
  if(!stbtt_InitFont(&info, (const unsigned char*)font_file.contents, 0)){
    printf("Failed");
  }


  char* file_name = "./data/fonts.dat";

  HANDLE file_handle = CreateFileA(file_name, GENERIC_READ|GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
  CloseHandle(file_handle);
  file_handle = CreateFileA(file_name, FILE_APPEND_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

  float pixel_height = 200.0f;
  float scale = stbtt_ScaleForPixelHeight(&info, pixel_height);

  for(int i = 32; i < 128; ++i) {

    int width, height, x_offset, y_offset;

    uint8_t * mono_bitmap = stbtt_GetCodepointBitmap(&info, 0, scale, (char)i, &width, &height, &x_offset, &y_offset);


    LoadedBitmap font_bitmap = make_empty_bitmap(width, height);

    if(i == 32){
      font_bitmap = make_empty_bitmap(10, scale);
    } else{
      uint8_t * source   = mono_bitmap;
      uint8_t * dest_row = (uint8_t*)font_bitmap.pixels + (height -1)* font_bitmap.pitch;
      for(int y = 0; y < height; y++){
        uint32_t * dest = (uint32_t *)dest_row;
        for(int x = width-1; x >= 0; --x){
          uint8_t alpha = source[x];

          *dest++ = ((alpha << 24) | (alpha << 16) | (alpha << 8) | (alpha << 0) );
        }
        source += width;
        dest_row -= font_bitmap.pitch;
      }

    }
    write_append_file(font_bitmap, file_handle);
  }
  CloseHandle(file_handle);
  return 0;
}









