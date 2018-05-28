// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneEventSection.h"
#include "AIGeneric.h"

#include "AINotify.generated.h"

/**
 * 
 */
USTRUCT(Blueprintable)
struct AIEXTENSION_API FAINotify
{
	GENERATED_USTRUCT_BODY()

	/** The name of the event to trigger */
	UPROPERTY(EditAnywhere, Category = Notify)
	FName EventName;

	/** The event parameters */
	UPROPERTY(EditAnywhere, Category = Notify, meta = (ShowOnlyInnerProperties))
	FMovieSceneEventParameters Parameters;


	FORCEINLINE void TriggerNotify(UObject& EventContextObject) const
	{
		TriggerNotify(EventContextObject, EventName, Parameters);
	}

	static bool TriggerNotify(UObject& EventContextObject, const FName EventName, const FMovieSceneEventParameters& InParams)
	{
		UFunction* EventFunction = EventContextObject.FindFunction(EventName);

		if (EventFunction == nullptr)
		{
			// Don't want to log out a warning for every event context.
			return false;
		}
		else
		{
			FStructOnScope ParameterStruct(nullptr);
			InParams.GetInstance(ParameterStruct);

			uint8* Params = ParameterStruct.GetStructMemory();

			const UStruct* Struct = ParameterStruct.GetStruct();
			if (EventFunction->ReturnValueOffset != MAX_uint16)
			{
				UE_LOG(LogTemp, Warning, TEXT("AI Notify: Cannot trigger events that return values (for event '%s')."), *EventName.ToString());
				return false;
			}
			else
			{
				TFieldIterator<UProperty> ParamIt(EventFunction);
				TFieldIterator<UProperty> ParamInstanceIt(Struct);
				for (int32 NumParams = 0; ParamIt || ParamInstanceIt; ++NumParams, ++ParamIt, ++ParamInstanceIt)
				{
					if (!ParamInstanceIt)
					{
						UE_LOG(LogTemp, Warning, TEXT("AI Notify: Parameter count mistatch for event '%s'. Required parameter of type '%s' at index '%d'."), *EventName.ToString(), *ParamIt->GetName(), NumParams);
						return false;
					}
					else if (!ParamIt)
					{
						// Mismatch (too many params)
						UE_LOG(LogTemp, Warning, TEXT("AI Notify: Parameter count mistatch for event '%s'. Parameter struct contains too many parameters ('%s' is superfluous at index '%d'."), *EventName.ToString(), *ParamInstanceIt->GetName(), NumParams);
						return false;
					}
					else if (!ParamInstanceIt->SameType(*ParamIt) || ParamInstanceIt->GetOffset_ForUFunction() != ParamIt->GetOffset_ForUFunction() || ParamInstanceIt->GetSize() != ParamIt->GetSize())
					{
						UE_LOG(LogTemp, Warning, TEXT("AI Notify: Parameter type mistatch for event '%s' ('%s' != '%s')."),
							*EventName.ToString(),
							*ParamInstanceIt->GetClass()->GetName(),
							*ParamIt->GetClass()->GetName()
							);
						return false;
					}
				}
			}

			// Technically, anything bound to the event could mutate the parameter payload,
			// but we're going to treat that as misuse, rather than copy the parameters each time
			EventContextObject.ProcessEvent(EventFunction, Params);
			return true;
		}
	}
};
