import socket
import wave
import json
from vosk import Model, KaldiRecognizer

# 設定伺服器和模型
server_ip = '0.0.0.0'
server_port = 12345
esp32_ip = '192.168.1.12'  # 替換為你的ESP32的IP地址
esp32_port = 12346  # ESP32接收回傳資料的端口
model = Model("vosk-model-cn-0.22")
# WAV 文件的配置
output_filename = 'recorded_audio12321.wav'
channels = 1  # 單聲道
sample_width = 2  # 16-bit，即2字節
sample_rate = 16000  # 取樣率（與ESP32設定一致）

recording = False

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((server_ip, server_port))

print("等待音訊資料...")

try:
    while True:
        data, addr = sock.recvfrom(1024)
        if data == b'START':
            print("開始錄音...")
            # 打開 WAV 文件並準備寫入
            wav_file = wave.open(output_filename, 'wb')
            wav_file.setnchannels(channels)
            wav_file.setsampwidth(sample_width)
            wav_file.setframerate(sample_rate)
            recording = True
        elif data == b'END':
            if recording:
                print("錄音結束")
                wav_file.close()  # 完成錄音並關閉文件
                recording = False
                
                # 進行語音識別
                print("開始語音識別...")
                 # 請確保模型路徑正確
                wf = wave.open(output_filename, "rb")
                rec = KaldiRecognizer(model, wf.getframerate())
                rec.SetWords(True)

                recognized_text = ""

                while True:
                    data = wf.readframes(4000)
                    if len(data) == 0:
                        break
                    if rec.AcceptWaveform(data):
                        result = rec.Result()
                        text = json.loads(result)["text"]
                        print("識別結果:", text)
                        recognized_text += text + " "

                final_result = rec.FinalResult()
                final_text = json.loads(final_result)["text"]
                print("最終識別結果:", final_text)
                recognized_text += final_text

                wf.close()
                
                # 回傳辨識結果給ESP32
                if recognized_text.strip():
                    print(f"回傳結果給ESP32: {recognized_text.strip()}")
                    sock.sendto(recognized_text.strip().encode(), (esp32_ip, esp32_port))
        elif recording:
            wav_file.writeframes(data)  # 將音訊資料寫入 WAV 文件
except KeyboardInterrupt:
    print("錄音手動結束")
    if recording:
        wav_file.close()
finally:
    sock.close()
