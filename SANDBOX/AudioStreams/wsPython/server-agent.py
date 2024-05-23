import time
import pyaudio
import numpy as np
import asyncio
import websockets
import wave

CHUNK_SIZE = 1024

audio = pyaudio.PyAudio()

stream = audio.open(format=pyaudio.paInt16,
                    channels=1,
                    rate=16000,
                    output=True,
                    frames_per_buffer=CHUNK_SIZE)
# stupid comment


def record_wav(wf):
    CHUNK = 1024
    data = 1
    while data:
        data = wf.readframes(CHUNK)
        yield data




async def websocket_handler(websocket, path):

    asyncio.create_task(send(websocket))

    try:
        async for message in websocket:
            audio_data = np.frombuffer(message, dtype=np.int16)
            stream.write(audio_data.tobytes())

    finally:
        stream.stop_stream()
        stream.close()
        audio.terminate()



async def start_websocket_server():
    async with websockets.serve(websocket_handler, '', 8888, ping_interval=None):
        await asyncio.Future()  # Keep the server running


# async def start_agent_association():
#         async with websockets.serve(associate_agent_websocket_connection, '', 7777, ping_interval=None):
#                 await asyncio.Future()  # Keep the server running


async def send(websocket):
        while True:
                await asyncio.sleep(10)
                await websocket.send("msg")



async def associate_agent_websocket_connection(ws, path, agents = None):

		data = await ws.recv()
		reply = f"Data recieved as:  {data}!"
		print(reply)

		use_esp_id = int(data)

		# wf = wave.open("test.wav", 'rb')

		# generator = record_wav(wf)

		if agents is not None:
			for agent in agents:
				if agent.esp_id == use_esp_id:
					agent.associate_ws_connection(ws)

		# try:
		# 		for data in generator:
		# 				await ws.send(data)
		# finally:
		# 		wf.close()



if __name__ == '__main__':
    #loop = asyncio.new_event_loop()
    loop = asyncio.get_event_loop()
    loop.run_until_complete(websockets.serve(websocket_handler, '', 8888, ping_interval=None))
    loop.run_until_complete(websockets.serve(associate_agent_websocket_connection, '', 7777, ping_interval=None))
    loop.run_forever()

    #asyncio.run(start_websocket_server())
    #asyncio.run(start_websocket_audio())

    #asyncio.run(main())


