// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "ActionReflection.h"

#include "EdGraphSchema_K2.h"


TOptional<FActionProperties> ActionReflection::GetVisibleProperties(UClass* Class)
{
	if(!Class)
	{
		return {};
	}

	FActionProperties Properties {};
	for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* const Property = *PropertyIt;

		// Delegate properties
		if(auto* DelegateProperty = Cast<UMulticastDelegateProperty>(Property))
		{
			if (DelegateProperty && Property->HasAllPropertyFlags(CPF_BlueprintAssignable))
			{
				FDelegateActionProperty ActionProperty{ DelegateProperty };
				if(ActionProperty.HasParams())
				{
					Properties.ComplexDelegates.Add(ActionProperty);
				}
				else
				{
					Properties.SimpleDelegates.Add(ActionProperty);
				}
			}
		}
		// Variable properties
		else if (UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property) &&
			!Property->HasAnyPropertyFlags(CPF_Parm) &&
			!Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance) &&
			Property->HasAllPropertyFlags(CPF_BlueprintVisible))
		{
			Properties.Variables.Add({ Property });
		}
	}
	return Properties;
}

