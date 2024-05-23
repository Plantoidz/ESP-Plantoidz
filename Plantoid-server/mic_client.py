import pyaudio
import asyncio
import websockets

def record_microphone(stream):
    CHUNK = 1024
    while True:
        data = stream.read(CHUNK)
        print("running..")
        yield data
        

async def on_message(ws):
	while True:
		print("waiting for message")
		message = await ws.recv()
		print("got --> ", message)
		await asyncio.sleep(1)

async def send_audio():
    async with websockets.connect('ws://localhost:8888') as ws:
        
        await ws.send("1")
        asyncio.create_task(on_message(ws))

        p = pyaudio.PyAudio()
        # obtain the index of available mic
        mic_device_index = None
        # for i in range(p.get_device_count()):
        #     device_info = p.get_device_info_by_index(i)
        #     if device_info['maxInputChannels'] > 0:
        #         mic_device_index = i
        #         break

        # if mic_device_index is None:
        #     print("there is no mic")
        #     return

        stream = p.open(format=pyaudio.paInt16,
                        channels=1,
                        rate=32000,
                        input=True,
                        frames_per_buffer=1024,
                        input_device_index=mic_device_index)

        for data in record_microphone(stream):
            await ws.send(data)
        


asyncio.get_event_loop().run_until_complete(send_audio())
#asyncio.run(send_audio())
