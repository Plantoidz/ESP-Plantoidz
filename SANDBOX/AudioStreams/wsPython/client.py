import pyaudio
import asyncio
import websockets

def record_microphone(stream):
    CHUNK = 1024
    while True:
        data = stream.read(CHUNK)
        yield data

def on_message(ws, msg):
	print(message)

async def send_audio():
    async with websockets.connect('ws://localhost:8888') as ws:
        ws.on_message = on_message

        p = pyaudio.PyAudio()
        # obtain the index of available mic
        mic_device_index = None
        for i in range(p.get_device_count()):
            device_info = p.get_device_info_by_index(i)
            if device_info['maxInputChannels'] > 0:
                mic_device_index = i
                break

        if mic_device_index is None:
            print("there is no mic")
            return

        stream = p.open(format=pyaudio.paInt16,
                        channels=1,
                        rate=16000,
                        input=True,
                        frames_per_buffer=1024,
                        input_device_index=mic_device_index)

        for data in record_microphone(stream):
            await ws.send(data)

        async for message in ws:
            print("received", message)

asyncio.get_event_loop().run_until_complete(send_audio())
