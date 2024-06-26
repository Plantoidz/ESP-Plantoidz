import time
import pyaudio
import numpy as np
import asyncio
import websockets
import wave

CHUNK_SIZE = 1024

MODE = 0 ### 0 is idle, 1 is LISTEN, 2 is SPEAKING

audio = pyaudio.PyAudio()

stream = audio.open(format=pyaudio.paInt16,
                    channels=1,
                    rate=16000,
                    output=True,
                    frames_per_buffer=CHUNK_SIZE)

def record_wav(wf):
    CHUNK = 1024
    data = 1
    while data:
        data = wf.readframes(CHUNK)
        yield data


async def websocket_handler(websocket, path):

    global MODE

    esp_id = await websocket.recv();

    print("Welcome to " + esp_id)  ### this should call the registration function for associating socket to esp_id for the relevant agent


    await websocket.send(str(MODE)) ### this is communicating the current modality to the client
    
    asyncio.create_task(switch_modes(websocket)) ### this is just a mimic to simulate the modality switching


    while True:
        
        await asyncio.sleep(1)
        
        match MODE:

            case 1:  ###LISTEN
        
                # next_msg = asyncio.create_task(websocket.__anext__())
                
                # while (MODE == 1):
                #     await asyncio.wait([next_msg], timeout=1,)
                #     if next_msg.done():
                #         try:
                #             msg = next_msg.result()
                #         except StopAsyncIteration:
                #             break;
                #         else:
                #             audio_data = np.frombuffer(msg, dtype=np.int16)
                #             stream.write(audio_data.tobytes())
        
                try:
                    async for message in websocket:
                        if(type(message) == bytes): ## we received audio stream !
                            print("we received audio stream ! ")
                        
                            audio_data = np.frombuffer(message, dtype=np.int16)
                            stream.write(audio_data.tobytes())
                            
                        elif( type(message) == str):  ## we received an instruction !
                            print("we received instructions: " + message)
                            
                            # stream.stop_stream()
                            # stream.close()
                            # audio.terminate()
                    
                            MODE = int(message)
                            break
                        
                except:
                    stream.stop_stream()
                    stream.close()
                    audio.terminate()
                    
                        
                    

            case 2: ### SPEAK

                wf = wave.open("test.wav", 'rb')
                generator = record_wav(wf)

                try:
                    for data in generator:
                        print("sending data....")
                        await websocket.send(data)
                except:
                    wf.close()
                    
                print("End of data stream, switching back to idle mode")
                MODE = 0
                



async def start_websocket_server():
    async with websockets.serve(websocket_handler, '', 8888, ping_interval=None):
        await asyncio.Future()  # Keep the server running
        
        
        


#async def start_websocket_audio():
#        async with websockets.serve(send_audio, '', 7777, ping_interval=None):
#                await asyncio.Future()  # Keep the server running


async def switch_modes(websocket):
    
        global MODE;

        while True:
            
                await asyncio.sleep(10)
                
                if(MODE == 1): MODE = 2 
                else: MODE = 1  ## switch from LISTEN to SPEAK mode
                
                print("switching to MODE: ", MODE)
                await websocket.send(str(MODE))




#async def send_audio(ws, path):
#        wf = wave.open("test.wav", 'rb')
#        generator = record_wav(wf);
#        try:
#                for data in generator:
#                        await ws.send(data)
#        finally:
#                wf.close()


#async def main():
#    server1 = await websockets.serve(websocket_handler, '', 8888, ping_interval=None)
#    server2 = await websockets.serve(send_audio, '', 7777, ping_interval=None)
#    await asyncio.gather(server1.wait_closed(), server2.wait_closed())

if __name__ == '__main__':
    #loop = asyncio.new_event_loop()
    #loop = asyncio.get_event_loop()
    #loop.run_until_complete(websockets.serve(websocket_handler, '', 8888, ping_interval=None))
    #loop.run_until_complete(websockets.serve(send_audio, '', 7777, ping_interval=None))
    #loop.run_forever()

    #asyncio.run(start_websocket_server())
    #asyncio.run(start_websocket_audio())

    #asyncio.run(main())

    asyncio.run(start_websocket_server())

