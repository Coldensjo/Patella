// Differential tests of graphics.cpp against the original code.
//
// The image's malloc/free and stdio are redirected (PatchScript), so both
// sides load the real Tibia.spr and Tibia.pic (next to Tibia.exe): every
// sprite and every picture tile is decoded on both sides and compared. The
// surface is a recording fake: both sides' GetSurface return the same
// TSurfaceGL object (the image's surface pointer 0x71d0cc is set to it), whose
// slots write their arguments, and the tiles they get, to a log per side.
// After each operation the logs, the memory cache, the video cache, their
// hash tables and LRU lists are compared. The caches are small so that items
// are replaced all the time (and the video cache's slot 0 bug is reached).
// Objects and outfits use the real Tibia.dat, loaded on both sides.
// Uninitialised fields of freshly allocated cache items (never read before
// they are written) are cleared on both sides so that they can be compared.
// Failing assertions are not exercised.

#include "difftest.h"
#include "../../src/graphics.h"
#include "../../src/Objects.h"
#include "../../src/Status.h"
#include "../../src/Surface.h"
#include <direct.h>
#include <string>
#include <vector>

static_assert(sizeof(TRGBColor) == 12, "TRGBColor is 12 bytes");
static_assert(sizeof(TRect) == 16, "TRect is 16 bytes");
static_assert(sizeof(TFont) == 20, "TFont is 20 bytes");
static_assert(sizeof(TSprite) == 0x10, "TSprite is 0x10 bytes");
static_assert(sizeof(TPicture) == 0x18, "TPicture is 0x18 bytes");
static_assert(sizeof(TFontInfo) == 0x71c, "TFontInfo is 0x71c bytes");
static_assert(sizeof(TMemoryCacheItem) == 0x28, "TMemoryCacheItem is 0x28 bytes");
static_assert(sizeof(TVideoCacheItem) == 0x5c, "TVideoCacheItem is 0x5c bytes");
static_assert(offsetof(TVideoCacheItem, Color) == 0x1c, "TVideoCacheItem::Color at +0x1c");
static_assert(offsetof(TVideoCacheItem, Previous) == 0x48, "TVideoCacheItem::Previous at +0x48");

// --- The recording surface -------------------------------------------------

enum { OURS = 0, ORIGINAL = 1 };
static int Side;
static std::string Log[2];
static int CacheSurfaces = 2;
static int CacheWidth = 96;
static int CacheHeight = 64;
static int Writes, Blits, DecodedSprites, DecodedTiles; // our side's statistics

static void Put(int Value) { Log[Side].append((const char *)&Value, sizeof(Value)); }
static void PutColor(TRGBColor Color) { Put(Color.R); Put(Color.G); Put(Color.B); }
static void PutRect(TRect Rect) { Put(Rect.x); Put(Rect.y); Put(Rect.Width); Put(Rect.Height); }
static void PutTile(const uint8 *Data)
{
	if (Data == NULL)
		Put(-1);
	else
		Log[Side].append((const char *)Data, 32 * 32 * 3);
}

TSurfaceGL::TSurfaceGL() {}
void TSurfaceGL::WriteImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data)
{
	Writes += Side == OURS;
	Put(0x28); Put(ToSurface); Put(x); Put(y); Put(Width); Put(Height); PutTile(Data);
}
void TSurfaceGL::WriteMaskedImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data,
	uint8 *Mask, TRGBColor *Colors)
{
	Writes += Side == OURS;
	Put(0x2c); Put(ToSurface); Put(x); Put(y); Put(Width); Put(Height); PutTile(Data); PutTile(Mask);
	for (int i = 0; i < 4; i++)
		PutColor(Colors[i]);
}
void TSurfaceGL::Blit(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width,
	int Height, TRGBColor ColorKey)
{
	Blits += Side == OURS;
	Put(0x34); Put(ToSurface); Put(x); Put(y); Put(FromSurface); Put(FromX); Put(FromY); Put(Width);
	Put(Height); PutColor(ColorKey);
}
void TSurfaceGL::BlitClipped(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY,
	int Width, int Height, TRGBColor ColorKey, TRect Clip)
{
	Blits += Side == OURS;
	Put(0x3c); Put(ToSurface); Put(x); Put(y); Put(FromSurface); Put(FromX); Put(FromY); Put(Width);
	Put(Height); PutColor(ColorKey); PutRect(Clip);
}
void TSurfaceGL::StretchBlit(int ToSurface, int x, int y, int Width, int Height, int FromSurface,
	int FromX, int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight,
	TRect Clip)
{
	Blits += Side == OURS;
	Put(0x48); Put(ToSurface); Put(x); Put(y); Put(Width); Put(Height); Put(FromSurface); Put(FromX);
	Put(FromY); Put(FromWidth); Put(FromHeight); PutColor(ColorKey); PutColor(Highlight); PutRect(Clip);
}
void TSurfaceGL::Unknown54(int a, int b, int c, int d)
{
	Put(0x54); Put(a); Put(b); Put(c); Put(d);
}
int TSurfaceGL::GetCacheSurfaces()
{
	Put(0x90);
	return CacheSurfaces;
}
void TSurfaceGL::GetSurfaceRect(int Surface, TRect *Rect)
{
	Put(0x94); Put(Surface); PutRect(*Rect);
	Rect->Width = CacheWidth;
	Rect->Height = CacheHeight;
}

// --- Original functions and data ------------------------------------------------

typedef void (__cdecl *VoidFn)();
typedef void (__cdecl *LoadFn)(const char *);
typedef void (__cdecl *IntFn)(int);
typedef int (__cdecl *IntRetFn)(int);
typedef void (__cdecl *Int2Fn)(int, int);
typedef int (__cdecl *Find3Fn)(int, int, int);
typedef void (__cdecl *LoadSpriteFn)(int, uint8 *);
typedef void (__cdecl *LoadTileFn)(int, int, int, uint8 *);
typedef TMemoryCacheItem *(__cdecl *MemSpriteFn)(int);
typedef TMemoryCacheItem *(__cdecl *MemPictureFn)(int, int, int);
typedef void (__cdecl *DrawSpriteFn)(int, int, int, int);
typedef void (__cdecl *DrawClippedFn)(int, int, int, int, TRect);
typedef void (__cdecl *DrawScaledFn)(int, int, int, int, int, int, TRGBColor, TRect);
typedef void (__cdecl *DrawMaskedFn)(int, int, int, int, int, TRGBColor *);
typedef void (__cdecl *DrawMaskedScaledFn)(int, int, int, int, int, int, int, TRGBColor *, TRGBColor, TRect);
typedef void (__cdecl *DrawObjectFn)(int, int, int, int, TObject, TRGBColor, TRect, TFont, bool);
typedef void (__cdecl *DrawOutfitFn)(int, int, int, int, int, int *, TRGBColor, TRect, TFont);
typedef void (__cdecl *PartFn)(int, int, int, int, int, int, int, int);
typedef void (__cdecl *PartClippedFn)(int, int, int, int, int, int, int, int, TRect);
typedef void (__cdecl *TextPartFn)(int, int, int, int, TRGBColor, const char *, int, int);
typedef void (__cdecl *TextClippedFn)(int, int, int, int, TRGBColor, const char *, int, TRect);
typedef void (__cdecl *TextFn)(int, int, int, int, TRGBColor, const char *, int);
typedef void (__cdecl *FormatFn)(int, int, int, TFont, const char *);
typedef void (__cdecl *FormatClippedFn)(int, int, int, TFont, const char *, TRect);
typedef void (__cdecl *FormatPartFn)(int, int, int, TFont, const char *, int);
typedef int (__cdecl *WidthFn)(int, const char *);
typedef int (__cdecl *PartWidthFn)(int, const char *, int);
typedef void (__cdecl *PositionFn)(int *, int *, int, int, int, int, TFont *);

#define ORIGINAL_DATA(Type, Name, Address) static Type &O##Name = Global<Type>(Address)
ORIGINAL_DATA(int, NumberOfFonts, 0x5cc750);
ORIGINAL_DATA(int, NumberOfPictures, 0x5cc754);
ORIGINAL_DATA(uint32, PictureSignature, 0x5cc758);
ORIGINAL_DATA(int, NumberOfSprites, 0x5ccb60);
ORIGINAL_DATA(uint32, SpriteSignature, 0x5ccb64);
ORIGINAL_DATA(int, LeastRecentMemoryItem, 0x5ccf68);
ORIGINAL_DATA(int, MostRecentMemoryItem, 0x5ccf6c);
ORIGINAL_DATA(int, MemoryCacheSize, 0x5ccf70);
ORIGINAL_DATA(uint8 *, MemoryCacheData, 0x5ccf74);
ORIGINAL_DATA(int, LeastRecentVideoItem, 0x5ccf78);
ORIGINAL_DATA(int, MostRecentVideoItem, 0x5ccf7c);
ORIGINAL_DATA(int, VideoCacheSize, 0x5ccf80);
ORIGINAL_DATA(TVideoCacheItem *, VideoCache, 0x5ccf84);
ORIGINAL_DATA(TMemoryCacheItem *, MemoryCache, 0x5ccf88);
ORIGINAL_DATA(TSprite *, Sprites, 0x5ccf8c);
ORIGINAL_DATA(TPicture *, Pictures, 0x5ccf90);
ORIGINAL_DATA(TFontInfo *, Fonts, 0x5ccf94);
static int *OMemoryHashTable = (int *)(uintptr_t)0x5cc430;
static int *OVideoHashTable = (int *)(uintptr_t)0x5cc5c0;
static const char *OSpriteFileName = (const char *)(uintptr_t)0x5ccb68;
static const char *OPictureFileName = (const char *)(uintptr_t)0x5cc760;

// --- Comparisons -----------------------------------------------------------------

static void CompareLogs(const char *What)
{
	bool Same = Log[OURS] == Log[ORIGINAL];
	if (!Same) {
		size_t i = 0;
		while (i < Log[OURS].size() && i < Log[ORIGINAL].size() && Log[OURS][i] == Log[ORIGINAL][i])
			i++;
		CHECK(Same, "%s: surface calls differ at byte %u (%u/%u bytes)", What, (unsigned)i,
			(unsigned)Log[OURS].size(), (unsigned)Log[ORIGINAL].size());
	} else {
		CHECK(Same, "%s", What);
	}
	Log[OURS].clear();
	Log[ORIGINAL].clear();
}

static void CompareSprites(const char *What)
{
	CHECK(SpriteSignature == OSpriteSignature && NumberOfSprites == ONumberOfSprites,
		"%s: sprite file %x/%d, original %x/%d", What, SpriteSignature, NumberOfSprites, OSpriteSignature,
		ONumberOfSprites);
	CHECK(strcmp(SpriteFileName, OSpriteFileName) == 0, "%s: sprite file name", What);
	if (NumberOfSprites != ONumberOfSprites || Sprites == NULL || OSprites == NULL)
		return;
	CHECK(memcmp(Sprites, OSprites, NumberOfSprites * sizeof(TSprite)) == 0, "%s: sprite table differs", What);
}

static void ComparePictures(const char *What)
{
	CHECK(PictureSignature == OPictureSignature && NumberOfPictures == ONumberOfPictures,
		"%s: picture file %x/%d, original %x/%d", What, PictureSignature, NumberOfPictures,
		OPictureSignature, ONumberOfPictures);
	CHECK(strcmp(PictureFileName, OPictureFileName) == 0, "%s: picture file name", What);
	if (NumberOfPictures != ONumberOfPictures || Pictures == NULL || OPictures == NULL)
		return;
	for (int i = 0; i < NumberOfPictures; i++) {
		TPicture &A = Pictures[i], &B = OPictures[i];
		bool Same = A.TilesX == B.TilesX && A.TilesY == B.TilesY && A.ColorKey == B.ColorKey
			&& memcmp(A.Offsets, B.Offsets, A.TilesX * A.TilesY * sizeof(int)) == 0;
		CHECK(Same, "%s: picture %d differs", What, i + 1);
	}
}

static void CompareFonts(const char *What)
{
	CHECK(NumberOfFonts == ONumberOfFonts, "%s: %d fonts, original %d", What, NumberOfFonts, ONumberOfFonts);
	if (Fonts == NULL || OFonts == NULL) {
		CHECK(Fonts == NULL && OFonts == NULL, "%s: font table presence", What);
		return;
	}
	for (int i = 0; i < NumberOfFonts && i < ONumberOfFonts; i++)
		CHECK(memcmp(&Fonts[i], &OFonts[i], sizeof(TFontInfo)) == 0, "%s: font %d differs", What, i + 1);
}

static void CompareMemoryCache(const char *What)
{
	CHECK(MemoryCacheSize == OMemoryCacheSize && LeastRecentMemoryItem == OLeastRecentMemoryItem
		&& MostRecentMemoryItem == OMostRecentMemoryItem, "%s: memory cache %d %d-%d, original %d %d-%d",
		What, MemoryCacheSize, MostRecentMemoryItem, LeastRecentMemoryItem, OMemoryCacheSize,
		OMostRecentMemoryItem, OLeastRecentMemoryItem);
	CHECK(memcmp(MemoryHashTable, OMemoryHashTable, sizeof(MemoryHashTable)) == 0, "%s: memory hash table", What);
	if (MemoryCacheSize != OMemoryCacheSize || MemoryCache == NULL || OMemoryCache == NULL)
		return;
	for (int i = 0; i < MemoryCacheSize; i++) {
		TMemoryCacheItem A = MemoryCache[i], B = OMemoryCache[i];
		bool Same = A.Data - MemoryCacheData == B.Data - OMemoryCacheData;
		A.Data = B.Data = NULL;
		Same = Same && memcmp(&A, &B, sizeof(A)) == 0;
		CHECK(Same, "%s: memory cache item %d differs", What, i);
	}
	CHECK(memcmp(MemoryCacheData, OMemoryCacheData, MemoryCacheSize * 32 * 32 * 3) == 0,
		"%s: memory cache data differs", What);
}

static void CompareVideoCache(const char *What)
{
	CHECK((VideoCache == NULL) == (OVideoCache == NULL), "%s: video cache presence", What);
	CHECK(VideoCacheSize == OVideoCacheSize && LeastRecentVideoItem == OLeastRecentVideoItem
		&& MostRecentVideoItem == OMostRecentVideoItem, "%s: video cache %d %d-%d, original %d %d-%d",
		What, VideoCacheSize, MostRecentVideoItem, LeastRecentVideoItem, OVideoCacheSize,
		OMostRecentVideoItem, OLeastRecentVideoItem);
	CHECK(memcmp(VideoHashTable, OVideoHashTable, sizeof(VideoHashTable)) == 0, "%s: video hash table", What);
	if (VideoCache == NULL || OVideoCache == NULL || VideoCacheSize != OVideoCacheSize)
		return;
	for (int i = 0; i < VideoCacheSize; i++)
		CHECK(memcmp(&VideoCache[i], &OVideoCache[i], sizeof(TVideoCacheItem)) == 0,
			"%s: video cache item %d differs", What, i);
}

static void CompareCaches(const char *What)
{
	CompareLogs(What);
	CompareMemoryCache(What);
	CompareVideoCache(What);
}

// Clears the fields of new cache items that are written before they are
// read, on both sides.
static void ClearNewMemoryItems()
{
	for (int i = 0; i < MemoryCacheSize; i++) {
		MemoryCache[i].Number = MemoryCache[i].TileX = MemoryCache[i].TileY = 0;
		OMemoryCache[i].Number = OMemoryCache[i].TileX = OMemoryCache[i].TileY = 0;
	}
	memset(MemoryCacheData, 0, MemoryCacheSize * 32 * 32 * 3);
	memset(OMemoryCacheData, 0, OMemoryCacheSize * 32 * 32 * 3);
}

static void ClearNewVideoItems()
{
	if (VideoCache == NULL || OVideoCache == NULL)
		return;
	for (int i = 0; i < VideoCacheSize; i++) {
		VideoCache[i].Number = VideoCache[i].MaskNumber = 0;
		OVideoCache[i].Number = OVideoCache[i].MaskNumber = 0;
	}
}

// --- Inputs ----------------------------------------------------------------------

static int Range(int Min, int Max)
{
	return Min + (int)(Random() % (uint32)(Max - Min + 1));
}

static TRGBColor RandomColor()
{
	return TRGBColor(Range(0, 255), Range(0, 255), Range(0, 255));
}

static TRect RandomRect()
{
	TRect Rect = {Range(-20, 300), Range(-20, 300), Range(0, 200), Range(0, 200)};
	return Rect;
}

static TFont RandomFont()
{
	TFont Font = {Range(1, 4), RandomColor(), Range(0, 2)};
	return Font;
}

// Any byte but 0, mostly letters, with blanks, newlines and control characters.
static void RandomString(char *Text, int Length)
{
	for (int i = 0; i < Length; i++) {
		switch (Random() % 4) {
		case 0: Text[i] = (char)Range(1, 255); break;
		case 1: Text[i] = " \n\t"[Random() % 3]; break;
		default: Text[i] = (char)Range('A', 'z'); break;
		}
	}
	Text[Length] = 0;
}

// Object types and outfits whose sprites can be drawn without failing an
// assertion (decided with our Objects.cpp; an exception skips the type).
static std::vector<int> DrawableObjects;
static std::vector<int> DrawableOutfits;

static bool SpriteOK(int SpriteNumber) { return SpriteNumber >= 0 && SpriteNumber <= NumberOfSprites; }

static void FindDrawables()
{
	for (int Type = ObjectProperties->min; Type < ObjectProperties->min + ObjectProperties->d; Type++) {
		try {
			if (GetObjectExactSize(Type) <= 0 || GetObjectPhases(Type) <= 0)
				continue;
			bool OK = true;
			for (int Phase = 1; Phase <= GetObjectPhases(Type) && OK; Phase++)
				for (int Layer = 1; Layer <= GetObjectLayers(Type) && OK; Layer++)
					for (int x = 1; x <= GetObjectWidth(Type) && OK; x++)
						for (int y = 1; y <= GetObjectHeight(Type) && OK; y++)
							for (int px = 1; px <= GetObjectPatternWidth(Type) && OK; px++)
								for (int py = 1; py <= GetObjectPatternHeight(Type) && OK; py++)
									OK = SpriteOK(GetObjectSpriteNumber(Type, x, y, Layer, px, py, 1, Phase));
			if (OK)
				DrawableObjects.push_back(Type);
		} catch (...) {
		}
	}
	for (int Outfit = OutfitProperties->min; Outfit < OutfitProperties->min + OutfitProperties->d; Outfit++) {
		try {
			if (GetOutfitExactSize(Outfit) <= 0)
				continue;
			bool OK = true;
			for (int x = 1; x <= GetOutfitWidth(Outfit) && OK; x++) {
				for (int y = 1; y <= GetOutfitHeight(Outfit) && OK; y++) {
					int Sprite = GetOutfitSpriteNumber(Outfit, x, y, 1, 2, 1);
					int Mask = GetOutfitLayers(Outfit) == 2 ? GetOutfitSpriteNumber(Outfit, x, y, 2, 2, 1) : 0;
					OK = SpriteOK(Sprite) && SpriteOK(Mask) && (Mask == 0 || Sprite >= 1);
				}
			}
			if (OK)
				DrawableOutfits.push_back(Outfit);
		} catch (...) {
		}
	}
}

static void SetEngine(int Engine)
{
	curEngine = Engine;
	Global<int>(0x71d0c4) = Engine;
}

// --- The tests -------------------------------------------------------------------

static void TestFiles(const std::string &Spr, const std::string &Pic)
{
	LoadSprites(Spr.c_str());
	At<LoadFn>(0x48b550)(Spr.c_str());
	CompareSprites("LoadSprites");
	static uint8 A[32 * 32 * 3], B[32 * 32 * 3];
	for (int s = 1; s <= NumberOfSprites && s <= ONumberOfSprites; s++) {
		memset(A, 0x55, sizeof(A));
		memset(B, 0x55, sizeof(B));
		LoadSprite(s, A);
		DecodedSprites++;
		At<LoadSpriteFn>(0x48b9a0)(s, B);
		CHECK(memcmp(A, B, sizeof(A)) == 0 && Sprites[s - 1].ColorKey == OSprites[s - 1].ColorKey,
			"LoadSprite(%d) differs", s);
	}
	LoadPictures(Pic.c_str());
	At<LoadFn>(0x48bea0)(Pic.c_str());
	ComparePictures("LoadPictures");
	for (int p = 1; p <= NumberOfPictures && p <= ONumberOfPictures; p++) {
		for (int ty = 0; ty < Pictures[p - 1].TilesY; ty++) {
			for (int tx = 0; tx < Pictures[p - 1].TilesX; tx++) {
				memset(A, 0x55, sizeof(A));
				memset(B, 0x55, sizeof(B));
				LoadPictureTile(p, tx, ty, A);
				DecodedTiles++;
				At<LoadTileFn>(0x48c3b0)(p, tx, ty, B);
				CHECK(memcmp(A, B, sizeof(A)) == 0, "LoadPictureTile(%d, %d, %d) differs", p, tx, ty);
			}
		}
	}
}

static void TestMemoryCache()
{
	InitMemoryCache(7 * 32 * 32 * 3 + 1000);
	At<IntFn>(0x48c9c0)(7 * 32 * 32 * 3 + 1000);
	ClearNewMemoryItems();
	CompareMemoryCache("InitMemoryCache");
	InitFonts();
	At<VoidFn>(0x492c30)();
	CompareFonts("InitFonts");
	CompareMemoryCache("InitFonts");
	for (int t = 0; t < 4000; t++) {
		int Op = Random() % 10;
		if (Op < 4) {
			int s = Range(1, NumberOfSprites);
			if (t % 3 == 0)
				s = Range(1, 30);
			TMemoryCacheItem *A = GetMemorySprite(s);
			TMemoryCacheItem *B = At<MemSpriteFn>(0x48d440)(s);
			CHECK(A - MemoryCache == B - OMemoryCache, "GetMemorySprite(%d): %d, original %d", s,
				(int)(A - MemoryCache), (int)(B - OMemoryCache));
		} else if (Op < 8) {
			int p = Range(1, NumberOfPictures);
			int tx = Range(0, Pictures[p - 1].TilesX - 1), ty = Range(0, Pictures[p - 1].TilesY - 1);
			TMemoryCacheItem *A = GetMemoryPicture(p, tx, ty);
			TMemoryCacheItem *B = At<MemPictureFn>(0x48d740)(p, tx, ty);
			CHECK(A - MemoryCache == B - OMemoryCache, "GetMemoryPicture(%d, %d, %d)", p, tx, ty);
		} else if (Op == 8) {
			int s = Range(1, 30);
			CHECK(FindMemorySprite(s) == At<IntRetFn>(0x48ce00)(s), "FindMemorySprite(%d)", s);
			int p = Range(1, NumberOfPictures), tx = Range(0, 3), ty = Range(0, 3);
			CHECK(FindMemoryPicture(p, tx, ty) == At<Find3Fn>(0x48ce70)(p, tx, ty), "FindMemoryPicture(%d, %d, %d)",
				p, tx, ty);
		} else if (t % 50 == 9) {
			ClearMemoryCache();
			At<VoidFn>(0x48b460)();
		}
		CompareMemoryCache("memory cache operation");
	}
}

static void TestText()
{
	char Text[80];
	for (int t = 0; t < 20000; t++) {
		RandomString(Text, Range(0, 60));
		int Font = Range(1, 4);
		int Length = Range(0, (int)strlen(Text));
		CHECK(GetTextWidth(Font, Text) == At<WidthFn>(0x494570)(Font, Text), "GetTextWidth(%d, '%s')", Font, Text);
		CHECK(GetTextPartWidth(Font, Text, Length) == At<PartWidthFn>(0x491b80)(Font, Text, Length),
			"GetTextPartWidth(%d, '%s', %d)", Font, Text, Length);
		CHECK(GetTextHeight(Font, Text) == At<WidthFn>(0x491880)(Font, Text), "GetTextHeight(%d, '%s')", Font, Text);
		int Width = Range(-20, 400);
		CHECK(GetFittingTextLength(Font, Text, Width) == At<PartWidthFn>(0x491f50)(Font, Text, Width),
			"GetFittingTextLength(%d, '%s', %d)", Font, Text, Width);
		TFont Format = RandomFont();
		int x = Range(-50, 500), y = Range(-50, 500), w = Range(-10, 300), h = Range(-10, 100);
		int ax = 0, ay = 0, bx = 1, by = 1;
		GetTextPosition(&ax, &ay, x, y, w, h, &Format);
		At<PositionFn>(0x492260)(&bx, &by, x, y, w, h, &Format);
		CHECK(ax == bx && ay == by, "GetTextPosition: %d,%d, original %d,%d", ax, ay, bx, by);
	}
	for (int Font = -3; Font <= 6; Font++) {
		if (Font != 0) // reads before the table, on each side's own heap
			CHECK(GetFontLineHeight(Font) == At<IntRetFn>(0x48b430)(Font), "GetFontLineHeight(%d)", Font);
		if (Font >= 1 && Font <= 4)
			CHECK(GetFontHeight(Font) == At<IntRetFn>(0x491710)(Font), "GetFontHeight(%d)", Font);
	}
}

// One random drawing operation on both sides. Every input is drawn before
// the two calls.
static void RandomDraw(int t)
{
	int Surface = Range(0, 5);
	int x = Range(-40, 400), y = Range(-40, 400);
	TRect Clip = RandomRect();
	TRGBColor Highlight = RandomColor();
	TRGBColor Colors[4] = {RandomColor(), RandomColor(), RandomColor(), RandomColor()};
	if (t % 4 == 0) // repeated colours, so that cached masked sprites are found again
		Colors[0] = Colors[1] = Colors[2] = Colors[3] = TRGBColor(t % 3, 0, 0);
	int Sprite = t % 2 ? Range(0, 40) : Range(0, NumberOfSprites);
	char Text[80];
	RandomString(Text, Range(0, 30));
	int Length = Range(0, (int)strlen(Text));
	int Width = Range(1, 64), Height = Range(1, 64);
	int SmallMask = t % 3 == 0 ? 0 : Range(1, 40);
	int LargeMask = t % 3 == 0 ? 0 : Range(1, NumberOfSprites);
	int Picture = 1 + t % NumberOfPictures;
	int SourceX = Range(-40, 300), SourceY = Range(-40, 300);
	int PartWidth = Range(-2, 200), PartHeight = Range(-2, 200);
	TFont F = RandomFont();
	if (t % 2)
		F.Color = TRGBColor(0, 0, 0); // the colour of plain picture tiles
	TObject Obj = {0, 0, 0};
	if (t % 7 != 0) {
		Obj.Type = DrawableObjects[Random() % DrawableObjects.size()];
		Obj.Data1 = Range(0, 120);
	}
	int Size = t % 2 ? 32 : Range(8, 64);
	bool ShowCount = (Random() & 1) != 0;
	int Outfit = t % 5 == 0 ? 0 : DrawableOutfits[Random() % DrawableOutfits.size()];
	int OutfitColors[4] = {Range(0, 132), Range(0, 132), Range(0, 132), Range(0, 132)};
	if (Outfit == 0)
		OutfitColors[0] = t % 3 ? DrawableObjects[Random() % DrawableObjects.size()] : 0;
	int Op = Random() % 17;
	for (int Pass = 0; Pass < 2; Pass++) {
		bool O = Pass == ORIGINAL;
		Side = Pass;
		switch (Op) {
		case 0:
			if (O) At<DrawSpriteFn>(0x48f870)(Surface, x, y, Sprite);
			else DrawSprite(Surface, x, y, Sprite);
			break;
		case 1:
			if (O) At<DrawClippedFn>(0x48fa60)(Surface, x, y, Sprite - 2, Clip);
			else DrawSpriteClipped(Surface, x, y, Sprite - 2, Clip);
			break;
		case 2:
			if (O) At<DrawScaledFn>(0x48fd80)(Surface, x, y, Width, Height, Sprite, Highlight, Clip);
			else DrawSpriteScaled(Surface, x, y, Width, Height, Sprite, Highlight, Clip);
			break;
		case 3: {
			int s = SmallMask == 0 ? Sprite : (Sprite == 0 ? 1 : Sprite);
			if (O) At<DrawMaskedFn>(0x4900d0)(Surface, x, y, s, SmallMask, Colors);
			else DrawMaskedSprite(Surface, x, y, s, SmallMask, Colors);
			break;
		}
		case 4: {
			int s = LargeMask == 0 ? Sprite : (Sprite == 0 ? 1 : Sprite);
			if (O) At<DrawMaskedScaledFn>(0x4904b0)(Surface, x, y, Width, Height, s, LargeMask, Colors, Highlight, Clip);
			else DrawMaskedSpriteScaled(Surface, x, y, Width, Height, s, LargeMask, Colors, Highlight, Clip);
			break;
		}
		case 5:
			if (O) At<DrawSpriteFn>(0x490920)(Surface, x, y, Picture);
			else DrawPicture(Surface, x, y, Picture);
			break;
		case 6:
			if (O) At<PartFn>(0x490c60)(Surface, x, y, Picture, SourceX, SourceY, PartWidth, PartHeight);
			else DrawPicturePart(Surface, x, y, Picture, SourceX, SourceY, PartWidth, PartHeight);
			break;
		case 7:
			if (O) At<PartClippedFn>(0x4910e0)(Surface, x, y, Picture, SourceX, SourceY, Width * 3, Height * 3, Clip);
			else DrawPicturePartClipped(Surface, x, y, Picture, SourceX, SourceY, Width * 3, Height * 3, Clip);
			break;
		case 8:
			if (O) At<TextPartFn>(0x492e30)(Surface, x, y, F.FontNumber, F.Color, Text, Length, F.Alignment);
			else PrintTextPart(Surface, x, y, F.FontNumber, F.Color, Text, Length, F.Alignment);
			break;
		case 9:
			if (O) At<TextClippedFn>(0x4934e0)(Surface, x, y, F.FontNumber, F.Color, Text, F.Alignment, Clip);
			else PrintTextClipped(Surface, x, y, F.FontNumber, F.Color, Text, F.Alignment, Clip);
			break;
		case 10:
			if (O) At<TextFn>(0x493af0)(Surface, x, y, F.FontNumber, F.Color, Text, F.Alignment);
			else PrintText(Surface, x, y, F.FontNumber, F.Color, Text, F.Alignment);
			break;
		case 11:
			if (O) At<FormatFn>(0x493f60)(Surface, x, y, F, Text);
			else PrintFormattedText(Surface, x, y, F, Text);
			break;
		case 12:
			if (O) At<FormatClippedFn>(0x4941d0)(Surface, x, y, F, Text, Clip);
			else PrintFormattedTextClipped(Surface, x, y, F, Text, Clip);
			break;
		case 13:
			if (O) At<FormatPartFn>(0x4943b0)(Surface, x, y, F, Text, Length);
			else PrintFormattedTextPart(Surface, x, y, F, Text, Length);
			break;
		case 14:
		case 15:
			if (O) At<DrawObjectFn>(0x494ca0)(Surface, x, y, Size, Obj, Highlight, Clip, F, ShowCount);
			else DrawObject(Surface, x, y, Size, Obj, Highlight, Clip, F, ShowCount);
			break;
		case 16:
			if (O) At<DrawOutfitFn>(0x495080)(Surface, x, y, Size, Outfit, OutfitColors, Highlight, Clip, F);
			else DrawOutfit(Surface, x, y, Size, Outfit, OutfitColors, Highlight, Clip, F);
			break;
		}
	}
	char What[64];
	sprintf(What, "drawing operation %d (%d)", Op, t);
	CompareCaches(What);
}

static void TestVideoCache()
{
	SetEngine(1); // our GetSurface creates the (fake) OpenGL surface
	TSurface *Fake = GetSurface();
	Global<TSurface *>(0x71d0cc) = Fake;
	for (int Round = 0; Round < 6; Round++) {
		SetEngine(Round % 3);
		CacheSurfaces = 1 + Round % 2;
		CacheWidth = 96 + 32 * (Round % 3) + Round;
		CacheHeight = 64 + 7 * Round;
		Side = OURS;
		if (Round == 0)
			InitVideoCache();
		else
			ResetVideoCache();
		Side = ORIGINAL;
		At<VoidFn>(Round == 0 ? 0x48da50 : 0x4922c0)();
		ClearNewVideoItems();
		CompareCaches("InitVideoCache");
		for (int t = 0; t < 2500; t++) {
			if (t % 100 == 0) {
				IncrementActionNumber();
				Global<int>(0x71c5ec) = GetActionNumber();
			}
			RandomDraw(Round * 10000 + t);
		}
	}
	// No cache surfaces: the cache becomes NULL, its size stays.
	CacheSurfaces = 0;
	Side = OURS;
	ResetVideoCache();
	Side = ORIGINAL;
	At<VoidFn>(0x4922c0)();
	CompareCaches("ResetVideoCache without cache surfaces");
	CacheSurfaces = 2;
	Side = OURS;
	InitVideoCache();
	Side = ORIGINAL;
	At<VoidFn>(0x48da50)();
	ClearNewVideoItems();
	CompareCaches("InitVideoCache again");
}

void TestGraphics(const char *ExePath)
{
	PatchScript();
	int OldChecks = Checks, OldFailures = Failures;
	std::string Dir = ExePath;
	size_t Slash = Dir.find_last_of("\\/");
	Dir = Slash == std::string::npos ? std::string() : Dir.substr(0, Slash + 1);
	std::string Spr = Dir + "Tibia.spr", Pic = Dir + "Tibia.pic", Dat = Dir + "Tibia.dat";
	for (const std::string *Name : {&Spr, &Pic, &Dat}) {
		FILE *Probe = fopen(Name->c_str(), "rb");
		if (Probe == NULL) {
			printf("note: %s not found, graphics.cpp not tested\n", Name->c_str());
			return;
		}
		fclose(Probe);
	}
	TestFiles(Spr, Pic);
	TestMemoryCache();
	TestText();

	LoadObjects(Dat.c_str());
	At<LoadFn>(0x4d7380)(Dat.c_str());
	FindDrawables();
	CHECK(!DrawableObjects.empty() && !DrawableOutfits.empty(), "no drawable objects (%d) or outfits (%d)",
		(int)DrawableObjects.size(), (int)DrawableOutfits.size());
	int SavedEngine = curEngine;
	if (!DrawableObjects.empty() && !DrawableOutfits.empty())
		TestVideoCache();

	// InitGraphics, ReloadSprites and ReloadPictures read the files in the
	// current directory.
	char Saved[MAX_PATH];
	_getcwd(Saved, MAX_PATH);
	_chdir(Dir.c_str());
	Side = OURS;
	ReloadSprites();
	Side = ORIGINAL;
	At<VoidFn>(0x492490)();
	ClearNewVideoItems();
	CompareSprites("ReloadSprites");
	CompareCaches("ReloadSprites");
	Side = OURS;
	ReloadPictures();
	Side = ORIGINAL;
	At<VoidFn>(0x494900)();
	ClearNewVideoItems();
	ComparePictures("ReloadPictures");
	CompareFonts("ReloadPictures");
	CompareCaches("ReloadPictures");
	ExitGraphics();
	At<VoidFn>(0x48b4a0)();
	CHECK(VideoCache == NULL && MemoryCacheData == NULL && MemoryCache == NULL && Fonts == NULL && Pictures == NULL
		&& Sprites == NULL, "ExitGraphics leaves a table");
	CHECK(OVideoCache == NULL && OMemoryCacheData == NULL && OMemoryCache == NULL && OFonts == NULL
		&& OPictures == NULL && OSprites == NULL, "the original ExitGraphics leaves a table");
	Side = OURS;
	InitGraphics(5 * 32 * 32 * 3);
	Side = ORIGINAL;
	At<IntFn>(0x494af0)(5 * 32 * 32 * 3);
	CompareSprites("InitGraphics");
	ComparePictures("InitGraphics");
	CompareFonts("InitGraphics");
	CompareLogs("InitGraphics");
	ExitGraphics();
	At<VoidFn>(0x48b4a0)();
	_chdir(Saved);

	ReleaseObjects();
	At<VoidFn>(0x4d70c0)();
	SetEngine(SavedEngine);
	printf("Graphics: %d checks, %d failures (%d sprites and %d picture tiles decoded; %d tiles written to the "
		"video cache, %d blits)\n", Checks - OldChecks, Failures - OldFailures, DecodedSprites, DecodedTiles, Writes,
		Blits);
}
