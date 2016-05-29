#include <boost/test/unit_test.hpp>

#include <tox/tox.h>
#include "../src/Data.hpp"
#include "../src/ToxTun.hpp"

BOOST_AUTO_TEST_SUITE(DataTest)

BOOST_AUTO_TEST_CASE(fromTunData) {
	constexpr uint8_t buffer[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	const Data data = Data::fromTunData(buffer, 10);

	BOOST_CHECK(data.getToxHeader() == Data::PacketId::Data);
	BOOST_CHECK_EQUAL(data.getIpDataLen(), 10);
	BOOST_CHECK_EQUAL(data.getToxDataLen(), 11);
	BOOST_CHECK_THROW(data.getIpPostfix(), ToxTunError);
	BOOST_CHECK_THROW(data.getIpSubnet(), ToxTunError);
	BOOST_CHECK_EQUAL(data.isValidFragment(), false);
	BOOST_CHECK_THROW(data.getSplittedDataIndex(), ToxTunError);
	BOOST_CHECK_THROW(data.getFragmentsCount(), ToxTunError);
	BOOST_CHECK(data.getSendTox() == Data::SendTox::Lossy);
	BOOST_CHECK_EQUAL(data.getToxData()[0], static_cast<uint8_t>(Data::PacketId::Data));
	BOOST_CHECK_EQUAL_COLLECTIONS(buffer, buffer+10, data.getIpData(), data.getIpData()+10);
	BOOST_CHECK_EQUAL_COLLECTIONS(buffer, buffer+10, data.getToxData()+1, data.getToxData()+11);
}

BOOST_AUTO_TEST_CASE(fromToxData) {
	constexpr uint8_t buffer[10] = {200, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	const Data data = Data::fromToxData(buffer, 10);

	BOOST_CHECK(data.getToxHeader() == Data::PacketId::Data);
	BOOST_CHECK_EQUAL(data.getIpDataLen(), 9);
	BOOST_CHECK_EQUAL(data.getToxDataLen(), 10);
	BOOST_CHECK_THROW(data.getIpPostfix(), ToxTunError);
	BOOST_CHECK_THROW(data.getIpSubnet(), ToxTunError);
	BOOST_CHECK_EQUAL(data.isValidFragment(), false);
	BOOST_CHECK_THROW(data.getSplittedDataIndex(), ToxTunError);
	BOOST_CHECK_THROW(data.getFragmentsCount(), ToxTunError);
	BOOST_CHECK(data.getSendTox() == Data::SendTox::Lossy);
	BOOST_CHECK_EQUAL_COLLECTIONS(buffer+1, buffer+10, data.getIpData(), data.getIpData()+9);
	BOOST_CHECK_EQUAL_COLLECTIONS(buffer, buffer+10, data.getToxData(), data.getToxData()+10);
}

BOOST_AUTO_TEST_CASE(fragments) {
	uint8_t buffer[TOX_MAX_CUSTOM_PACKET_SIZE];
	for (size_t i = 0; i < TOX_MAX_CUSTOM_PACKET_SIZE; ++i) buffer[i] = i;
	const Data data = Data::fromTunData(buffer, TOX_MAX_CUSTOM_PACKET_SIZE);
	const std::forward_list<Data> fragments = data.getSplitted(1);

	for (const auto &fragment : fragments) {
		BOOST_CHECK(fragment.getToxHeader() == Data::PacketId::Fragment);
		BOOST_CHECK_THROW(fragment.getIpPostfix(), ToxTunError);
		BOOST_CHECK_THROW(fragment.getIpSubnet(), ToxTunError);
		BOOST_CHECK_EQUAL(fragment.isValidFragment(), true);
		BOOST_CHECK_EQUAL(fragment.getSplittedDataIndex(), 1);
		BOOST_CHECK_EQUAL(fragment.getFragmentsCount(), 2);
		BOOST_CHECK(fragment.getSendTox() == Data::SendTox::Lossy);
	}

	{
		auto fragment = fragments.cbegin();
		BOOST_CHECK_EQUAL(fragment->getIpDataLen(), 8);
		BOOST_CHECK_EQUAL(fragment->getToxDataLen(), 9);
		++fragment;
		BOOST_CHECK_EQUAL(fragment->getIpDataLen(), TOX_MAX_CUSTOM_PACKET_SIZE-1);
		BOOST_CHECK_EQUAL(fragment->getToxDataLen(), TOX_MAX_CUSTOM_PACKET_SIZE);
	}

	std::list<Data> fragmentsList;
	for (const auto &fragment : fragments) fragmentsList.push_front(fragment);

	const Data data2 = Data::fromFragments(fragmentsList);
	BOOST_CHECK_EQUAL_COLLECTIONS(data.getToxData(), data.getToxData()+TOX_MAX_CUSTOM_PACKET_SIZE, data2.getToxData(), data2.getToxData()+TOX_MAX_CUSTOM_PACKET_SIZE);
}

BOOST_AUTO_TEST_CASE(fromIpPostfix) {
	constexpr uint8_t subnet = 123;
	constexpr uint8_t postfix = 234;
	const Data data = Data::fromIpPostfix(subnet, postfix);

	BOOST_CHECK(data.getToxHeader() == Data::PacketId::IpProposal);
	BOOST_CHECK_EQUAL(data.getIpDataLen(), 2);
	BOOST_CHECK_EQUAL(data.getToxDataLen(), 3);
	BOOST_CHECK_EQUAL(data.getIpPostfix(), postfix);
	BOOST_CHECK_EQUAL(data.getIpSubnet(), subnet);
	BOOST_CHECK_EQUAL(data.isValidFragment(), false);
	BOOST_CHECK_THROW(data.getSplittedDataIndex(), ToxTunError);
	BOOST_CHECK_THROW(data.getFragmentsCount(), ToxTunError);
	BOOST_CHECK(data.getSendTox() == Data::SendTox::Lossless);
}

BOOST_AUTO_TEST_CASE(fromPacketId) {
	const Data data = Data::fromPacketId(Data::PacketId::ConnectionRequest);

	BOOST_CHECK(data.getToxHeader() == Data::PacketId::ConnectionRequest);
	BOOST_CHECK_EQUAL(data.getIpDataLen(), 0);
	BOOST_CHECK_EQUAL(data.getToxDataLen(), 1);
	BOOST_CHECK_THROW(data.getIpPostfix(), ToxTunError);
	BOOST_CHECK_THROW(data.getIpSubnet(), ToxTunError);
	BOOST_CHECK_EQUAL(data.isValidFragment(), false);
	BOOST_CHECK_THROW(data.getSplittedDataIndex(), ToxTunError);
	BOOST_CHECK_THROW(data.getFragmentsCount(), ToxTunError);
	BOOST_CHECK(data.getSendTox() == Data::SendTox::Lossless);
}

BOOST_AUTO_TEST_CASE(setToxHeader) {
	Data data = Data::fromPacketId(Data::PacketId::ConnectionRequest);
	data.setToxHeader(Data::PacketId::ConnectionReject);

	BOOST_CHECK(data.getToxHeader() == Data::PacketId::ConnectionReject);
	BOOST_CHECK_EQUAL(data.getIpDataLen(), 0);
	BOOST_CHECK_EQUAL(data.getToxDataLen(), 1);
	BOOST_CHECK_THROW(data.getIpPostfix(), ToxTunError);
	BOOST_CHECK_THROW(data.getIpSubnet(), ToxTunError);
	BOOST_CHECK_EQUAL(data.isValidFragment(), false);
	BOOST_CHECK_THROW(data.getSplittedDataIndex(), ToxTunError);
	BOOST_CHECK_THROW(data.getFragmentsCount(), ToxTunError);
	BOOST_CHECK(data.getSendTox() == Data::SendTox::Lossless);
}

BOOST_AUTO_TEST_SUITE_END()
