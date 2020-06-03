/*************************************************************************/
/*  stream_peer_unix.cpp                                                 */
/*  Copyright (c) 2020 Leroy Hopson					 */
/*************************************************************************/
/*      Derived from stream_peer_tcp.cpp, and net_socket_posix.cpp,      */
/*                      files that are part of:                          */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "stream_peer_unix.h"

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

Error StreamPeerUnix::_poll(NetSocket::PollType p_type, int p_timeout) const {

	struct pollfd pfd;
	pfd.fd = sock;
	pfd.events = POLLIN;
	pfd.revents = 0;

	switch (p_type) {
		case NetSocket::POLL_TYPE_IN:
			pfd.events = POLLIN;
			break;
		case NetSocket::POLL_TYPE_OUT:
			pfd.events = POLLOUT;
			break;
		case NetSocket::POLL_TYPE_IN_OUT:
			pfd.events = POLLOUT | POLLIN;
	}

	int ret = poll(&pfd, 1, p_timeout);

	if (ret < 0 || pfd.revents & POLLERR) {
		print_verbose("Error when polling socket.");
		return FAILED;
	}

	if (ret == 0)
		return ERR_BUSY;

	return OK;
}

Error StreamPeerUnix::_poll_connection() {

	Error err = open(pathname);

	if (err == OK) {
		status = STATUS_CONNECTED;
		return OK;
	}

	close();
	status = STATUS_ERROR;
	return ERR_CONNECTION_ERROR;
}

Error StreamPeerUnix::open(char pname[108]) {

	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, pname);

	if (::connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0) {
		ERR_PRINT("Connection to stream socket failed!");
		close();
		return FAILED;
	}

	status = STATUS_CONNECTED;

	strcpy(pathname, pname);

	return OK;
}

Error StreamPeerUnix::write(const uint8_t *p_data, int p_bytes, int &r_sent, bool p_block) {

	if (status == STATUS_NONE || status == STATUS_ERROR) {

		return FAILED;
	}

	if (status != STATUS_CONNECTED) {

		if (_poll_connection() != OK) {

			return FAILED;
		}

		if (status != STATUS_CONNECTED) {
			r_sent = 0;
			return OK;
		}
	}

	int data_to_send = p_bytes;
	const uint8_t *offset = p_data;
	
	if (::write(sock, offset, data_to_send) < 0) {
		close();
		status = STATUS_ERROR;
		return FAILED;
	}

	r_sent = data_to_send;

	return OK;
}

Error StreamPeerUnix::read(uint8_t *p_buffer, int p_bytes, int &r_received, bool p_block) {

	if (!is_connected_to_path()) {

		return FAILED;
	}

	int to_read = p_bytes;
	r_received = 0;

	int read = recv(sock, p_buffer, to_read, 0);

	if (read < 0) {

		close();
		return FAILED;

	} else if (read == 0) {

		close();
		r_received = read;
		return ERR_FILE_EOF;

	} else {

		r_received = read;
		return OK;
	}
}

bool StreamPeerUnix::is_connected_to_path() const {

	return status == STATUS_CONNECTED;
}

StreamPeerUnix::Status StreamPeerUnix::get_status() {

	if (status == STATUS_CONNECTED) {
		Error err;
		err = _poll(NetSocket::POLL_TYPE_IN, 0);
		if (err == OK) {
			// FIN received
			if (get_available_bytes() == 0) {
				close();
				return status;
			}
		}
		// Also poll write
		err = _poll(NetSocket::POLL_TYPE_IN_OUT, 0);
		if (err != OK && err != ERR_BUSY) {
			// Got an error
			close();
			status = STATUS_ERROR;
		}
	}

	return status;
}

void StreamPeerUnix::close() {

	::close(sock);

	status = STATUS_NONE;
	strcpy(pathname, "");
}

Error StreamPeerUnix::put_data(const uint8_t *p_data, int p_bytes) {

	int total;
	return write(p_data, p_bytes, total, true);
}

Error StreamPeerUnix::put_partial_data(const uint8_t *p_data, int p_bytes, int &r_sent) {

	return write(p_data, p_bytes, r_sent, false);
}

Error StreamPeerUnix::get_data(uint8_t *p_buffer, int p_bytes) {

	int total;
	return read(p_buffer, p_bytes, total, true);
}

Error StreamPeerUnix::get_partial_data(uint8_t *p_buffer, int p_bytes, int &r_received) {

	return read(p_buffer, p_bytes, r_received, false);
}

int StreamPeerUnix::get_available_bytes() const {
	
	unsigned long len;
	int ret = ioctl(sock, FIONREAD, &len);
	if (ret == 1) {
		print_verbose("Error when checking available bytes on socket.");
		return -1;
	}
	return len;
}

String StreamPeerUnix::get_connected_path() const {

	return pathname;
}

Error StreamPeerUnix::_open(const String &pname) {

	char pathname[108];
	strcpy(pathname, pname.ascii());

	struct stat buffer;
	if (stat(pathname, &buffer) != 0)
		return ERR_FILE_NOT_FOUND;

	return open(pathname);
}

void StreamPeerUnix::_bind_methods() {

	ClassDB::bind_method(D_METHOD("open", "pathname"), &StreamPeerUnix::_open);
	ClassDB::bind_method(D_METHOD("is_connected_to_path"), &StreamPeerUnix::is_connected_to_path);
	ClassDB::bind_method(D_METHOD("get_status"), &StreamPeerUnix::get_status);
	ClassDB::bind_method(D_METHOD("get_connected_path"), &StreamPeerUnix::get_connected_path);
	ClassDB::bind_method(D_METHOD("close"), &StreamPeerUnix::close);

	BIND_ENUM_CONSTANT(STATUS_NONE);
	BIND_ENUM_CONSTANT(STATUS_CONNECTED);
	BIND_ENUM_CONSTANT(STATUS_ERROR);
}

StreamPeerUnix::StreamPeerUnix() :
		sock(socket(AF_UNIX, SOCK_STREAM, 0)),
		status(STATUS_NONE),
		pathname("") {
}

StreamPeerUnix::~StreamPeerUnix() {
	close();
}
