// Copyright Two Neurons, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GrappleAnimationInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGrappleAnimationInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DEMO_API IGrappleAnimationInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation|Jump")
	void SetJumpTriggered(bool IsJumping);

};
