#ifndef ASSET

struct LoadedSound{
  U32 sample_count;
  U32 channel_count;
  S16* samples[2];
};

#pragma pack(push,1)

struct WAVE_header{
  U32 RIFFID;
  U32 size;
  U32 WAVED;
};

#define RIFF_CODE(a, b, c, d) (((U32)(a) << 0) | ((U32)(b) << 8) | ((U32)(c) << 16) | ((U32)(d) << 24))

enum{
  WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
  WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
  WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
  WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};

struct WAVE_chunk{
  U32 ID;
  U32 size;
};

struct WAVE_fmt {
    U16 wFormatTag;
    U16 nChannels;
    U32 nSamplesPerSec;
    U32 nAvgBytesPerSec;
    U16 nBlockAlign;
    U16 wBitsPerSample;
    U16 cbSize;
    U16 wValidBitsPerSample;
    U32 dwChannelMask;
    U8 SubFormat[16];
};
#pragma pack(pop)




#define ASSET
#endif
