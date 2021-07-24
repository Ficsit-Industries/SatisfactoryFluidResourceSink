
#include "FluidResourceSinkModule.h"

#include "SML/Public/Patching/NativeHookManager.h"
#include "Buildables/FGBuildableResourceSink.h"
#include "FGPipeConnectionFactory.h"
#include "Resources/FGItemDescriptor.h"
#include "FGResourceSinkSubsystem.h"

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
#endif
}

IMPLEMENT_GAME_MODULE(FFluidResourceSinkModule, FluidResourceSink);
