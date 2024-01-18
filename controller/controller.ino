/*            
  ｺﾝﾄﾛｰﾗ用ﾌﾟﾛｸﾞﾗﾑ
      R5年度 E3A-3班
*/

//ｲﾝｸﾙｰﾄﾞ設定
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include <esp_now.h>

// ﾋﾟﾝ番号・変数設定
#define SW_LT      27
#define SW_RT      25
#define SW_MODE    26
#define VOL_LEV    39

Adafruit_SSD1306 display(-1);

unsigned long TIM_ST_1;
unsigned long TIM_NOW_1;
unsigned long TIM_TMP_1;
int TIM_MODE_1 = 0;

unsigned long TIM_ST_2;
unsigned long TIM_NOW_2;
unsigned long TIM_TMP_2;
int TIM_MODE_2 = 0;

int VOL_TMP;
int VOL_NOTCH;
int TR_MODE = 0;
int TR_TMP = 0;
int TR_STOP = 0;

int SEND = 0;
int SETUP = 1;

const uint8_t addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t dataSend[3];
uint8_t dataTmp[6];

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
    dataSend[0] = 1;         // 送信時の識別番号
    dataSend[1] = TR_MODE;   // 現在の操作ﾓｰﾄﾞ
    dataSend[2] = VOL_NOTCH; // 現在のﾉｯﾁ

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
  if (dataRecv[0] == 0) {
    dataTmp[3] = dataTmp[0]; // 現在の速度
    dataTmp[4] = dataTmp[1]; // 現在の位置
    dataTmp[5] = dataTmp[2]; // 停車
    
    dataTmp[0] = dataRecv[1]; // 現在の速度
    dataTmp[1] = dataRecv[2]; // 現在の位置
    dataTmp[2] = dataRecv[3]; // 停車
    serial(0, "受信に成功しました。");
  }
}

// ﾃﾞｨｽﾌﾟﾚｲ制御
void oprDisp() {
  if ((dataTmp[0] != dataTmp[3]) || (SETUP == 1)) {
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.print("Speed : ");
    display.setTextColor(BLACK);
    display.print(dataTmp[3]);

    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.print("Speed : ");
    display.print(dataTmp[0]);

    display.setCursor(70, 0);
    display.println("km/h");
    display.display();
  }

  if ((dataTmp[1] != dataTmp[4]) || (SETUP == 1)) {
    display.setCursor(0, 8);
    display.setTextColor(WHITE);
    display.print("Point : ");
    display.setTextColor(BLACK);
    display.print(dataTmp[1]);
    
    display.setCursor(0, 8);
    display.setTextColor(WHITE);
    display.print("Point : ");
    display.print(dataTmp[1]);

    display.display();
  }
  
  if ((TR_MODE != TR_TMP) || (SETUP == 1)) {
    display.setCursor(0, 16);
    display.setTextColor(WHITE);
    display.print("Mode  : ");
    display.setTextColor(BLACK);
    display.print(TR_TMP);
    
    display.setCursor(0, 16);
    display.setTextColor(WHITE);
    display.print("Mode  : ");
    display.print(TR_MODE);
    
    display.display();
  }

  if ((VOL_NOTCH != VOL_TMP) || (SETUP == 1)) {
    display.setCursor(0, 24);
    display.setTextColor(WHITE);
    display.print("Notch : ");
    display.setTextColor(BLACK);
    display.print(VOL_TMP);
    
    display.setCursor(0, 24);
    display.setTextColor(WHITE);
    display.print("Notch : ");
    display.print(VOL_NOTCH);
    
    display.display();
  }
}

// 初期設定
void setup() {
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);

  pinMode(SW_LT, INPUT_PULLUP);
  pinMode(SW_RT, INPUT_PULLUP);
  pinMode(SW_MODE, INPUT_PULLUP);
  pinMode(VOL_LEV, INPUT);

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

  VOL_NOTCH = analogRead(VOL_LEV) / (4096 / 11) - 4;
  serial(0, "ﾉｯﾁは ", VOL_NOTCH, " です。");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  oprDisp();
  
  SETUP = 0;
}

// ｺﾝﾄﾛｰﾗ操作
void oprCntl() {
  VOL_TMP = VOL_NOTCH;
  VOL_NOTCH = analogRead(VOL_LEV) / (4096 / 11) - 4;

  if (VOL_NOTCH != VOL_TMP) {
    serial(0, "ﾉｯﾁが ", VOL_NOTCH, " に変更されました。");
    
  }
  
  if (VOL_NOTCH == 0) {
    if ((digitalRead(SW_LT) == LOW) && (TR_MODE == 0)) {
      if (TIM_MODE_1 == 0) {
       timer_1(1000);
      }
      if (timer_1() == 0) {
        if (digitalRead(SW_RT) == LOW) {
          TR_TMP = TR_MODE;
          TR_MODE = 1;
          TIM_MODE_1 = 1;
          serial(0, "ATO運転を開始します。");
        }
      } else if (timer_1() == 1) {
        TIM_MODE_1 = 0;
        serial(2, "規定数のｽｲｯﾁを押下してください。");
      }
    } else if ((digitalRead(SW_RT) == LOW) && (TR_MODE == 0)) {
      if (TIM_MODE_1 == 0) {
       timer_1(1000);
      }
      if (timer_1() == 0) {
        if (digitalRead(SW_LT) == LOW) {
          TR_TMP = TR_MODE;
          TR_MODE = 1;
          TIM_MODE_1 = 1;
          serial(0, "ATO運転を開始します。");
        }
      } else if (timer_1() == 1) {
        TIM_MODE_1 = 0;
        serial(2, "規定数のｽｲｯﾁを押下してください。");
      }
    } else {
      TIM_MODE_1 = 0;
    }
  }

  if ((digitalRead(SW_MODE) == LOW) && (TR_MODE == 1)) {
    TR_TMP = TR_MODE;
    TR_MODE = 2;
    serial(0, "手動操作に切り替えます。");
  }

  if (((((VOL_NOTCH == -4) || (TR_STOP == 1)) && (TR_MODE == 2)) || dataTmp[2] == 1) && (TR_MODE != 0)) {
    TR_TMP = TR_MODE;
    TR_MODE = 0;
    serial(0, "停車しました。");
    serial(0, "発車する際は再度ｽｲｯﾁを押下してください。");
  }
}

void loop() {
  oprCntl();
  oprDisp();
  oprSend();
}
