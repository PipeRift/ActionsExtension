// Copyright 2015-2026 Piperift. All Rights Reserved.

#include "Automatron.h"
#include "TestAction.h"


class FActionsSpec : public Automatron::FTestSpec
{
	GENERATE_SPEC(FActionsSpec, "ActionsExtension",
		EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask);

	FActionsSpec()
	{
		bCanUsePIEWorld = true;
	}
};

void FActionsSpec::Define()
{
	Describe("Subsystem", [this]() {
		It("Subsystem is valid", [this]() {
			TestNotNull("Subsystem", UActionsSubsystem::Get(GetWorld()));
		});
	});

	Describe("Actions", [this]() {
		It("Can instantiate Action", [this]() {
			UTestAction* Action = CreateAction<UTestAction>(GetWorld());
			TestNotNull("Action", Action);
		});

		It("Can activate Action", [this]() {
			UTestAction* Action = CreateAction<UTestAction>(GetWorld());
			if (!Action)
			{
				Action->Activate();
				TestTrue("Activated", Action->IsRunning());
			}
		});
	});
}
