import time
import pyaudio
import numpy as np
import asyncio
import websockets
import wave

INIT = 0
FILE = "test.wav"

agents = []; # where all agents encountered so far are stored

def register_esp(esp, ws):  # registration function for associating socket to esp_id for the relevant agent
        print("registering agent id: " , esp , " with socket = ", ws)
        agents.append( { "id": esp, "ws": ws })

def unregister_esp(esp):
        for a in agents:
                if a["id"] == esp: agents.pop(a)


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



async def server_audio(ws, path):

        wf = wave.open(FILE, 'rb')

        generator = record_wav(wf);

        try:
                for data in generator:
                        await ws.send(data)
        finally:
                wf.close()
                
                

async def websocket_handler(websocket, path):

        # initialisation stuff
        
        global INIT

        esp_id = await websocket.recv();
        print("Welcome to " , esp_id)   
        register_esp(esp_id, websocket);

        # await websocket.send(str(MODE)) ### this is communicating the current modality to the client

        if(not INIT):
                print("INIT IS UNSET....")
                asyncio.create_task(switch_modes()) ### this is just a mimic to simulate the modality switching
                INIT = 1


        # actual code of the server listening to the socket from the ESP, and managing it (right now just playing it for fun)

        try:
                async for message in websocket:
                        audio_data = np.frombuffer(message, dtype=np.int16)
                        stream.write(audio_data.tobytes())

        finally:
                stream.stop_stream()
                stream.close()
                audio.terminate()
                unregister_esp(esp_id)




async def start_websocket_server():
        async with websockets.serve(websocket_handler, '', 8888, ping_interval=None):
                await asyncio.Future()  # Keep the server running


async def start_websocket_audio():
        async with websockets.serve(send_audio, '', 7777, ping_interval=None):
                await asyncio.Future()  # Keep the server running


async def switch_modes():

        global FILE
        
        print("switch mode activated")
        
        ## dummy list of tasks to do to the various ESP
        tasks = [ 
                { "esp": "3", "mode": 3, "arg": "test.wav" },
                { "esp": "4", "mode": 3, "arg": "two.wav" },
                { "esp": "3", "mode": 1, "arg": None },
                { "esp": "3", "mode": 3, "arg": "three.wav" },
                { "esp": "3", "mode": 1, "arg": None },

                # { "esp": 4, "mode": 3, "arg": "esp2.wav" },
                # { "esp": 4, "mode": 1 },
                #  { "esp": 5, "mode": 3, "file": "fail.wav" },  # test fallback if an ESP was never encountered
                #  { "esp": 3, "mode": 3, "file": "fail.wav" },  # test fallback if an ESP has lost connection
                ]


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
                        else: print("didnt found a match !")
                
                if ws == None: ## find the next available agents that is connected and reassign
                        for a in agents:
                                if(a["ws"]): ws = a["ws"]
                                print("fallbacking to ESP ", a["id"])
                        

                MODE = task["mode"]
                FILE = task["arg"]
                
                await ws.send(str(MODE))

                await asyncio.sleep(7) # wait 10 seconds before changing modalities







if __name__ == '__main__':
        #loop = asyncio.new_event_loop()
        loop = asyncio.get_event_loop()
        loop.run_until_complete(websockets.serve(websocket_handler, '', 8888, ping_interval=None))
        loop.run_until_complete(websockets.serve(server_audio, '', 7777, ping_interval=None))
        loop.run_forever()




