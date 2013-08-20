#pragma once

#pragma pack(push, 1)

class CSocket : public CIOSocket
{
public:
	CSocket() {};
	~CSocket() {};

	/* 090 */ virtual void SetAddress(in_addr ip) { this->client_ip = ip; };
	/* 098 */ virtual in_addr GetAddress() { return this->client_ip; };
	/* 0A0 */ 

	/* 00B0 */ unsigned int _uUnkVal00B0[2];
	/* 00B8 */ in_addr client_ip;
	/* 00BC */ unsigned int _uUnkVal00BC;
	/* 00C0 */ UINT64* pPacketTable;
	/* 00C8 */ unsigned int _uUnkVal00C8[14];
	/* 0100 */ 
};

#pragma pack(pop)

CompileTimeOffsetCheck(CSocket, client_ip, 0x00B8);
CompileTimeSizeCheck(CSocket, 0x0100);