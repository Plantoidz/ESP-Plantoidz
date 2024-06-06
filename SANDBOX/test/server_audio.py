import asyncio
import websockets
import wave
import logging

# Set up logging
logging.basicConfig(level=logging.INFO)

# Define the server's port as a variable
PORT = 7777

async def read_wav_file(wf):
    """
    Asynchronously read chunks of data from a WAV file.
    """
    CHUNK = 1024
    while True:
        data = wf.readframes(CHUNK)
        if not data:
            break
        yield data

async def send_audio(websocket, path):
    """
    Send audio data to the connected WebSocket client.
    """
    wf = wave.open("test.wav", 'rb')
    logging.info("Client connected, streaming audio.")
    try:
        async for data in read_wav_file(wf):
            await websocket.send(data)
            
    except websockets.ConnectionClosed:
        logging.info("Connection closed by client.")
    except Exception as e:
        logging.error(f"An error occurred: {e}")
    finally:
        wf.close()
        logging.info("Audio file closed.")

async def start_websocket_server():
    """
    Start the WebSocket server and serve clients.
    """
    async with websockets.serve(send_audio, '', PORT, ping_interval=None):
        logging.info(f"Server started on ws://localhost:{PORT}")
        await asyncio.Future()  # Keep the server running

if __name__ == '__main__':
    asyncio.run(start_websocket_server())
