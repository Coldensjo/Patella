// graphics.h: the sprites of Tibia.spr, the pictures of Tibia.pic, the fonts
// (cut from pictures), the memory cache of decoded 32x32 tiles, the video
// cache of tiles on the engine's cache surfaces, and the drawing functions
// that put sprites, objects, outfits, pictures and text on a surface.
//
// A sprite is 32x32 pixels; a picture is TilesX x TilesY tiles of 32x32.
// Decoded tiles are 32*32 RGB triples (3072 bytes). The memory cache holds
// MemoryCacheSize of them (InitGraphics' MemoryCacheBytes / 3072), the video
// cache VideoCacheSize slots on the cache surfaces (surfaces 6, 7, ...); both
// are LRU lists with a hash table of HASH_TABLE_SIZE chains.
//
// Names from the binary (assertion and catch strings): FileName, PATH_MAX,
// SpriteNumber, NumberOfSprites, MaskNumber, PictureNumber,
// NumberOfPictures, TileX, TileY, Picture.TilesX, Picture.TilesY, Buffer,
// Position, MemoryCacheSize, VideoCacheSize, Item.Type, EMPTY_ITEM, HashValue,
// HASH_TABLE_SIZE, FontNumber, NumberOfFonts, FontName, CharsPerLine,
// CharWidth, CharHeight, Surface, Width, Height, Text, Length, Alignment,
// LEFT_ALIGN, CENTER_ALIGN, RIGHT_ALIGN, Format.Alignment, MemoryCacheBytes.
// Everything else is ours (notes/decisions.md, "graphics.cpp").

#pragma once

#include "common.h"
#include "external.h" // TRGBColor, TRect, TFont, TObject

#define PATH_MAX 1024
#define HASH_TABLE_SIZE 100

// Text alignment (TFont::Alignment, the Alignment parameters).
enum {
	LEFT_ALIGN = 0,
	CENTER_ALIGN = 1,
	RIGHT_ALIGN = 2,
};

// Kinds of cache items. Pictures and font characters are both PICTURE_ITEM.
enum {
	EMPTY_ITEM = 0,
	SPRITE_ITEM = 1,
	PICTURE_ITEM = 2,
};

// A sprite of Tibia.spr (0x10 bytes). Offset 0 is an empty sprite.
struct TSprite {
	TSprite() : ColorKey(0, 0, 0) {}
	int Offset;          // +0x00, in Tibia.spr
	TRGBColor ColorKey;  // +0x04, the transparent colour, from the sprite's first three bytes
};

// A picture of Tibia.pic (0x18 bytes); "Picture" in the assertions.
struct TPicture {
	TPicture() : ColorKey(0, 0, 0) {}
	int TilesX;          // +0x00
	int TilesY;          // +0x04
	int *Offsets;        // +0x08, TilesX*TilesY offsets in Tibia.pic, row by row
	TRGBColor ColorKey;  // +0x0c
};

// A font cut from a picture (0x71c bytes). Characters 33-127 have the
// indices 1-95, characters 128-255 the indices 96-223.
struct TFontInfo {
	int PictureNumber;   // +0x000
	int CharsPerLine;    // +0x004
	int CharWidth;       // +0x008
	int CharHeight;      // +0x00c
	int Widths[224];     // +0x010, the visible width of each character
	int Heights[224];    // +0x390, the visible height of each character
	int Spacing;         // +0x710, added after every character
	int Height;          // +0x714, the highest visible height
	int LineHeight;      // +0x718
};

// A decoded tile in the memory cache (0x28 bytes); "Item" in the assertions.
struct TMemoryCacheItem {
	uint8 *Data;         // +0x00, 32*32 RGB
	int Type;            // +0x04
	int Number;          // +0x08, sprite or picture number
	int TileX;           // +0x0c
	int TileY;           // +0x10
	int Previous;        // +0x14, LRU list: towards the most recent item
	int Next;            // +0x18
	int HashPrevious;    // +0x1c
	int HashNext;        // +0x20
	int HashValue;       // +0x24
};

// A tile on a cache surface (0x5c bytes). Masked sprites (outfits) keep the
// mask and four colours, pictures the tile, font characters the tile and the
// text colour, in the same place.
struct TVideoCacheItem {
	TVideoCacheItem();
	int Surface;         // +0x00, cache surface; the engine's surface number is Surface+6
	int x;               // +0x04
	int y;               // +0x08
	int Type;            // +0x0c
	int Number;          // +0x10, sprite or picture number
	union {
		int MaskNumber;  // +0x14, sprites; 0 without mask
		int TileX;       // +0x14, pictures
	};
	union {
		TRGBColor Colors[4];     // +0x18, masked sprites
		struct {
			int TileY;           // +0x18, pictures
			TRGBColor Color;     // +0x1c, font characters; 0,0,0 for pictures
		};
	};
	int Previous;        // +0x48
	int Next;            // +0x4c
	int HashPrevious;    // +0x50
	int HashNext;        // +0x54
	int HashValue;       // +0x58
};

// Data. Only graphics.cpp uses them except the two signatures (sent at login
// and when patching); the others are declared for the differential tests.
extern int MemoryHashTable[HASH_TABLE_SIZE];  // 0x5cc430
extern int VideoHashTable[HASH_TABLE_SIZE];   // 0x5cc5c0
extern int NumberOfFonts;                     // 0x5cc750
extern int NumberOfPictures;                  // 0x5cc754
extern uint32 PictureSignature;               // 0x5cc758
extern char PictureFileName[PATH_MAX];        // 0x5cc760
extern int NumberOfSprites;                   // 0x5ccb60
extern uint32 SpriteSignature;                // 0x5ccb64
extern char SpriteFileName[PATH_MAX];         // 0x5ccb68
extern int LeastRecentMemoryItem;             // 0x5ccf68
extern int MostRecentMemoryItem;              // 0x5ccf6c
extern int MemoryCacheSize;                   // 0x5ccf70
extern uint8 *MemoryCacheData;                // 0x5ccf74
extern int LeastRecentVideoItem;              // 0x5ccf78
extern int MostRecentVideoItem;               // 0x5ccf7c
extern int VideoCacheSize;                    // 0x5ccf80
extern TVideoCacheItem *VideoCache;           // 0x5ccf84
extern TMemoryCacheItem *MemoryCache;         // 0x5ccf88
extern TSprite *Sprites;                      // 0x5ccf8c
extern TPicture *Pictures;                    // 0x5ccf90
extern TFontInfo *Fonts;                      // 0x5ccf94

// Start-up and shut-down.
void InitGraphics(int MemoryCacheBytes);
void InitVideoCache();
void ResetVideoCache();
void ReloadSprites();
void ReloadPictures();
void ExitGraphics();

// The files and the fonts.
void LoadSprites(const char *FileName);
void LoadSprite(int SpriteNumber, uint8 *Buffer);
void LoadPictures(const char *FileName);
void LoadPictureTile(int PictureNumber, int TileX, int TileY, uint8 *Buffer);
void InitFont(int FontName, int PictureNumber, int CharsPerLine, int CharWidth, int CharHeight,
	int Spacing, int LineHeight);
void InitFonts();

// The memory cache.
void InitMemoryCache(int MemoryCacheBytes);
void ClearMemoryCache();
void TouchMemoryItem(int Position);
int FindMemorySprite(int SpriteNumber);
int FindMemoryPicture(int PictureNumber, int TileX, int TileY);
void RemoveMemoryItem(int Position);
void InsertMemoryItem(int Position, int HashValue);
TMemoryCacheItem *GetMemorySprite(int SpriteNumber);
TMemoryCacheItem *GetMemoryPicture(int PictureNumber, int TileX, int TileY);

// The video cache.
void TouchVideoItem(int Position);
int FindVideoSprite(int SpriteNumber);
int FindVideoMaskedSprite(int SpriteNumber, int MaskNumber, TRGBColor *Colors);
int FindVideoPicture(int PictureNumber, int TileX, int TileY);
int FindVideoFontChar(int PictureNumber, int TileX, int TileY, TRGBColor Color);
void RemoveVideoItem(int Position);
void InsertVideoItem(int Position, int HashValue);
TVideoCacheItem *GetVideoSprite(int SpriteNumber);
TVideoCacheItem *GetVideoMaskedSprite(int SpriteNumber, int MaskNumber, TRGBColor *Colors);
TVideoCacheItem *GetVideoPicture(int PictureNumber, int TileX, int TileY);
TVideoCacheItem *GetVideoFontChar(int FontNumber, int TileX, int TileY, TRGBColor Color);

// Sprites, objects and outfits. Sprite number 0 draws nothing (except in the
// masked forms, where it fails the assertion).
void DrawSprite(int Surface, int x, int y, int SpriteNumber);
void DrawSpriteClipped(int Surface, int x, int y, int SpriteNumber, TRect Clip);
void DrawSpriteScaled(int Surface, int x, int y, int Width, int Height, int SpriteNumber,
	TRGBColor Highlight, TRect Clip);
void DrawMaskedSprite(int Surface, int x, int y, int SpriteNumber, int MaskNumber, TRGBColor *Colors);
void DrawMaskedSpriteScaled(int Surface, int x, int y, int Width, int Height, int SpriteNumber,
	int MaskNumber, TRGBColor *Colors, TRGBColor Highlight, TRect Clip);
void DrawObject(int Surface, int x, int y, int Size, TObject Obj, TRGBColor Highlight, TRect Clip,
	TFont Font, bool ShowCount);
void DrawOutfit(int Surface, int x, int y, int Size, int OutfitNumber, int *Colors,
	TRGBColor Highlight, TRect Clip, TFont Font);

// Pictures.
void DrawPicture(int Surface, int x, int y, int PictureNumber);
void DrawPicturePart(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
	int Width, int Height);
void DrawPicturePartClipped(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
	int Width, int Height, TRect Clip);

// Text. Characters 10 and 32 are blanks, other control characters show
// character 127.
void PrintTextPart(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
	int Length, int Alignment);
void PrintTextClipped(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
	int Alignment, TRect Clip);
void PrintText(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
	int Alignment);
void PrintFormattedText(int Surface, int x, int y, TFont Format, const char *Text);
void PrintFormattedTextClipped(int Surface, int x, int y, TFont Format, const char *Text, TRect Clip);
void PrintFormattedTextPart(int Surface, int x, int y, TFont Format, const char *Text, int Length);
int GetFontLineHeight(int FontNumber);
int GetFontHeight(int FontNumber);
int GetTextHeight(int FontNumber, const char *Text);
int GetTextWidth(int FontNumber, const char *Text);
int GetTextPartWidth(int FontNumber, const char *Text, int Length);
int GetFittingTextLength(int FontNumber, const char *Text, int Width);
void GetTextPosition(int *TextX, int *TextY, int x, int y, int Width, int Height, TFont *Format);
