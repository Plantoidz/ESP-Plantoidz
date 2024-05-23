import time
import pyaudio
import numpy as np
import asyncio
import websockets

CHUNK_SIZE = 1024

audio = pyaudio.PyAudio()

stream = audio.open(format=pyaudio.paInt16,
                    channels=1,
                    rate=16000,
                    output=True,
                    frames_per_buffer=CHUNK_SIZE)




async def websocket_handler(websocket, path):

    asyncio.create_task(send(websocket))

    try:
        async for message in websocket:
#            print(message)
            audio_data = np.frombuffer(message, dtype=np.int16)
            stream.write(audio_data.tobytes())

    finally:
        stream.stop_stream()
        stream.close()
        audio.terminate()



async def start_websocket_server():
    async with websockets.serve(websocket_handler, '', 8888, ping_interval=None):
        await asyncio.Future()  # Keep the server running


async def send(websocket):
	while True:
		await asyncio.sleep(10)
		await websocket.send("msg")

if __name__ == '__main__':
    asyncio.run(start_websocket_server())
