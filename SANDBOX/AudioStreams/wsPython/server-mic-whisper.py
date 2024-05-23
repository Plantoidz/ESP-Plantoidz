import time
import pyaudio
import numpy as np
import asyncio
import websockets
import wave
import traceback
import logging
from pydub import AudioSegment
from io import BytesIO

# import whisper
from faster_whisper import WhisperModel

# model = whisper.load_model('base.en')
# model = WhisperModel("large-v2",  compute_type="auto")
model = WhisperModel("small",  compute_type="auto")


CHUNK_SIZE = 1024

audio = pyaudio.PyAudio()

stream = audio.open(format=pyaudio.paInt16,
                    channels=1,
                    rate=16000,
                    output=True,
                    frames_per_buffer=CHUNK_SIZE)








def process_wav_bytes(webm_bytes: bytes, sample_rate: int = 32000):
        wf = wave.open("output.wav", 'wb')
        wf.setnchannels(1)
        p = pyaudio.PyAudio()
        wf.setsampwidth(p.get_sample_size(pyaudio.paInt16))
        wf.setframerate(32000)
        wf.writeframes(webm_bytes)
        wf.close()
        waveform = whisper.audio.load_audio("output.wav", sr=sample_rate)
        
        return waveform
        
async def transcribe_socket(ws):
        while not ws.closed:
                message = await ws.recv()
                if message:
                        print('message received', len(message), type(message))
                try:
                        if isinstance(message, str):
                                message = base64.b64decode(message)
                        
                        audio = process_wav_bytes(bytes(message)).reshape(1, -1)
                        # audio = process_wav_bytes(audio)
                        # audio = load_audio(audio, 16000)
                        
                        audio = whisper.pad_or_trim(audio)
                        transcription = whisper.transcribe(
                        model,
                        audio,
                        fp16=False
                        )
                        print("************************************************************** " , transcription)
                except Exception as e:
                        traceback.print_exc()







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
    










async def websocket_handler(websocket, path):

    asyncio.create_task(send(websocket))

    audio_data = bytearray()

    try:
        # await transcribe_socket(websocket)

        async for msg in websocket:
            
            audio_data.extend(msg)
            
            if len(audio_data) >= 32000 * 2 * 2:
                current_audio_data = audio_data.copy()
                audio_data = bytearray()
                
                asyncio.create_task(save_and_transcribe(current_audio_data))
            
#            print(message)
            # audio_data = np.frombuffer(msg, dtype=np.int16)
            # stream.write(audio_data.tobytes())

    finally:
        stream.stop_stream()
        stream.close()
        audio.terminate()



async def start_websocket_server():
    async with websockets.serve(websocket_handler, '', 8888, ping_interval=None):
        await asyncio.Future()  # Keep the server running


async def send(websocket):
	while True:
		await asyncio.sleep(10)
		await websocket.send("msg")

if __name__ == '__main__':
    asyncio.run(start_websocket_server())
