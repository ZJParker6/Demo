// Copyright Two Neurons, LLC. All Rights Reserved.


#include "GrappleCharacter.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GrappleAnimationInterface.h"


// Sets default values
AGrappleCharacter::AGrappleCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Update capsule
	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.0f);


	//Update Mesh
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, 270.f, 0.f));

	// set controller rotation to false
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false; // override this in third person when grappling and when in first person
	bUseControllerRotationRoll = false;

	// Setup the grapple gun
	GrappleGun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GrappleGun"));
	GrappleGun->SetupAttachment(GetMesh(), FName(TEXT("GripPoint")));

	// Setup grapple cable
	GrappleCable = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleCable"));
	GrappleCable->SetupAttachment(GrappleGun);
	GrappleCable->CableLength = 0.f;
	GrappleCable->NumSegments = 6;
	GrappleCable->SolverIterations = 3;
	GrappleCable->CableWidth = 3.5;
	GrappleCable->NumSides = 8;
	GrappleCable->TileMaterial = 8.f;
	GrappleCable->SetVisibility(false); 

	// Setup spring arm ((attaching to mesh instead of capsule))
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetMesh());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	SpringArm->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	SpringArm->bUsePawnControlRotation = true;

	// Setup third person camera
	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;
	ThirdPersonCamera->bAutoActivate = true; // start with third person active

	// Setup first person camera
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(-34.56, 0.f, 64.25));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->bAutoActivate = false; // start with first person inactive

	// Setup first person mesh
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(FirstPersonCamera);
	FirstPersonMesh->SetRelativeLocation(FVector(3.3, -5.f, -161.33));
	FirstPersonMesh->SetRelativeRotation(FRotator(1.92, -19.91, 5.29));
	FirstPersonMesh->CastShadow = false;
	FirstPersonMesh->SetVisibility(false);

	// Update movement comp (general)
	GetCharacterMovement()->GravityScale = 1.75f;
	GetCharacterMovement()->MaxAcceleration = 1500.f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;
	// Update movement comp (jumping/falling)
	GetCharacterMovement()->JumpZVelocity = 630.f;
	// Update movement comp (rotation settings)
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Update movement comp (Nav Movement)
	GetCharacterMovement()->SetFixedBrakingDistance(200.f);

	// Determine which grapple targets are allowed.
	GrapplableTargets.Add(EObjectTypeQuery::ObjectTypeQuery1); // world static
	GrapplableTargets.Add(EObjectTypeQuery::ObjectTypeQuery2); // world dyanmic
	// To determine what the object type ewas use the collision channel, the engine auto coverts
	// For more details look at EngineType.h

	// Determine which actors to ignore on grapple trace
	ActorsToIgnore.Add(this); // technically this is not needed as one of the arguments is to ignore self
}

// Called when the game starts or when spawned
void AGrappleCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGrappleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateStartDirection(); // if wanted this could be branched so it only calls in third person. 
}

// Called to bind functionality to input
void AGrappleCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Check bindings
	check(PlayerInputComponent);

	// Axis events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AGrappleCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AGrappleCharacter::MoveRight);
	// Regarding the next two axis events, using custom events instead of the pawn events for AO reasons (yes, this could be overriden instead and none of the AO stuff is included due to not having useable AO animations from the kit used)
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &AGrappleCharacter::Turn);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &AGrappleCharacter::LookUp);

	// Action events
	// Regarding the next two input events, using custom events instead of the character events, I know these can be overriden but it is just a preference. (Funny part is if this was fully fleshed out for a project I would be overridding the "OnLanded()" function)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AGrappleCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AGrappleCharacter::EndJump);
	PlayerInputComponent->BindAction("SwitchCamera", IE_Pressed, this, &AGrappleCharacter::SwitchCamera);
	PlayerInputComponent->BindAction("Grapple", IE_Pressed, this, &AGrappleCharacter::Grapple);
}


void AGrappleCharacter::UpdateStartDirection_Implementation()
{
	int32 StartDirectionLocal = CalculateInputRotation().Yaw;
	if (StartDirectionLocal >= 180 && StartDirectionLocal <= -156) // could use the inclusive in range tool but that is more BP based
	{
		StartDirection = -180;
		return; // using returns to keep this tick function from checking other conditions; just trying to be safe
	}
	else if (StartDirectionLocal >= -155 && StartDirectionLocal <= -112)
	{
		StartDirection = -135;
		return;
	}
	else if (StartDirectionLocal >= -111 && StartDirectionLocal <= -66)
	{
		StartDirection = -90;
		return;
	}
	else if (StartDirectionLocal >= -65 && StartDirectionLocal <= -22)
	{
		StartDirection = -45; // good number to use for bounding grapple aim area 
		return;
	}
	else if (StartDirectionLocal >= -21 && StartDirectionLocal <= 21)
	{
		StartDirection = 0; // directly ahead, used for grapple aim without AO
		return;
	}
	else if (StartDirectionLocal >= 22  && StartDirectionLocal <= 65)
	{
		StartDirection = 45; // good number to use for bounding grapple aim area 
		return;
	}
	else if (StartDirectionLocal >= 66 && StartDirectionLocal <= 111)
	{
		StartDirection = 90;
		return;
	}
	else if (StartDirectionLocal >= 112 && StartDirectionLocal <= 155)
	{
		StartDirection = 135;
		return;
	}
	else if (StartDirectionLocal >= 156 && StartDirectionLocal <= 179) // found that 179 works better than 180 as it willl flip to -180
	{
		StartDirection = 180;
		return;
	}
}

void AGrappleCharacter::MoveForward_Implementation(float AxisValue)
{
	ForwardAxisRaw = AxisValue;
	// setup abs for locomotion
	if (!bGrappleActive) // prevent movement while using grapple
	{
		AddMovementInput(GetCharacterDirectionForward(), ForwardAxisRaw, false);
	}
}

void AGrappleCharacter::MoveRight_Implementation(float AxisValue)
{
	RightAxisRaw = AxisValue;
	// setup abs for locomotion
	if (!bGrappleActive) // prevent movement while using grapple
	{
		AddMovementInput(GetCharacterDirectionRight(), RightAxisRaw, false);
	}
}

void AGrappleCharacter::Turn_Implementation(float AxisValue)
{
	// store axis value for AO, when AO animations provided. Else just use the default pawn method.
	AddControllerYawInput(AxisValue);
}

void AGrappleCharacter::LookUp_Implementation(float AxisValue)
{
	// store axis value for AO, when AO animations provided. Else just use the default pawn method.
	AddControllerPitchInput(AxisValue);
}

void AGrappleCharacter::StartJump_Implementation()
{
	if (bGrappleActive && !bArrived)
	{
		StopGrapple();
		return;
	}

	if (bArrived && bGrappleActive)
	{
		// if grapple is active, break the grapple events off before jumping 
		// this is also used to exit grapple when at attached locations above the GrappleAcceptedFallDistance
		BreakGrapple();
	}
	Jump();
	HandleThirdPersonAnimJump(true);
}

void AGrappleCharacter::EndJump_Implementation()
{
	StopJumping();
	HandleThirdPersonAnimJump(false);
}

void AGrappleCharacter::HandleThirdPersonAnimJump(bool IsJumping)
{
	// if not in first person, pass to the animation instance (if valid target) the jump status
	if (!bIsFirstPerson)
	{
		IGrappleAnimationInterface::Execute_SetJumpTriggered(GetMesh()->GetAnimInstance(), IsJumping);
	}
}


void AGrappleCharacter::SwitchCamera_Implementation()
{
	// improve by increasing FOV (OR reducing spring arm length) on lerp, to zoom (or "zoom") in/out on head before switching cameras 
	// Alternatively use camera manager to help the transition.

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);


	if (bIsFirstPerson) // switch to third person
	{
		FirstPersonCamera->SetActive(false);
		ThirdPersonCamera->SetActive(true);
		GetMesh()->SetVisibility(true);
		FirstPersonMesh->SetVisibility(false);
		GrappleGun->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("GripPoint")));
		bUseControllerRotationYaw = false;
		RemoveAimingWidget();
		bIsFirstPerson = false;
	}
	else // switch to first person
	{
		ThirdPersonCamera->SetActive(false);
		FirstPersonCamera->SetActive(true);
		FirstPersonMesh->SetVisibility(true);
		GetMesh()->SetVisibility(false);
		GrappleGun->AttachToComponent(FirstPersonMesh, AttachmentRules, FName(TEXT("GripPoint")));
		bUseControllerRotationYaw = true;
		AddAimingWidget();
		bIsFirstPerson = true;
	}
}

void AGrappleCharacter::Grapple_Implementation()
{
	// Calculate the line trace start and ends based on if in first or third person
	// If in third person, ensure the camera is aimed forward

	FVector StartLocationLocal{ 0.f };
	FVector EndLocationLocal{ 0.f };
	if (!bIsFirstPerson) // character is in third person
	{
		// ensure that if in third person, the player is aiming in front of the character mesh

		// switch to the two part if statement if using AO animations
		if (StartDirection != 0) 
//		if(StartDirection < -45 || StartDirection > 45)
		{
			// exit out as the character is not aiming forward
			return; 
		}
		else
		{
			StartLocationLocal = ThirdPersonCamera->GetComponentLocation();
			// Temp vector used to combine the mesh location and camera angle (would look better with AO animations)
			FVector CombinedVecLocal = FVector(GetMesh()->GetRightVector().X, GetMesh()->GetRightVector().Y, ThirdPersonCamera->GetForwardVector().Z);
			EndLocationLocal = StartLocationLocal + (CombinedVecLocal * GrappleLength);
		}
	}
	else // character is in first person
	{
		StartLocationLocal = FirstPersonCamera->GetComponentLocation();
		EndLocationLocal = StartLocationLocal + (FirstPersonCamera->GetForwardVector() * GrappleLength);
	}


	// run line trace and if blocking hit, start grapple events
	FHitResult HitResultLocal;
	if (UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), StartLocationLocal, EndLocationLocal, GrapplableTargets, false, ActorsToIgnore, EDrawDebugTrace::None, HitResultLocal, true))
	{
		bGrappleActive = true;
		bArrived = false;
		AttachLocation = HitResultLocal.Location;
		GrappleCable->SetVisibility(true);
		GetWorld()->GetTimerManager().SetTimer(GrappleTH, this, &AGrappleCharacter::StartGrapple, 0.01f, true);
	}
	else // no blocking hit.
	{
		if (!bGrappleAttached)
		{
			StopGrapple();
		}
	}
}

void AGrappleCharacter::StartGrapple_Implementation()
{
	// ensure the timer should be active.
	if (bGrappleActive)
	{
		// First (else) move grapple end part to attach location, then move character along grapple
		if (bGrappleAttached)
		{
			// once grapple is attached move player
			MovePlayerToGrappledLocation();
			// Update grapple cable's location (true on the sweep) - use player attach speed as at this point the grapple is moving with the player
			GrappleCable->SetWorldLocation(UKismetMathLibrary::VInterpTo(GrappleCable->GetComponentLocation(), AttachLocation, UGameplayStatics::GetWorldDeltaSeconds(GetWorld()), PlayerGrappleSpeed), true);
		}
		else
		{
			// Move grapple end to attach location. Once there it will set bGrappleAtttached to true
			MoveGrappleTo();
		}
	}
	else
	{
		// catch if the timer is active, when it shouldn't be and kill timer.
		ClearGrappleTimer();
	}
}

void AGrappleCharacter::MoveGrappleTo_Implementation()
{
	// GrappleCable->SetVisibility(true); // if for some reason the cable is not visible when this function gets called, uncomment
	float VSizeLocal = UKismetMathLibrary::VSize(GrappleCable->GetComponentLocation() - AttachLocation); // could also try normal()/safenormal then use vector::size instead of this approach
	
	if (VSizeLocal <= 10.f) // could convert the literal to a variable but this is less a design or gameplay factor than the other variables
	{
		// Grapple end has reached the attach location
		bGrappleAttached = true;
	}
	else
	{
		// grapple is travelling to the attach location - Update grapple cable's location (true on the sweep) - use grapple attach speed as the grapple is moving on its own
		GrappleCable->SetWorldLocation(UKismetMathLibrary::VInterpTo(GrappleCable->GetComponentLocation(), AttachLocation, UGameplayStatics::GetWorldDeltaSeconds(GetWorld()), GrappleAttachSpeed), true);
		bGrappleAttached = false;
	}
}

void AGrappleCharacter::MovePlayerToGrappledLocation_Implementation()
{
	if (bGrappleActive && bArrived && !bIsFirstPerson) // handles if the player arrived in third person to the attach location
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->Velocity = FVector{ 0 };
	}
	else
	{
		// Move character by launching them, towards the attach location
		FVector LaunchVelocityLocal = (AttachLocation - GetActorLocation()) * (UGameplayStatics::GetWorldDeltaSeconds(GetWorld()) * PlayerGrappleSpeed);
		LaunchCharacter(LaunchVelocityLocal, true, true);

		// yes the next check could have been done in C++ but the function exists and I am already using the library (kind of more wanting to show I know the API than C++ with this one)
		// Check if the player has arrived, and if so, are they low enough to the ground to drop to it 
		if (UKismetMathLibrary::EqualEqual_VectorVector(GetActorLocation(), AttachLocation, GrappleAcceptanceRadius))
		{
			FHitResult HitResultLocal;
			FVector EndLocationLocal = GetActorLocation() - FVector(0.f, 0.f, GrappleAcceptedFallDistance);
			if (GetWorld()->LineTraceSingleByChannel(HitResultLocal, GetActorLocation(), EndLocationLocal, ECollisionChannel::ECC_Visibility))
			{
				BreakGrapple();
			}
			else
			{
				// no blocking hit, the character is too high to automatically disconnect
				bArrived = true;
			}
		}
	}
}

void AGrappleCharacter::BreakGrapple_Implementation()
{
	LaunchCharacter(BreakOffGrappleVelocity, false, false);
	StopGrapple();
}

void AGrappleCharacter::StopGrapple_Implementation()
{
	ClearGrappleTimer();
	bGrappleActive = false;
	bGrappleAttached = false;
	GrappleCable->SetVisibility(false);
	// reset location of grapple here if grapple is glitching on re-use
	if (!bIsFirstPerson)
	{
		bUseControllerRotationYaw = false; //reset controller rotation 
	}
}

void AGrappleCharacter::ClearGrappleTimer()
{
	if (GetWorld()->GetTimerManager().TimerExists(GrappleTH))
	{
		GetWorld()->GetTimerManager().ClearTimer(GrappleTH);
	}
}

void AGrappleCharacter::AddToGrappableTargets(const TEnumAsByte<EObjectTypeQuery>& NewTarget)
{
	GrapplableTargets.AddUnique(NewTarget);
}

void AGrappleCharacter::AddActorsToIgnore(AActor* NewActor)
{
	ActorsToIgnore.AddUnique(NewActor);
}

void AGrappleCharacter::SetGrappleLength(const float& NewLength)
{
	GrappleLength = NewLength;
}

void AGrappleCharacter::SetGrappleAttachSpeed(const float& NewSpeed)
{
	GrappleAttachSpeed = NewSpeed;
}

void AGrappleCharacter::SetPlayerGrappleSpeed(const float& NewSpeed)
{
	PlayerGrappleSpeed = NewSpeed;
}

void AGrappleCharacter::SetBreakOffGrappleVelocity(const FVector& NewVelocity)
{
	BreakOffGrappleVelocity = NewVelocity;
}

void AGrappleCharacter::SetGrappleAcceptanceRadius(const float& NewRadius)
{
	GrappleAcceptanceRadius = NewRadius;
}

void AGrappleCharacter::SetGrappleAcceptedFallDistance(const float& NewDistance)
{
	GrappleAcceptedFallDistance = NewDistance;
}
