import time
import pyaudio
import numpy as np
import asyncio
import websockets
import wave
import logging
import os
from dotenv import load_dotenv
from pydub import AudioSegment
from io import BytesIO
from faster_whisper import WhisperModel
from elevenlabs.client import ElevenLabs, AsyncElevenLabs
from elevenlabs import stream, Voice, VoiceSettings, play
import av
import librosa
import io

# Define Ports
TRANSCRIBE_PORT = 8888
STREAM_PORT = 7777

# Load environment variables from .env file
load_dotenv()

ELEVENLABS_API_KEY = os.environ.get("ELEVENLABS_API_KEY")
print("eleven: " + ELEVENLABS_API_KEY)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

CHUNK_SIZE = 1024
INIT = 0
FILE = "test.wav"
agents = []  # where all agents encountered so far are stored

model = WhisperModel("small", compute_type="auto", device="cpu")
audio = pyaudio.PyAudio()
stream = audio.open(format=pyaudio.paInt16, channels=1, rate=16000, output=True, frames_per_buffer=CHUNK_SIZE)

client = ElevenLabs(
  api_key=ELEVENLABS_API_KEY
)

mictask = None


voice_id  = "K5W90fMZclFpp7zIpkCc"
voice_id = "QjTUCwbG3OsMT4XPxAZi" # plantony :D

async def save_and_transcribe(audio_data):
    timestamp = time.strftime("%Y-%m-%dT%H-%M-%S", time.gmtime())
    logging.info(f"Processing audio data at {timestamp}")

    audio = AudioSegment.from_raw(BytesIO(audio_data), sample_width=2, frame_rate=8000, channels=2)
    mono_audio = audio.set_channels(1)
    resampled_audio = mono_audio.set_frame_rate(8000)

    # Export to in-memory bytes buffer
    buffer = BytesIO()
    resampled_audio.export(buffer, format="wav")
    buffer.seek(0)

    # Pass in-memory buffer to whisper model
    segments, info = model.transcribe(buffer, beam_size=5, language="en")
    # logging.info(f"Detected language '{info.language}' with probability {info.language_probability}")

    for segment in segments:
        logging.info("[%.2fs -> %.2fs] %s" % (segment.start, segment.end, segment.text))

def register_esp(esp, ws):
    logging.info(f"Registering agent id: {esp} with socket: {ws}")
    agents.append({"id": esp, "ws": ws})

def unregister_esp(esp):
    logging.info(f"Unregistering ESP {esp}")
    for i, a in enumerate(agents):
        if a["id"] == esp:
            agents.pop(i)
            break

# def read_wav(wf):
#     CHUNK = 1024
#     data = wf.readframes(CHUNK)
#     while data:
#         yield data
#         data = wf.readframes(CHUNK)

# async def server_audio(ws, path):
#     wf = wave.open(FILE, 'rb')
#     generator = read_wav(wf)
#     try:
#         for data in generator:
#             await ws.send(data)
#     finally:
#         wf.close()

async def transcribe_audio(websocket, path):
    global INIT
    global mictask
    
    esp_id = await websocket.recv()
    logging.info(f"Welcome to {esp_id}")
    register_esp(esp_id, websocket)

    if not INIT:
        logging.info("INIT is unset, starting switch modes task.")
        asyncio.create_task(switch_modes())
        INIT = 1

    audio_data = bytearray()
    try:
        async for msg in websocket:
            audio_data.extend(msg)
            if len(audio_data) >= 32000 * 2 * 2:
                current_audio_data = audio_data.copy()
                audio_data = bytearray()
                mictask = asyncio.create_task(save_and_transcribe(current_audio_data))
    finally:
        stream.stop_stream()
        stream.close()
        audio.terminate()
        unregister_esp(esp_id)



    

def convert_mp3_chunk(chunk):
    print("calling conversion")
    sound = AudioSegment.from_mp3(chunk) 
    wav_form = sound.export(format="wav")  
    print("returning... ", wav_form) 
    return wav_form

def convert_mp3_chunk_rosa(chunk):
    audio = librosa.load(io.BytesIO(chunk), sr=44100, mono=True)
    # chunk_8khz = librosa.resample(audio, 8000)
    # chunk_ulaw = audioop.lin2ulaw(chunk_8khz, audio.sample_width)
    return audio


global wav_buffer
wav_buffer = b''

def convert_chunk(chunk):
    
        # Add the chunk to the buffer
        global wav_buffer
        wav_buffer += chunk

        # Try to decode the buffer as WAV
        try:
            audio = AudioSegment.from_mp3(io.BytesIO(wav_buffer))
            wav_data = audio.export(format='wav').read() # Convert to WAV
        except Exception:
            logging.info("Cannot decode mp3 into wav")
            return None

        logging.info("decoding was successful")
        # If decoding was successful, empty the buffer
        wav_buffer = b''

        # # Ensure audio is mono and 16-bit
        # if audio.channels != 1 or audio.sample_width != 2:
        #     audio = audio.set_channels(1).set_sample_width(2)

        # # Sample rate conversion
        # chunk_8khz, self.state = audioop.ratecv(wav_data, 2, 1, audio.frame_rate, 8000, self.state)

        # # μ-law conversion
        # chunk_ulaw = audioop.lin2ulaw(chunk_8khz, 2)

        # return chunk_ulaw
    
    
# using a global variable PLAYBACK for playback - not sure if that's the most elegant way to handle this!
global PLAYBACK
PLAYBACK = None

async def send_stream_to_websocket(websocket, path):
    global PLAYBACK
    global mictask
    
    # if(mictask): 
    #     logging.info("!@#!@#!@#!@#@!#!@#!@#!@#!@#!@#!@#!@#@!#!@#!@#!@#!@#! canceling the task:  ", mictask)
    #     mictask.cancel()
    #     logging.info("!@#!@#!@#!@#@!#!@#!@#!@#!@#!@#!________________________ - new the task:  ", mictask)

    
    print("new agent connected")
    
    if PLAYBACK == None:
        PLAYBACK = "Hello my name is plantoid i am a blockchain based life form"
    
    # Generate the audio stream
    audio_stream = client.generate(
        text=PLAYBACK,
        model="eleven_turbo_v2",
        voice=voice_id,
        stream=True,
        output_format="pcm_16000"
    )
    # format Must be one of the following: typing.Literal['mp3_22050_32', 'mp3_44100_32', 'mp3_44100_64', 'mp3_44100_96', 'mp3_44100_128', 'mp3_44100_192', 'pcm_16000', 'pcm_22050', 'pcm_24000', 'pcm_44100', 'ulaw_8000']"}}
    

    # Send the audio stream data over the websocket, this is an MP3
    try:
        for chunk in audio_stream:
            if chunk:
                chunk_wav = chunk
                # chunk_wav = convert_mp3_chunk_rosa(chunk)
                if(chunk_wav):
                    await websocket.send(chunk_wav)
                    logging.info(f"Sent audio stream chunk of size: {len(chunk_wav)} bytes")
                    
            # else: 
            #     websocket.close()
                    
    except Exception as e:
        logging.error(f"An error occurred while sending audio stream: {e}")

    # Send the audio by converting the MP3 into WAV

    # out_file = io.BytesIO()
    # out_container = av.open(out_file, 'w', format='wav')
    # out_stream = out_container.add_stream(codec_name='pcm_s16le', rate=44100)

    # for i, frame in enumerate(container.decode(audio_stream)):
    #     for packet in out_stream.encode(frame):
    #         out_container.mux(packet)

    #     if i > 500:
    #         break
    # # flush
    # for packet in out_stream.encode(None):
    #     out_container.mux(packet)
    # out_container.close()
    # print(len(out_file.getvalue()))


async def switch_modes():
    global FILE
    global PLAYBACK
    logging.info("Switch mode activated")

    tasks = [
        
        {"esp": "88", "mode": 3, "arg": "Hello I am alive !!"},
        
        # {"esp": "95", "mode": 3, "arg": "Hello I am alive !!"},
        # {"esp": "95", "mode": 3, "arg": "Hello I am alive !!"},
        # {"esp": "95", "mode": 3, "arg": "Hello I am alive !!"},
        # {"esp": "95", "mode": 3, "arg": "Hello I am alive !!"},
        # {"esp": "95", "mode": 3, "arg": "Hello I am alive !!"},
        # {"esp": "95", "mode": 3, "arg": "Hello I am alive !!"},

        # {"esp": "98", "mode": 1, "arg": None},
        # {"esp": "99", "mode": 3, "arg": "Would you like to feed me some crypto?"},
        
        # {"esp": "95", "mode": 3, "arg": "I'm am a Plantoid and i'm happy and i'm speaking to you, how ar you?"},
        # {"esp": "98", "mode": 1, "arg": None},
        
        # {"esp": "94", "mode": 3, "arg": "Would you like to feed me some crypto?"},
        # {"esp": "98", "mode": 1, "arg": None},
        
        # {"esp": "99", "mode": 3, "arg": "I'm hungry hungry hungry -- can you feed me some pizza?"},
        # {"esp": "98", "mode": 1, "arg": None},


        # {"esp": "98", "mode": 3, "arg": "I am hungry for crypto, please feed me feed me feed me NOW !"},
    ]

    await asyncio.sleep(3) #NOTE: mock

    while True:
        await asyncio.sleep(3)
        
        for task in tasks:
            logging.info(f"Processing task: esp={task['esp']} mode={task['mode']}")
            ws = None
            for a in agents:
                if a["id"] == task["esp"]:
                    ws = a["ws"]
                    logging.info(f"Found a match for {task['esp']} with socket: {ws}")
                    break

            if ws is None:
                continue
            
                for a in agents:
                    if a["ws"]:
                        ws = a["ws"]
                        logging.info(f"Fallback to ESP {a['id']}")
                        break

            MODE = task["mode"]
            PLAYBACK = task["arg"]
            print("sending MODE to ESP")
            await ws.send(str(MODE))
            await asyncio.sleep(5) #NOTE: mock

def main():
    logging.info("Starting server")
    loop = asyncio.get_event_loop()
    loop.run_until_complete(websockets.serve(transcribe_audio, '', TRANSCRIBE_PORT, ping_interval=None))
    loop.run_until_complete(websockets.serve(send_stream_to_websocket, '', STREAM_PORT, ping_interval=None))
    loop.run_forever()

if __name__ == '__main__':
    main()