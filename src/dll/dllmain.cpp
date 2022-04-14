#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <detours.h>
#include <cstdio>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>

/////////////////////////

struct LicenseDataStruct
{
	LicenseDataStruct() : hostPort("14900") { }

	std::unordered_map<std::string, std::string> hostResolves;
	std::string hostPort;
};

static LicenseDataStruct LicenseData{  };

static const char * ogHostNames[] = {
	"loginserver.graalonline.com",
	"Graalonline.com",
	"loginserver2.graalonline.com",
	"loginserver3.graalonline.com"
};

static const char * licenseFilePaths[] = {
	"license.graal",
	"license/license.graal"
};

bool ReadLicensesFile(const std::string& file)
{
	LicenseData = {};

	std::ifstream input(file);
	if (input.fail())
		return false;

	std::string line;

	auto idx = 0;
	while (std::getline(input, line))
	{
		if (idx == 0)
			LicenseData.hostResolves[ogHostNames[0]] = line;
		else if (idx == 1)
			LicenseData.hostPort = line;
		else if (idx == 2)
			LicenseData.hostResolves[ogHostNames[1]] = line;
		else if (idx == 5)
			LicenseData.hostResolves[ogHostNames[2]] = line;
		else if (idx == 6)
			LicenseData.hostResolves[ogHostNames[3]] = line;

		++idx;
	}

	input.close();
	return LicenseData.hostResolves.size() > 0;
}

/////////////////////////

// Address of the real API
hostent* (WSAAPI * True_gethostbyname)(const char* name) = gethostbyname;
int (WSAAPI * True_connect)(SOCKET s, const sockaddr* name, int namelen) = connect;

// Our intercept function
hostent* WSAAPI Hooked_gethostbyname(const char* name)
{
	if (!LicenseData.hostResolves.empty())
	{
		auto it = LicenseData.hostResolves.find(name);

		if (it != LicenseData.hostResolves.end())
			return True_gethostbyname(it->second.c_str());
	}

	return True_gethostbyname(name);
}

int WSAAPI Hooked_connect(SOCKET s, const sockaddr* name, int namelen)
{
	struct sockaddr_in *addr = (sockaddr_in *)name;

	// The client connects to the server on port 14900 so we can use this to inject our port
	if (ntohs(addr->sin_port) == 14900)
	{

		uint16_t port = std::stoi(LicenseData.hostPort);
		addr->sin_port = htons(port);
	}

	return True_connect(s, (struct sockaddr *)addr, namelen);
}

/////////////////////////

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(&(PVOID&)True_gethostbyname, Hooked_gethostbyname);
			DetourAttach(&(PVOID&)True_connect, Hooked_connect);

			LONG lError = DetourTransactionCommit();
			if (lError != NO_ERROR) {
				MessageBox(HWND_DESKTOP, "Failed to detour", "timb3r", MB_OK);
				return FALSE;
			}

			for (const auto& file : licenseFilePaths)
			{
				if (ReadLicensesFile(file))
					break;
			}
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourDetach(&(PVOID&)True_gethostbyname, Hooked_gethostbyname);
			DetourDetach(&(PVOID&)True_connect, Hooked_connect);

			LONG lError = DetourTransactionCommit();
			if (lError != NO_ERROR) {
				MessageBox(HWND_DESKTOP, "Failed to detour", "timb3r", MB_OK);
				return FALSE;
			}
		}
		break;
	}

	return TRUE;
}

