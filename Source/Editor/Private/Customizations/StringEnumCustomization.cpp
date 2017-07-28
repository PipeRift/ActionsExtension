// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionEditorPrivatePCH.h"

#include "DetailWidgetRow.h"


#include "StringEnumCustomization.h"

#define LOCTEXT_NAMESPACE "FStringEnumCustomization"

void FStringEnumCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) 
{
    if (CanCustomizeHeader(StructPropertyHandle, HeaderRow, StructCustomizationUtils)) {

        UpdateItems();
        
        HeaderRow.NameContent()
        [
            StructPropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .MaxDesiredWidth(0.0f)
        .MinDesiredWidth(125.0f)
        [
            SAssignNew(ComboBox, SComboBox<TSharedPtr<FString>>)
            .OptionsSource(&CachedItems)
            .OnGenerateWidget(this, &FStringEnumCustomization::HandleStringEnumComboBoxGenerateWidget)
            .OnSelectionChanged(this, &FStringEnumCustomization::OnSelectionChanged)
            //.InitiallySelectedItem(GetVariableFactionValue())
            [
                SNew(STextBlock)
                .Text(this, &FStringEnumCustomization::GetSelectedItem)
            ]
        ];
    }

}


void FStringEnumCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    //Do Nothing
}



/** Return the representation of the the column names to display */
TSharedRef<SWidget> FStringEnumCustomization::HandleStringEnumComboBoxGenerateWidget(TSharedPtr<FString> Item)
{
    return SNew(STextBlock)
        .Text(FText::FromString(*Item));
}

/** Update the root data on a change of selection */
void FStringEnumCustomization::OnSelectionChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo)
{
    if (SelectedItem.IsValid()) {
        //UpdateItems();
        OnItemSelected(*SelectedItem);
    }
}

/** Display the current column selection */
void FStringEnumCustomization::UpdateItems()
{
    const TArray<FString>& Items = GetEnumItems();
    CachedItems.Empty();
    
    //Convert FString to Shared Ptrs and Populate the array
    for (auto It = Items.CreateConstIterator(); It; ++It)
    {
        if (!(*It).IsEmpty())
        {
            TSharedPtr<FString> Name = MakeShareable(new FString(*It));
            CachedItems.Add(Name);
        }
    }

    if (ComboBox.IsValid()) {
        ComboBox->RefreshOptions();
    }
}

const TArray<FString> FStringEnumCustomization::GetEnumItems() {
    TArray<FString> Values;
    Values.Add(FString("None"));
    return Values;
}

/** Display the current column selection */
FText FStringEnumCustomization::GetSelectedItem() const
{
    return FText::FromString("None");
}


#undef LOCTEXT_NAMESPACE