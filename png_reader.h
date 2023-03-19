#include "math.h"

#pragma pack(push, 1)
struct png_header {
  U8 Signature[8];
};
global_variable U8 PNGSignature[] = {137, 80, 78, 71, 13, 10, 26, 10};

struct PNGchunk_posHeader{
  U32 Length;
  U32 TypeU32;
};

struct PNGchunk_posFooter {
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

struct Streamingchunk_pos{
  U32   size;
  void* contents;
  Streamingchunk_pos *next;
};

struct StreamingBuffer {
  U32 size;
  void *contents;
    
  U32 bit_count;
  U32 bit_buf;
    
  B32 under_flowed;
    
  Streamingchunk_pos *first;
  Streamingchunk_pos *last;
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


global_variable PNGHuffmanEntry PNGLengthExtra[] = {
  {3, 0}, //   257
  {4, 0}, //   258
  {5, 0}, //   259
  {6, 0}, //   260
  {7, 0}, //   261
  {8, 0}, //   262
  {9, 0}, //   263
  {10, 0}, //  264
  {11, 1}, //  265
  {13, 1}, //  266
  {15, 1}, //  267
  {17, 1}, //  268
  {19, 2}, //  269
  {23, 2}, //  270
  {27, 2}, //  271
  {31, 2}, //  272
  {35, 3}, //  273
  {43, 3}, //  274
  {51, 3}, //  275
  {59, 3}, //  276
  {67, 4}, //  277
  {83, 4}, //  278
  {99, 4}, //  279
  {115, 4}, // 280
  {131, 5}, // 281
  {163, 5}, // 282
  {195, 5}, // 283
  {227, 5}, // 284
  {258, 0}, // 285
};

global_variable PNGHuffmanEntry PNGDistExtra[] = {
  {1, 0}, //     0
  {2, 0}, //     1
  {3, 0}, //     2
  {4, 0}, //     3
  {5, 1}, //     4
  {7, 1}, //     5
  {9, 2}, //     6
  {13, 2}, //    7
  {17, 3}, //    8
  {25, 3}, //    9
  {33, 4}, //    10
  {49, 4}, //    11
  {65, 5}, //    12
  {97, 5}, //    13
  {129, 6}, //   14
  {193, 6}, //   15
  {257, 7}, //   16
  {385, 7}, //   17
  {513, 8}, //   18
  {769, 8}, //   19
  {1025, 9}, //  20
  {1537, 9}, //  21
  {2049, 10}, // 22
  {3073, 10}, // 23
  {4097, 11}, // 24
  {6145, 11}, // 25
  {8193, 12}, // 26
  {12289, 12}, //27
  {16385, 13}, //28
  {24577, 13}, //29
};


#define MAX_REVERSE_BITS_COUNT 13
global_variable U16 ReversedBits[1 << MAX_REVERSE_BITS_COUNT];
