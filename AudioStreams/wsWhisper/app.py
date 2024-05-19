from flask import Flask, render_template
from flask_sockets import Sockets


app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')


sockets = Sockets(app)


if __name__ == "__main__":
    from gevent import pywsgi
    from geventwebsocket.handler import WebSocketHandler

    server = pywsgi.WSGIServer(('', 8888), app, handler_class=WebSocketHandler)
    server.serve_forever()


model = whisper.load_model('base.en')

def process_wav_bytes(webm_bytes: bytes, sample_rate: int = 16000):
    print("function called process_wav_bytes")
    with tempfile.NamedTemporaryFile(suffix='.wav', delete=True) as temp_file:
        temp_file.write(webm_bytes)
        temp_file.flush()
        waveform = whisper.load_audio(temp_file.name, sr=sample_rate)
        return waveform

def transcribe_socket(ws):
    print("in trasrcibe... ")
    while not ws.closed:
        message = ws.receive()
        if message:
            print('message received', len(message), type(message))
            try:
                if isinstance(message, str):
                    message = base64.b64decode(message)
                audio = process_wav_bytes(bytes(message)).reshape(1, -1)
                audio = whisper.pad_or_trim(audio)
                transcription = whisper.transcribe(
                    model,
                    audio
                )
            except Exception as e:
                traceback.print_exc()

sockets.url_map.add(Rule('/transcribe', endpoint=transcribe_socket, websocket=True))

app.run()
