#include "Sha1.h"

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//  功能：初始化psi结构体
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool SHA_1::Sha1Init(HANDLE hFile)
{
	LONGLONG	dwSize = 0;

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

	ctx.length = GetFileSize(hFile, (DWORD *)&dwSize);
	if (dwSize)
        ctx.length += ((dwSize & 0x00000000ffffffff) << 32);
    ctx.uplength = ctx.length;

	// 初始化算法的几个常量，魔术数
    ctx._hash[0] = 0x67452301;
    ctx._hash[1] = 0xefcdab89;
    ctx._hash[2] = 0x98badcfe;
    ctx._hash[3] = 0x10325476;
    ctx._hash[4] = 0xc3d2e1f0;

	return true;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//	功能：复制数据同时改变大小端模式
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SHA_1::SwapMemCopy(PBYTE to, PBYTE from, int len)
{
	int i, remain = (4 - (len & 3)) & 3;

	RtlCopyMemory(to, from, len);

	if (remain)		// 调整为4的倍数
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
//	功能：将生成的字串转换为16进制文本字串
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int SHA_1::ToHexString(PBYTE in, wchar_t *out, int cb)		// _itow_s显示0有点问题
{
	wchar_t szBuffer[4];
	
	if (!out || !in || cb < (SHA1_HASH_SIZE+1) * sizeof(wchar_t))	// (SHA1_HASH_SIZE+1) * 2
		return -1;

	out[0] = '\0';
		
	for (int i=0; i<SHA1_HASH_SIZE; ++i)
	{

		wsprintf(szBuffer, L"%02X", in[i]);
		wcscat_s(out, cb/2, szBuffer);
	}

	return 0;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//	功能：标准64Byte块的计算
//  hash	- 储存hash中间值
//  Block	- 文件块， 64B大小， 16个DWORD
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
	
	// 处理
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
//	功能：对最后小于64Byte的数据进行填充和计算
//  psi		- 储存hash相关信息的结构体指针
//  pBuffer	- 文件指针
//  pResult	- 存放hash结果缓冲区
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

	//获取插入0x80的位置
	iIndex = (DWORD)(psi->length & 63) >> 2;
	iShift = (DWORD)(psi->length & 3) << 3;

	//插入0x80并补0
	message[iIndex] &= ~(-1 << iShift);
	message[iIndex++] ^= 0x80<<iShift;

	//如果这个Block剩下的长度不足64Bits, 则先补0处理
	if (iIndex > 14)
	{
		while(iIndex < 16)
		{
			message[iIndex++] = 0;
		}
		Sha1ProcessBlock(psi->_hash, message);
		iIndex = 0;
	}

	// 最后一个只含文件大小的Block
	while (iIndex < 14)
	{
		message[iIndex++] = 0;
	}

	// Bit数，乘8
	lFileLen = psi->length << 3;

	// 小端模式转换为大端
#if	c_BytesOrder == ZEN_LITTLE_ENDIAN
	lFileLen = ZEN_SWAP_UINT64(lFileLen);
#endif

	message[14] = (DWORD)(lFileLen & 0x00000000FFFFFFFF);
	message[15] = (DWORD)((lFileLen & 0xFFFFFFFF00000000) >> 32);

	Sha1ProcessBlock(psi->_hash, message);

	// 将结果转回小端
#if c_BytesOrder == ZEN_LITTLE_ENDIAN
	SwapMemCopy((PBYTE)pResult, (PBYTE)psi->_hash, SHA1_HASH_SIZE);
#else
	RtlCopyMemory(pResult, psi->_hash, SHA1_HASH_SIZE);
#endif
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//  功能：外部调用的接口
//  szFileName		- 目标文件名
//  pbHashResult	- 接收结果的缓冲区
//  iSize			- 缓冲区大小
//  bContinue		- 终止标志
//  返回值：0-正常结束， 1-用户终止， -1-无法打开文件
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int SHA_1::CalculateSha1(wchar_t *szFileName, wchar_t* pbHashResult, int cb, bool bContinue)
{
	HANDLE		hFile;
	BYTE		pbResultTmp[20], pbHashBuffer[SHA1_READ_BYTES+10];
	DWORD		dwBytesRead;
	int			iLoop;

	if (INVALID_HANDLE_VALUE != 
				(hFile = CreateFile(szFileName, GENERIC_READ, 
					FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)))
	{
		Sha1Init(hFile);
		ReadFile(hFile, pbHashBuffer, SHA1_READ_BYTES, &dwBytesRead, NULL);
		pData = pbHashBuffer;

		// 每次读取512个BLOCK计算
		while (bContinue && dwBytesRead == SHA1_READ_BYTES)
		{
			ctx.uplength -= SHA1_READ_BYTES;
			iLoop = 512;
			while(iLoop--)
			{
				Sha1ProcessBlock(ctx._hash, (DWORD *)pData);
				pData += SHA1_BLOCK_SIZE;
			}
			ReadFile(hFile, pbHashBuffer, SHA1_READ_BYTES, &dwBytesRead, NULL);
			pData = pbHashBuffer;
		}
		// 处理剩余部分
		if (bContinue)				
		{
			iLoop		= dwBytesRead / SHA1_BLOCK_SIZE;
            ctx.uplength	-= iLoop * SHA1_BLOCK_SIZE;
			while(iLoop--)
			{
				Sha1ProcessBlock(ctx._hash, (DWORD *)pData);
				pData += SHA1_BLOCK_SIZE;
			}
			Sha1ProcessFinal(&ctx, pData, pbResultTmp);
		}
		CloseHandle(hFile);
		if (bContinue)
		{
			ToHexString(pbResultTmp, pbHashResult, cb);
			return 0;
		}
		else return 1;
	}
	return -1;
}

int SHA_1::CalculateSha1 (wchar_t *szFileName, ULONG64 offset, ULONG64 len, wchar_t *pbHashResult, int cb, bool bContinue)
{
    return 0;
}