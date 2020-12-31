#include "engine.h"
#include "utils.h"

std::string FNameEntry::String()
{
	std::string name("\x0", Len);
	String(name.data());
	return name;
}

void FNameEntry::String(char* buf)
{
	if (bIsWide)
	{
		auto copied = WideCharToMultiByte(CP_UTF8, 0, WideName, Len, buf, Len, 0, 0);
		if (copied == 0) { buf[0] = '\x0'; }
	}
	else
	{
		memcpy(buf, AnsiName, Len);
	}
}

FNameEntry* FNamePool::GetEntry(FNameEntryHandle handle) const
{
	return reinterpret_cast<FNameEntry*>(Blocks[handle.Block] + 2 * static_cast<uint64_t>(handle.Offset));
}

std::string FName::GetName()
{
	auto entry = NamePoolData->GetEntry(Index);
	auto name = entry->String();
	if (Number > 0)
	{
		name += '_' + std::to_string(Number);
	}
	auto pos = name.rfind('/');
	if (pos != std::string::npos)
	{
		name = name.substr(pos + 1);
	}
	return name;
}

std::string UObject::GetName()
{
	return NamePrivate.GetName();
}

std::string UObject::GetFullName()
{
	std::string temp;
	for (auto outer = OuterPrivate; outer; outer = outer->OuterPrivate) { temp = outer->GetName() + "." + temp; }
	std::string name = ClassPrivate->GetName() + " " + temp + this->GetName();
	return name;
}

bool UObject::IsA(void* cmp)
{
	for (auto super = ClassPrivate; super; super = static_cast<UClass*>(super->SuperStruct)) { if (super == cmp) { return true; } }
	return false;
}

void UObject::ProcessEvent(void* fn, void* parms)
{
	auto vtable = *reinterpret_cast<void***>(this);
	reinterpret_cast<void(*)(void*, void*, void*)>(vtable[0x42])(this, fn, parms);
}

UObject* TUObjectArray::GetObjectPtr(uint32_t id) const
{
	if (id >= NumElements) return nullptr;
	uint64_t chunkIndex = id / 65536;
	if (chunkIndex >= NumChunks) return nullptr;
	auto chunk = Objects[chunkIndex];
	if (!chunk) return nullptr;
	uint32_t withinChunkIndex = id % 65536 * 24;
	auto item = *reinterpret_cast<UObject**>(chunk + withinChunkIndex);
	return item;
}

UObject* TUObjectArray::FindObject(const char* name) const
{
	for (auto i = 0u; i < NumElements; i++)
	{
		auto object = GetObjectPtr(i);
		if (object->GetFullName() == name) { return object; }
	}
	return nullptr;
}

UEngine** Engine = nullptr;
TUObjectArray* ObjObjects = nullptr;
FNamePool* NamePoolData = nullptr;

void UCanvas::K2_DrawText(void* RenderFont, FString& RenderText, FVector2D& ScreenPosition, FVector2D& Scale, FLinearColor& RenderColor, float Kerning, FLinearColor& ShadowColor, FVector2D& ShadowOffset, bool bCentreX, bool bCentreY, bool bOutlined, FLinearColor& OutlineColor)
{
	static auto fn = ObjObjects->FindObject("Function Engine.Canvas.K2_DrawText");
	struct {
		void* RenderFont;
		FString RenderText;
		FVector2D ScreenPosition;
		FVector2D Scale;
		FLinearColor RenderColor;
		float Kerning;
		FLinearColor ShadowColor;
		FVector2D ShadowOffset;
		bool bCentreX;
		bool bCentreY;
		bool bOutlined;
		FLinearColor OutlineColor;
	} parms;
	parms = { RenderFont , RenderText, ScreenPosition, Scale, RenderColor, Kerning, ShadowColor, ShadowOffset, bCentreX, bCentreY, bOutlined, OutlineColor};
	ProcessEvent(fn, &parms);
}

FVector AActor::K2_GetActorLocation()
{
	static auto fn = ObjObjects->FindObject("Function Engine.Actor.K2_GetActorLocation");
	struct {
		FVector ReturnValue;
	} parms;
	ProcessEvent(fn, &parms);
	return parms.ReturnValue;
}

APawn* AController::K2_GetPawn()
{
	static auto fn = ObjObjects->FindObject("Function Engine.Controller.K2_GetPawn");
	struct {
		APawn* ReturnValue;
	} parms;
	ProcessEvent(fn, &parms);
	return parms.ReturnValue;
}

bool APlayerController::ProjectWorldLocationToScreen(FVector& WorldLocation, FVector2D& ScreenLocation, bool bPlayerViewportRelative)
{
	static auto fn = ObjObjects->FindObject("Function Engine.PlayerController.ProjectWorldLocationToScreen");
	struct {
		FVector WorldLocation;
		FVector2D ScreenLocation;
		bool bPlayerViewportRelative;
		bool ReturnValue;
	} parms;
	parms = { WorldLocation, ScreenLocation, bPlayerViewportRelative };
	ProcessEvent(fn, &parms);
	ScreenLocation = parms.ScreenLocation;
	return parms.ReturnValue;
}


bool EngineInit()
{
	auto main = GetModuleHandleA(nullptr);

	static byte engineSig[] = { 0x48, 0x8b, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x88, 0x08, 0x0b, 0x00, 0x00 };
	Engine = reinterpret_cast<decltype(Engine)>(FindPointer(main, engineSig, sizeof(engineSig), 0));
	if (!Engine) return false;

	static byte objSig[] = { 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0C, 0xC8, 0x48, 0x8D, 0x04, 0xD1, 0xEB };
	ObjObjects = reinterpret_cast<decltype(ObjObjects)>(FindPointer(main, objSig, sizeof(objSig), 0));
	if (!ObjObjects) return false;

	static byte poolSig[] = { 0x48, 0x8d, 0x35, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x16 };
	NamePoolData = reinterpret_cast<decltype(NamePoolData)>(FindPointer(main, poolSig, sizeof(poolSig), 0));
	if (!NamePoolData) return false;

	return true;
}