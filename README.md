# StreamPeerUnix

This module implements a very basic UNIX domain socket StreamPeer for the Godot game engine.
Similar to [StreamPeerTCP](https://docs.godotengine.org/en/stable/classes/class_streampeertcp.html).
Only tested on and developed for Linux with Godot 3.2.1.


## Installation
1. Clone the source code of [godot-engine](https://github.com/godotengine/godot).
2. Clone this module and put it into `godot/modules/`. Ensure that the directory name of this module is `stream_peer_unix`.
3. [Recopmile the godot engine](https://docs.godotengine.org/en/stable/development/compiling/).


## Usage

```gdscript
extends Node


var stream_peer_unix: StreamPeerUnix


func _ready():
	stream_peer_unix = StreamPeerUnix.new()

	var err = stream_peer_unix.open("/home/leroy/tmp/socket")
	if err != OK:
		print("stream_peer_unix not connected!")
	else:
		print("stream_peer_unix connected!")
		stream_peer_unix.put_data("Hello from Godot!".to_ascii())


func _process(delta):
	if stream_peer_unix.get_status() == StreamPeerUnix.STATUS_CONNECTED:
		var available_bytes = stream_peer_unix.get_available_bytes()

		if available_bytes > 0:
			var data = stream_peer_unix.get_data(available_bytes)
			print("Data from socket: ", data[1].get_string_from_utf8())


func _exit_tree():
	stream_peer_unix.close()

```
