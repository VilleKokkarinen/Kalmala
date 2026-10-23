#include "RenderMenuCommandlet.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaSettingsWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/ISlateRHIRendererModule.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RenderingThread.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "TextureResource.h"
#include "AssetCompilingManager.h"

URenderMenuCommandlet::URenderMenuCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 URenderMenuCommandlet::Main(const FString& Params)
{
    FString Menu, Output, Menus, Outputs, WidgetClass;
    int32 Width = 1920, Height = 1080, TextScale = 100, Contrast = 0;
    FParse::Value(*Params, TEXT("Menu="), Menu);
    FParse::Value(*Params, TEXT("Output="), Output);
    const bool bHasMenus = FParse::Value(*Params, TEXT("Menus="), Menus);
    const bool bHasOutputs = FParse::Value(*Params, TEXT("Outputs="), Outputs);
    FParse::Value(*Params, TEXT("WidgetClass="), WidgetClass);
    FParse::Value(*Params, TEXT("Width="), Width); FParse::Value(*Params, TEXT("Height="), Height);
    FParse::Value(*Params, TEXT("TextScale="), TextScale); FParse::Value(*Params, TEXT("Contrast="), Contrast);
    if (bHasMenus || bHasOutputs)
    {
        TArray<FString> MenuNames, OutputNames;
        Menus.ParseIntoArray(MenuNames, TEXT("|"), true);
        Outputs.ParseIntoArray(OutputNames, TEXT("|"), true);
        if (!bHasMenus || !bHasOutputs || MenuNames.IsEmpty() || MenuNames.Num() != OutputNames.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("Batch rendering requires matching -Menus and -Outputs lists separated by |."));
            return 1;
        }
        // Reuse this process but keep each preview on the normal isolated-world path.
        for (int32 Index = 0; Index < MenuNames.Num(); ++Index)
        {
            FString SingleParams = FString::Printf(TEXT("-Menu=\"%s\" -Output=\"%s\" -Width=%d -Height=%d -TextScale=%d -Contrast=%d"),
                *MenuNames[Index], *OutputNames[Index], Width, Height, TextScale, Contrast);
            if (FParse::Param(*Params, TEXT("Empty"))) SingleParams += TEXT(" -Empty");
            if (!WidgetClass.IsEmpty()) SingleParams += FString::Printf(TEXT(" -WidgetClass=\"%s\""), *WidgetClass);

            const int32 Result = Main(SingleParams);
            if (Result != 0)
            {
                UE_LOG(LogTemp, Error, TEXT("MenuRender batch stopped: Menu=%s Code=%d"), *MenuNames[Index], Result);
                return Result;
            }
        }
        return 0;
    }
    Menu.ToLowerInline();
    const TMap<FString, int32> Tabs = {{TEXT("settings.home"),5},{TEXT("settings.options"),4},
        {TEXT("settings.audio"),0},{TEXT("settings.controls"),1},{TEXT("settings.accessibility"),2},
        {TEXT("settings.status"),3},{TEXT("settings.video"),6},{TEXT("settings.controller"),7}};
    const bool bInventory = WidgetClass.IsEmpty() && (Menu == TEXT("inventory") || Menu == TEXT("inventory-crafting"));
    const bool bWorkbenchMenu = Menu == TEXT("crafting");
    const bool bForgeMenu = Menu == TEXT("forge") || Menu == TEXT("forge.upgrade");
    const bool bCrafting = WidgetClass.IsEmpty() && (bWorkbenchMenu || bForgeMenu
        || Menu == TEXT("inventory-crafting") || Menu == TEXT("build"));
    if ((!bInventory && !bCrafting && !Tabs.Contains(Menu) && WidgetClass.IsEmpty())
        || Width < 320 || Height < 240 || Width > 4096 || Height > 2160 || Output.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Unknown menu or invalid request. Use inventory, crafting, forge, forge.upgrade, inventory-crafting, build, settings.{home,options,audio,controls,controller,video,accessibility,status}, or WidgetClass=/Script/...; Output=...png."));
        return 1;
    }
    Output = FPaths::ConvertRelativePathToFull(Output);
    if (Output.Len() >= 260 || FPaths::GetExtension(Output).ToLower() != TEXT("png") || FPaths::FileExists(Output)) return 2;
    const double RenderStartSeconds = FPlatformTime::Seconds();
    static bool bSlateRendererInitialized = false;
    if (!bSlateRendererInitialized)
    {
        if (!FSlateApplication::IsInitialized())
            FSlateApplication::InitializeAsStandaloneApplication(FModuleManager::LoadModuleChecked<ISlateRHIRendererModule>(TEXT("SlateRHIRenderer")).CreateSlateRHIRenderer());
        else
            FSlateApplication::Get().InitializeRenderer(FModuleManager::LoadModuleChecked<ISlateRHIRendererModule>(TEXT("SlateRHIRenderer")).CreateSlateRHIRenderer());
        bSlateRendererInitialized = true;
    }
    UKalmalaSettingsWidget::SetTextScalePercent(TextScale);
    UKalmalaSettingsWidget::SetContrastMode(Contrast);
    UKalmalaSettingsWidget::SetReducedMotionEnabled(true);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!World) return 3;
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    AKalmalaCharacter* Pawn = World->SpawnActorDeferred<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(), FTransform::Identity);
    if (!Controller || !Pawn) { World->DestroyWorld(false); return 3; }
    ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
    Controller->SetPlayer(Player);
    Controller->SetPawn(Pawn);
    if (!Player->GetPlayerController(World)) World->AddController(Controller);
    UKalmalaInventoryComponent* Inventory = Pawn->FindComponentByClass<UKalmalaInventoryComponent>();
    if (Inventory && !FParse::Param(*Params, TEXT("Empty")))
    {
        Inventory->TryGrantFromServer(TEXT("Wood"), 8);
        Inventory->TryGrantFromServer(TEXT("Stone"), 3);
        if (FParse::Param(*Params, TEXT("A9Gear")))
        {
            for (const FName Item : {FName(TEXT("OpalHelm")), FName(TEXT("GarnetNecklace")), FName(TEXT("DuskRing"))})
            {
                FString Reason;
                if (!Inventory->TryGrantFromServer(Item, 1)) { World->DestroyWorld(false); return 3; }
                const auto* Stack = Inventory->GetStacks().FindByPredicate([Item](const auto& Row) { return Row.ItemId == Item; });
                if (!Stack || !Inventory->EquipGearFromServer(Stack->ItemGuids[0], Reason)) { World->DestroyWorld(false); return 3; }
            }
            Inventory->TryGrantFromServer(TEXT("StarOpal"), 1);
            Inventory->TryGrantFromServer(TEXT("StormSapphire"), 1);
            UE_LOG(LogTemp, Display, TEXT("A9 menu render fixture: Head=OpalHelm Necklace=GarnetNecklace Ring=DuskRing"));
        }
    }
    TSharedRef<SConstraintCanvas> Canvas = SNew(SConstraintCanvas);
    TArray<TStrongObjectPtr<UUserWidget>> Widgets;
    auto MakeWidget = [&](UClass* Class)
    {
        // CreateWidget's ordinary context expects a game instance/viewport. This
        // isolated world supplies the same local-player context explicitly.
        auto* Widget = NewObject<UUserWidget>(Controller, Class);
        Widget->SetPlayerContext(FLocalPlayerContext(Player, World));
        Widget->Initialize();
        return Widget;
    };
    auto Mount = [&](UUserWidget* Widget, FVector2D Position, FVector2D Size)
    {
        UE_LOG(LogTemp, Display, TEXT("MenuRender widget=%s root=%s context=%d visibility=%d size=%s"),
            *Widget->GetClass()->GetName(), *GetNameSafe(Widget->GetRootWidget()), Widget->GetPlayerContext().IsValid(),
            int32(Widget->GetVisibility()), *Size.ToString());
        Widgets.Emplace(Widget);
        Canvas->AddSlot().Offset(FMargin(Position.X, Position.Y, Size.X, Size.Y))
            .Anchors(FAnchors(0,0)).Alignment(FVector2D::ZeroVector)[Widget->TakeWidget()];
    };
    const FVector2D Viewport(Width, Height);
    if (bInventory)
    {
        auto* Widget = CastChecked<UKalmalaInventoryMenuWidget>(MakeWidget(UKalmalaInventoryMenuWidget::StaticClass()));
        Widget->PrepareMenuRender(Viewport);
        Mount(Widget, FVector2D(16), Widget->GetPanelSizeForVerification());
    }
    if (bCrafting)
    {
        auto* Widget = CastChecked<UKalmalaCraftingWidget>(MakeWidget(UKalmalaCraftingWidget::StaticClass()));
        const FString StationSection = Menu == TEXT("forge.upgrade") ? TEXT("Upgrade") : TEXT("Craft");
        Widget->PrepareMenuRender(Menu == TEXT("inventory-crafting"), bWorkbenchMenu, bForgeMenu,
            StationSection);
        const bool bBuild = Menu == TEXT("build");
        const FVector2D Size = bBuild ? FVector2D(FMath::Min(840.f,Width - 32.f),FMath::Min(980.f,Height - 32.f))
            : Menu == TEXT("inventory-crafting") ? UKalmalaCraftingWidget::GetInventoryCraftPanelSize(Viewport)
            : FVector2D(FMath::Min(860.f, FMath::Max(80.f, Viewport.X * .52f)),
                FMath::Min(760.f, FMath::Max(80.f, Viewport.Y - 32.f)));
        Mount(Widget, bBuild ? (Viewport - Size) * .5f : FVector2D(Width - Size.X - 16,16), Size);
    }
    if (const int32* Tab = WidgetClass.IsEmpty() ? Tabs.Find(Menu) : nullptr)
    {
        auto* Widget = CastChecked<UKalmalaSettingsWidget>(MakeWidget(UKalmalaSettingsWidget::StaticClass()));
        Widget->PrepareMenuRender(*Tab);
        Mount(Widget, FVector2D::ZeroVector, Viewport);
    }
    if (!WidgetClass.IsEmpty())
    {
        UClass* Class = LoadClass<UUserWidget>(nullptr, *WidgetClass);
        if (!Class) { World->DestroyWorld(false); return 4; }
        auto* Widget = MakeWidget(Class);
        Widget->SetVisibility(ESlateVisibility::Visible);
        Mount(Widget, FVector2D::ZeroVector, Viewport);
    }
    FAssetCompilingManager::Get().FinishAllCompilation();
    FlushRenderingCommands();
    // The target performs sRGB encoding; avoid a second shader gamma conversion.
    FWidgetRenderer Renderer(false);
    UTextureRenderTarget2D* Target = FWidgetRenderer::CreateTargetFor(Viewport, TF_Bilinear, true);
    TStrongObjectPtr<UTextureRenderTarget2D> KeepTarget(Target);
    Target->ClearColor = FLinearColor(.035f,.04f,.045f,1);
    // Warm the real Slate layout, native ticks, icon geometry and font atlas.
    for (int32 Frame = 0; Frame < 5; ++Frame)
    {
        Canvas->SlatePrepass(1.f);
        Renderer.DrawWidget(Target, Canvas, Viewport, 1.f / 60.f);
        FlushRenderingCommands();
    }
    TArray<FColor> Pixels;
    FReadSurfaceDataFlags ReadFlags;
    ReadFlags.SetLinearToGamma(false);
    if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)) { World->DestroyWorld(false); return 5; }
    for (FColor& Pixel : Pixels) Pixel.A = 255;
    if (Pixels.IsEmpty() || !Pixels.ContainsByPredicate([&Pixels](const FColor& Pixel) { return Pixel != Pixels[0]; }))
    {
        UE_LOG(LogTemp, Error, TEXT("MenuRender produced a uniform/blank image; capture rejected."));
        Widgets.Reset(); World->DestroyWorld(false); return 7;
    }
    TArray64<uint8> PNG;
    FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(Pixels.GetData(),Pixels.Num()), PNG);
    const bool bSaved = FFileHelper::SaveArrayToFile(PNG, *Output, &IFileManager::Get(), FILEWRITE_NoReplaceExisting);
    Widgets.Reset(); World->DestroyWorld(false);
    UE_LOG(LogTemp, Display, TEXT("MenuRender: Menu=%s Size=%dx%d Saved=%d Seconds=%.2f Output=%s"),
        *Menu, Width, Height, bSaved, FPlatformTime::Seconds() - RenderStartSeconds, *Output);
    return bSaved ? 0 : 6;
}
