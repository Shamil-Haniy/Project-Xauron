/*
 * DC Sentinel - firmware v2
 *
 * v2 puts the storage layer and module boundaries BACK in the code.
 * v1 logged straight to a file and had no rotation or offline upload.
 * that was the "out of the code blocks" mistake. fixed here.
 *
 * modules (all in this file for easy flashing, but kept separate):
 *   sensors   - INA219 read (V + I)
 *   algorithm - Welford baseline + z-score + persistence + CUSUM
 *   storage   - RAM ring buffer -> LittleFS rotation -> cloud upload
 *   net       - telegram alerts + supabase insert (HMAC signed)
 *
 * not production ready. FFT not done. temp is crude. be honest in docs.
 */
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <mbedtls/md.h>
#include <esp_task_wdt.h>

#define PIN_SDA 4
#define PIN_SCL 5
#define PIN_LED 10

#define INA_ADDR 0x40
#define SHUNT_R 0.1f
#define SHUNT_LSB 0.00001f
#define BUS_LSB 0.004f

#define TRAIN_N 3000          // 5 min @10Hz
#define Z_THRESH_I 4.0f
#define Z_THRESH_V 3.5f
#define PERSIST_I 15
#define SIGMA_FLOOR_I 0.005f
#define SIGMA_FLOOR_V 0.015f
#define CUSUM_K 0.05f
#define CUSUM_H 2.0f
#define SAMPLE_MS 100

// storage tuning
#define LOGBUF_SZ 4096
#define LOG_FILES 5
#define LOG_ROT_KB 50

// REPLACE ALL BEFORE FLASHING
const char* SECRET = "REPLACE_WITH_64_CHAR_RANDOM_STRING";
const char* SB_URL = "https://YOUR-PROJECT.supabase.co/rest/v1/device_logs";
const char* SB_KEY = "YOUR-ANON-KEY";
const char* TG_TOK = "YOUR-BOT-TOKEN";
const char* TG_ME  = "YOUR-CHAT-ID";

/* ---------- algorithm: Welford online baseline ---------- */
struct Welford { float n=0, mean=0, m2=0;
  void update(float x){ n++; float d=x-mean; mean+=d/n; m2+=d*(x-mean); }
  float var(){ return n>1? m2/(n-1):0; }
  float std(){ return sqrt(var()); }
  float z(float x){ float s=std(); return s>1e-9?(x-mean)/s:0; }
};
Welford wi, wv;
bool trained=false;
float c_hi=0,c_lo=0; int persist=0;
bool cusum(float v,float base){
  c_hi=max(0.0f,c_hi+v-base-CUSUM_K);
  c_lo=max(0.0f,c_lo-v+base-CUSUM_K);
  return (c_hi>CUSUM_H||c_lo>CUSUM_H);
}

/* ---------- sensors ---------- */
bool inaInit(){
  Wire.begin(PIN_SDA,PIN_SCL); Wire.setClock(400000);
  Wire.beginTransmission(INA_ADDR); if(Wire.endTransmission()!=0)return false;
  Wire.beginTransmission(INA_ADDR); Wire.write(0x00); Wire.write(0x07); Wire.write(0xFF); Wire.endTransmission();
  Wire.beginTransmission(INA_ADDR); Wire.write(0x05); Wire.write(0x10); Wire.write(0x00); Wire.endTransmission();
  return true;
}
float readI(){ Wire.beginTransmission(INA_ADDR); Wire.write(0x01); Wire.endTransmission();
  Wire.requestFrom(INA_ADDR,2); int16_t r=(Wire.read()<<8)|Wire.read(); return (r*SHUNT_LSB)/SHUNT_R; }
float readV(){ Wire.beginTransmission(INA_ADDR); Wire.write(0x02); Wire.endTransmission();
  Wire.requestFrom(INA_ADDR,2); int16_t r=(Wire.read()<<8)|Wire.read(); return (float)(r>>3)*BUS_LSB; }

/* ---------- storage: RAM -> LittleFS -> cloud ---------- */
static char lbuf[LOGBUF_SZ]; static int lidx=0; static int lfile=0;
void log_flush();
void log_write(const char* line){
  int len=strlen(line);
  if(lidx+len+2>=LOGBUF_SZ) log_flush();
  if(lidx+len+2<LOGBUF_SZ){ memcpy(&lbuf[lidx],line,len); lidx+=len; lbuf[lidx++]='\n'; }
}
void log_flush(){
  if(lidx==0)return;
  if(LittleFS.usedBytes()>LittleFS.totalBytes()*0.9)
    LittleFS.remove(String("/evt_")+((lfile+1)%LOG_FILES)+".csv");
  File f=LittleFS.open(String("/evt_")+lfile+".csv",FILE_APPEND);
  if(!f){ lfile=(lfile+1)%LOG_FILES; f=LittleFS.open(String("/evt_")+lfile+".csv",FILE_APPEND); }
  if(f){ f.write((uint8_t*)lbuf,lidx); size_t sz=f.size(); f.close(); lidx=0;
         if(sz>LOG_ROT_KB*1024) lfile=(lfile+1)%LOG_FILES; }
  else lidx=0;
}
void log_upload(){
  if(WiFi.status()!=WL_CONNECTED)return;
  for(int i=0;i<LOG_FILES;i++){
    String fn=String("/evt_")+i+".csv";
    if(!LittleFS.exists(fn))continue;
    File f=LittleFS.open(fn,FILE_READ); if(!f)continue;
    bool all=true;
    while(f.available()){
      String line=f.readStringUntil('\n'); if(line.length()==0)continue;
      HTTPClient h; h.begin(SB_URL);
      h.addHeader("apikey",SB_KEY);
      h.addHeader("Authorization",String("Bearer ")+SB_KEY);
      h.addHeader("Content-Type","text/plain");
      int c=h.POST(line); h.end();
      if(c!=201){all=false;break;}
    }
    f.close(); if(all)LittleFS.remove(fn);
  }
}

/* ---------- net ---------- */
String hmac(const char* d){ byte hm[32]; char hx[65];
  mbedtls_md_context_t c; mbedtls_md_init(&c);
  const mbedtls_md_info_t* mi=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_setup(&c,mi,1);
  mbedtls_md_hmac_starts(&c,(const byte*)SECRET,strlen(SECRET));
  mbedtls_md_hmac_update(&c,(const byte*)d,strlen(d));
  mbedtls_md_hmac_finish(&c,hm); mbedtls_md_free(&c);
  for(int i=0;i<32;i++)snprintf(&hx[i*2],3,"%02x",hm[i]); hx[64]=0; return String(hx); }
void sendAlert(const char* m){ if(WiFi.status()!=WL_CONNECTED)return;
  HTTPClient h; char u[128]; snprintf(u,sizeof(u),"https://api.telegram.org/bot%s/sendMessage",TG_TOK);
  h.begin(u); h.addHeader("Content-Type","application/x-www-form-urlencoded");
  char p[512]; snprintf(p,sizeof(p),"chat_id=%s&text=%s",TG_ME,m);
  h.POST((uint8_t*)p,strlen(p)); h.end(); }

String device_id; unsigned long t_flush=0;

void setup(){
  Serial.begin(115200);
  esp_task_wdt_init(30,false); esp_task_wdt_add(NULL);
  pinMode(PIN_LED,OUTPUT);
  if(!inaInit()){Serial.println("INA219 fail");while(1)esp_task_wdt_reset();}
  LittleFS.begin(true);
  uint8_t m[6]; WiFi.macAddress(m);
  char id[12]; snprintf(id,sizeof(id),"DC-%02X%02X%02X",m[3],m[4],m[5]); device_id=id;
  WiFi.begin("YOUR_SSID","YOUR_PASS");
  unsigned long t=millis();
  while(WiFi.status()!=WL_CONNECTED&&millis()-t<10000){esp_task_wdt_reset();delay(100);}
  sendAlert("DC SENTINEL ONLINE");
}

void loop(){
  esp_task_wdt_reset();
  float i=readI(), v=readV();
  if(i<0||i>5000){delay(SAMPLE_MS);return;}   // spike guard

  if(!trained){
    wi.update(i); wv.update(v);
    if(wi.n>=TRAIN_N){ trained=true;
      if(wi.std()<SIGMA_FLOOR_I)wi.m2=wi.n*SIGMA_FLOOR_I*SIGMA_FLOOR_I;
      if(wv.std()<SIGMA_FLOOR_V)wv.m2=wv.n*SIGMA_FLOOR_V*SIGMA_FLOOR_V;
      sendAlert("BASELINE LOCKED"); }
    delay(SAMPLE_MS); return;
  }

  float zi=fabs(wi.z(i)), zv=fabs(wv.z(v));
  bool drift=cusum(i,wi.mean);
  bool anom=(zi>Z_THRESH_I||zv>Z_THRESH_V||drift);
  if(anom){ if(++persist>=PERSIST_I){
      char m[256]; snprintf(m,sizeof(m),"ANOMALY I=%.3fA V=%.2fV zi=%.1f zv=%.1f",i,v,zi,zv);
      sendAlert(m); persist=0; } }
  else persist=0;

  StaticJsonDocument<256> doc;
  doc["ts"]=millis()/1000; doc["v"]=v; doc["i"]=i;
  doc["zi"]=zi; doc["zv"]=zv; doc["drift"]=drift; doc["alert"]=anom;
  char j[256]; serializeJson(doc,j);
  String line=String(j)+","+hmac(j);
  log_write(line.c_str());

  if(millis()-t_flush>300000){ log_flush(); log_upload(); t_flush=millis(); }
  delay(SAMPLE_MS);
}
