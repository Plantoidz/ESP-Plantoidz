import pyaudio
import socket
import wave
import array

UDP_IP = "192.168.1.185"
UDP_PORT = 3333
WAVE_OUTPUT_FILENAME = "output.wav"
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

p = pyaudio.PyAudio()

stream = p.open(format=pyaudio.paInt16, channels=1, rate=44100, output=True)



frames = []

try:
    while True:



        data, addr = sock.recvfrom(1024) # buffer de 1024 bytes
#        print(addr)
#        print(len(data))

        res = ""
        for val in data:  ## NB: data is the int value of the ascii for the number
            res = res + chr(val)

        print("resultant string === ", int(str(res)))


        int16 = int(str(res))	


        output_bytes = array.array('i', [ int16 ]).tobytes(); 

        frames.append(output_bytes)

        
        stream.write(output_bytes)
       
 
except KeyboardInterrupt:  
    print("Cerrando...")
    wf = wave.open(WAVE_OUTPUT_FILENAME, 'wb')
    wf.setnchannels(1)
    wf.setsampwidth(2)
    wf.setframerate(44100)
    wf.writeframes(b''.join(frames))
    wf.close()
    # stream.stop_stream()
    # stream.close()
    # p.terminate()
