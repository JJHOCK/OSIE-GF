#include "stdafx.h"

HANDLE g_Server = NULL;

#define MSG_BUFFER_SIZE 8192

void WriteMemoryBYTES(unsigned int uAddress, void* bytes, unsigned int len)
{
	DWORD flOldProtect;
	SIZE_T uNumberOfBytesWritten;
	HANDLE hServer;
	
	if((hServer = g_Server) && len)
	{
		VirtualProtectEx(hServer, (LPVOID)uAddress, len, PAGE_WRITECOPY, &flOldProtect);
		WriteProcessMemory(hServer, (LPVOID)uAddress, bytes, len, &uNumberOfBytesWritten);
		FlushInstructionCache(hServer, (LPVOID)uAddress, len);
		VirtualProtectEx(hServer, (LPVOID)uAddress, len, flOldProtect, &flOldProtect);
	}
}

void WriteMemoryQWORD(unsigned int uAddress, unsigned __int64 value) { WriteMemoryBYTES(uAddress, &value, sizeof(unsigned __int64)); }
void WriteMemoryDWORD(unsigned int uAddress, unsigned int value)	 { WriteMemoryBYTES(uAddress, &value, sizeof(unsigned int)); }
void WriteMemoryWORD(unsigned int uAddress, unsigned short value)	 { WriteMemoryBYTES(uAddress, &value, sizeof(unsigned short)); }
void WriteMemoryBYTE(unsigned int uAddress, unsigned char value)	 { WriteMemoryBYTES(uAddress, &value, sizeof(unsigned char)); }

void NOPMemory(unsigned int uAddress, unsigned int len)
{
	unsigned int dword_count = (len / 4), byte_count = (len % 4);
	unsigned char Byte = 0x90; 
	unsigned int Dword = 0x90666666;

	DWORD flOldProtect;
	SIZE_T uNumberOfBytesWritten;
	HANDLE hServer;

	if((hServer = g_Server) && len)
	{
		VirtualProtectEx(hServer, (LPVOID)uAddress, len, PAGE_WRITECOPY, &flOldProtect);
		while(dword_count != NULL)
		{
			WriteProcessMemory(hServer, (LPVOID)uAddress, &Dword, sizeof(unsigned int), &uNumberOfBytesWritten);
			uAddress += sizeof(unsigned int);
			dword_count--;
		}
		while(byte_count != NULL)
		{
			WriteProcessMemory(hServer, (LPVOID)uAddress, &Byte, sizeof(unsigned char), &uNumberOfBytesWritten);
			uAddress += sizeof(unsigned char);
			byte_count--;
		}
		FlushInstructionCache(hServer, (LPVOID)uAddress, len);
		VirtualProtectEx(hServer, (LPVOID)uAddress, len, flOldProtect, &flOldProtect);
	}
}

void NULLMemory(unsigned int uAddress, unsigned int len)
{
	unsigned int dword_count = (len / 4), byte_count = (len % 4);
	unsigned char Byte = 0x00; 
	unsigned int Dword = 0x00000000;

	DWORD flOldProtect;
	SIZE_T uNumberOfBytesWritten;
	HANDLE hServer;

	if((hServer = g_Server) && len)
	{
		VirtualProtectEx(hServer, (LPVOID)uAddress, len, PAGE_WRITECOPY, &flOldProtect);
		while(dword_count != NULL)
		{
			WriteProcessMemory(hServer, (LPVOID)uAddress, &Dword, sizeof(unsigned int), &uNumberOfBytesWritten);
			uAddress += sizeof(unsigned int);
			dword_count--;
		}
		while(byte_count != NULL)
		{
			WriteProcessMemory(hServer, (LPVOID)uAddress, &Byte, sizeof(unsigned char), &uNumberOfBytesWritten);
			uAddress += sizeof(unsigned char);
			byte_count--;
		}
		FlushInstructionCache(hServer, (LPVOID)uAddress, len);
		VirtualProtectEx(hServer, (LPVOID)uAddress, len, flOldProtect, &flOldProtect);
	}
}

void WriteInstruction(unsigned int uAddress, unsigned int uDestination, unsigned char uFirstByte)
{
	unsigned char ExecLine[5];
	ExecLine[0] = uFirstByte;
	*((int*)(ExecLine + 1)) = (((int)uDestination) - (((int)uAddress) + 5));
	WriteMemoryBYTES(uAddress, ExecLine, 5);
}

void WriteInstructionCallJmpEax(unsigned int uAddress, unsigned int uDestination, unsigned int uNopEnd)
{
	unsigned char ExecLine[7];
	ExecLine[0] = 0xE8;
	*((int*)(ExecLine + 1)) = (((int)uDestination) - (((int)uAddress) + 5));
	*((unsigned short*)(ExecLine + 5)) = 0xE0FF;
	WriteMemoryBYTES(uAddress, ExecLine, 7);
	if(uNopEnd && uNopEnd > (uAddress + 7))
		NOPMemory((uAddress + 7), (uNopEnd - (uAddress + 7)));
}

void WriteInstructionCall(unsigned int uAddress, unsigned int uDestination, unsigned int uNopEnd)
{
	unsigned char ExecLine[5];
	ExecLine[0] = 0xE8;
	*((int*)(ExecLine + 1)) = (((int)uDestination) - (((int)uAddress) + 5));
	WriteMemoryBYTES(uAddress, ExecLine, 5);
	if(uNopEnd && uNopEnd > (uAddress + 5))
		NOPMemory((uAddress + 5), (uNopEnd - (uAddress + 5)));
}

void WriteInstructionJmp(unsigned int uAddress, unsigned int uDestination, unsigned int uNopEnd)
{
	unsigned char ExecLine[5];
	ExecLine[0] = 0xE9;
	*((int*)(ExecLine + 1)) = (((int)uDestination) - (((int)uAddress) + 5));
	WriteMemoryBYTES(uAddress, ExecLine, 5);
	if(uNopEnd && uNopEnd > (uAddress + 5))
		NOPMemory((uAddress + 5), (uNopEnd - (uAddress + 5)));
}

// =================================================================================================================

CCodeRestorator::CCodeRestorator(bool _bIsUseLog, bool _bIsShowRate)
{
	this->pFileMem = NULL;
	this->uBaseAddrOffset = 0x400000;
	this->uTextAddrOffset = (this->uBaseAddrOffset + 0x1000);
	this->uTextSectionOffset = 0x400;
	this->bIsUseLog = _bIsUseLog;
	this->bIsShowRate = _bIsShowRate;
	this->pLogFile = NULL;
}

bool CCodeRestorator::OpenParentFile(HANDLE _hServer, unsigned __int64 uAppHash)
{
	FILE* _pFile;
	BYTE* _pFileMem;
	wchar_t pFilePath[MAX_PATH];

	GetModuleFileNameW((HMODULE)this->uBaseAddrOffset, pFilePath, MAX_PATH);

	if(!_wfopen_s(&_pFile, pFilePath, L"rb"))
	{
		fseek(_pFile, 0, SEEK_END);
		UINT32 uSize = (UINT32)ftell(_pFile);
		fseek(_pFile, 0, SEEK_SET);

		if(uSize < 33554432 && (_pFileMem = (BYTE*)malloc(uSize)))
		{
			if(fread(_pFileMem, uSize, 1, _pFile) == 1)
			{
				fclose(_pFile);
				if(*((UINT64*)(_pFileMem + this->uTextSectionOffset)) == uAppHash)
				{
					this->pFileMem = _pFileMem;
					this->hServer = _hServer;

					if(this->bIsUseLog)
					{
						size_t uPathSize = wcslen(pFilePath);
						if(uPathSize > 7)
						{
							uPathSize--; pFilePath[uPathSize] = 'l';
							uPathSize--; pFilePath[uPathSize] = 'r';
							uPathSize--; pFilePath[uPathSize] = 'c';
							uPathSize--; 

							for(; uPathSize != NULL; uPathSize--)
							{
								if(pFilePath[uPathSize] == '\\')
								{
									uPathSize++;
									break;
								}
							}
							if(uPathSize != NULL)
							{
								if(!_wfopen_s(&_pFile, &pFilePath[uPathSize], L"wb"))
								{
									unsigned char Encoding[2] = { 0xFF, 0xFE };
									if(fwrite(Encoding, 2, 1, _pFile) == 1)
										this->pLogFile = _pFile;
									else
										fclose(_pFile);
								}
							}
						}

						SYSTEMTIME log_time;
						GetLocalTime(&log_time);
						this->AddToLog(L" === Code Restorator Log === %02d/%02d/%04d %02d:%02d:%02d.%03d === ", log_time.wMonth, log_time.wDay, log_time.wYear, 
						log_time.wHour, log_time.wMinute, log_time.wSecond, log_time.wMilliseconds);
					}
					return true;
				}
				else
					free(_pFileMem);
			}
			else
			{
				free(_pFileMem);
				fclose(_pFile);
			}
		}
		else
			fclose(_pFile);
	}
	return false;
}

bool CCodeRestorator::RestoreFunctionCode(unsigned int uAddress, unsigned int uControlSize)
{
	if(uAddress > this->uTextAddrOffset)
	{
		unsigned int uPercentage, len = 0, uRealOffset = ((uAddress - this->uTextAddrOffset) + this->uTextSectionOffset);
		unsigned char uch, *pI, *pCode = &this->pFileMem[uRealOffset];
		pI = pCode;

		while(true)
		{
			if((uch = *pI) == 0xCC)
			{
				if(*((unsigned short*)(pI - 7)) == 0xFF48)
				{
					uPercentage = 80;
					break;
				}
				else if(*((unsigned char*)(pI - 5)) == 0xE9)
				{
					uPercentage = 80;
					break;
				}
				else if(*((unsigned int*)pI) == 0xCCCCCCCC)
				{
					uPercentage = 95;
					break;
				}
				else if(*((unsigned short*)pI) == 0xCCCC)
				{
					uPercentage = 66;
					break;
				}
			}
			else if(uch == 0xC3)
			{
				if(*((unsigned int*)pI) == 0xCCCCCCC3)
				{
					uPercentage = 96;
					if(*((unsigned char*)(pI + 4)) == 0x40)
						uPercentage += 4;
					if((uch = *((unsigned char*)(pI - 1))) >= 0x5B && uch <= 0x60)
						uPercentage += 4;
					pI++;
					break;
				}
				else if(*((unsigned short*)pI) == 0xCCC3)
				{
					if(*((unsigned char*)(pI + 2)) == 0xCC)
					{
						uPercentage = 75;
						if(*((unsigned char*)(pI + 3)) == 0x40)
							uPercentage += 4;
					}
					else
					{
						uPercentage = 50;
						if(*((unsigned char*)(pI + 2)) == 0x40)
							uPercentage += 4;
					}
					if((uch = *((unsigned char*)(pI - 1))) >= 0x5B && uch <= 0x60)
						uPercentage += 4;
					pI++;
					break;
				}
				else if(*((unsigned char*)(pI + 1)) == 0x40 && ((uch = *((unsigned char*)(pI - 1))) >= 0x5B && uch <= 0x60))
				{
					uPercentage = 55;
					pI++;
					break;
				}
			}
			pI++;
		}
		len = (unsigned int)(pI - pCode);

		if(uControlSize)
		{
			if(uControlSize != len)
			{
				if(uControlSize > len)
				{
					this->AddToLog(L"offset 0x%08X size check error, func size %u < control size %u, use %u", uAddress, len, uControlSize, uControlSize);
					len = uControlSize;
				}
				else
					this->AddToLog(L"offset 0x%08X size check error, func size %u > control size %u, use %u", uAddress, len, uControlSize, len);
			}
		}
		else
		{
			if(this->bIsShowRate)
				this->AddToLog(L"offset 0x%08X, func size %u bytes, %u %%", uAddress, len, uPercentage);
		}

		DWORD flOldProtect;
		SIZE_T uNumberOfBytesWritten;

		if(!VirtualProtectEx(this->hServer, (LPVOID)uAddress, len, PAGE_WRITECOPY, &flOldProtect))
			this->AddToLog(L"VirtualProtectEx begin failed at offset 0x%08X, func size %u bytes, %u %%", uAddress, len, uPercentage);

		if(!WriteProcessMemory(this->hServer, (LPVOID)uAddress, pCode, len, &uNumberOfBytesWritten))
			this->AddToLog(L"WriteProcessMemory failed at offset 0x%08X, func size %u bytes, %u %%", uAddress, len, uPercentage);

		if(!FlushInstructionCache(this->hServer, (LPVOID)uAddress, len))
			this->AddToLog(L"FlushInstructionCache failed at offset 0x%08X, func size %u bytes, %u %%", uAddress, len, uPercentage);

		if(!VirtualProtectEx(this->hServer, (LPVOID)uAddress, len, flOldProtect, &flOldProtect))
			this->AddToLog(L"VirtualProtectEx end failed at offset 0x%08X, func size %u bytes, %u %%", uAddress, len, uPercentage);

		return true;
	}
	return false;
}

void CCodeRestorator::AddToLog(const wchar_t* format, ...)
{
	va_list va;
	va_start(va, format);
	wchar_t pBuff[MSG_BUFFER_SIZE];
	unsigned int uSize = (unsigned int)vswprintf_s(pBuff, (MSG_BUFFER_SIZE - 2), format, va);
	if(uSize < (MSG_BUFFER_SIZE - 2) && this->pLogFile && this->bIsUseLog)
	{
		pBuff[uSize] = 0x0D; uSize++;
		pBuff[uSize] = 0x0A; uSize++;
		fwrite((void*)&pBuff[0], (uSize * 2), 1, this->pLogFile);
	}
	va_end(va);
}

// =================================================================================================================

bool CFileLog::OpenLogFile(const wchar_t* pLogFilePath)
{
	SYSTEMTIME _log_time;
	GetLocalTime(&_log_time);
	wchar_t pFilePath[MAX_PATH];
	UINT32 uPathSize = GetModuleFileNameW(NULL, pFilePath, MAX_PATH), uFilePathSize = (UINT32)wcslen(pLogFilePath);
	if(uPathSize < (MAX_PATH - 15) && *pLogFilePath != '\\')
	{
		for(; uPathSize != NULL; uPathSize--)
		{
			if(pFilePath[uPathSize] == '\\')
			{
				uPathSize++;
				break;
			}
		}
		if(((MAX_PATH - 15) - uPathSize) > uFilePathSize)
		{
			memcpy(this->log_file_path, pFilePath, (uPathSize * 2));
			memcpy(&this->log_file_path[uPathSize], pLogFilePath, (uFilePathSize * 2));
			uPathSize += uFilePathSize;
			if(((UINT32)swprintf_s(&this->log_file_path[uPathSize], (MAX_PATH - uPathSize), L"-%04u-%02u-%02u.log", _log_time.wYear, _log_time.wMonth, _log_time.wDay)) < MAX_PATH)
				return true;
		}
	}
	return false;
}

void CFileLog::AddToLog(const wchar_t* format, ...)
{
	va_list va;
	va_start(va, format);
	SYSTEMTIME _log_time;
	wchar_t pBuff[FILE_LOG_ELEMENT_BUFFER_SIZE];
	GetLocalTime(&_log_time);
	unsigned int uLineIndex, uSize = (unsigned int)vswprintf_s(pBuff, (FILE_LOG_ELEMENT_BUFFER_SIZE - 2), format, va);
	if(uSize < (FILE_LOG_ELEMENT_BUFFER_SIZE - 2))
	{
		pBuff[uSize] = 0x0D; uSize++;
		pBuff[uSize] = 0x0A; uSize *= 2;

		this->Enter();

		if(!(uLineIndex = (this->log_data_counter++ % FILE_LOG_COUNT_TO_SAVE)))
			this->Save();

		memcpy(&this->log_line[uLineIndex].log_time, &_log_time, sizeof(SYSTEMTIME));
		memcpy(this->log_line[uLineIndex].buff, pBuff, uSize);
		this->log_line[uLineIndex].buff_size = uSize;

		this->Leave();
	}
	va_end(va);
}

void CFileLog::Save()
{
	FILE* pFile;
	wchar_t pBuff[96];
	if(!_wfopen_s(&pFile, this->log_file_path, L"ab"))
	{
		fseek(pFile, 0, SEEK_END);
		UINT32 uSize = (UINT32)ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		if(!uSize)
		{
			unsigned char Encoding[2] = { 0xFF, 0xFE };
			fwrite(Encoding, 2, 1, pFile);
		}

		for(unsigned int i = 0; i < FILE_LOG_COUNT_TO_SAVE; i++)
		{
			if(this->log_line[i].buff_size)
			{
				unsigned int uSize = (unsigned int)swprintf_s(pBuff, 96, L"%02u/%02u/%04u %02u:%02u:%02u.%03u, ", this->log_line[i].log_time.wMonth, this->log_line[i].log_time.wDay, this->log_line[i].log_time.wYear, this->log_line[i].log_time.wHour, this->log_line[i].log_time.wMinute, this->log_line[i].log_time.wSecond, this->log_line[i].log_time.wMilliseconds);
				if(uSize < 96)
				{
					fwrite(pBuff, (uSize * 2), 1, pFile);
					fwrite(this->log_line[i].buff, this->log_line[i].buff_size, 1, pFile);
				}
			}
		}
		fclose(pFile);
	}
}

// =================================================================================================================

void Msg(const char* title, const char* format, ...)
{
	char pBuff[MSG_BUFFER_SIZE];
	va_list va;
	va_start(va, format);
	UINT32 ret_size = (UINT32)vsprintf_s(pBuff, MSG_BUFFER_SIZE, format, va);
	if(ret_size < MSG_BUFFER_SIZE)
		MessageBoxA(NULL, pBuff, title, MB_OK);
	va_end(va);
}

void Msg(const wchar_t* title, const wchar_t* format, ...)
{
	wchar_t pBuff[MSG_BUFFER_SIZE];
	va_list va;
	va_start(va, format);
	UINT32 ret_size = (UINT32)vswprintf_s(pBuff, MSG_BUFFER_SIZE, format, va);
	if(ret_size < MSG_BUFFER_SIZE)
		MessageBoxW(NULL, pBuff, title, MB_OK);
	va_end(va);
}

//
