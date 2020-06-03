/*************************************************************************/
/*  stream_peer_unix.h                                                   */
/*  Copyright (c) 2020 Leroy Hopson					 */
/*************************************************************************/
/*        Derived from stream_peer_tcp.h, a file that is part of:        */
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

#ifndef STREAM_PEER_UNIX_H
#define STREAM_PEER_UNIX_H

#include "core/io/net_socket.h"
#include "core/io/stream_peer.h"
#include <sys/socket.h>
#include <sys/un.h>

class StreamPeerUnix : public StreamPeer {

	GDCLASS(StreamPeerUnix, StreamPeer);
	OBJ_CATEGORY("Networking");

public:
	enum Status {

		STATUS_NONE,
		STATUS_CONNECTED,
		STATUS_ERROR,
	};

protected:
	int sock, msgsock, rval;
	Status status;
	struct sockaddr_un server;
	char pathname[108];

	Error _open(const String &pname);
	Error _poll(NetSocket::PollType p_type, int p_timeout) const;
	Error _poll_connection();
	Error write(const uint8_t *p_data, int p_bytes, int &r_sent, bool p_block);
	Error read(uint8_t *p_buffer, int p_bytes, int &r_received, bool p_block);

	static void _bind_methods();

public:
	Error open(char pname[108]);
	bool is_connected_to_path() const;
	String get_connected_path() const;
	void close();

	int get_available_bytes() const;
	Status get_status();

	// Read/Write from StreamPeer
	Error put_data(const uint8_t *p_data, int p_bytes);
	Error put_partial_data(const uint8_t *p_data, int p_bytes, int &r_sent);
	Error get_data(uint8_t *p_buffer, int p_bytes);
	Error get_partial_data(uint8_t *p_buffer, int p_bytes, int &r_received);

	StreamPeerUnix();
	~StreamPeerUnix();
};

VARIANT_ENUM_CAST(StreamPeerUnix::Status);

#endif
