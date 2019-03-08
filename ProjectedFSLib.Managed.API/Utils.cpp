#include "stdafx.h"
#include "Utils.h"

using namespace System;
using namespace Microsoft::Windows::ProjFS;

bool Utils::TryGetOnDiskFileState(String^ fullPath, [Out] OnDiskFileState% fileState)
{
    pin_ptr<const WCHAR> path = PtrToStringChars(fullPath);
    PRJ_FILE_STATE localFileState;
    if (FAILED(::PrjGetOnDiskFileState(path, &localFileState)))
    {
        return false;
    }

    fileState = static_cast<OnDiskFileState>(localFileState);

    return true;
}

bool Utils::IsFileNameMatch(String^ fileNameToCheck, String^ pattern)
{
    pin_ptr<const WCHAR> pFileNameToCheck = PtrToStringChars(fileNameToCheck);
    pin_ptr<const WCHAR> pPattern = PtrToStringChars(pattern);

    return ::PrjFileNameMatch(pFileNameToCheck, pPattern);
}

int Utils::FileNameCompare(String^ fileName1, String^ fileName2)
{
    pin_ptr<const WCHAR> pFileName1 = PtrToStringChars(fileName1);
    pin_ptr<const WCHAR> pFileName2 = PtrToStringChars(fileName2);

    return ::PrjFileNameCompare(pFileName1, pFileName2);
}

bool Utils::DoesNameContainWildCards(String^ fileName)
{
    pin_ptr<const WCHAR> pFileName = PtrToStringChars(fileName);

    return ::PrjDoesNameContainWildCards(pFileName);
}
