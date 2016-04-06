/* 
 * Copyright (C) 2015 Johannes Schwab
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef _WIN32

#include "TunWin.hpp"
#include "ToxTun.hpp"
#include "Logger.hpp"
#include "Data.hpp"

#include <iphlpapi.h>
#include <winioctl.h>

TunWin::TunWin(const Tox *tox)
:
	TunInterface(tox),
	handle(INVALID_HANDLE_VALUE),
	ipPostfix(255),
	bytesRead(0),
	readState(ReadState::Idle),
	ipIsSet(false)
{
	constexpr char ADAPTER_KEY[] = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
	HKEY adapterKey;
	LONG status;
	DWORD len;
	std::list<std::string> devGuids;
	size_t i = 0;

	status = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			ADAPTER_KEY,
			0,
			KEY_READ,
			&adapterKey
	);

	if (status != ERROR_SUCCESS) {
		throw ToxTunError(Logger::concat("Can't open registry key ", ADAPTER_KEY));
	}

	while (true) {
		char enumName[256];
		std::string unitString;
		HKEY unitKey;
		DWORD dataType;

		len = sizeof(enumName);
		status = RegEnumKeyEx(
				adapterKey,
				i,
				enumName,
				&len,
				nullptr,
				nullptr,
				nullptr,
				nullptr
		);

		++i;

		if (status == ERROR_NO_MORE_ITEMS) break;

		if (status != ERROR_SUCCESS) {
			throw ToxTunError(Logger::concat("Error while reading registry subkeys of ", ADAPTER_KEY));
		}

		unitString = ADAPTER_KEY;
		unitString += "\\";
		unitString += enumName;

		status = RegOpenKeyEx(
				HKEY_LOCAL_MACHINE,
				unitString.c_str(),
				0,
				KEY_READ,
				&unitKey
		);

		if (status != ERROR_SUCCESS) {
			Logger::debug("Can't open registry key ", unitString);
			continue;
		}

		constexpr char COMPONENT_ID[] = "ComponentId";
		unsigned char componentId[256];
		len = sizeof(componentId);

		status = RegQueryValueEx(
				unitKey,
				COMPONENT_ID,
				NULL,
				&dataType,
				componentId,
				&len
		);

		if (status != ERROR_SUCCESS || dataType != REG_SZ) {
			Logger::debug("Can't open registry key ",
					unitString, "\\", COMPONENT_ID);
			RegCloseKey(unitKey);
			continue;
		}

		constexpr char NET_CFG_INSTANCE_ID[] = "NetCfgInstanceId";
		unsigned char netCfgInstanceId[256];
		len = sizeof(netCfgInstanceId);

		status = RegQueryValueEx(
				unitKey,
				NET_CFG_INSTANCE_ID,
				NULL,
				&dataType,
				netCfgInstanceId,
				&len
		);

		if (status == ERROR_SUCCESS && dataType == REG_SZ) {
			if (!strcmp(reinterpret_cast<char*>(componentId), "tap0901")) {
				devGuids.emplace_back(reinterpret_cast<char*>(netCfgInstanceId));
			}
		}

		RegCloseKey(unitKey);
	}

	RegCloseKey(adapterKey);

	Logger::debug("Found devices:");
	for (const auto &devGuid : devGuids) {
		Logger::debug("\t", devGuid);
	}

	for (const auto &devGuid : devGuids) {
		std::string devPath = "\\\\.\\Global\\";
		devPath += devGuid;
		devPath += ".tap";

		handle = CreateFile(
				devPath.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0,
				0,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
				0
		);

		if (handle != INVALID_HANDLE_VALUE) {
			Logger::debug("Succesfully opened TAP device \"", devPath, "\"");
			this->devGuid = devGuid;
			break;
		}

		Logger::debug("Can't open tun device file ", devPath);
	}

	if (handle == INVALID_HANDLE_VALUE) {
		throw ToxTunError("Can't open a tun device");
	}

	DWORD TAP_WIN_IOCTL_SET_MEDIA_STATUS = CTL_CODE(
			FILE_DEVICE_UNKNOWN,
			6,
			METHOD_BUFFERED,
			FILE_ANY_ACCESS
	);

	ULONG t = true;
	status = DeviceIoControl(
			handle,
			TAP_WIN_IOCTL_SET_MEDIA_STATUS,
			&t,
			sizeof(t),
			&t,
			sizeof(t),
			&len, 
			nullptr
	);

	if (!status) {
		throw ToxTunError("Can't set tun device to connected");
	}
}

TunWin::~TunWin() {
	unsetIp();

	DWORD TAP_WIN_IOCTL_SET_MEDIA_STATUS = CTL_CODE(
			FILE_DEVICE_UNKNOWN,
			6,
			METHOD_BUFFERED,
			FILE_ANY_ACCESS
	);

	ULONG f = false;
	DWORD len;
	DWORD status = DeviceIoControl(
			handle,
			TAP_WIN_IOCTL_SET_MEDIA_STATUS,
			&f,
			sizeof(f),
			&f,
			sizeof(f),
			&len, 
			nullptr
	);

	if (!status) {
		Logger::error("Can't set tun device to disconnected");
		return;
	}

	if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
}

void TunWin::setIp(uint8_t subnet, uint8_t postfix) noexcept {
	DWORD status;

	uint32_t ip = 0xc0a80000 | (static_cast<uint32_t>(subnet) << 8) | postfix;
	uint32_t netmask = 0xFFFFFF00;

	try {
		DWORD index = getAdapterIndex();
		ULONG NTEInstance;

		status = AddIPAddress(
				htonl(ip),
				htonl(netmask),
				index,
				&ipApiContext,
				&NTEInstance
		);

		if (status != NO_ERROR) throw ToxTunError("AddIpAddress failed");

		Logger::debug("Set IP to ", ipv4FromPostfix(subnet, postfix));
		ipIsSet = true;
	} catch (ToxTunError &error) {
		Logger::error("Can't set IP address. Pleas set IP to ", ipv4FromPostfix(subnet, postfix), " manually");
	}

	Logger::debug("Tun device successfully started");
}

ULONG TunWin::getAdapterIndex() const {
	DWORD index, status;

	std::wstring adapterName = L"\\DEVICE\\TCPIP_";
	for (const auto &c : devGuid) adapterName += c;

	status = GetAdapterIndex(
			const_cast<wchar_t*>(adapterName.c_str()),
			&index
	);

	if (status == NO_ERROR) return index;

	Logger::debug("GetAdapterIndex failed");

	ULONG size = 0;
	status = GetAdaptersInfo(nullptr, &size);
	if (status != ERROR_BUFFER_OVERFLOW) {
		throw ToxTunError("GetAdapterInfo (size) failed");
	}

	IP_ADAPTER_INFO *adapterInfo = new IP_ADAPTER_INFO[size];
	if (!adapterInfo) throw ToxTunError("GetAdapterIndo failed");

	status = GetAdaptersInfo(adapterInfo, &size);
	if (status != NO_ERROR) {
		throw ToxTunError("GetAdapterInfo failed");
	}

	IP_ADAPTER_INFO *tmp = adapterInfo;
	while (tmp) {
		if (devGuid == tmp->AdapterName) break;
		tmp = tmp->Next;
	}

	if (tmp != nullptr) {
		index = tmp->Index;
		delete[] adapterInfo;
		return index;
	}

	delete[] adapterInfo;
	throw ToxTunError("No matching adapter in IP_ADAPTER_INFO");
}

void TunWin::unsetIp() {
	DWORD status;

	if (ipIsSet) {
		status = DeleteIPAddress(ipApiContext);
		if (status != NO_ERROR) {
			Logger::error("Can't remove IPv4 from tun device");
		}
		ipIsSet = false;
	}

	Logger::debug("Tun shutted down");
}

std::list<std::array<uint8_t, 4>> TunWin::getUsedIp4Addresses() {
	ULONG size = 0;
	DWORD status = GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &size);
	if (status != ERROR_BUFFER_OVERFLOW) {
		throw ToxTunError("GetAdapterAddresses (size) failed");
	}

	IP_ADAPTER_ADDRESSES *adapterAddresses = new IP_ADAPTER_ADDRESSES[size];
	if (!adapterAddresses) throw ToxTunError("GetAdapterAddresses failed");

	status = GetAdaptersAddresses(AF_INET, 0, nullptr, adapterAddresses, &size);
	if (status != NO_ERROR) {
		throw ToxTunError("GetAdapterAddresses failed");
	}

	std::list<std::array<uint8_t, 4>> usedIps;

	IP_ADAPTER_ADDRESSES *tmp = adapterAddresses;
	while (tmp) {
		IP_ADAPTER_UNICAST_ADDRESS *uAddr = tmp->FirstUnicastAddress;
		while (uAddr) {
			SOCKET_ADDRESS addr = uAddr->Address;
			if (addr.lpSockaddr->sa_family == AF_INET) {
				struct sockaddr_in *a = reinterpret_cast<struct sockaddr_in*>(
						addr.lpSockaddr
				);
				std::array<uint8_t, 4> t;
				t[0] = a->sin_addr.S_un.S_un_b.s_b1;
				t[1] = a->sin_addr.S_un.S_un_b.s_b2;
				t[2] = a->sin_addr.S_un.S_un_b.s_b3;
				t[3] = a->sin_addr.S_un.S_un_b.s_b4;
				usedIps.push_back(t);
			}
			uAddr = uAddr->Next;
		}
		tmp = tmp->Next;
	}

	delete[] adapterAddresses;
	return usedIps;
}

bool TunWin::dataPending() {
	switch (readState) {
		case ReadState::Queued:
			if (HasOverlappedIoCompleted(&overlappedRead)) {
				setBytesRead();
				readState = ReadState::Ready;
				return true;
			} else {
				return false;
			}
		case ReadState::Idle:
			queueRead();
			return (readState == ReadState::Ready);
		case ReadState::Ready:
			return true;
		default:
			throw ToxTunError("Unhandled state in TunWin::dataPending");
	}
}

void TunWin::queueRead() {
	memset(&overlappedRead, 0, sizeof(overlappedRead));

	bool status = ReadFile(
			handle,
			readBuffer,
			sizeof(readBuffer),
			nullptr,
			&overlappedRead
	);

	if (status) {
		setBytesRead();
		readState = ReadState::Ready;
		Logger::debug("ReadFile returned immediatly");
	} else if (GetLastError() == ERROR_IO_PENDING) {
		Logger::debug("ReadFile queued");
		readState = ReadState::Queued;
	} else {
		throw ToxTunError("ReadFile failed");
	}
}

void TunWin::setBytesRead() {
	bool status = GetOverlappedResult(
			handle,
			&overlappedRead,
			&bytesRead,
			false
	);

	if (status) {
		Logger::debug(bytesRead, " bytes read from tun");
	} else if (GetLastError() == ERROR_IO_INCOMPLETE) {
		Logger::debug("setBytesRead called while read is still pending. Trying to fix this...");
		readState = ReadState::Queued;
	} else {
		throw ToxTunError("Error while calling GetOverlappedResult");
	}
}

Data TunWin::getDataBackend() {
	if (readState != ReadState::Ready) {
		throw ToxTunError("Wrong readState in getData");
	}

	readState = ReadState::Idle;
	queueRead();

	Logger::debug("readBuffer returned");

	return Data::fromTunData(readBuffer, bytesRead);
}

void TunWin::sendData(const Data &data) {
	bool status;
	DWORD written;

	overlappedWrite.emplace_front();
	memset(&overlappedWrite.front(), 0, sizeof(overlappedWrite.front()));

	status = WriteFile(
			handle,
			data.getIpData(),
			data.getIpDataLen(),
			&written,
			&overlappedWrite.front()
	);

	if (status) {
		overlappedWrite.pop_front();
	} else {
		if (GetLastError() != ERROR_IO_PENDING) {
			throw ToxTunError("Writing to tun failed");
		}
	}

	while (!overlappedWrite.empty() && HasOverlappedIoCompleted(&overlappedWrite.back())) {
		overlappedWrite.pop_back();
	}

	Logger::debug(written, " bytes written to TUN");
}

#endif //_WIN32
