#include "math.h"

#pragma pack(push, 1)
struct png_header {
  U8 Signature[8];
};
global_variable U8 PNGSignature[] = {137, 80, 78, 71, 13, 10, 26, 10};

struct PNGChunkHeader{
  U32 Length;
  U32 TypeU32;
};

struct PNGChunkFooter {
  U32 crc;
};

struct PNGIhdr {
  U32 width;
  U32 height;
  U8  bit_depth;
  U8  color_type;
  U8  compression_method;
  U8  filter_method;
  U8  interlace_method;
};

struct PNGIdatHeader {
  U8 zlib_method_flag;
  U8 additional_flag;
};

struct PNGIdatFooter {
  U32 check_value;
};

#pragma pack(pop)

struct StreamingChunk{
  U32   size;
  void* contents;
  StreamingChunk *next;
};

struct StreamingBuffer {
  U32 size;
  void *contents;
    
  U32 bit_count;
  U32 bit_buf;
    
  B32 under_flowed;
    
  StreamingChunk *first;
  StreamingChunk *last;
};

struct PNGHuffmanEntry{
  U16 symbol;
  U16 bits_used;
};

#define PNG_HUFFMAN_MAX_BIT_COUNT 16
struct PNGHuffman{
  U32 MaxCodeLengthInBits;
  U32 EntryCount;
  PNGHuffmanEntry *Entries;
};


