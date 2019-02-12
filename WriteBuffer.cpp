#include "stdafx.h"
#include "WriteBuffer.h"

using namespace System;
using namespace System::IO;
using namespace Microsoft::Windows::ProjFS;

WriteBuffer::WriteBuffer(
    unsigned long bufferSize,
    unsigned long alignment)
{
    this->buffer = (unsigned char*) _aligned_malloc(bufferSize, alignment);
    if (this->buffer == nullptr)
    {
        throw gcnew OutOfMemoryException("Unable to allocate WriteBuffer");
    }

    this->namespaceCtx = nullptr;
    this->stream = gcnew UnmanagedMemoryStream(buffer, bufferSize, bufferSize, FileAccess::Write);
}

WriteBuffer::WriteBuffer(
    unsigned long bufferSize,
    PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceCtx,
    ApiHelper^ apiHelper)
{
    this->buffer = (unsigned char*) apiHelper->_PrjAllocateAlignedBuffer(namespaceCtx, bufferSize);
    if (this->buffer == nullptr)
    {
        throw gcnew OutOfMemoryException("Unable to allocate WriteBuffer");
    }

    this->namespaceCtx = namespaceCtx;
    this->apiHelper = apiHelper;
    this->stream = gcnew UnmanagedMemoryStream(buffer, bufferSize, bufferSize, FileAccess::Write);
}

WriteBuffer::~WriteBuffer()
{
    delete this->stream;
    this->!WriteBuffer();
}

WriteBuffer::!WriteBuffer()
{
    if (this->namespaceCtx)
    {
        this->apiHelper->_PrjFreeAlignedBuffer(this->buffer);
    }
    else
    {
        _aligned_free(this->buffer);
    }
}

long long WriteBuffer::Length::get(void)
{
    return this->stream->Length;
}

UnmanagedMemoryStream^ WriteBuffer::Stream::get(void)
{
    return this->stream;
}

IntPtr WriteBuffer::Pointer::get(void)
{
    return IntPtr(this->buffer);
}
