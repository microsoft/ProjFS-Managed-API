// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "stdafx.h"
#include "ApiHelper.h"
#include "Utils.h"

using namespace System;
using namespace System::Globalization;
using namespace System::IO;
using namespace Microsoft::Windows::ProjFS;

ApiHelper::ApiHelper() :
    useRS5Api(false)
{
    auto projFsLib = ::LoadLibraryW(L"ProjectedFSLib.dll");
    if (!projFsLib)
    {
        throw gcnew FileLoadException(String::Format(CultureInfo::InvariantCulture, "Could not load ProjectedFSLib.dll to set up entry points."));
    }

    if (::GetProcAddress(projFsLib, "PrjStartVirtualizing") != nullptr)
    {
        // We have the API introduced in Windows 10 version 1809.

        this->_PrjStartVirtualizing = reinterpret_cast<t_PrjStartVirtualizing>(::GetProcAddress(projFsLib,
                                                                                                "PrjStartVirtualizing"));

        this->_PrjStopVirtualizing = reinterpret_cast<t_PrjStopVirtualizing>(::GetProcAddress(projFsLib,
                                                                                              "PrjStopVirtualizing"));

        this->_PrjWriteFileData = reinterpret_cast<t_PrjWriteFileData>(::GetProcAddress(projFsLib,
                                                                                        "PrjWriteFileData"));

        this->_PrjWritePlaceholderInfo = reinterpret_cast<t_PrjWritePlaceholderInfo>(::GetProcAddress(projFsLib,
                                                                                                      "PrjWritePlaceholderInfo"));

        this->_PrjAllocateAlignedBuffer = reinterpret_cast<t_PrjAllocateAlignedBuffer>(::GetProcAddress(projFsLib,
                                                                                                        "PrjAllocateAlignedBuffer"));

        this->_PrjFreeAlignedBuffer = reinterpret_cast<t_PrjFreeAlignedBuffer>(::GetProcAddress(projFsLib,
                                                                                                "PrjFreeAlignedBuffer"));

        this->_PrjGetVirtualizationInstanceInfo = reinterpret_cast<t_PrjGetVirtualizationInstanceInfo>(::GetProcAddress(projFsLib,
                                                                                                                        "PrjGetVirtualizationInstanceInfo"));

        this->_PrjUpdateFileIfNeeded = reinterpret_cast<t_PrjUpdateFileIfNeeded>(::GetProcAddress(projFsLib,
                                                                                                  "PrjUpdateFileIfNeeded"));

        this->_PrjMarkDirectoryAsPlaceholder = reinterpret_cast<t_PrjMarkDirectoryAsPlaceholder>(::GetProcAddress(projFsLib,
                                                                                                                  "PrjMarkDirectoryAsPlaceholder"));
        bool environmentSupportsSymlinks = false;
        if (::GetProcAddress(projFsLib, "PrjWritePlaceholderInfo2") != nullptr)
        {
            // We have the API introduced in Windows 10 version 2004.
            this->_PrjWritePlaceholderInfo2 = reinterpret_cast<t_PrjWritePlaceholderInfo2>(::GetProcAddress(projFsLib,
                "PrjWritePlaceholderInfo2"));
            environmentSupportsSymlinks = true;
        }

        ::FreeLibrary(projFsLib);

        if (!this->_PrjStartVirtualizing ||
            !this->_PrjStopVirtualizing ||
            !this->_PrjWriteFileData ||
            !this->_PrjWritePlaceholderInfo ||
            (environmentSupportsSymlinks && !this->_PrjWritePlaceholderInfo2) ||
            !this->_PrjAllocateAlignedBuffer ||
            !this->_PrjFreeAlignedBuffer ||
            !this->_PrjGetVirtualizationInstanceInfo ||
            !this->_PrjUpdateFileIfNeeded ||
            !this->_PrjMarkDirectoryAsPlaceholder)
        {
            throw gcnew EntryPointNotFoundException(String::Format(CultureInfo::InvariantCulture,
                                                                   "Could not get a required entry point."));
        }

        this->useRS5Api = true;
    }
    else if (::GetProcAddress(projFsLib, "PrjStartVirtualizationInstance") == nullptr)
    {
        // Something is wrong; we didn't find the 1809 API nor can we find the 1803 API even though
        // we loaded ProjectedFSLib.dll.

        ::FreeLibrary(projFsLib);

        throw gcnew EntryPointNotFoundException(String::Format((CultureInfo::InvariantCulture,
                                                                "Cannot find ProjFS API.")));
    }
    else
    {
        // We have the beta API introduced in Windows 10 version 1803.

        this->_PrjStartVirtualizationInstance = reinterpret_cast<t_PrjStartVirtualizationInstance>(::GetProcAddress(projFsLib,
                                                                                                                    "PrjStartVirtualizationInstance"));

        this->_PrjStartVirtualizationInstanceEx = reinterpret_cast<t_PrjStartVirtualizationInstanceEx>(::GetProcAddress(projFsLib,
                                                                                                                        "PrjStartVirtualizationInstanceEx"));

        this->_PrjStopVirtualizationInstance = reinterpret_cast<t_PrjStopVirtualizationInstance>(::GetProcAddress(projFsLib,
                                                                                                                  "PrjStopVirtualizationInstance"));

        this->_PrjGetVirtualizationInstanceIdFromHandle = reinterpret_cast<t_PrjGetVirtualizationInstanceIdFromHandle>(::GetProcAddress(projFsLib,
                                                                                                                                        "PrjGetVirtualizationInstanceIdFromHandle"));

        this->_PrjConvertDirectoryToPlaceholder = reinterpret_cast<t_PrjConvertDirectoryToPlaceholder>(::GetProcAddress(projFsLib,
                                                                                                                        "PrjConvertDirectoryToPlaceholder"));

        this->_PrjWritePlaceholderInformation = reinterpret_cast<t_PrjWritePlaceholderInformation>(::GetProcAddress(projFsLib,
                                                                                                                    "PrjWritePlaceholderInformation"));

        this->_PrjUpdatePlaceholderIfNeeded = reinterpret_cast<t_PrjUpdatePlaceholderIfNeeded>(::GetProcAddress(projFsLib,
                                                                                                                "PrjUpdatePlaceholderIfNeeded"));

        this->_PrjWriteFile = reinterpret_cast<t_PrjWriteFile>(::GetProcAddress(projFsLib,
                                                                                "PrjWriteFile"));

        this->_PrjCommandCallbacksInit = reinterpret_cast<t_PrjCommandCallbacksInit>(::GetProcAddress(projFsLib,
                                                                                                      "PrjCommandCallbacksInit"));

        ::FreeLibrary(projFsLib);

        if (!this->_PrjStartVirtualizationInstance ||
            !this->_PrjStartVirtualizationInstanceEx ||
            !this->_PrjStopVirtualizationInstance ||
            !this->_PrjGetVirtualizationInstanceIdFromHandle ||
            !this->_PrjConvertDirectoryToPlaceholder ||
            !this->_PrjWritePlaceholderInformation ||
            !this->_PrjUpdatePlaceholderIfNeeded ||
            !this->_PrjWriteFile ||
            !this->_PrjCommandCallbacksInit)
        {
            throw gcnew EntryPointNotFoundException(String::Format(CultureInfo::InvariantCulture,
                                                                   "Could not get a required entry point."));
        }
    }
}

bool ApiHelper::UseRS5Api::get(void)
{
    return this->useRS5Api;
}
