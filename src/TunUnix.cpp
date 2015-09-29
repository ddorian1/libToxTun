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

#ifdef __unix

#include "TunUnix.hpp"
#include "Logger.hpp"
#include "Error.hpp"
#include "Data.hpp"

#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h> 
#include <cstdio>
#include <cstdlib>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <tox/tox.h>

TunUnix::TunUnix() 
:
	fd(open("/dev/net/tun", O_RDWR))
{
	if (fd < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("Error while opening \"/dev/net/tun\": ", errStr);
		throw Error(Error::Err::Permanent);
	}

	struct ifreq ifr = {}; //{} initializes ifr with 0

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

	if (ioctl(fd, TUNSETIFF, reinterpret_cast<void *>(&ifr)) < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("ioctl failed: ", errStr);
		close(fd);
		throw Error(Error::Err::Permanent);
	}

	name = ifr.ifr_name;

	Logger::debug("Succesfully opened TAP device \"", ifr.ifr_name, "\"");
}

TunUnix::~TunUnix() {
	if (fd >= 0) close(fd);
}

void TunUnix::setIp(const uint8_t postfix) {
	struct ifreq ifr = {};
	struct sockaddr_in sai = {};
	in_addr_t in_addr = {};

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);

	in_addr = inet_addr(ipv4FromPostfix(postfix).c_str());
	sai.sin_family = AF_INET;
	sai.sin_addr.s_addr = in_addr;
	memcpy(&ifr.ifr_addr, &sai, sizeof(struct sockaddr));

	if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("Setting ip with ioctl failed: ", errStr);
		Logger::error("Please set IPv4 to ", ipv4FromPostfix(postfix), " manually");
	}

	in_addr = inet_addr("255.255.255.0");
	sai.sin_addr.s_addr = in_addr;
	memcpy(&ifr.ifr_addr, &sai, sizeof(struct sockaddr));

	if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("Setting netmask with ioctl failed: ", errStr);
		Logger::error("Please set netmask to 255.255.255.0 manually");
	}

	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("Getting socket flags with ioctl failed: ", errStr);
		Logger::error("Please set ", name, " up manually");
		close(fd);
		return;
	}

	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("Setting socket flags with ioctl failed: ", errStr);
		Logger::error("Please set ", name, " up manually");
	} else {
		Logger::debug("Tun interface successfully set up");
	}

	close(fd);
}

void TunUnix::unsetIp() {
	struct ifreq ifr = {};

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);

	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("Getting socket flags with ioctl failed: ", errStr);
		Logger::error("Please shut down ", name, " manually");
		close(fd);
		return;
	}

	ifr.ifr_flags &= ~IFF_UP & ~IFF_RUNNING;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("Setting socket flags with ioctl failed: ", errStr);
		Logger::error("Please shut down ", name, " manually");
	} else {
		Logger::debug("Tun interface shutted down");
	}

	close(fd);
}


bool TunUnix::dataPending() {
	fd_set set;
	FD_ZERO(&set);
	FD_SET(fd, &set);
	struct timeval time = {0, 0};

	if (select(fd+1, &set, nullptr, nullptr, &time) < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("select failed: ", errStr);
	}

	return FD_ISSET(fd, &set);
}

Data TunUnix::getData() {
	constexpr size_t bufferSize = 65535 + 40; //Max length of an IP packet
	uint8_t buffer[bufferSize];

	int n = read(fd, buffer, bufferSize);
	if (n < 0) {
		Logger::error("Reading from TUN returns ", n);
		throw Error(Error::Err::Temp);
	} else {
		Logger::debug(n, " bytes read from TUN");
	}

	return Data::fromTunData(buffer, n);
}

void TunUnix::sendData(const Data &data) {
	if (data.getIpDataLen() == 0) {
		Logger::debug("Dropping empty data");
		return;
	}

	int n = write(fd, data.getIpData(), data.getIpDataLen());
	if (n < 0) {
		const char *errStr = std::strerror(errno);
		Logger::error("Writing to tun failed: ", errStr);
		throw Error(Error::Err::Temp);
	}

	Logger::debug(n, " bytes written to TUN");
}

#endif //__unix