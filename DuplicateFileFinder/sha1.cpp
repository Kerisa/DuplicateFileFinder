#include "Sha1.h"

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//  ���ܣ���ʼ��psi�ṹ��
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool SHA_1::Sha1Init(ULONG64 size)
{
    ctx.uplength = ctx.length = size;

    // ��ʼ���㷨�ļ���������ħ����
    ctx._hash[0] = 0x67452301;
    ctx._hash[1] = 0xefcdab89;
    ctx._hash[2] = 0x98badcfe;
    ctx._hash[3] = 0x10325476;
    ctx._hash[4] = 0xc3d2e1f0;

    return true;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//	���ܣ���������ͬʱ�ı��С��ģʽ
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SHA_1::SwapMemCopy(PBYTE to, PBYTE from, int len)
{
    int i, remain = (4 - (len & 3)) & 3;

    RtlCopyMemory(to, from, len);

    if (remain)		// ����Ϊ4�ı���
    {
        for (i=0; i<remain; ++i)
        {
            *(to + len + i) = 0;
        }
        len += remain;
    }

    for (i=0; i<len/4; ++i)
    {
        ((DWORD *)to)[i] = ZEN_SWAP_UINT32(((DWORD *)from)[i]);
    }
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//	���ܣ������ɵ��ִ�ת��Ϊ16�����ı��ִ�
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int SHA_1::ToHexString(PBYTE in, wchar_t *out, int cb)		// _itow_s��ʾ0�е�����
{
    wchar_t szBuffer[4];
    
    if (!out || !in || cb < (SHA1_HASH_SIZE+1) * sizeof(wchar_t))	// (SHA1_HASH_SIZE+1) * 2
        return -1;

    out[0] = '\0';
        
    for (int i=0; i<SHA1_HASH_SIZE; ++i)
    {

        StringCchPrintf(szBuffer, 4, L"%02X", in[i]);
        wcscat_s(out, cb/2, szBuffer);
    }

    return 0;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//	���ܣ���׼64Byte��ļ���
//  hash	- ����hash�м�ֵ
//  Block	- �ļ��飬 64B��С�� 16��DWORD
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SHA_1::Sha1ProcessBlock(DWORD hash[5], DWORD Block[SHA1_BLOCK_SIZE / 4])
{
    int		t;
    DWORD	a, b, c, d, e, tmp;
    DWORD	wBlock[80];

#if	c_BytesOrder == ZEN_LITTLE_ENDIAN
    SwapMemCopy((PBYTE)wBlock, (PBYTE)Block, SHA1_BLOCK_SIZE);
#else
    RtlCopyMemory(wBlock, Block, SHA1_BLOCK_SIZE);
#endif
    
    // ����
    for (t=16; t<80; ++t)
    {
        wBlock[t] = ROTL32(wBlock[t-3] ^ wBlock[t-8] ^ wBlock[t-14] ^ wBlock[t-16], 1);
    }
    a = hash[0];
    b = hash[1];
    c = hash[2];
    d = hash[3];
    e = hash[4];
/*
TEMP = S5(A) + ft(B,C,D) + E + Wt + Kt; 
E = D; 
D = C; 
C = S30(B); 
B = A; 
A = TEMP;

Kt = 0x5A827999  (0 <= t <= 19)
Kt = 0x6ED9EBA1 (20 <= t <= 39)
Kt = 0x8F1BBCDC (40 <= t <= 59)
Kt = 0xCA62C1D6 (60 <= t <= 79).

ft(B,C,D) = (B AND C) OR ((NOT B) AND D)		( 0 <= t <= 19)
ft(B,C,D) = B XOR C XOR D						(20 <= t <= 39)
ft(B,C,D) = (B AND C) OR (B AND D) OR (C AND D) (40 <= t <= 59)
ft(B,C,D) = B XOR C XOR D						(60 <= t <= 79).
*/
    for (t = 0; t < 20; t++)  
     {  
         /* the following is faster than ((B & C) | ((~B) & D)) */  
         tmp =  ROTL32(a, 5) + (((c ^ d) & b) ^ d)  
                 + e + wBlock[t] + 0x5A827999;  
         e = d;  
         d = c;  
         c = ROTL32(b, 30);  
         b = a;  
         a = tmp;  
     }  

    for (t=20; t<40; ++t)
    {
        tmp = ROTL32(a, 5) + (b^c^d) + e + wBlock[t] + 0x6ED9EBA1;
        e = d;
        d = c;
        c = ROTL32(b, 30);
        b = a;
        a = tmp;
    }

    for (t=40; t<60; ++t)
    {
        tmp = ROTL32(a, 5) + ((b&c)|(b&d)|(c&d)) + e + wBlock[t] + 0x8F1BBCDC;
        e = d;
        d = c;
        c = ROTL32(b, 30);
        b = a;
        a = tmp;
    }

    for (t=60; t<80; ++t)
    {
        tmp = ROTL32(a, 5) + (b^c^d) + e + wBlock[t] + 0xCA62C1D6;
        e = d;
        d = c;
        c = ROTL32(b, 30);
        b = a;
        a = tmp;
    }

    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//	���ܣ������С��64Byte�����ݽ������ͼ���
//  psi		- ����hash�����Ϣ�Ľṹ��ָ��
//  pBuffer	- �ļ�ָ��
//  pResult	- ���hash���������
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SHA_1::Sha1ProcessFinal(PSHA1INFO psi, PBYTE pBuffer, PBYTE pResult)
{
    LONGLONG lFileLen;	
    DWORD	 message[SHA1_BLOCK_SIZE / 4];
    int		 iIndex, iShift;

    if (psi->uplength)
    {
        RtlCopyMemory(message, pBuffer, (DWORD)psi->uplength);
    }

    //��ȡ����0x80��λ��
    iIndex = (DWORD)(psi->length & 63) >> 2;
    iShift = (DWORD)(psi->length & 3) << 3;

    //����0x80����0
    message[iIndex] &= ~(-1 << iShift);
    message[iIndex++] ^= 0x80<<iShift;

    //������Blockʣ�µĳ��Ȳ���64Bits, ���Ȳ�0����
    if (iIndex > 14)
    {
        while(iIndex < 16)
        {
            message[iIndex++] = 0;
        }
        Sha1ProcessBlock(psi->_hash, message);
        iIndex = 0;
    }

    // ���һ��ֻ���ļ���С��Block
    while (iIndex < 14)
    {
        message[iIndex++] = 0;
    }

    // Bit������8
    lFileLen = psi->length << 3;

    // С��ģʽת��Ϊ���
#if	c_BytesOrder == ZEN_LITTLE_ENDIAN
    lFileLen = ZEN_SWAP_UINT64(lFileLen);
#endif

    message[14] = (DWORD)(lFileLen & 0x00000000FFFFFFFF);
    message[15] = (DWORD)((lFileLen & 0xFFFFFFFF00000000) >> 32);

    Sha1ProcessBlock(psi->_hash, message);

    // �����ת��С��
#if c_BytesOrder == ZEN_LITTLE_ENDIAN
    SwapMemCopy((PBYTE)pResult, (PBYTE)psi->_hash, SHA1_HASH_SIZE);
#else
    RtlCopyMemory(pResult, psi->_hash, SHA1_HASH_SIZE);
#endif
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//  ���ܣ��ⲿ���õĽӿ�
//  szFileName		- Ŀ���ļ���
//  pbHashResult	- ���ս���Ļ�����
//  iSize			- ��������С
//  bContinue		- ��ֹ��־
//  ����ֵ��0-���������� -1-�û���ֹ�� -2-�޷����ļ�, -3-��������, -4-???
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int SHA_1::CalculateSha1(wchar_t *szFileName, CallBack callback)
{
    HANDLE		hFile;
    BYTE		pbResultTmp[20];//, pbHashBuffer[SHA1_READ_BYTES+10];
    DWORD		dwBytesRead;
    int			iLoop, cnt = 0, ret = 0;
    bool        Continue = true;


    PBYTE pbHashBuffer = (PBYTE)VirtualAlloc(
        NULL, SHA1_READ_BYTES+10, MEM_COMMIT, PAGE_READWRITE);
    if (!pbHashBuffer) return S_ERR_OTHERS;

    if (INVALID_HANDLE_VALUE != 
                (hFile = CreateFile(szFileName, GENERIC_READ, 
                    FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)))
    {
        ULARGE_INTEGER size;
        size.LowPart = GetFileSize(hFile, &size.HighPart);
        Sha1Init(size.QuadPart);

        ReadFile(hFile, pbHashBuffer, SHA1_READ_BYTES, &dwBytesRead, NULL);
        pData = pbHashBuffer;

        // ÿ�ζ�ȡ512��BLOCK����
        cnt = 0;
        while (Continue && dwBytesRead == SHA1_READ_BYTES)
        {
            ctx.uplength -= SHA1_READ_BYTES;
            iLoop = __Scale;
            while(iLoop--)
            {
                Sha1ProcessBlock(ctx._hash, (PDWORD)pData);
                pData += SHA1_BLOCK_SIZE;
            }
            ReadFile(hFile, pbHashBuffer, SHA1_READ_BYTES, &dwBytesRead, NULL);
            pData = pbHashBuffer;

            if (++cnt == 10)
            {
                __try
                {
                    Continue = callback(&ctx.length, &ctx.uplength);
                }
                __except(1)
                {
                }
                cnt = 0;
            }
        }


        // ����ʣ�ಿ��
        if (Continue)
        {		
            iLoop = dwBytesRead / SHA1_BLOCK_SIZE;
            ctx.uplength -= iLoop * SHA1_BLOCK_SIZE;
            int cnt = 0;
            while(iLoop--)
            {
                Sha1ProcessBlock(ctx._hash, (PDWORD)pData);
                pData += SHA1_BLOCK_SIZE;
            }

            Sha1ProcessFinal(&ctx, pData, pbResultTmp);

            __try
            {
                Continue = callback(&ctx.length, &ctx.uplength);
            }
            __except(1)
            {
            }
        }
                
        if (Continue)
        {
            ret = S_NO_ERR;
            ToHexString(pbResultTmp, m_Result, sizeof(m_Result)-2);
        }
        else
            ret = S_TERMINATED;
    }
    else
        ret = S_FILE_OPEN_FAILED;
    
    CloseHandle(hFile);
    VirtualFree(pbHashBuffer, 0, MEM_RELEASE);

    return ret;
}

int SHA_1::CalculateSha1 (wchar_t *szFileName, ULONG64 offset, ULONG64 len, CallBack callback)
{
    HANDLE		hFile;
    DWORD		dwBytesRead = 0;
    int			iLoop, cnt = 0, ret = 0;
    BYTE		pbResultTmp[20];//, pbHashBuffer[SHA1_READ_BYTES+10];
    bool        Continue = true;

    PBYTE pbHashBuffer = (PBYTE)VirtualAlloc(
    NULL, SHA1_READ_BYTES+10, MEM_COMMIT, PAGE_READWRITE);
    if (!pbHashBuffer) return S_ERR_OTHERS;

    do 
    {
        memset(m_Result, 0, sizeof(m_Result));

        if (INVALID_HANDLE_VALUE != 
                    (hFile = CreateFile(szFileName, GENERIC_READ, 
                        FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)))
        {
            ULARGE_INTEGER ui;
            ui.LowPart = GetFileSize(hFile, (PDWORD)&ui.HighPart);
            if (ui.QuadPart < offset + len)
            {
                ret = S_INVALID_PARAMETER;
                break;
            }

            Sha1Init(len);

            ui.QuadPart = offset;

            if (INVALID_SET_FILE_POINTER ==
                SetFilePointer(hFile, ui.LowPart, (PLONG)&ui.HighPart, FILE_BEGIN))
            {
                ret = S_INVALID_PARAMETER;
                break;
            }

            if (ctx.uplength >= SHA1_READ_BYTES)
            {
                if (!ReadFile(hFile, pbHashBuffer, SHA1_READ_BYTES, &dwBytesRead, NULL)
                    || dwBytesRead != SHA1_READ_BYTES)
                {
                    ret = S_INVALID_PARAMETER;
                    break;
                }
            }
            else
            {
                if (!ReadFile(hFile, pbHashBuffer, ctx.uplength, &dwBytesRead, NULL)
                    || dwBytesRead != ctx.uplength)
                {
                    ret = S_INVALID_PARAMETER;
                    break;
                }
            }

        
            pData = pbHashBuffer;

            // ÿ�ζ�ȡ512��BLOCK����
            cnt = 0;
            while (Continue && ctx.uplength >= SHA1_READ_BYTES)
            {
                ctx.uplength -= SHA1_READ_BYTES;
                iLoop = __Scale;
                while(iLoop--)
                {
                    Sha1ProcessBlock(ctx._hash, (PDWORD)pData);
                    pData += SHA1_BLOCK_SIZE;
                }
                
                ReadFile(hFile, pbHashBuffer,
                    ctx.uplength>=SHA1_READ_BYTES ? SHA1_READ_BYTES : ctx.uplength,
                    &dwBytesRead, NULL);
                
                pData = pbHashBuffer;

                if (++cnt == 10)
                {
                    __try
                    {
                        Continue = callback(&ctx.length, &ctx.uplength);
                    }
                    __except(1)
                    {
                    }
                    cnt = 0;
                }
            }


            // ����ʣ�ಿ��
            if (Continue)
            {		
                iLoop = dwBytesRead / SHA1_BLOCK_SIZE;
                ctx.uplength -= iLoop * SHA1_BLOCK_SIZE;
                if (ctx.uplength < 0)
                {
                    ret = S_ERR_OTHERS;
                    break;
                }

                int cnt = 0;
                while(iLoop--)
                {
                    Sha1ProcessBlock(ctx._hash, (PDWORD)pData);
                    pData += SHA1_BLOCK_SIZE;
                }

                Sha1ProcessFinal(&ctx, pData, pbResultTmp);

                __try
                {
                    Continue = callback(&ctx.length, &ctx.uplength);
                }
                __except(1)
                {
                }
            }
                    
            if (Continue)
            {
                ret = S_OK;
                ToHexString(pbResultTmp, m_Result, sizeof(m_Result)-2);
            }
            else
                ret = S_TERMINATED;
        }
        else
            ret = S_FILE_OPEN_FAILED;
    }
    while (0);

    
    CloseHandle(hFile);

    VirtualFree(pbHashBuffer, 0, MEM_RELEASE);

    return ret;

}