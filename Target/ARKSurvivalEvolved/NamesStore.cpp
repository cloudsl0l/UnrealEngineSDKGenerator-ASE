#include <windows.h>

#include "PatternFinder.hpp"
#include "NamesStore.hpp"

#include "EngineClasses.hpp"

class FNameEntry
{
public:
	static const auto NAME_WIDE_MASK = 0x1;
	static const auto NAME_INDEX_SHIFT = 1;

	int32_t Index;
	char UnknownData00[0x04];
	FNameEntry* HashNext;
	union
	{
		char AnsiName[1024];
		wchar_t WideName[1024];
	};

	int32_t GetIndex() const
	{
		return Index >> NAME_INDEX_SHIFT;
	}

	bool IsWide() const
	{
		return Index & NAME_WIDE_MASK;
	}

	const char* GetAnsiName() const
	{
		return AnsiName;
	}

	const wchar_t* GetWideName() const
	{
		return WideName;
	}
};

template<typename ElementType, int32_t MaxTotalElements, int32_t ElementsPerChunk>
class TStaticIndirectArrayThreadSafeRead
{
public:
	int32_t Num() const
	{
		return NumElements;
	}

	bool IsValidIndex(int32_t index) const
	{
		return index >= 0 && index < Num() && Chunks[index / ElementsPerChunk] != nullptr && GetById(index) != nullptr;
	}

	ElementType const* const& GetById(int32_t index) const
	{
		return *GetItemPtr(index);
	}

private:
	ElementType const* const* GetItemPtr(int32_t Index) const
	{
		int32_t ChunkIndex = Index / ElementsPerChunk;
		int32_t WithinChunkIndex = Index % ElementsPerChunk;
		ElementType** Chunk = Chunks[ChunkIndex];
		return Chunk + WithinChunkIndex;
	}

	enum
	{
		ChunkTableSize = (MaxTotalElements + ElementsPerChunk - 1) / ElementsPerChunk
	};

	ElementType** Chunks[ChunkTableSize];
	int32_t NumElements;
	int32_t NumChunks;
};

// ShooterGame.pdb: TStaticIndirectArrayThreadSafeRead<FNameEntry,4194304,16384>, sizeof == 0x808
using TNameEntryArray = TStaticIndirectArrayThreadSafeRead<FNameEntry, 4 * 1024 * 1024, 16384>;

TNameEntryArray* GlobalNames = nullptr;

bool NamesStore::Initialize()
{
	const auto module = GetModuleHandleW(nullptr);

	// FName::GetNames(): sub rsp,28h / mov rax,[rip+Names] / ... / mov ecx,808h (sizeof(TNameEntryArray))
	const auto address = FindPattern(module, reinterpret_cast<const unsigned char*>("\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x38\x05\x00\x00\x00\x00\x48\x89\x5C\x24\x20\x74\x00\xFF\x15\x00\x00\x00\x00\x3B\x05\x00\x00\x00\x00\x74\x00\xFF\x15\x00\x00\x00\x00\x3B\x05\x00\x00\x00\x00\x74\x00\x4C\x8D\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xB9\x08\x08\x00\x00"), "xxxxxxx????xxxxx????xx????xxxxxx?xx????xx????x?xx????xx????x?xxx????xxx????xxx????xx????x????xxxxx");

	std::uintptr_t namesPtr;
	if (address != -1)
	{
		namesPtr = address + 11 + *reinterpret_cast<int32_t*>(address + 7);
	}
	else
	{
		constexpr std::uintptr_t offset = 0x4BF2DC8; // FName::GetNames()::Names
		namesPtr = reinterpret_cast<std::uintptr_t>(module) + offset;
	}

	GlobalNames = *reinterpret_cast<TNameEntryArray**>(namesPtr);

	return GlobalNames != nullptr;
}

void* NamesStore::GetAddress()
{
	return GlobalNames;
}

size_t NamesStore::GetNamesNum() const
{
	return GlobalNames->Num();
}

bool NamesStore::IsValid(size_t id) const
{
	return GlobalNames->IsValidIndex(static_cast<int32_t>(id));
}

std::string NamesStore::GetById(size_t id) const
{
	return GlobalNames->GetById(static_cast<int32_t>(id))->GetAnsiName();
}
