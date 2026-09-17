#include <windows.h>

#include "PatternFinder.hpp"
#include "ObjectsStore.hpp"

#include "EngineClasses.hpp"

class FUObjectItem
{
public:
	UObject* Object;
	int32_t SerialNumber;
};

class FChunkedFixedUObjectArray
{
public:
	enum
	{
		NumElementsPerChunk = 64 * 1024
	};

	FUObjectItem** Objects;
	FUObjectItem* PreAllocatedObjects;
	int32_t MaxElements;
	int32_t NumElements;
	int32_t MaxChunks;
	int32_t NumChunks;

	int32_t Num() const
	{
		return NumElements;
	}

	FUObjectItem* GetObjectPtr(int32_t index) const
	{
		if (index < 0 || index >= NumElements)
		{
			return nullptr;
		}
		const int32_t chunkIndex = index / NumElementsPerChunk;
		if (chunkIndex >= NumChunks || Objects[chunkIndex] == nullptr)
		{
			return nullptr;
		}
		return Objects[chunkIndex] + index % NumElementsPerChunk;
	}
};

class FUObjectArray
{
public:
	int32_t ObjFirstGCIndex;
	int32_t ObjLastNonGCIndex;
	int32_t MaxObjectsNotConsideredByGC;
	bool OpenForDisregardForGC;

	FChunkedFixedUObjectArray ObjObjects; //0x0010
};

FUObjectArray* GlobalObjects = nullptr;

bool ObjectsStore::Initialize()
{
	const auto module = GetModuleHandleW(nullptr);

	// FWeakObjectPtr::Get(): cmp ebx, [rip+GUObjectArray.ObjObjects.NumElements]
	const auto address = FindPattern(module, reinterpret_cast<const unsigned char*>("\x83\x79\x04\x00\x48\x8B\xF9\x0F\x84\x00\x00\x00\x00\x8B\x19\x85\xDB\x0F\x88\x00\x00\x00\x00\x3B\x1D\x00\x00\x00\x00"), "xxxxxxxxx????xxxxxx????xx????");

	if (address != -1)
	{
		const auto numElements = address + 0x1D + *reinterpret_cast<int32_t*>(address + 0x19);
		GlobalObjects = reinterpret_cast<decltype(GlobalObjects)>(numElements - 0x24);
	}
	else
	{
		constexpr std::uintptr_t offset = 0x4BA6568; // GUObjectArray
		GlobalObjects = reinterpret_cast<decltype(GlobalObjects)>(reinterpret_cast<std::uintptr_t>(module) + offset);
	}

	return true;
}

void* ObjectsStore::GetAddress()
{
	return GlobalObjects;
}

size_t ObjectsStore::GetObjectsNum() const
{
	return GlobalObjects->ObjObjects.Num();
}

UEObject ObjectsStore::GetById(size_t id) const
{
	const auto item = GlobalObjects->ObjObjects.GetObjectPtr(static_cast<int32_t>(id));
	return item != nullptr ? item->Object : nullptr;
}
