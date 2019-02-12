#pragma once

namespace Microsoft {
namespace Windows {
namespace ProjFS {
    /// <summary>
    /// Defines values that describe why an attempt to update or delete a file in a virtualization
    /// root has failed.
    /// </summary>
    /// <remarks>
    /// These values are used in the <paramref name="failureReason"/> output parameter of 
    /// <c>ProjFS.VirtualizationInstance.UpdateFileIfNeeded</c> and <c>ProjFS.VirtualizationInstance.DeleteFile</c>.
    /// These are set if the API returns <c>HResult.VirtualizationInvalidOp</c> because the file state
    /// does not allow the operation with the <paramref name="updateFlags"/> value(s) passed to the API.
    /// </remarks> 
    [System::FlagsAttribute]
    public enum class UpdateFailureCause : unsigned long
    {
        /// <summary>
        /// The update did not fail.
        /// </summary>
        NoFailure = PRJ_UPDATE_FAILURE_CAUSE_NONE,

        /// <summary>
        /// The item was a dirty placeholder (hydrated or not), and the provider did not specify
        /// <c>UpdateType.AllowDirtyMetadata</c>.
        /// </summary>
        DirtyMetadata = PRJ_UPDATE_FAILURE_CAUSE_DIRTY_METADATA,

        /// <summary>
        /// The item was a full file and the provider did not specify <c>UpdateType.AllowDirtyData</c>.
        /// </summary>
        DirtyData = PRJ_UPDATE_FAILURE_CAUSE_DIRTY_DATA,

        /// <summary>
        /// The item was a tombstone and the provider did not specify <c>UpdateType.AllowTombstone</c>.
        /// </summary>
        Tombstone = PRJ_UPDATE_FAILURE_CAUSE_TOMBSTONE,

        /// <summary>
        /// The item had the DOS read-only bit set and the provider did not specify <c>UpdateType.AllowReadOnly.</c>
        /// </summary>
        ReadOnly = PRJ_UPDATE_FAILURE_CAUSE_READ_ONLY
    };
}}} // namespace Microsoft.Windows.ProjFS