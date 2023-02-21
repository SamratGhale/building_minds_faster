#include "png_reader.h"
#include "math.h"

function U32 swap_rb(U32 c) {
  U32 result = ((c & 0xFF00FF00) | ((c >> 16) & 0xFF) | ((c & 0xFF) << 16));
  return(result);
}

//This functions alters the pixles to be in correct alignment
function void write_image_top_down_rgba(U32 width, U32 height, U8 *pixels) {
  U32 output_pixel_size = 4 * width*height;

  U32 mid_point_y = ((height + 1) / 2);
  U32 *Row0 = (U32 *)pixels;
  U32 *Row1 = Row0 + ((height - 1)*width);

  for (U32 Y = 0; Y < mid_point_y; ++Y) {
    U32 *Pix0 = Row0;
    U32 *Pix1 = Row1;

    for (U32 X = 0; X < width; ++X) {
      U32 C0 = swap_rb(*Pix0);
      U32 C1 = swap_rb(*Pix1);

      *Pix0++ = C1;
      *Pix1++ = C0;
    }
    Row0 += width;
    Row1 -= width;
  }
}

#define CONSUME(file, type) (type *)consume_size(file, sizeof(type))

function void * consume_size(StreamingBuffer *file, U32 size) {
  void *Result = 0;

  if ((file->size == 0) && file->first) {
    StreamingChunk *This = file->first;
    file->size = This->size;
    file->contents = This->contents;
    file->first = This->next;
  }

  if (file->size >= size) {
    Result = file->contents;
    file->contents = (U8 *)file->contents + size;
    file->size -= size;
  }
  else {
    //fprintf(stderr, "file underflow\n");
    file->size = 0;
    file->under_flowed = true;
  }
  assert(!file->under_flowed);
  return(Result);
}

function void endian_swap(U32 *value) {
  U32 v = (*value);
  *value = ((v << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) | (v >> 24));
}

function void endian_swap(U16 *value) {
  U16 v = (*value);
  *value = ((v << 8) | (v >> 8));
}

function void * allocate_pixels(U32 Width, U32 Height, U32 BPP, U32 ExtraBytes = 0) {
  void *Result = malloc(Width*Height*BPP + (ExtraBytes*Height));
  return(Result);
}

function StreamingChunk * allocate_chunk(void) {
  StreamingChunk *Result = (StreamingChunk *)malloc(sizeof(StreamingChunk));
  return(Result);
}

function PNGHuffman allocate_huffman(U32 MaxCodeLengthInBits) {
  assert(MaxCodeLengthInBits <= PNG_HUFFMAN_MAX_BIT_COUNT);
  PNGHuffman Result = {};
  Result.MaxCodeLengthInBits = MaxCodeLengthInBits;
  Result.EntryCount = (1 << MaxCodeLengthInBits);
  Result.Entries = (PNGHuffmanEntry *)malloc(sizeof(PNGHuffmanEntry)*Result.EntryCount);

  return(Result);
}
function void flush_byte(StreamingBuffer *Buf) {
  Buf->bit_count = 0;
  Buf->bit_buf = 0;
}

function U32 peek_bits(StreamingBuffer *buf, U32 bit_count) {
  assert(bit_count <= 32);

  U32 result = 0;
  while ((buf->bit_count < bit_count) && !buf->under_flowed) {
    U32 byte = *CONSUME(buf, U8);
    buf->bit_buf |= (byte << buf->bit_count);
    buf->bit_count += 8;
  }
  result = buf->bit_buf & ((1 << bit_count) - 1);

  return(result);
}

function void discard_bits(StreamingBuffer *Buf, U32 bit_count) {
  Buf->bit_count -= bit_count;
  Buf->bit_buf >>= bit_count;
}

function U32 consume_bits(StreamingBuffer *buf, U32 bit_count) {
  U32 Result = peek_bits(buf, bit_count);
  discard_bits(buf, bit_count);

  return(Result);
}

function U32 reverse_bits(U32 V, U32 BitCount) {
  U32 result = 0;

  for (U32 BitIndex = 0; BitIndex <= (BitCount / 2); ++BitIndex) {
    U32 Inv = (BitCount - (BitIndex + 1));
    result |= ((V >> BitIndex) & 0x1) << Inv;
    result |= ((V >> Inv) & 0x1) << BitIndex;
  }
  return(result);
}

function void compute_huffman(U32 SymbolCount, U32 *SymbolCodeLength, PNGHuffman *Result, U32 SymbolAddend = 0) {
  U32 CodeLengthHist[PNG_HUFFMAN_MAX_BIT_COUNT] = {};

  for (U32 SymbolIndex = 0; SymbolIndex < SymbolCount; ++SymbolIndex) {
    U32 Count = SymbolCodeLength[SymbolIndex];
    assert(Count <= array_count(CodeLengthHist));
    ++CodeLengthHist[Count];
  }

  U32 NextUnusedCode[PNG_HUFFMAN_MAX_BIT_COUNT];
  NextUnusedCode[0] = 0;
  CodeLengthHist[0] = 0;
  for (U32 BitIndex = 1; BitIndex < array_count(NextUnusedCode); ++BitIndex) {
    NextUnusedCode[BitIndex] = ((NextUnusedCode[BitIndex - 1] + CodeLengthHist[BitIndex - 1]) << 1);
  }

  for (U32 SymbolIndex = 0; SymbolIndex < SymbolCount; ++SymbolIndex) {
    U32 CodeLengthInBits = SymbolCodeLength[SymbolIndex];
    if (CodeLengthInBits) {
      assert(CodeLengthInBits < array_count(NextUnusedCode));
      U32 Code = NextUnusedCode[CodeLengthInBits]++;

      U32 ArbitraryBits = Result->MaxCodeLengthInBits - CodeLengthInBits;
      U32 EntryCount = (1 << ArbitraryBits);

      for (U32 EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex) {
	U32 BaseIndex = (Code << ArbitraryBits) | EntryIndex;
	U32 Index = reverse_bits(BaseIndex, Result->MaxCodeLengthInBits);

	PNGHuffmanEntry *Entry = Result->Entries + Index;
	U32 Symbol = (SymbolIndex + SymbolAddend);
	Entry->bits_used = (U16)CodeLengthInBits;
	Entry->symbol = (U16)Symbol;
	assert(Entry->bits_used == CodeLengthInBits);
	assert(Entry->symbol == Symbol);
      }
    }
  }
}

function U32 huffman_decode(PNGHuffman *Huffman, StreamingBuffer *Input) {
  U32 EntryIndex = peek_bits(Input, Huffman->MaxCodeLengthInBits);
  assert(EntryIndex < Huffman->EntryCount);

  PNGHuffmanEntry Entry = Huffman->Entries[EntryIndex];

  U32 Result = Entry.symbol;
  discard_bits(Input, Entry.bits_used);
  assert(Entry.bits_used);

  return(Result);
}

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

function U8 png_filter_1and2(U8 *x, U8 *a, U32 Channel) {
  U8 Result = (U8)x[Channel] + (U8)a[Channel];
  return(Result);
}

function U8 png_filter3(U8 *x, U8 *a, U8 *b, U32 Channel) {
  U32 Average = ((U32)a[Channel] + (U32)b[Channel]) / 2;
  U8 Result = (U8)x[Channel] + (U8)Average;
  return(Result);
}

function U8 png_filter4(U8 *x, U8 *aFull, U8 *bFull, U8 *cFull, U32 Channel) {
  S32 a = (S32)aFull[Channel];
  S32 b = (S32)bFull[Channel];
  S32 c = (S32)cFull[Channel];
  S32 p = a + b - c;

  S32 pa = p - a;
  if (pa < 0) { pa = -pa; }

  S32 pb = p - b;
  if (pb < 0) { pb = -pb; }

  S32 pc = p - c;
  if (pc < 0) { pc = -pc; }

  S32 Paeth;
  if ((pa <= pb) && (pa <= pc)) {
    Paeth = a;
  } else if (pb <= pc) {
    Paeth = b;
  } else {
    Paeth = c;
  }

  U8 Result = (U8)x[Channel] + (U8)Paeth;
  return(Result);
}

function void png_filter_reconstruct(U32 Width, U32 Height, U8 *DecompressedPixels, U8 *final_pixels) {
  // seems tailor-made for SIMD - you could go 4-wide trivially.
  U32 Zero = 0;
  U8 *PriorRow = (U8 *)&Zero;
  S32 PriorRowAdvance = 0;
  U8 *Source = DecompressedPixels;
  U8 *Dest = final_pixels;

  for (U32 Y = 0; Y < Height; ++Y) {
    U8 Filter = *Source++;
    U8 *CurrentRow = Dest;

    switch (Filter) {
    case 0: {
      for (U32 X = 0; X < Width; ++X) {

	*(U32 *)Dest = *(U32 *)Source;
	Dest += 4;
	Source += 4;
      }
    } break;

    case 1: {
      U32 APixel = 0;
      for (U32 X = 0; X < Width; ++X) {
	Dest[0] = png_filter_1and2(Source, (U8 *)&APixel, 0);
	Dest[1] = png_filter_1and2(Source, (U8 *)&APixel, 1);
	Dest[2] = png_filter_1and2(Source, (U8 *)&APixel, 2);
	Dest[3] = png_filter_1and2(Source, (U8 *)&APixel, 3);

	APixel = *(U32 *)Dest;

	Dest += 4;
	Source += 4;
      }
    } break;
    case 2: {
      U8 *BPixel = PriorRow;
      for (U32 X = 0; X < Width; ++X) {
	Dest[0] = png_filter_1and2(Source, BPixel, 0);
	Dest[1] = png_filter_1and2(Source, BPixel, 1);
	Dest[2] = png_filter_1and2(Source, BPixel, 2);
	Dest[3] = png_filter_1and2(Source, BPixel, 3);

	BPixel += PriorRowAdvance;
	Dest += 4;
	Source += 4;
      }
    } break;

    case 3: {
      U32 APixel = 0;
      U8 *BPixel = PriorRow;
      for (U32 X = 0; X < Width; ++X) {
	Dest[0] = png_filter3(Source, (U8 *)&APixel, BPixel, 0);
	Dest[1] = png_filter3(Source, (U8 *)&APixel, BPixel, 1);
	Dest[2] = png_filter3(Source, (U8 *)&APixel, BPixel, 2);
	Dest[3] = png_filter3(Source, (U8 *)&APixel, BPixel, 3);
	APixel = *(U32 *)Dest;
	BPixel += PriorRowAdvance;
	Dest += 4;
	Source += 4;
      }
    } break;

    case 4: {
      U32 APixel = 0;
      U32 CPixel = 0;
      U8 *BPixel = PriorRow;
      for (U32 X = 0; X < Width; ++X) {
	Dest[0] = png_filter4(Source, (U8 *)&APixel, BPixel, (U8 *)&CPixel, 0);
	Dest[1] = png_filter4(Source, (U8 *)&APixel, BPixel, (U8 *)&CPixel, 1);
	Dest[2] = png_filter4(Source, (U8 *)&APixel, BPixel, (U8 *)&CPixel, 2);
	Dest[3] = png_filter4(Source, (U8 *)&APixel, BPixel, (U8 *)&CPixel, 3);

	CPixel = *(U32 *)BPixel;
	APixel = *(U32 *)Dest;

	BPixel += PriorRowAdvance;
	Dest += 4;
	Source += 4;
      }
    } break;

    default: {
      assert(0);
      //"ERROR: Unrecognized row filter %u.\n"
    } break;

    }
    PriorRow = CurrentRow;
    PriorRowAdvance = 4;
  }
}

function ImageU32 parse_png(char* file_name) {
  // what we expect, and is happy to crash otherwise.
  ReadFileResult file_result = read_entire_file(file_name);
  StreamingBuffer file = {};

  file.contents = file_result.contents;
  file.size = file_result.content_size;


  StreamingBuffer *At = &file;
  B32 supported = false;

  for (U16 i = 0; i < array_count(ReversedBits); ++i) {
    ReversedBits[i] = (U16)reverse_bits(i, MAX_REVERSE_BITS_COUNT);
  }

  U32 width = 0;
  U32 height = 0;
  U8 *final_pixels = 0;

  png_header *FileHeader = CONSUME(At, png_header);
  if (FileHeader) {
    StreamingBuffer CompData = {};
    while (At->size > 0) {
      PNGChunkHeader *ChunkHeader = CONSUME(At, PNGChunkHeader);
      if (ChunkHeader) {
	endian_swap(&ChunkHeader->Length);
	endian_swap(&ChunkHeader->TypeU32);

	void *chunk_data = consume_size(At, ChunkHeader->Length);
	PNGChunkFooter *ChunkFooter = CONSUME(At, PNGChunkFooter);
	endian_swap(&ChunkFooter->crc);

	if (ChunkHeader->TypeU32 == 'IHDR') {
	  //fprintf(stdout, "IHDR\n");

	  PNGIhdr *ihdr = (PNGIhdr*)chunk_data;

	  endian_swap(&ihdr->width);
	  endian_swap(&ihdr->height);

	  if ((ihdr->bit_depth == 8) && (ihdr->compression_method == 0) && (ihdr->filter_method == 0) && (ihdr->interlace_method == 0)) {
	    width = ihdr->width;
	    height = ihdr->height;
	    supported = true;
	  }

	}
	else if (ChunkHeader->TypeU32 == 'IDAT') {
	  //fprintf(stdout, "IDAT (%u)\n", ChunkHeader->Length);
	  StreamingChunk *chunk = allocate_chunk();
	  chunk->size = ChunkHeader->Length;
	  chunk->contents = chunk_data;
	  chunk->next = 0;
	  CompData.last = ((CompData.last ? CompData.last->next : CompData.first) = chunk);
	}
      }
    }

    if (supported) {
      PNGIdatHeader *idat_head = CONSUME(&CompData, PNGIdatHeader);
      U8 CM = (idat_head->zlib_method_flag & 0xF);
      U8 CINFO = (idat_head->zlib_method_flag >> 4);

      U8 FCHECK = (idat_head->additional_flag & 0x1F);
      U8 FDICT = (idat_head->additional_flag >> 5) & 0x1;
      U8 FLEVEL = (idat_head->additional_flag >> 6);

      supported = ((CM == 8) && (FDICT == 0));

      if (supported) {
	final_pixels = (U8 *)allocate_pixels(width, height, 4);
	U8 *DecompressedPixels = (U8 *)allocate_pixels(width, height, 4, 1);
	U8 *DecompressedPixelsEnd = DecompressedPixels + (height*((width * 4) + 1));
	U8 *Dest = DecompressedPixels;

	U32 BFINAL = 0;
	while (BFINAL == 0) {
	  BFINAL = consume_bits(&CompData, 1);
	  U32 BTYPE = consume_bits(&CompData, 2);

	  if (BTYPE == 0) {
	    flush_byte(&CompData);

	    U16 LEN = *CONSUME(&CompData, U16);
	    U16 NLEN = *CONSUME(&CompData, U16);
	    if ((U16)LEN != (U16)~NLEN) {
	      assert(0);
	      //fprintf(stderr, "ERROR: LEN/NLEN mismatch.\n");
	    }

	    U8 *Source = (U8 *)consume_size(&CompData, LEN);
	    if (Source) {
	      while (LEN--) {
		*Dest++ = *Source++;
	      }
	    }
	  }
	  else if (BTYPE == 3) {
	    assert(0);
	    //fprintf(stderr, "ERROR: BTYPE of %u encountered.\n", BTYPE);
	  }
	  else {
	    U32 LitLenDistTable[512];
	    PNGHuffman LitLenHuffman = allocate_huffman(15);
	    PNGHuffman DistHuffman = allocate_huffman(15);

	    U32 HLIT = 0;
	    U32 HDIST = 0;
	    if (BTYPE == 2) {
	      HLIT = consume_bits(&CompData, 5);
	      HDIST = consume_bits(&CompData, 5);
	      U32 HCLEN = consume_bits(&CompData, 4);

	      HLIT += 257;
	      HDIST += 1;
	      HCLEN += 4;

	      U32 HCLENSwizzle[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15, };

	      assert(HCLEN <= array_count(HCLENSwizzle));
	      U32 HCLENTable[array_count(HCLENSwizzle)] = {};

	      for (U32 Index = 0; Index < HCLEN; ++Index) {
		HCLENTable[HCLENSwizzle[Index]] = consume_bits(&CompData, 3);
	      }

	      PNGHuffman DictHuffman = allocate_huffman(7);
	      compute_huffman(array_count(HCLENSwizzle), HCLENTable, &DictHuffman);

	      U32 LitLenCount = 0;
	      U32 LenCount = HLIT + HDIST;
	      assert(LenCount <= array_count(LitLenDistTable));

	      while (LitLenCount < LenCount) {
		U32 RepCount = 1;
		U32 RepVal = 0;
		U32 EncodedLen = huffman_decode(&DictHuffman, &CompData);

		if (EncodedLen <= 15) {
		  RepVal = EncodedLen;
		}
		else if (EncodedLen == 16) {
		  RepCount = 3 + consume_bits(&CompData, 2);

		  assert(LitLenCount > 0);
		  RepVal = LitLenDistTable[LitLenCount - 1];
		}
		else if (EncodedLen == 17) {
		  RepCount = 3 + consume_bits(&CompData, 3);
		}
		else if (EncodedLen == 18) {
		  RepCount = 11 + consume_bits(&CompData, 7);
		}
		else {
		  assert(0);
		  //fprintf(stderr, "ERROR: EncodedLen of %u encountered.\n", EncodedLen);
		}

		while (RepCount--) {
		  LitLenDistTable[LitLenCount++] = RepVal;
		}
	      }
	      assert(LitLenCount == LenCount);
	    }
	    else if (BTYPE == 1) {
	      HLIT = 288;
	      HDIST = 32;

	      U32 BitCounts[][2] = { {143, 8}, {255, 9}, {279, 7}, {287, 8}, {319, 5}, };
	      U32 BitCountIndex = 0;
	      for (U32 RangeIndex = 0; RangeIndex < array_count(BitCounts); ++RangeIndex) {
		U32 BitCount = BitCounts[RangeIndex][1];
		U32 LastValue = BitCounts[RangeIndex][0];

		while (BitCountIndex <= LastValue) {
		  LitLenDistTable[BitCountIndex++] = BitCount;
		}
	      }
	    }
	    else {
	      assert(0);
	      //fprintf(stderr, "ERROR: BTYPE of %u encountered.\n", BTYPE);
	    }

	    compute_huffman(HLIT, LitLenDistTable, &LitLenHuffman);
	    compute_huffman(HDIST, LitLenDistTable + HLIT, &DistHuffman);

	    for (;;) {
	      U32 LitLen = huffman_decode(&LitLenHuffman, &CompData);

	      if (LitLen <= 255) {
		U32 Out = (LitLen & 0xFF);
		*Dest++ = (U8)Out;
	      }
	      else if (LitLen >= 257) {
		U32 LenTabIndex = (LitLen - 257);
		PNGHuffmanEntry LenTab = PNGLengthExtra[LenTabIndex];
		U32 Len = LenTab.symbol;
		if (LenTab.bits_used) {
		  U32 ExtraBits = consume_bits(&CompData, LenTab.bits_used);
		  Len += ExtraBits;
		}

		U32 DistTabIndex = huffman_decode(&DistHuffman, &CompData);
		PNGHuffmanEntry DistTab = PNGDistExtra[DistTabIndex];
		U32 Distance = DistTab.symbol;
		if (DistTab.bits_used) {
		  U32 ExtraBits = consume_bits(&CompData, DistTab.bits_used);
		  Distance += ExtraBits;
		}

		U8 *Source = Dest - Distance;
		assert((Source + Len) <= DecompressedPixelsEnd);
		assert((Dest + Len) <= DecompressedPixelsEnd);
		assert(Source >= DecompressedPixels);

		while (Len--) {
		  *Dest++ = *Source++;
		}
	      }
	      else {
		break;
	      }
	    }
	  }
	}
	assert(Dest == DecompressedPixelsEnd);
	png_filter_reconstruct(width, height, DecompressedPixels, final_pixels);
      }
    }
  }
  ImageU32 result = {};
  result.width = width;
  result.height = height;
  result.pixels = (U32 *)final_pixels;
  write_image_top_down_rgba(result.width, result.height, (U8*)result.pixels);
  return(result);
}
