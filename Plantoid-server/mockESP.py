import pyaudio
import websockets
import time
import asyncio
import websocket

p = pyaudio.PyAudio()

stream = p.open(output=True, format = pyaudio.paInt16, channels=1, rate = 16000)

async def on_open(ws):
        print(f"Connected to WebSocket server at ws://localhost:8888")
        await ws.send("88")
    

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

    uri = f'ws://localhost:8888'
    ws_app = websocket.WebSocketApp(uri,
                                        on_message=on_message)
    
    ws_app.on_open = on_open

    ws_app.run_forever()
    

main()


