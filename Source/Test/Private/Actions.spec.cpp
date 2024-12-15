// Copyright 2015-2023 Piperift. All Rights Reserved.

#include "TestAction.h"
#include "TestHelpers.h"


constexpr const EAutomationTestFlags Flags_Product =
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask;
constexpr const EAutomationTestFlags Flags_Smoke =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;


#define BASE_SPEC FACESpec


BEGIN_TESTSPEC(FSubsystemSpec, "ActionsExtension.Subsystem", Flags_Product)
UWorld* World = nullptr;
END_TESTSPEC(FSubsystemSpec)
void FSubsystemSpec::Define()
{
	BeforeEach([this]() {
		World = GetTestWorld();
	});

	It("Subsystem is valid", [this]() {
		TestNotNull("Subsystem", UActionsSubsystem::Get(World));
	});
}


BEGIN_TESTSPEC(FActionsSpec, "ActionsExtension.Actions", Flags_Smoke)
UWorld* World = nullptr;
END_TESTSPEC(FActionsSpec)
void FActionsSpec::Define()
{
	BeforeEach([this]() {
		World = GetTestWorld();
	});

	It("Can instantiate Action", [this]() {
		UTestAction* Action = CreateAction<UTestAction>(World);
		TestNotNull("Action", Action);
	});

	It("Can activate Action", [this]() {
		UTestAction* Action = CreateAction<UTestAction>(World);
		if (!Action)
		{
			Action->Activate();
			TestTrue("Activated", Action->IsRunning());
		}
	});
}
