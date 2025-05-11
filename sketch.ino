#include <WiFi.h>           //Untuk koneksi WiFi
#include <PubSubClient.h>   //Untuk komunikasi MQTT
#include <DHTesp.h>         //Untuk membaca sensor DHT (khusus ESP)

//Inisialisasi dan konfigurasi 
const int DHT_PIN = 15;         //Pin tempat sensor DHT22 terhubung
DHTesp dht;                     //Objek sensor
const char* ssid = "Wokwi-GUEST";   //nama WiFi (SSID)
const char* password = "";          //Password WiFi
const char* mqtt_server = "test.mosquitto.org";   //Alamat broker MQTT

//Deklarasi objek 
WiFiClient espClient;            //Membuat objek WiFi client 
PubSubClient client(espClient);  //Membuat objek MQTT client dari WiFi client 
unsigned long lastMsg = 0;       //Menyimpan waktu pengiriman pesan terakhir
float temp = 0;                 //Variabel untuk suhu
float hum = 0;                  //Variabel untuk kelembapan

//Fungsi untuk menghubungkan ESP32 ke jaringan WiFi.
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);     //Hubungkan ke jaringan
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//Fungsi menangani pesan yang diterima dari topik MQTT yang disubscribe.
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);     //Menampilkan isi pesan
  }
}

//Fungsi jika koneksi ke broker MQTT terputus, fungsi ini akan mencoba menyambungkan ulang.
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.publish("/ThinkIOT/Publish", "Welcome");
      client.subscribe("/ThinkIOT/Subscribe");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); //Coba lagi setelah 5 detik
    }
  }
}

//Fungsi awal dijalankan saat ESP32 menyala. 
void setup() {
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  setup_wifi();                         //Menghubungkan ke WiFi
  client.setServer(mqtt_server, 1883);  //Set alamat broker MQTT
  client.setCallback(callback);         //Set fungsi callback untuk pesan masuk 
  dht.setup(DHT_PIN, DHTesp::DHT22);    //Inisiasi sensor DHT22
}

//Fungsi loop untuk mengirim data suhu dan kelembaban setiap 2 detik ke broker MQTT. 
void loop() {
  if (!client.connected()) {
    reconnect();   //PAstikan koneksi MQTT aktif
  }
  client.loop();   //Jaga koneksi tetap hidup

  unsigned long now = millis();
  if (now - lastMsg > 2000) {   //Cek apakah sudah 2 detik
    lastMsg = now;
    TempAndHumidity data = dht.getTempAndHumidity();    //Ambil data sensor

    String temp = String(data.temperature, 2);
    client.publish("/ThinkIOT/temp", temp.c_str());     //Kirim suhu ke MQTT
    String hum = String(data.humidity, 1);
    client.publish("/ThinkIOT/hum", hum.c_str());       //Kirim kelembapan ke MQTT

    //Tampilkan data di serial monitor
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(hum);
  }
}