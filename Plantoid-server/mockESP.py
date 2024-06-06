import pyaudio
import websockets
import time
import asyncio

p = pyaudio.PyAudio()

stream = p.open(output=True, format = pyaudio.paInt16, channels=1, rate = 16000)

async def on_open():
    async with websockets.connect('ws://localhost:8888') as ws:
        print(f"Connected to WebSocket server at ws://localhost:8888")
        await ws.send("88")
        ws.on_message = on_message

def on_stream(ws):
	while True:
		print("receiving streaming data ..........")
		data = ws.recv()
		stream.write(data)

def on_message(ws):
        data = ws.recv()
        print("received : ", data)
        if(data == "3"):
            print("opening 777 connection")
            ws77 = websockets.connect("ws://localhost:7777")
            ws77.on_message = on_stream
            
        time.sleep(1)    

def main():
    print("connecting...")
    
    asyncio.get_event_loop().run_until_complete(on_open())

    

main()


