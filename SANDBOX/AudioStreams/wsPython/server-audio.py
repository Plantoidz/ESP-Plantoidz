import pyaudio
import websocket
import json
import struct
import wave
import pickle
import asyncio
import websockets

#record
CHUNK = 1024




p = pyaudio.PyAudio()

wf = wave.open("test.wav", 'rb')

stream = p.open(format= p.get_format_from_width(wf.getsampwidth()),
                channels=wf.getnchannels(),
                rate=wf.getframerate(),
                output=True)



async def start_websocket_server():
	async with websockets.serve(websocket_handler, '', 7777, ping_interval=None):
		await asyncio.Future()  # Keep the server running


async def websocket_handler(websocket, path):

	frames = stream.readframes(CHUNK)
	
	try:
		send(pickle.dumps(frames), opcode=websocket.ABNF.OPCODE_BINARY)
		print("sending...")
	
	finally:
		stream.stop_stream()
		stream.close()
		audio.terminate()
	


if __name__ == '__main__':
	asyncio.run(start_websocket_server())

