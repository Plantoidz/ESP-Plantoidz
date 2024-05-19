run 2 servers:
- server-audio2.py (which bumps test.wav into the websocket)
- server-mic2.py (which plays whatever the client sends to it)

2 clients:
- miniclient.py (which connects to server-audio2, and plays the wav file)
- client2.py (which records the mic stream and send it to server-mic2)

