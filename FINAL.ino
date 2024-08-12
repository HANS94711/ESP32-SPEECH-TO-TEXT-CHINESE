#include <WiFi.h>
#include <WiFiUdp.h>
#include <driver/i2s.h>
// Wi-Fi 設定
const char* ssid = "GMES3F";
const char* password = "00000000";

// 伺服器地址與端口
const char* serverIP = "192.168.1.14"; // 改成你的Python伺服器IP地址 // 改成你的Python伺服器IP地址
const int serverPort = 12345; // 用於傳送音訊資料
const int localUdpPort = 12346; // 用於接收識別結果的本地端口

// I2S 設定
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14

// 按鈕設定
const int buttonPin = 4;

WiFiUDP udp;
bool isRecording = false;
char incomingPacket[255];

void setup() {
  Serial.begin(115200);

  // 連接 Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // 初始化 I2S
  i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = 16000,
      .bits_per_sample = i2s_bits_per_sample_t(16),
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  // 初始化按鈕
  pinMode(buttonPin, INPUT);

  // 開始UDP
  udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}

void loop() {
  int buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH && !isRecording) {
     
    
    // 按鈕按下，開始錄音，傳送開始標誌包
    isRecording = true;
    udp.beginPacket(serverIP, serverPort);
    udp.write((const uint8_t*)"START", 5);  // 發送 "START" 標誌包
    udp.endPacket();
    Serial.println("Recording started");
  } else if (buttonState == LOW && isRecording) {
    // 按鈕釋放，停止錄音，傳送結束標誌包
    isRecording = false;
    udp.beginPacket(serverIP, serverPort);
    udp.write((const uint8_t*)"END", 3);  // 發送 "END" 標誌包
    udp.endPacket();
    Serial.println("Recording stopped");
  }

  while (isRecording) {
    size_t bytesRead;
    uint8_t i2sData[128];
    i2s_read(I2S_NUM_0, (void*) i2sData, sizeof(i2sData), &bytesRead, portMAX_DELAY);

    if (bytesRead > 0) {
      udp.beginPacket(serverIP, serverPort);
      udp.write(i2sData, bytesRead);
      udp.endPacket();
    }
    buttonState = digitalRead(buttonPin);
    if(buttonState == LOW){
      delay(90);
      buttonState = digitalRead(buttonPin);
      if(buttonState == LOW){
        break;
        }
      
      }
  }

  // 接收來自 Python 的識別結果
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
      Serial.printf("Received recognition result: %s\n", incomingPacket);
    }
  }

  delay(10); // 確保按鈕狀態的穩定讀取
}
