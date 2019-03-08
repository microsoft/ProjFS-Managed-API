// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "OnDiskFileState.h"
#include "HResult.h"

using namespace System::Runtime::InteropServices;

namespace Microsoft {
namespace Windows {
namespace ProjFS {
    /// <summary>
    /// Provides utility methods for ProjFS providers.
    /// </summary>
    public ref class Utils abstract sealed
    {
    public:
        /// <summary>
        /// Returns the on-disk state of the specified file or directory.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     This routine tells the caller what the ProjFS caching state is of the specified file or
        ///     directory.  For example, the caller can use this routine to determine whether the given item
        ///     is a placeholder or full file.
        ///     </para>
        ///     <para>
        ///     A running provider should be cautious if using this routine on files or directories within
        ///     one of its virtualization instances, as it may cause callbacks to be invoked in the provider.
        ///     Depending on the design of the provider this may lead to deadlocks.
        ///     </para>
        /// </remarks>
        /// <param name="fullPath">Full path of the file or the directory.</param>
        /// <param name="fileState">On successful return contains a bitwise-OR of <see cref="OnDiskFileState"/>
        ///     values describing the file state.</param>
        /// <returns>
        /// <c>false</c> if <paramref name="fullPath"/> does not exist.
        /// </returns>
        static bool TryGetOnDiskFileState(
            System::String^ fullPath,
            [Out] OnDiskFileState% fileState);

        /// <summary>
        /// Determines whether a file name string matches a pattern, potentially containing
        /// wildcard characters, according to the rules used by the file system.
        /// </summary>
        /// <remarks>
        /// A provider should use this routine in its implementation of the <c>GetDirectoryEnumerationCallback</c>
        /// delegate to determine whether a name it its backing store matches the search expression
        /// from the <c>filterFileName</c> parameter of the <c>GetDirectoryEnumerationCallback</c>
        /// delegate.
        /// </remarks>
        /// <param name="fileNameToCheck">The file name to check against <paramref name="pattern"/>.</param>
        /// <param name="pattern">The pattern for which to search.</param>
        /// <returns><c>true</c> if <paramref name="fileNameToCheck"/> matches <paramref name="pattern"/>,
        /// <c>false</c> otherwise.</returns>
        static bool IsFileNameMatch(
            System::String^ fileNameToCheck,
            System::String^ pattern);

        /// <summary>
        /// Compares two file names and returns a value that indicates their relative collation order.
        /// </summary>
        /// <remarks>
        /// The provider may use this routine to determine how to sort file names in the same order
        /// that the file system does.
        /// </remarks>
        /// <param name="fileName1">The first name to compare.</param>
        /// <param name="fileName2">The second name to compare.</param>
        /// <returns>
        /// <para>A negative number if <paramref name="fileName1"/> is before <paramref name="fileName2"/> in collation order.</para>
        /// <para>0 if <paramref name="fileName1"/> is equal to <paramref name="fileName2"/>.</para>
        /// <para>A positive number if <paramref name="fileName1"/> is after <paramref name="fileName2"/> in collation order.</para>
        /// </returns>
        static int FileNameCompare(
            System::String^ fileName1,
            System::String^ fileName2);

        /// <summary>Determines whether a string contains any wildcard characters.</summary>
        /// <remarks>
        /// <para>
        /// This routine checks for the wildcard characters recognized by the file system.  These
        /// wildcards are sent by programs such as the cmd.exe command interpreter.
        /// </para>
        /// <para>
        /// <list type="table">
        ///     <listheader>
        ///         <term>Character</term>
        ///         <term>Meaning</term>
        ///     </listheader>
        ///     <item>
        ///         <description>*</description>
        ///         <description>Matches 0 or more characters.</description>
        ///     </item>
        ///     <item>
        ///         <description>?</description>
        ///         <description>Matches exactly one character.</description>
        ///     </item>
        ///     <item>
        ///         <description>DOS_DOT (")</description>
        ///         <description>Matches either a ".", or zero characters beyond the name string.</description>
        ///     </item>
        ///     <item>
        ///         <description>DOS_STAR (&lt;)</description>
        ///         <description>Matches 0 or more characters until encountering and matching the final "." in the name.</description>
        ///     </item>
        ///     <item>
        ///         <description>DOS_QM (&gt;)</description>
        ///         <description>Matches any single character, or upon encountering a period or end of name string, advances the expression
        ///         to the end of the set of contiguous DOS_QMs.</description>
        ///     </item>
        /// </list>
        /// </para>
        /// </remarks>
        /// <param name="fileName">A string to check for wildcard characters.</param>
        /// <returns><c>true</c> if <paramref name="fileName"/> contains any wildcard characters,
        /// <c>false</c> otherwise.</returns>
        static bool DoesNameContainWildCards(
            System::String^ fileName);
    };
}}} // namespace Microsoft.Windows.ProjFS