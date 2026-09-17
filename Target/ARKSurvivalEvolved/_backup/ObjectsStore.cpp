#include <windows.h>

#include "PatternFinder.hpp"
#include "ObjectsStore.hpp"

#include "EngineClasses.hpp"

class FUObjectArray
{
public:
	int32_t ObjFirstGCIndex;
	int32_t ObjLastNonGCIndex;
	int32_t OpenForDisregardForGC;

	TArray<UObject*> ObjObjects;
	TArray<int32_t> ObjAvailable;
};

FUObjectArray* GlobalObjects = nullptr;

bool ObjectsStore::Initialize()
{
	GlobalObjects = reinterpret_cast<decltype(GlobalObjects)>(0x39E0DFF);

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
	return GlobalObjects->ObjObjects[id];
}
