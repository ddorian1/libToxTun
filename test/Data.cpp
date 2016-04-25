#include <boost/test/unit_test.hpp>

#include "../src/Data.hpp"
#include "../src/ToxTun.hpp"

BOOST_AUTO_TEST_SUITE(DataTest)

BOOST_AUTO_TEST_CASE(fromTunData) {
	uint8_t buffer[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	Data data = Data::fromTunData(buffer, 10);

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
	uint8_t buffer[10] = {200, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	Data data = Data::fromToxData(buffer, 10);

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

BOOST_AUTO_TEST_SUITE_END()
