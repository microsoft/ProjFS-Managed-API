#pragma once


namespace Microsoft {
namespace Windows {
namespace ProjFS {

/// <summary>
/// Interface to allow for easier unit testing of a virtualization provider.
/// </summary>
/// <remarks>
/// This class defines the interface implemented by the <c>Microsoft.Windows.ProjFS.DirectoryEnumerationResults</c>
/// class.  This interface class is provided for use by unit tests to mock up the interface to ProjFS.
/// </remarks>
public interface class IDirectoryEnumerationResults
{
public:

    /// <summary>
    /// When overridden in a derived class, adds one entry to a directory enumeration result.
    /// </summary>
    /// <remarks>
    /// <para>
    /// In its implementation of a <c>GetDirectoryEnumerationCallback</c> delegate the provider
    /// calls this method for each matching file or directory in the enumeration.
    /// </para>
    /// <para>
    /// If this method returns <c>HResult.InsufficientBuffer</c>, the provider returns <c>HResult.Ok</c> 
    /// and waits for the next <c>GetDirectoryEnumerationCallback</c>. Then it resumes filling 
    /// the enumeration with the entry it was trying to add when it got <c>HResult.InsufficientBuffer</c>.
    /// </para>
    /// <para>
    /// If the function returns <c>HResult.InsufficientBuffer</c> for the first file or
    /// directory in the enumeration, the provider returns <c>HResult.InsufficientBuffer</c> from
    /// the delegate.
    /// </para>
    /// </remarks>
    /// <param name="fileName">The name of the file or directory.</param>
    /// <param name="fileSize">The size of the file.</param>
    /// <param name="isDirectory"><c>true</c> if this item is a directory, <c>false</c> if it is a file.</param>
    /// <returns>
    /// <para>
    /// <c>true</c> if the entry was successfully added to the enumeration buffer, <c>false</c> otherwise.
    /// </para>
    /// <para>
    /// If this method returns <c>false</c>, the provider returns <c>HResult.Ok</c> and waits for
    /// the next <c>GetDirectoryEnumerationCallback</c>.  Then it resumes filling the enumeration with
    /// the entry it was trying to add when it got <c>false</c>. 
    /// </para>
    /// <para>
    /// If the function returns <c>false</c> for the first file or directory in the enumeration, the
    /// provider returns <c>HResult.InsufficientBuffer</c> from the <c>GetDirectoryEnumerationCallback</c>
    /// method.
    /// </para>
    /// </returns>
    bool Add(
        System::String^ fileName,
        long long fileSize,
        bool isDirectory
    );

    /// <summary>
    /// When overridden in a derived class, adds one entry to a directory enumeration result.
    /// </summary>
    /// <remarks>
    /// <para>
    /// In its implementation of a <c>GetDirectoryEnumerationCallback</c> delegate the provider
    /// calls this method for each matching file or directory in the enumeration.
    /// </para>
    /// <para>
    /// If this method returns <c>HResult.InsufficientBuffer</c>, the provider returns <c>HResult.Ok</c> 
    /// and waits for the next <c>GetDirectoryEnumerationCallback</c>. Then it resumes filling 
    /// the enumeration with the entry it was trying to add when it got <c>HResult.InsufficientBuffer</c>.
    /// </para>
    /// <para>
    /// If the function returns <c>HResult.InsufficientBuffer</c> for the first file or
    /// directory in the enumeration, the provider returns <c>HResult.InsufficientBuffer</c> from
    /// the delegate.
    /// </para>
    /// </remarks>
    /// <param name="fileName">The name of the file or directory.</param>
    /// <param name="fileSize">The size of the file.</param>
    /// <param name="isDirectory"><c>true</c> if this item is a directory, <c>false</c> if it is a file.</param>
    /// <param name="fileAttributes">Specifies one or more FILE_ATTRIBUTE_XXX flags. For descriptions of these flags,
    /// see the documentation for the <c>GetFileAttributes</c> function in the Microsoft Windows SDK.</param>
    /// <param name="creationTime">Specifies the time that the file was created.</param>
    /// <param name="lastAccessTime">Specifies the time that the file was last accessed.</param>
    /// <param name="lastWriteTime">Specifies the time that the file was last written to.</param>
    /// <param name="changeTime">Specifies the last time the file was changed.</param>
    /// <returns>
    /// <para>
    /// <c>true</c> if the entry was successfully added to the enumeration buffer, <c>false</c> otherwise.
    /// </para>
    /// <para>
    /// If this method returns <c>false</c>, the provider returns <c>HResult.Ok</c> and waits for
    /// the next <c>GetDirectoryEnumerationCallback</c>.  Then it resumes filling the enumeration with
    /// the entry it was trying to add when it got <c>false</c>. 
    /// </para>
    /// <para>
    /// If the function returns <c>false</c> for the first file or directory in the enumeration, the
    /// provider returns <c>HResult.InsufficientBuffer</c> from the <c>GetDirectoryEnumerationCallback</c>
    /// method.
    /// </para>
    /// </returns>
    bool Add(
        System::String^ fileName,
        long long fileSize,
        bool isDirectory,
        System::IO::FileAttributes fileAttributes,
        System::DateTime creationTime,
        System::DateTime lastAccessTime,
        System::DateTime lastWriteTime,
        System::DateTime changeTime
    );
};

}}} // namespace Microsoft.Windows.ProjFS