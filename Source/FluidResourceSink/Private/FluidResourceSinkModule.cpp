
#include "FluidResourceSinkModule.h"

#include "SML/Public/Patching/NativeHookManager.h"
#include "SML/Public/Patching/BlueprintHookManager.h"
#include "SML/Public/Patching/BlueprintHookHelper.h"
#include "Buildables/FGBuildableResourceSink.h"
#include "FGPipeConnectionFactory.h"
#include "Resources/FGItemDescriptor.h"
#include "FGResourceSinkSubsystem.h"
#include "UI/FGInteractWidget.h"
#include "FGGameMode.h"

DEFINE_LOG_CATEGORY(LogFluidResourceSink);



void FFluidResourceSinkModule::StartupModule() {
#if !WITH_EDITOR
	AFGBuildableResourceSink* ResourceSinkCDO = GetMutableDefault<AFGBuildableResourceSink>();
	SUBSCRIBE_METHOD_VIRTUAL(AFGBuildableResourceSink::Factory_CollectInput_Implementation, ResourceSinkCDO, [](auto& sope, AFGBuildableResourceSink* self) {
		if (!self->mResourceSinkSubsystem)
			return;

		// Check if we are a liquid sink
		FString className;
		self->GetClass()->GetName(className);
		if (className.StartsWith(TEXT("FRS_"))) {
			UFGPipeConnectionFactory* pipeConnectionFactory = self->FindComponentByClass<UFGPipeConnectionFactory>();

			FFluidBox* fluidBox = pipeConnectionFactory->GetFluidBox();
			if (!fluidBox) return;

			// try pull as much liquid as we can
			int32 contentLiters = fluidBox->GetContentInLiters();
			if (contentLiters >= 1000) {
				TArray< UFGPipeConnectionComponent* > connections = pipeConnectionFactory->GetPipeConnections();
				if (connections.Num() == 0) return;

				// Find the Fluid / Gas Descriptor we're dealing with
				TSubclassOf< UFGItemDescriptor > itemDesc;
				for (int i = 0, len = connections.Num(); i < len; i++) {
					itemDesc = connections[0]->GetFluidDescriptor();
					if (itemDesc) break;
				}
				
				if (!itemDesc) return;
				

				// Try to consume the fluid / gas
				if (self->mResourceSinkSubsystem->AddPoints_ThreadSafe(itemDesc)) {
					fluidBox->RemoveContentInLiters(1000);
					self->mProducingTimer = 3.0f;
				}
			}
		}
	});

	AFGGameMode* gameModeCDO = GetMutableDefault<AFGGameMode>();
	SUBSCRIBE_METHOD_VIRTUAL(AFGGameMode::PostLogin, gameModeCDO, [](auto& scope, AFGGameMode* self, APlayerController* playerController) {
		// fixup the blueprint so it works with buildings that don't inherit from Build_ResourceSink, but from FGBuildableResourceSink.
		UBlueprintHookManager* HookManager = GEngine->GetEngineSubsystem<UBlueprintHookManager>();
		check(HookManager);
		UClass* ResourceSinkWidgetClass = LoadObject<UClass>(nullptr, TEXT("/Game/FactoryGame/Buildable/Factory/ResourceSink/UI/BPW_ResourceSink.BPW_ResourceSink_C"));
		check(ResourceSinkWidgetClass);
		UFunction* RSWConstructFunction = ResourceSinkWidgetClass->FindFunctionByName(TEXT("ExecuteUbergraph_BPW_ResourceSink"));
		check(RSWConstructFunction);

		HookManager->HookBlueprintFunction(RSWConstructFunction, [](FBlueprintHookHelper& helper) {
			UObject* context = helper.GetContext();

			FObjectProperty* mResourceSinkProperty = CastFieldChecked<FObjectProperty>(context->GetClass()->FindPropertyByName(TEXT("mResourceSink")));
			check(mResourceSinkProperty);
			UObject* mResourceSink = mResourceSinkProperty->GetPropertyValue_InContainer(context, 0);
			if (!mResourceSink) {
				UFGInteractWidget* interactWidget = CastChecked<UFGInteractWidget>(context);
				mResourceSinkProperty->SetPropertyValue_InContainer(context, interactWidget->mInteractObject, 0);
			}
		}, 1707);
	});
#endif
}

IMPLEMENT_GAME_MODULE(FFluidResourceSinkModule, FluidResourceSink);
