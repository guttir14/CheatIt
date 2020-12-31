#include "cheat.h"
#include "engine.h"
#include "utils.h"

void** Address;
void(__thiscall* PostRenderOriginal)(UGameViewportClient*, UCanvas*);

void* Font = nullptr;

void PostRenderHook(UGameViewportClient* viewport, UCanvas* canvas)
{
    do
    {
        auto world = viewport->World;
        auto game = world->OwningGameInstance;
        auto state = world->GameState;
        if (!state) break;
        auto localPlayer = game->LocalPlayers.Data[0];
        auto localController = localPlayer->PlayerController;
        auto localCamera = localController->PlayerCameraManager;
        if (!localCamera) break;
        auto cameraLocation = localCamera->K2_GetActorLocation();
        auto localPawn = localController->K2_GetPawn();
        auto players = state->PlayerArray;
        for (auto i = 0; i < players.Count; i++)
        {
            auto player = players.Data[i];
            auto pawn = player->PawnPrivate;
            if (!pawn || pawn == localPawn) continue;
            auto location = pawn->K2_GetActorLocation();
            FVector2D screen;
            if (localController->ProjectWorldLocationToScreen(location, screen, true))
            {
                auto name = player->PlayerNamePrivate;
                FVector2D scale{ 1.f, 1.f };
                FLinearColor color{ 1.f, 0.43f, 0.f, 1.f };
                FLinearColor shadow{ 0.f, 0.f, 0.f, 0.f };
                FLinearColor outline{ 0.f, 0.f, 0.f, 1.f };
                canvas->K2_DrawText(Font, name, screen, scale, color, false, shadow, scale, true, true, true, outline);
            };
        }
    } while (false);

    PostRenderOriginal(viewport, canvas);
}

bool CheatInit()
{
    auto engine = *Engine;
    if (!engine) return false;

    Font = engine->SubtitleFont;
    if (!Font) return false;

    auto viewport = engine->GameViewport;
    if (!viewport) return false; 

    auto vtable = viewport->VFTable;
    if (!vtable) return false;

    Address = vtable + 0x61;
    PostRenderOriginal = reinterpret_cast<decltype(PostRenderOriginal)>(Address[0]);

    auto hook = &PostRenderHook;
    return PatchMem(Address, &hook, 8);
}

void CheatRemove()
{
   PatchMem(Address, &PostRenderOriginal, 8);
}