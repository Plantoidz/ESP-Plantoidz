import time
import pyaudio
import numpy as np
import asyncio
import websockets
import wave
# import ffmpeg
# import traceback
import logging
from pydub import AudioSegment
from io import BytesIO

# import whisper
from faster_whisper import WhisperModel

# model = whisper.load_model('base.en')
# model = WhisperModel("large-v2",  compute_type="auto")
model = WhisperModel("small",  compute_type="auto", device="cpu")


async def save_and_transcribe(audio_data):
        timestamp = time.strftime("%Y-%m-%dT%H-%M-%S", time.gmtime())
        original_file_name = f"tmp/original_{timestamp}.wav"
        with wave.open(original_file_name, "wb") as wav_file:
                wav_file.setnchannels(2)
                wav_file.setsampwidth(2)
                wav_file.setframerate(16000)
                wav_file.writeframes(audio_data)
        logging.info(f"Original audio file saved as {original_file_name}")

        audio = AudioSegment.from_raw(BytesIO(audio_data), sample_width=2, frame_rate=16000, channels=2)
        mono_audio = audio.set_channels(1)
        resampled_audio = mono_audio.set_frame_rate(16000)
        resampled_file_name = f"tmp/resampled_{timestamp}.wav"
        resampled_audio.export(resampled_file_name, format="wav")
        logging.info(f"Resampled audio file saved as {resampled_file_name}")

        segments, info = model.transcribe(resampled_file_name, beam_size=5)
        # segments = model.transcribe(resampled_file_name, beam_size=5)

        print("Detected language '%s' with probability %f" % (info.language, info.language_probability))

        for segment in segments:
                print("[%.2fs -> %.2fs] %s" % (segment.start, segment.end, segment.text))
                # print(segment)
        

# def process_wav_bytes(webm_bytes: bytes, sample_rate: int = 16000):
#         wf = wave.open("output.wav", 'wb')
#         wf.setnchannels(1)
#         p = pyaudio.PyAudio()
#         wf.setsampwidth(p.get_sample_size(pyaudio.paInt16))
#         wf.setframerate(16000)
#         wf.writeframes(webm_bytes)
#         wf.close()
#         waveform = whisper.audio.load_audio("output.wav", sr=sample_rate)
        
#         return waveform
        
# async def transcribe_socket(ws):
#         while not ws.closed:
#                 message = await ws.recv()
#                 if message:
#                         print('message received', len(message), type(message))
#                 try:
#                         if isinstance(message, str):
#                                 message = base64.b64decode(message)
                        
#                         audio = process_wav_bytes(bytes(message)).reshape(1, -1)
#                         # audio = process_wav_bytes(audio)
#                         # audio = load_audio(audio, 16000)
                        
#                         audio = whisper.pad_or_trim(audio)
#                         transcription = whisper.transcribe(
#                         model,
#                         audio
#                         )
#                         print("************************************************************** " , transcription)
#                 except Exception as e:
#                         traceback.print_exc()


INIT = 0
FILE = "test.wav"

agents = []; # where all agents encountered so far are stored

def register_esp(esp, ws):  # registration function for associating socket to esp_id for the relevant agent
        print("registering agent id: " , esp , " with socket = ", ws)
        agents.append( { "id": esp, "ws": ws })

def unregister_esp(esp):
        print("UN-REGISTERING ESP " + esp)
        i = 0
        for a in agents:
                if a["id"] == esp: agents.pop(i)
                i = i+1


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

        audio_data = bytearray()

        try:
                # async for message in websocket:
                #         audio_data = np.frombuffer(message, dtype=np.int16)
                        # stream.write(audio_data.tobytes())
                        # await transcribe_socket(websocket)
                        
                async for msg in websocket:
        
                        audio_data.extend(msg)
        
                        if len(audio_data) >= 32000 * 2 * 2:
                                current_audio_data = audio_data.copy()
                                audio_data = bytearray()
                
                                asyncio.create_task(save_and_transcribe(current_audio_data))


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
                # { "esp": "3", "mode": 3, "arg": "test.wav" },
                # { "esp": "4", "mode": 3, "arg": "two.wav" },
                { "esp": "3", "mode": 1, "arg": None },
                # { "esp": "3", "mode": 3, "arg": "three.wav" },
                # { "esp": "4", "mode": 1, "arg": None },

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




