// Copyright Two Neurons, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "CableComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GrappleCharacter.generated.h"

UCLASS()
class AGrappleCharacter : public ACharacter
{
	GENERATED_BODY()

/*************************************
* ATTRIBUTES
*************************************/
protected:
	/********************************
	* COMPONENT ATTRIBUTES
	********************************/
	/** The skeletal mesh that will be used for the weapon/grappling gun*/
	UPROPERTY(BlueprintReadonly, EditDefaultsOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> GrappleGun;
	/** The cable that will represent the the grappling rope/cable */
	UPROPERTY(BlueprintReadonly, EditDefaultsOnly, Category = "Components")
	TObjectPtr<class UCableComponent> GrappleCable;
	/** Camera spring arm */
	UPROPERTY(BlueprintReadonly, EditDefaultsOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArm;
	/** Third person camera */
	UPROPERTY(BlueprintReadonly, EditDefaultsOnly, Category = "Components")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;
	/** First person camera */
	UPROPERTY(BlueprintReadonly, EditDefaultsOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;
	/** Arms mesh used for first person */
	UPROPERTY(BlueprintReadonly, EditDefaultsOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	/********************************
	* CAMERA ATTRIBUTES
	********************************/
	/** Is the first person camera active or not */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Camera|States")
	bool bIsFirstPerson{ false };

	/********************************
	* MOVEMENT ATTRIBUTES
	********************************/
	/** The forward axis value (used for calculations around locomotion and grappling - though for this demo only grappling is implemented. Locomotion system would also need an abs value) */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Movement")
	float ForwardAxisRaw{ 0.f };
	/** The right axis value (used for calculations around locomotion and grappling - though for this demo only grappling is implemented. Locomotion system would also need an abs value) */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Movement")
	float RightAxisRaw{ 0.f };
	/** The "start direction" is used for locomotion (to determine on a 2D BS which way to rotate the character on start movement) and to determine, when in third person, if the player is aiming in front for grappling (only grappling part is implemented in this demo) */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Movement")
	int32 StartDirection{ 0 };

	/********************************
	* GRAPPLING ATTRIBUTES
	********************************/
	/** What targets can the player grapple onto. By default these will be world static and world dynamic. If you want to ensure just specific targets are allowed, create a custom object type 
	Old style Enum (TEnumAsByte) used as this is what unreal uses for the line trace */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Settings")
	TArray<TEnumAsByte<EObjectTypeQuery>> GrapplableTargets;
	/** When doing grapple checks, what actors should be ignored (default is self/this) 
	using old pointer style to avoid conversion issues */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Settings")
	TArray<AActor*> ActorsToIgnore;
	/** How long can the grapple cable get (how far can the player grapple) */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Settings")
	float GrappleLength{ 10000.f };
	/** How fast does the cable launch from the character to the attach location */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Settings")
	float GrappleAttachSpeed{ 50.f };
	/** How fast does the character travel along the grapple to the attach location */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Settings")
	float PlayerGrappleSpeed{ 250.f };
	/** What velocity and direction does the character travel when they break off the grappling hook (there should be a Z value of some sort to give the appearance of detatching. Other values are optional) */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Settings")
	FVector BreakOffGrappleVelocity{ 0.f, 0.f, 750.f };
	/** How close to the attach location must the character be for the movement to accepted (larger values are useful here depending on the thickness of meshes to which the player can attach - a minimum of 45. based on starter content is recommended) */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Settings")
	float GrappleAcceptanceRadius{ 45.f };
	/** How far up from the ground can the character be before the grapple automatically releases them (when they arrived at the attach location). If using fall damage, this value should be SMALLER than the value used for fall damage */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Settings")
	float GrappleAcceptedFallDistance{ 150.f };

	/** Where is the grapple attached/where is the player travelling to */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Runtime")
	FVector AttachLocation{ 0.f };

	/** Has the player activated the grapple and has the grapple hit an acceptable attach location */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|States")
	bool bGrappleActive{false};
	/** Has the grapple attached to the attach location. There is a space of time (based on GrappleAttachSpeed) between the player activating the grapple and the grapple reaching the attach location */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|States")
	bool bGrappleAttached{ false };
	/** Has the player travelled the length of the grapple to reach the accepted area around the attach location */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|States")
	bool bArrived{ false };

	/** The timer handle used to control the grapple and player movement  */
	UPROPERTY(BlueprintReadwrite, EditDefaultsOnly, Category = "Grappling|Timer")
	FTimerHandle GrappleTH;

/*************************************
* METHODS
*************************************/
	/********************************
	* CONSTRUCTORS
	********************************/
public:
	// Sets default values for this character's properties
	AGrappleCharacter();

	/********************************
	* INHERITED METHODS
	********************************/
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/********************************
	* MEMBER METHODS
	********************************/
protected:
	/** Calculates the start direction for locomotion (not implemented) and use with the grappling system to help determine if the player can shoot grapple in look direction */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movement|Tick")
	void UpdateStartDirection();
	/* Gets the input rotation used to update start direction. Using FRotator return isntead of a ref based return to avoid seting yet another variable... already setting two in this just for readability - readability is needed, if these two variables cause an issue then something more profound is wrong than this inline method */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Movement|Tick")
	FRotator CalculateInputRotation() const
	{
		FRotator ALocal = UKismetMathLibrary::NormalizedDeltaRotator(ThirdPersonCamera->GetComponentRotation(), GetCapsuleComponent()->GetComponentRotation());
		FRotator BLocal = UKismetMathLibrary::MakeRotFromX(FVector(ForwardAxisRaw, (-1.f * RightAxisRaw), 0.f));
		return UKismetMathLibrary::NormalizedDeltaRotator(ALocal, BLocal);
	}

	/** Move forward/backwards input event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Movement")
	void MoveForward(float AxisValue);
	/** Move right/left input event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Movement")
	void MoveRight(float AxisValue);
	/** Calculates the forward vector based on control rotation. Using an FVector return instead of a ref based return to keep from having to store data in an yet another variable */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Input|Movement")
	FORCEINLINE FVector GetCharacterDirectionForward() const
	{
		return UKismetMathLibrary::GetForwardVector(FRotator(0.f, GetControlRotation().Yaw, 0.f));
	}
	/** Calculates the right vector based on control rotation. Using an FVector return instead of a ref based return to keep from having to store data in an yet another variable */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Input|Movement")
	FORCEINLINE FVector GetCharacterDirectionRight() const
	{
		return UKismetMathLibrary::GetRightVector(FRotator(0.f, GetControlRotation().Yaw, 0.f));
	}
	/** turn event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Camera")
	void Turn(float AxisValue);
	/** look up event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Camera")
	void LookUp(float AxisValue);


	/** When the character jumps breaks out of grapple event if grappling is occuring, and/else jumps */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Jump")
	void StartJump();
	/** Handles the jump release events */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Jump")
	void EndJump();
	/** Passes if the character is jumping to third person animations (first person is handld differently - I didn't change the first person animation) */
	UFUNCTION(BlueprintCallable, Category = "Input|Jump")
	void HandleThirdPersonAnimJump(bool IsJumping);
	/** Switches between third and first person camera */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Camera")
	void SwitchCamera();
	/** Turns on first person aiming widget - this is made implementable as the widget will have a C++ base and no reference will be stored in GrappleCharacter base class for the widget */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Input|Camera")
	void AddAimingWidget();
	/** Turns off first person aiming widget - this is made implementable as the widget will have a C++ base and no reference will be stored in GrappleCharacter base class for the widget */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Input|Camera")
	void RemoveAimingWidget();
	/** Starts the grapple events  */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input|Grapple")
	void Grapple();

	/** Timer event that runs the grapple system and starts/updates all other grapple events */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grapple")
	void StartGrapple();
	/** Moves the grapple cable's end part to the attach location (this happens before the player moves along the grapple cable) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grapple")
	void MoveGrappleTo();
	/** Moves player along the grapple cable to the attached location , also handles what to do when the player arrives at the location (in third person) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grapple")
	void MovePlayerToGrappledLocation();
	/** Breaks the character out of grappling state and then stops timer */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grapple")
	void BreakGrapple();
	/** Ends grapple timer and resets states */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grapple")
	void StopGrapple();
	/** Used by stop grapple to clear time and as a catch-all for the timer only. Ends the grapple timer in the case that it is active but the bool states are already false (this can happen if the timer is still firing when the bool states change or if the timer becomes out of sync) */
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void ClearGrappleTimer();

	/***********
	* Setters
	***********/
public:
	UFUNCTION(BlueprintCallable, Category = "Grapple|Setters", meta = (AutoCreateRefTerm = "NewTarget"))
	void AddToGrappableTargets(const TEnumAsByte<EObjectTypeQuery>& NewTarget);
	UFUNCTION(BlueprintCallable, Category = "Grapple|Setters")
	void AddActorsToIgnore(AActor* NewActor);
	UFUNCTION(BlueprintCallable, Category = "Grapple|Setters", meta = (AutoCreateRefTerm = "NewLength"))
	void SetGrappleLength(const float& NewLength);
	UFUNCTION(BlueprintCallable, Category = "Grapple|Setters", meta = (AutoCreateRefTerm = "NewSpeed"))
	void SetGrappleAttachSpeed(const float& NewSpeed);
	UFUNCTION(BlueprintCallable, Category = "Grapple|Setters", meta = (AutoCreateRefTerm = "NewSpeed"))
	void SetPlayerGrappleSpeed(const float& NewSpeed);
	UFUNCTION(BlueprintCallable, Category = "Grapple|Setters", meta = (AutoCreateRefTerm = "NewVelocity"))
	void SetBreakOffGrappleVelocity(const FVector& NewVelocity);
	UFUNCTION(BlueprintCallable, Category = "Grapple|Setters", meta = (AutoCreateRefTerm = "NewRadius"))
	void SetGrappleAcceptanceRadius(const float& NewRadius);
	UFUNCTION(BlueprintCallable, Category = "Grapple|Setters", meta = (AutoCreateRefTerm = "NewDistance"))
	void SetGrappleAcceptedFallDistance(const float& NewDistance);

	/***********
	* Getters
	***********/
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Components|Getters")
	FORCEINLINE USkeletalMeshComponent* GetGrappleGun() const 
	{ 
		if (GrappleGun)
			return GrappleGun;
		else
			return nullptr;
	}
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Components|Getters")
	FORCEINLINE class UCableComponent* GetGrappleCable() const 
	{ 
		if (GrappleCable) 
			return GrappleCable;
		else
			return nullptr;
	}
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Components|Getters")
	FORCEINLINE USpringArmComponent* GetSpringArm() const
	{ 
		if (SpringArm) 
			return SpringArm; 
		else
			return nullptr;
	}
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Components|Getters")
	FORCEINLINE UCameraComponent* GetThirdPersonCamera() const 
	{ 
		if (ThirdPersonCamera) 
			return ThirdPersonCamera; 
		else
			return nullptr;
	}
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Components|Getters")
	FORCEINLINE UCameraComponent* GetFirstPersonCamera() const 
	{ 
		if (FirstPersonCamera)
			return FirstPersonCamera;
		else
			return nullptr;
	}
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Components|Getters")
	FORCEINLINE UCameraComponent* GetActiveCamera() const
	{
		if (bIsFirstPerson)
			return FirstPersonCamera;
		else if (!bIsFirstPerson)
			return ThirdPersonCamera;
		else
			return nullptr;
	}
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Components|Getters")
	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const
	{
		if (FirstPersonMesh)
			return FirstPersonMesh;
		else
			return nullptr;
	}

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Camera|Getters")
	FORCEINLINE bool GetIsIfFirstPerson() const { return bIsFirstPerson;  }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Camera|Getters")
	FORCEINLINE bool CheckIfInThirdPerson() const { return !bIsFirstPerson; }

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Movement|Getters")
	FORCEINLINE float GetForwardAxisRaw() const { return ForwardAxisRaw; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Movement|Getters")
	FORCEINLINE float GetRightAxisRaw() const { return RightAxisRaw; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Movement|Getters")
	FORCEINLINE int32 GetStartDirection() const { return StartDirection; }

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE TArray<TEnumAsByte<EObjectTypeQuery>> GetGrapplableTargets() const { return GrapplableTargets; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE TArray<AActor*> GetActorsToIgnore() const { return ActorsToIgnore; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE float GetGrappleLength() const { return GrappleLength; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE float GetGrappleAttachSpeed() const { return GrappleAttachSpeed; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE float GetPlayerGrappleSpeed() const { return PlayerGrappleSpeed; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE FVector GetBreakOffGrappleVelocity() const { return BreakOffGrappleVelocity; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE float GetGrappleAcceptanceRadius() const { return GrappleAcceptanceRadius; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE float GetGrappleAcceptedFallDistance() const { return GrappleAcceptedFallDistance; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE FVector GetAttachLocation() const { return AttachLocation; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE bool GetGrappleActive() const { return bGrappleActive; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE bool GetGrappleAttached() const { return bGrappleAttached; }
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Grappling|Getters")
	FORCEINLINE bool GetArrived() const { return bArrived; }
};
