// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"

#include "SquadOrder.h"
#include "AIGeneric.h"

#include "AISquad.h"


void AAISquad::BeginPlay()
{
    //Called when game logic begins

    for(auto EditorMember : EditorMembers)
    {
        if (EditorMember)
            AddMember(Cast<AAIGeneric>(EditorMember->GetController()));
    }

    if (EditorLeader)
        SetLeader(Cast<AAIGeneric>(EditorLeader->GetController()));
}

void AAISquad::AddMember(AAIGeneric* Member)
{
    if (IsValid(Member)) {
        Members.AddUnique(Member);
    }
}

void AAISquad::RemoveMember(AAIGeneric* Member)
{
    Members.Remove(Member);
}

void AAISquad::SetLeader(AAIGeneric* NewLeader)
{
    if(Members.Contains(NewLeader))
    {
        Leader = NewLeader;
    }
}

void AAISquad::SendOrder(USquadOrder * order)
{
    CurrentOrder = order->GetClass();
}


#if WITH_EDITORONLY_DATA

bool AAISquad::CanEditChange(const UProperty* InProperty) const
{
    bool bIsEditable = Super::CanEditChange(InProperty);
    if (bIsEditable && InProperty)
    {
        FName PropertyName = (InProperty != nullptr) ? InProperty->GetFName() : NAME_None;

        if ((PropertyName == GET_MEMBER_NAME_CHECKED(AAISquad, EditorLeader)))
        {
            if (EditorMembers.Num() == 0) {
                return false;
            }

            bIsEditable = false;
            for (auto Member : EditorMembers)
            {
                if (Member)
                    bIsEditable = true;
            }
        }
    }

    return bIsEditable;
}

void AAISquad::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* Property = PropertyChangedEvent.Property;
    FName PropertyName = (Property != nullptr) ? Property->GetFName() : NAME_None;

    if ((PropertyName == GET_MEMBER_NAME_CHECKED(AAISquad, EditorMembers)))
    {
        //Sanitize Editor Members
        for (int32 Index = 0; Index != EditorMembers.Num(); ++Index)
        {
            auto EditorMember = EditorMembers[Index];

            if (!IsValid(EditorMember))
                continue;

            //Is this member duplicated, or its controller is not child of AIGeneric?
            if(!EditorMember->AIControllerClass || !EditorMember->AIControllerClass->IsChildOf<AAIGeneric>())
            {
                EditorMembers.RemoveAt(Index);
                EditorMembers.Insert(NULL, Index);
            }
        }
    }
    else if ((PropertyName == GET_MEMBER_NAME_CHECKED(AAISquad, EditorLeader)))
    {
        //Sanitize Editor Leaders
        if (EditorLeader && (!EditorMembers.Contains(EditorLeader) 
            || !EditorLeader->AIControllerClass
            || !EditorLeader->AIControllerClass->IsChildOf<AAIGeneric>()))
        {
            EditorLeader = NULL;
        }
    }
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif //WITH_EDITORONLY_DATA
