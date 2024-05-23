import os
from dotenv import load_dotenv
from elevenlabs.client import ElevenLabs, AsyncElevenLabs
from elevenlabs import stream, Voice, VoiceSettings, play

# Load environment variables from .env file
load_dotenv()

OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY")
ELEVENLABS_API_KEY = os.environ.get("ELEVENLABS_API_KEY")
ANTHROPIC_API_KEY = os.environ.get("ANTHROPIC_API_KEY")

client = ElevenLabs(
  api_key=ELEVENLABS_API_KEY
)

voice_id  = ""

def stream_text(self, response_stream):

    if isinstance(response_stream, str):
        return response_stream

    for chunk in response_stream:
        if 'choices' in chunk and chunk['choices'][0].get('delta', {}).get('content'):
            delta = chunk.choices[0].delta
            text_chunk = delta.content
            yield text_chunk
            print(text_chunk, end='', flush=True)

def send_stream_to_websocket(response):
    # with client.generate(
    #     text="Hello, how can I help you?",
    #     model="eleven_turbo_v2",
    #     voice=voice_id,
    #     stream=True
    # ) as response:
    #     for chunk in response:
    #         if 'choices' in chunk and chunk['choices'][0].get('delta', {}).get('content'):
    #             delta = chunk.choices[0].delta
    #             text_chunk = delta.content
    #             yield text_chunk
    #             print(text_chunk, end='', flush=True)
    # NOTE: I think websocket send can be in here somewhere

    audio_stream = client.generate(
                        text=stream_text(response),
                        model="eleven_turbo_v2",
                        voice=voice_id,
                        stream=True
                    )