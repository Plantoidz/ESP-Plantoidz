import pyaudio
import asyncio
import websockets
import threading

MODE = 0 ### 0 is idle, 1 is LISTEN, 2 is SPEAKING


p = pyaudio.PyAudio()

stream = p.open(output=True, format = pyaudio.paInt16, channels=1, rate = 16000)


async def record_microphone(stream):
    CHUNK = 1024
    while True:
        data =  stream.read(CHUNK)
        print("running..")
        yield data



async def on_message(ws):
    global MODE
    
    while True:
        print("waiting for message")
        message = await ws.recv()
        
        if( type(message) == str):  ## we received instructions
            print("got --> ", message)
            MODE = int(message)
            
        elif( type(message) == bytes):  ## we received data stream
            if(MODE == 2):  ## check that we are in SPEAK mode
                stream.write(message)



def init_mic():
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
    
    return stream

async def websocket_client_handler():
    async with websockets.connect('ws://localhost:8888') as ws:
        
        await ws.send("3") ### this is communicating the ESP_ID to the server
        MODE = int(await ws.recv()) #### this is receiving the current modality from the server
        
        print("Initial MODE = ", MODE)
        
        asyncio.create_task(on_message(ws))
        
        stream = init_mic()
        
        if(MODE == 1):  ### LISTEN

            async for data in record_microphone(stream):
                await ws.send(data)
            
            # event = threading.Event()
            # t = threading.Thread(target=record_microphone, args=(ws, event,))
            # t.start()

asyncio.get_event_loop().run_until_complete(websocket_client_handler())
#asyncio.run(send_audio())


