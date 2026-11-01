// Differential tests of Inventory.cpp against the original code.
//
// Random sequences of operations on both sides, generated so that no
// assertion fails; after each operation the observable state (body slots,
// containers, offers) is compared through the getters of both sides.

#include "difftest.h"
#include "../../src/Inventory.h"

typedef void (__cdecl *VoidFn)();
typedef int (__cdecl *IntFn)();
typedef bool (__cdecl *BoolIntFn)(int);
typedef int (__cdecl *IntIntFn)(int);
typedef char *(__cdecl *NameFn)(int);
typedef void (__cdecl *SetBodyFn)(int, TObject);
typedef TObject *(__cdecl *GetBodyFn)(int);
typedef void (__cdecl *OpenFn)(int, TObject, const char *, int, bool);
typedef void (__cdecl *ContainerObjectFn)(int, TObject);
typedef void (__cdecl *ChangeFn)(int, int, TObject);
typedef bool (__cdecl *DeleteFn)(int, int);
typedef TObject (__cdecl *GetContainerFn)(int);
typedef TObject (__cdecl *GetContentFn)(int, int);
typedef bool (__cdecl *SetOfferFn)(const char *);
typedef void (__cdecl *AddOfferFn)(TObject);
typedef char *(__cdecl *OfferNameFn)();
typedef TObject (__cdecl *OfferObjectFn)(int);

static bool Same(TObject a, TObject b)
{
	return a.Type == b.Type && a.Data1 == b.Data1 && a.Data2 == b.Data2;
}

static TObject RandomObject()
{
	TObject Object = {(int)(Random() % 4000), (int)(Random() % 100), (int)(Random() % 3)};
	return Object;
}

static void CompareState(const char *What)
{
	for (int Position = 1; Position <= BODY_POSITIONS; Position++)
		CHECK(Same(*GetBodyObject(Position), *At<GetBodyFn>(0x4a4880)(Position)), "body %d after %s", Position, What);
	for (int c = 0; c < OPEN_CONTAINERS; c++) {
		bool Open = IsContainerOpen(c);
		CHECK(Open == At<BoolIntFn>(0x4a5220)(c), "container %d open after %s", c, What);
		if (!Open)
			continue;
		int Count = GetContainerObjectCount(c);
		CHECK(Count == At<IntIntFn>(0x4a5bd0)(c), "container %d count after %s", c, What);
		CHECK(Same(GetContainerObject(c), At<GetContainerFn>(0x4a5e80)(c)), "container %d object", c);
		CHECK(strcmp(GetContainerName(c), At<NameFn>(0x4a64e0)(c)) == 0, "container %d name '%s' '%s'", c,
			GetContainerName(c), At<NameFn>(0x4a64e0)(c));
		CHECK(GetContainerCapacity(c) == At<IntIntFn>(0x4a6790)(c), "container %d capacity", c);
		CHECK(GetContainerHasParent(c) == At<BoolIntFn>(0x4a6a40)(c), "container %d parent", c);
		for (int p = 0; p < Count; p++)
			CHECK(Same(GetContainerContent(c, p), At<GetContentFn>(0x4a6150)(c, p)), "container %d slot %d after %s", c, p, What);
	}
	CHECK(GetFreeContainer() == At<IntFn>(0x4a6cf0)(), "GetFreeContainer after %s", What);
	int Own = GetOwnOfferCount(), Counter = GetCounterOfferCount();
	CHECK(Own == At<IntFn>(0x4a4530)() && Counter == At<IntFn>(0x4a4540)(), "offer counts after %s", What);
	if (Own > 0) {
		CHECK(strcmp(GetOwnOfferName(), At<OfferNameFn>(0x4a7760)()) == 0, "own offer name");
		for (int p = 0; p < Own; p++)
			CHECK(Same(GetOwnOfferObject(p), At<OfferObjectFn>(0x4a7a00)(p)), "own offer %d", p);
	}
	if (Counter > 0) {
		CHECK(strcmp(GetCounterOfferName(), At<OfferNameFn>(0x4a78b0)()) == 0, "counter offer name");
		for (int p = 0; p < Counter; p++)
			CHECK(Same(GetCounterOfferObject(p), At<OfferObjectFn>(0x4a7b90)(p)), "counter offer %d", p);
	}
}

void TestInventory()
{
	PatchImage();
	InitInventory();
	At<VoidFn>(0x4a7d20)();
	CompareState("InitInventory");
	for (int Step = 0; Step < 20000; Step++) {
		int c = (int)(Random() % OPEN_CONTAINERS);
		bool Open = IsContainerOpen(c);
		const char *What = "";
		switch (Random() % 14) {
		case 0: {
			int Position = 1 + (int)(Random() % BODY_POSITIONS);
			TObject Object = RandomObject();
			SetBodyObject(Position, Object);
			At<SetBodyFn>(0x4a4550)(Position, Object);
			What = "SetBodyObject";
			break;
		}
		case 1: {
			int Position = 1 + (int)(Random() % BODY_POSITIONS);
			DeleteBodyObject(Position);
			At<void (__cdecl *)(int)>(0x4a46d0)(Position);
			What = "DeleteBodyObject";
			break;
		}
		case 2:
			if (!Open) {
				char Name[30];
				RandomText(Name, (int)(Random() % 30));
				TObject Object = RandomObject();
				int Capacity = (int)(Random() % 40);
				bool Parent = (Random() & 1) != 0;
				OpenContainer(c, Object, Name, Capacity, Parent);
				At<OpenFn>(0x4a4b00)(c, Object, Name, Capacity, Parent);
				What = "OpenContainer";
			} else if (Random() % 3 == 0) {
				CloseContainer(c);
				At<void (__cdecl *)(int)>(0x4a4f70)(c);
				What = "CloseContainer";
			}
			break;
		case 3: case 4:
			if (Open) {
				TObject Object = RandomObject();
				AppendContainerObject(c, Object);
				At<ContainerObjectFn>(0x4a5380)(c, Object);
				What = "AppendContainerObject";
			}
			break;
		case 5:
			if (Open) {
				TObject Object = RandomObject();
				InsertContainerObject(c, Object);
				At<ContainerObjectFn>(0x4a7f00)(c, Object);
				What = "InsertContainerObject";
			}
			break;
		case 6:
			if (Open) {
				int Position = (int)(Random() % 40) - 2;
				TObject Object = RandomObject();
				ChangeContainerObject(c, Position, Object);
				At<ChangeFn>(0x4a5630)(c, Position, Object);
				What = "ChangeContainerObject";
			}
			break;
		case 7:
			if (Open) {
				int Position = (int)(Random() % 40) - 2;
				bool A = DeleteContainerObject(c, Position);
				bool B = At<DeleteFn>(0x4a58e0)(c, Position);
				CHECK(A == B, "DeleteContainerObject(%d, %d) %d %d", c, Position, A, B);
				What = "DeleteContainerObject";
			}
			break;
		case 8:
			if (GetOwnOfferCount() == 0) {
				char Name[30];
				RandomText(Name, (int)(Random() % 30));
				bool A = SetOwnOffer(Name), B = At<SetOfferFn>(0x4a6eb0)(Name);
				CHECK(A == B, "SetOwnOffer");
				What = "SetOwnOffer";
			}
			break;
		case 9: {
			char Name[30];
			RandomText(Name, (int)(Random() % 30));
			bool A = SetCounterOffer(Name), B = At<SetOfferFn>(0x4a7230)(Name);
			CHECK(A == B, "SetCounterOffer %d %d", A, B);
			What = "SetCounterOffer";
			break;
		}
		case 10:
			if (GetOwnOfferCount() < OBJECTS_PER_OFFER) {
				TObject Object = RandomObject();
				AddOwnOfferObject(Object);
				At<AddOfferFn>(0x4a7460)(Object);
				What = "AddOwnOfferObject";
			}
			break;
		case 11:
			if (GetCounterOfferCount() < OBJECTS_PER_OFFER) {
				TObject Object = RandomObject();
				AddCounterOfferObject(Object);
				At<AddOfferFn>(0x4a75e0)(Object);
				What = "AddCounterOfferObject";
			}
			break;
		case 12:
			if (Random() % 20 == 0) {
				ClearOffers();
				At<VoidFn>(0x4a4520)();
				What = "ClearOffers";
			}
			break;
		default:
			if (Random() % 50 == 0) {
				if (Random() & 1) {
					ResetInventory();
					At<VoidFn>(0x4a7d70)();
				} else {
					ClearBody();
					At<VoidFn>(0x4a4460)();
				}
				What = "reset";
			}
			break;
		}
		if (Step % 20 == 0)
			CompareState(What);
	}
	CompareState("the end");
}
