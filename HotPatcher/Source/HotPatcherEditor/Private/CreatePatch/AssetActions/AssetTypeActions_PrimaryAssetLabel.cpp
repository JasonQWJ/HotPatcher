#include "AssetTypeActions_PrimaryAssetLabel.h"
#include "Engine/PrimaryAssetLabel.h"
#include "ToolMenuSection.h"
#include "HotPatcherEditor.h"
#include "Flib/FLibAssetManageHelperEx.h"

#if WITH_EDITOR_SECTION
void FAssetTypeActions_PrimaryAssetLabel::GetActions(const TArray<UObject*>& InObjects,
	FToolMenuSection& Section)
{
	auto Labels = GetTypedWeakObjectPtrs<UPrimaryAssetLabel>(InObjects);
	Section.AddMenuEntry(
	"ObjectContext_AddToPatchIncludeFilters",
	NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToPatchIncludeFilters", "Add To Patch Include Filters"),
	NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToPatchIncludeFiltersTooltip", "Add the label to HotPatcher Include Filters."),
	FSlateIcon(),
	FUIAction(
		FExecuteAction::CreateSP(this, &FAssetTypeActions_PrimaryAssetLabel::ExecuteAddToPatchIncludeFilter, Labels)
	));
	Section.AddMenuEntry(
			"ObjectContext_AddToChunkConfig",
			NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToChunkConfig", "Add To Chunk Config"),
			NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToChunkConfigTooltip", "Add Label To Chunk Config"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_PrimaryAssetLabel::ExecuteAddToChunkConfig, Labels)
			));
}
TArray<FPatcherSpecifyAsset> GetLabelsAssets(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	TArray<FPatcherSpecifyAsset> LabelAsstes;
	for(auto& Label:Objects)
	{
		if( Label->Rules.CookRule == EPrimaryAssetCookRule::NeverCook )
			continue;
		for(const auto& Asset:Label->ExplicitAssets)
		{
			FPatcherSpecifyAsset CurrentAsset;
			CurrentAsset.Asset = Asset.ToSoftObjectPath();
			CurrentAsset.bAnalysisAssetDependencies = Label->bLabelAssetsInMyDirectory;
			CurrentAsset.AssetRegistryDependencyTypes.AddUnique(EAssetRegistryDependencyTypeEx::Packages);
			LabelAsstes.AddUnique(CurrentAsset);
		}
	}
	return LabelAsstes;
}

TArray<FDirectoryPath> GetLabelsDirs(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	TArray<FDirectoryPath> Dirs;
	for(auto& Label:Objects)
	{
		if(Label->bLabelAssetsInMyDirectory && (Label->Rules.CookRule != EPrimaryAssetCookRule::NeverCook))
		{
			FString PathName = Label->GetPathName();
			TArray<FString> DirNames;
			PathName.ParseIntoArray(DirNames,TEXT("/"));
			FString FinalPath;
			for(size_t index = 0;index < DirNames.Num() - 1;++index)
			{
				FinalPath += TEXT("/") + DirNames[index];
			}
			FDirectoryPath CurrentDir;
			CurrentDir.Path = FinalPath;
			Dirs.Add(CurrentDir);
		}
	}
	return Dirs;
}

void FAssetTypeActions_PrimaryAssetLabel::ExecuteAddToPatchIncludeFilter(
	TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	FHotPatcherEditorModule::Get().OpenDockTab();
	if(GPatchSettings)
	{
		GPatchSettings->IncludeSpecifyAssets.Append(GetLabelsAssets(Objects));
		GPatchSettings->AssetIncludeFilters.Append(GetLabelsDirs(Objects));
	}
}

TArray<FChunkInfo> GetChunksByAssetLabels(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	TArray<FChunkInfo> Chunks;

	for(const auto& Object:Objects)
	{
		FChunkInfo Chunk;
		Chunk.AssetIgnoreFilters = GetLabelsDirs(TArray<TWeakObjectPtr<UPrimaryAssetLabel>>{Object});
		Chunk.IncludeSpecifyAssets = GetLabelsAssets(TArray<TWeakObjectPtr<UPrimaryAssetLabel>>{Object});
		Chunk.bAnalysisFilterDependencies = Object->Rules.bApplyRecursively;
		FString LongPackageName;
		if(UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(Object->GetPathName(),LongPackageName))
		{
			TArray<FString> DirNames;
			LongPackageName.ParseIntoArray(DirNames,TEXT("/"));
			Chunk.ChunkName = DirNames[DirNames.Num()-1];
		}
	}
	return Chunks;
}

void FAssetTypeActions_PrimaryAssetLabel::ExecuteAddToChunkConfig(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	FHotPatcherEditorModule::Get().OpenDockTab();
	if(GPatchSettings)
	{
		GPatchSettings->bEnableChunk = true;
		GPatchSettings->ChunkInfos.Append(GetChunksByAssetLabels(Objects));
	}
}

#endif