// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "ActionReflection.h"

#include "EdGraphSchema_K2.h"


void FBaseActionProperty::RefreshProperty()
{
	if(Property)
	{
		Name = Property->GetFName();
		const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
		K2Schema->ConvertPropertyToPinType(Property, Type);
	}
}


bool ActionReflection::GetVisibleProperties(UClass* Class, FActionProperties& OutProperties)
{
	if(!Class)
	{
		return false;
	}

	OutProperties = {};
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
					OutProperties.ComplexDelegates.Add(ActionProperty);
				}
				else
				{
					OutProperties.SimpleDelegates.Add(ActionProperty);
				}
			}
		}
		// Variable properties
		else if (UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property) &&
			!Property->HasAnyPropertyFlags(CPF_Parm) &&
			!Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance) &&
			Property->HasAllPropertyFlags(CPF_BlueprintVisible))
		{
			OutProperties.Variables.Add({ Property });
		}
	}
	return true;
}

