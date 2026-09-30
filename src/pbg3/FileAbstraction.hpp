#pragma once

#include "diffbuild.hpp"
#include "inttypes.hpp"
#include <Windows.h>

namespace th06
{

#define PBG_EOF (-1)

class FileAbstraction
{
  public:
    virtual BOOL Open(const char *filename, const char *mode);
    virtual void Close();
    virtual BOOL Read(void *data, u32 dataLen, DWORD *numBytesRead);
    virtual BOOL Write(void *data, u32 dataLen, DWORD *outWritten);
    virtual i32 ReadByte();
    virtual i32 WriteByte(i32 b);
    virtual BOOL Seek(u32 amount, u32 seekFrom);
    virtual u32 Tell();
    virtual u32 GetSize();

    BOOL WriteString(void *Buffer);

    virtual LPVOID ReadWholeFile(u32 maxSize);

    FileAbstraction();
    ~FileAbstraction();

    BOOL HasNonNullHandle()
    {
        return this->handle != NULL;
    }
    // Yes, this is comparing against INVALID_HANDLE_VALUE instead of NULL. Why?
    // Unclear.
    BOOL HasValidHandle()
    {
        return this->handle != INVALID_HANDLE_VALUE;
    }

  protected:
    HANDLE handle;

  private:
    DWORD access;
};
ZUN_ASSERT_SIZE(FileAbstraction, 0xc);
} // namespace th06
