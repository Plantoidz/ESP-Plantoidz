import pyaudio
import asyncio
import websockets
import wave




def record_wav(wf):
    CHUNK = 1024
    data = 1
    while data:
        print("running..")
        data = wf.readframes(CHUNK)
        yield data





async def start_websocket_server():
        async with websockets.serve(send_audio, '', 7777, ping_interval=None):
                await asyncio.Future()  # Keep the server running



async def send_audio(ws, path):

	wf = wave.open("test.wav", 'rb')
       
	generator = record_wav(wf);	
 
	try:
		for data in generator:
            		await ws.send(data)
	finally:
		wf.close()
	


if __name__ == '__main__':
        asyncio.run(start_websocket_server())

