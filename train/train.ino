/*            
  列車制御用ﾌﾟﾛｸﾞﾗﾑ
      R5年度 E3A-3班
*/

//ｲﾝｸﾙｰﾄﾞ設定
#include <WiFi.h>
#include <esp_now.h>

// ﾋﾟﾝ番号・変数設定
#define IN1  33
#define IN2  32
#define VREF 25

int CH1 = 0;
int CH2 = 1;

unsigned long TIM_ST_1;
unsigned long TIM_NOW_1;
unsigned long TIM_TMP_1;
int TIM_MODE_1 = 0;

unsigned long TIM_ST_2;
unsigned long TIM_NOW_2;
unsigned long TIM_TMP_2;
int TIM_MODE_2 = 0;

int TR_SP = 0;
int TR_POS = 0;
int TR_STOP = 0;

int PHOTO = 36;
int HALL = 34;

const uint8_t addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t dataSend[4];
uint8_t dataTmp[3];

// ﾀｲﾏｰ関数 (単位 : ms, ｺﾝﾄﾛｰﾗ用)
int timer_1(unsigned long tim_1 = 0) {
  if (tim_1 != 0) {
    TIM_MODE_1 = 1;
    TIM_ST_1 = millis();
    TIM_NOW_1 = millis();
    TIM_TMP_1 = tim_1;
  }

  if (TIM_TMP_1 <= (TIM_NOW_1 - TIM_ST_1)) {
    TIM_MODE_1 = 0;
    return 1;
  } else {
    TIM_NOW_1 = millis();
    return 0;
  }
}

// ﾀｲﾏｰ関数 (単位 : ms, 送信用)
int timer_2(unsigned long tim_2 = 0) {
  if (tim_2 != 0) {
    TIM_MODE_2 = 1;
    TIM_ST_2 = millis();
    TIM_NOW_2 = millis();
    TIM_TMP_2 = tim_2;
  }

  if (TIM_TMP_2 <= (TIM_NOW_2 - TIM_ST_2)) {
    TIM_MODE_2 = 0;
    return 1;
  } else {
    TIM_NOW_2 = millis();
    return 0;
  }
}

// ﾃﾞﾊﾞｯｸﾞ関数
void serial(int type, String msg1 = "", int val = -256, String msg2 = "") {
  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");
  if (type == 0) {
    Serial.print("情報 : ");
  } else if (type == 1) {
    Serial.print("ｴﾗｰ  : ");
  } else if (type == 2) {
    Serial.print("警告 : ");
  }
  
  if (val != -256) {
    Serial.print(msg1);
    Serial.print(val);
  } else {
    Serial.print(msg1);
  }
  Serial.println(msg2);
}

// 送信側制御
void oprSend() {
  if (TIM_MODE_2 == 0) {
    timer_2(250);
  }
  
  if (timer_2() == 1) {
    dataSend[0] = 0;       // 送信時の識別番号
    dataSend[1] = TR_SP;   // 現在の速度
    dataSend[2] = TR_POS;  // 現在の位置
    dataSend[3] = TR_STOP; // 停車

    esp_err_t result = esp_now_send(addr, dataSend, sizeof(dataSend));
  
    switch (result) {
      case ESP_OK:
        serial(0, "送信に成功しました。");
        break;
      case ESP_ERR_ESPNOW_NOT_INIT:
        serial(1, "ESP-NOW が初期化されていません。");
        break;
      case ESP_ERR_ESPNOW_ARG:
        serial(1, "不正なﾊﾟﾗﾒｰﾀが指定されました。");
        break;
      case ESP_ERR_ESPNOW_INTERNAL:
        serial(1, "内部ｴﾗｰが発生しました。");
        break;
      case ESP_ERR_ESPNOW_NO_MEM:
        serial(1, "ﾒﾓﾘが不足しています。");
        break;
      case ESP_ERR_ESPNOW_NOT_FOUND:
        serial(1, "ﾋﾟｱが見つかりません。");
        break;
    }
    
    TIM_MODE_2 = 0;
  }
}

// 受信側制御
void oprRecv(const uint8_t *addr, const uint8_t *dataRecv, int dataSize)
{
  if (dataRecv[0] == 1) {
    dataTmp[0] = dataRecv[1];   // 現在の操作ﾓｰﾄﾞ
    dataTmp[1] = dataRecv[2];   // 現在のﾉｯﾁ
    serial(0, "受信に成功しました。");
  } else if (dataRecv[0] == 2) {
    dataTmp[2] = dataRecv[1];   // ﾎｰﾙﾗｯﾁの反応
    serial(0, "受信に成功しました。");
  }
}

// 初期設定
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); 
  pinMode(IN2, OUTPUT); 
  pinMode(VREF, OUTPUT);
  
  ledcSetup(CH1, 490, 10);
  ledcSetup(CH2, 490, 10);
  ledcAttachPin(IN1, CH1);
  ledcAttachPin(IN2, CH2);

  delay(1000);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() == ESP_OK) {
    serial(0, "初期化に成功しました。");
  }

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, addr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    serial(1, "ﾋﾟｱの追加に失敗しました。");
    return;
  }
  
  esp_now_register_recv_cb(oprRecv);
}

void loop() {
  while (1) {
    oprSend();
    
    if (dataTmp[0] == 0) {
      TR_STOP = 0;
    }
    
    if ((dataTmp[0] == 1) && (TR_STOP == 0)) {
      ledcWrite(CH1, 250);
      ledcWrite(CH2, LOW);
      serial(0, "速度を上げています。(1/3)");
      delay(1000);

      ledcWrite(CH1, 500);
      ledcWrite(CH2, LOW);
      serial(0, "速度を上げています。(2/3)");
      delay(1000);

      ledcWrite(CH1, 750);
      ledcWrite(CH2, LOW);
      serial(0, "速度を上げています。(3/3)");
      delay(1000);
    break;
    }
  }
  
  ledcWrite(CH1, 1023);
  ledcWrite(CH2, LOW);

  while (1) {
    oprSend();
    
    if (dataTmp[0] == 2) {
      if (dataTmp[1] < 200 && dataTmp[1] != 0) {
        ledcWrite(CH1, int(1023 - (1023 / (8 - dataTmp[1]))));
        ledcWrite(CH2, LOW);
      } else if (dataTmp[1] == 0) {
        ledcWrite(CH1, 1023);
        ledcWrite(CH2, LOW);
        digitalWrite(IN2, LOW);
      } else if (dataTmp[1] == 7) {
        ledcWrite(CH1, 10);
        ledcWrite(CH2, 1023);
        break;
      }
    }

    if (analogRead(PHOTO) < 2048) {
      ledcWrite(CH1, 750);
      ledcWrite(CH2, LOW);
      serial(0, "速度を落としています。(1/3)");
      delay(1000);

      ledcWrite(CH1, 500);
      ledcWrite(CH2, LOW);
      serial(0, "速度を落としています。(2/3)");
      delay(1000);

      ledcWrite(CH1, 250);
      ledcWrite(CH2, LOW);
      serial(0, "速度を落としています。(3/3)");
      delay(1000);
       
      ledcWrite(CH1, 10);
      ledcWrite(CH2, 1023);
      delay(3000);
      break;
    }
  }

  TR_STOP = 1;
}
