#pragma once

namespace Microsoft {
namespace Windows {
namespace ProjFS {
    /// <summary>Defines values describing when to allow a virtualized file to be deleted or updated.</summary>
    /// <remarks>
    /// <para>
    /// These values are used in the <paramref name="updateFlags"/> input parameter of 
    /// <c>ProjFS.VirtualizationInstance.UpdateFileIfNeeded</c> and <c>ProjFS.VirtualizationInstance.DeleteFile</c>.
    /// The flags control whether ProjFS should allow the update given the state of the file or directory on disk.
    /// </para>
    /// <para>
    /// See the documentation for <c>ProjFS.OnDiskFileState</c> for a description of possible file
    /// and directory states in ProjFS.
    /// </para>
    /// </remarks> 
    [System::FlagsAttribute]
    public enum class UpdateType : unsigned long
    {
        /// <summary>
        /// ProjFS will allow the update if the item is a placeholder or a dirty placeholder (whether hydrated or not).
        /// </summary>
        AllowDirtyMetadata = PRJ_UPDATE_ALLOW_DIRTY_METADATA,

        /// <summary>
        /// ProjFS will allow the update if the item is a placeholder or is a full file.
        /// </summary>
        AllowDirtyData = PRJ_UPDATE_ALLOW_DIRTY_DATA,

        /// <summary>
        /// ProjFS will allow the update if the item is a placeholder or is a tombstone.
        /// </summary>
        AllowTombstone = PRJ_UPDATE_ALLOW_TOMBSTONE,

        /// <summary>
        /// ProjFS will allow the update regardless of whether the DOS read-only bit is set on the item.
        /// </summary>
        AllowReadOnly = PRJ_UPDATE_ALLOW_READ_ONLY
    };
}}} // namespace Microsoft.Windows.ProjFS