#include <tox/tox.h>

void tox_iterate(Tox *tox) {}

void tox_callback_friend_lossy_packet(Tox *tox,
		tox_friend_lossy_packet_cb *callback,
		void *user_data
) {}

void tox_callback_friend_lossless_packet(Tox *tox,
		tox_friend_lossy_packet_cb *callback,
		void *user_data
) {}

uint16_t tox_self_get_udp_port(const Tox *tox, TOX_ERR_GET_PORT *error) {
	return 1234;
}
