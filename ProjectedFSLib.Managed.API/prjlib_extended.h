/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    prjlib_extended.h

Abstract:

    This module defines newer structures from 2004 in order for symlinks to work in ProjFS.

--*/

#pragma once

__if_not_exists (PRJ_EXT_INFO_TYPE)
{
    typedef enum PRJ_EXT_INFO_TYPE {
        PRJ_EXT_INFO_TYPE_SYMLINK = 1
    } PRJ_EXT_INFO_TYPE;
}

__if_not_exists (PRJ_EXTENDED_INFO)
{
    typedef struct PRJ_EXTENDED_INFO {

        PRJ_EXT_INFO_TYPE InfoType;

        ULONG NextInfoOffset;

        union {
            struct {
                PCWSTR TargetName;
            } Symlink;
        } DUMMYUNIONNAME;

    } PRJ_EXTENDED_INFO;
}

__if_not_exists (PrjWritePlaceholderInfo2)
{
    STDAPI
        PrjWritePlaceholderInfo2(
            _In_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceVirtualizationContext,
            _In_ PCWSTR destinationFileName,
            _In_reads_bytes_(placeholderInfoSize) const PRJ_PLACEHOLDER_INFO * placeholderInfo,
            _In_ UINT32 placeholderInfoSize,
            _In_opt_ const PRJ_EXTENDED_INFO * ExtendedInfo
        );
}

__if_not_exists (PrjFillDirEntryBuffer2)
{
    STDAPI
        PrjFillDirEntryBuffer2(
            _In_ PRJ_DIR_ENTRY_BUFFER_HANDLE dirEntryBufferHandle,
            _In_ PCWSTR fileName,
            _In_opt_ PRJ_FILE_BASIC_INFO * fileBasicInfo,
            _In_opt_ PRJ_EXTENDED_INFO * extendedInfo
        );
}