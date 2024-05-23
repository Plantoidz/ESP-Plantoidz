import time
import pyaudio
import numpy as np
import asyncio
import websockets
import wave


agents = []; # where all agents encountered so far are stored

def register_esp(esp, ws):  # registration function for associating socket to esp_id for the relevant agent
    print("registering agent id: " + esp + " with socket = ", ws)
    agents.append( { "id": esp, "ws": ws})


## global variables
INIT = 0
MODE = 0
FILE = None



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

    global INIT
    
    esp_id = await websocket.recv();
    print("Welcome to " + esp_id)   
    register_esp(esp_id, websocket);

    await websocket.send(str(MODE)) ### this is communicating the current modality to the client
    
    if(not INIT):
        asyncio.create_task(switch_modes()) ### this is just a mimic to simulate the modality switching
    else: INIT = 1



    while True:
                
        match MODE:

            case 1:  ###LISTEN
                print("entering listen mode")
                await listen(websocket)
        
            case 2: ### SPEAK
                print("entering speaking mode")
                await speak(websocket)
                
        
        await asyncio.sleep(1) # keep alive the socket
                    
                
async def listen(websocket):
    
    # await websocket.send("1")
    
    print("Starting to listen on socket ....")
    
    while True:
        try:
                async for message in websocket:
                        if(type(message) == bytes): ## we received audio stream !
                        
                            audio_data = np.frombuffer(message, dtype=np.int16)
                            stream.write(audio_data.tobytes())
                            
                        elif( type(message) == str):  ## we received an instruction !
                            print("we received instructions: " + message)
                            
                            # stream.stop_stream()
                            # stream.close()
                            # audio.terminate()
                    
                            MODE = int(message)
                            return
                        
        except:
                    stream.stop_stream()
                    stream.close()
                    audio.terminate()



async def speak(websocket):
    
        global MODE
        
        await websocket.recv()
        
        wf = wave.open(FILE, 'rb')
        generator = record_wav(wf)

        print("sending data: ", FILE)

        try:
                for data in generator:
                        
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


async def switch_modes(websocket=0):
    
        global MODE
        global FILE
        
        print("switch mode activated")
        
        ## dummy list of tasks to do to the various ESP
        tasks = [ 
                { "esp": "3", "mode": 2, "arg": "test.wav" },
                { "esp": "3", "mode": 2, "arg": "test.wav" },
                # { "esp": "3", "mode": 1, "arg": None },
                # { "esp": "3", "mode": 2, "arg": "test.wav" },
                # { "esp": 4, "mode": 2, "arg": "esp2.wav" },
                # { "esp": 4, "mode": 1 },
                #  { "esp": 5, "mode": 2, "file": "fail.wav" },  # test fallback if an ESP was never encountered
                #  { "esp": 3, "mode": 2, "file": "fail.wav" },  # test fallback if an ESP has lost connection
                ]

        # while True:
        ws = None
        
        for task in tasks:
            
            print("PROCESSING TASK: esp="+task["esp"]+ " mode=", task["mode"])

            # find the socket for the specific esp_id
            for a in agents:
                print(a)
                if(a["id"] == task["esp"]): 
                    ws = a["ws"]
                print ("found a match for ", task["esp"], " with socket = ", ws)
                break
            
            
            MODE = task["mode"]
            FILE = task["arg"]
            # if (m == 1):    await listen(ws)
            # elif (m == 2):  await speak(ws, task["arg"])
            
            await ws.send(str(MODE))

            await asyncio.sleep(10)
                
                
        
                
                
                
                # if(MODE == 1): MODE = 2 
                # else: MODE = 1  ## switch from LISTEN to SPEAK mode
                # print("switching to MODE: ", MODE)
                # await websocket.send(str(MODE))




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

    # asyncio.run(start_websocket_server())
    # asyncio.create_task(switch_modes()) ### this is just a mimic to simulate the modality switching from the orchestrator


    loop = asyncio.get_event_loop()
    loop.run_until_complete(websockets.serve(websocket_handler, '', 8888, ping_interval=None))
    # loop.run_until_complete(switch_modes()) ### this is just a mimic to simulate the modality switching from the orchestrator
    loop.run_forever()
