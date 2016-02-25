
#pragma once

#include <assert.h>
#include <windows.h>
#include <Strsafe.h>


class SHA_1
{
    enum {
        ZEN_LITTLE_ENDIAN,
        SHA1_BLOCK_SIZE = 64,
        SHA1_HASH_SIZE = 20,
        SHA1_READ_BYTES = SHA1_BLOCK_SIZE * 512,
    };

    typedef struct
    {
	    LONGLONG    length;		// �ļ�����
	    LONGLONG    uplength;	// δ������
	    DWORD       _hash[5];	// Hash���
    }
    SHA1INFO, *PSHA1INFO;

    SHA1INFO ctx;
    PBYTE    pData;
    wchar_t  m_Result;

    // С��ģʽ
    static const int c_BytesOrder = ZEN_LITTLE_ENDIAN;

    //��С��ת��
    ULONG ZEN_SWAP_UINT32(ULONG x)
    {
        return ((x & 0xff000000) >> 24) | ((x & 0x000000ff) << 24) |
		       ((x & 0x0000ff00) <<  8) | ((x & 0x00ff0000) >>  8);
    }

    ULONG64 ZEN_SWAP_UINT64(ULONG64 x)
    {
        return ((x & 0xff00000000000000) >> 56) | ((x & 0x00000000000000ff) << 56) | 
               ((x & 0x000000000000ff00) << 40) | ((x & 0x00ff000000000000) >> 40) |
		       ((x & 0x0000ff0000000000) >> 24) | ((x & 0x0000000000ff0000) << 24) | 
		       ((x & 0x00000000ff000000) <<  8) | ((x & 0x000000ff00000000) >>  8);
    }

    // ѭ��λ��
    ULONG   ROTL32(ULONG   dword, ULONG n) { return dword << n ^ dword >> (32 - n); }
    ULONG   ROTR32(ULONG   dword, ULONG n) { return dword >> n ^ dword << (32 - n); }
    ULONG64 ROTL64(ULONG64 qword, ULONG n) { return qword << n ^ qword >> (64 - n); }
    ULONG64 ROTR64(ULONG64 qword, ULONG n) { return qword >> n ^ qword << (64 - n); }

    
    bool  Sha1Init         (HANDLE hFile);
    void  SwapMemCopy      (PBYTE to, PBYTE from, int len);
    int   ToHexString      (PBYTE in, wchar_t *out, int cb);
    void  Sha1ProcessBlock (DWORD hash[5], DWORD Block[SHA1_BLOCK_SIZE / 4]);
    void  Sha1ProcessFinal (PSHA1INFO psi, PBYTE pBuffer, PBYTE pResult);

public:
    int   CalculateSha1    (wchar_t *szFileName, wchar_t *pbHashResult, int cb, bool bContinue);
    int   CalculateSha1    (wchar_t *szFileName, ULONG64 offset, ULONG64 len, wchar_t *pbHashResult, int cb, bool bContinue);
};