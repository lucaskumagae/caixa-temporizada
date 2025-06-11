#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include "SPIFFS.h"
#include "sqlite3.h"

// Credenciais WiFi
const char* ssid = "VIVOFIBRA-7AA1";
const char* password = "Dd13492131";

#define SENSOR_PIN 23
const int buzzerPin = 19;

int ultimoEstado = HIGH;
WebServer server(80);

int horarioProgramado = -1;
bool alarmeTocando = false;
bool alarmeDesativadoAposAbrir = false;

// NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600;
const int daylightOffset_sec = 0;

// SQLite
sqlite3 *db;
char *zErrMsg = 0;

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  // Conectar ao WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Iniciar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao iniciar SPIFFS");
    return;
  }

  // Abrir ou criar banco de dados
  if (sqlite3_open("/spiffs/alarmes.db", &db) == SQLITE_OK) {
    Serial.println("Banco de dados aberto");
    const char *sql = "CREATE TABLE IF NOT EXISTS alarmes (id INTEGER PRIMARY KEY AUTOINCREMENT, hora TEXT);";
    sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
  } else {
    Serial.println("Erro ao abrir BD");
  }

  // Restaura último horário salvo
  sqlite3_stmt *res;
  int rc = sqlite3_prepare_v2(db, "SELECT hora FROM alarmes ORDER BY id DESC LIMIT 1;", -1, &res, NULL);
  if (rc == SQLITE_OK && sqlite3_step(res) == SQLITE_ROW) {
    const char* horaStr = (const char*)sqlite3_column_text(res, 0);
    int hh = String(horaStr).substring(0, 2).toInt();
    int mm = String(horaStr).substring(3, 5).toInt();
    horarioProgramado = hh * 60 + mm;
    Serial.printf("Horario restaurado: %02d:%02d\n", hh, mm);
  }
  sqlite3_finalize(res);

  // Servidor Web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settime", HTTP_GET, handleSetTime);
  server.begin();
}

void loop() {
  server.handleClient();

  int estado = digitalRead(SENSOR_PIN);
  if (estado != ultimoEstado) {
    if (estado == LOW) {
      Serial.println("Caixa fechada!");
    } else {
      Serial.println("Caixa aberta!");
      if (alarmeTocando) {
        noTone(buzzerPin);
        alarmeTocando = false;
        alarmeDesativadoAposAbrir = true;
        Serial.println("Alarme desativado apos abrir a caixa.");
      }
    }
    ultimoEstado = estado;
  }

  if (horarioProgramado != -1) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      int agora = timeinfo.tm_hour * 60 + timeinfo.tm_min;
      if (agora == horarioProgramado && !alarmeDesativadoAposAbrir) {
        if (estado == LOW && !alarmeTocando) {
          tone(buzzerPin, 1000);
          alarmeTocando = true;
          Serial.println("Alarme tocando.");
        }
      } else if (alarmeTocando) {
        noTone(buzzerPin);
        alarmeTocando = false;
      }
    }
  }

  delay(100);
}

void handleRoot() {
  String page = "<html><body>";
  page += "<h1>Configurar horario para buzzer</h1>";
  page += "<form action=\"/settime\" method=\"get\">";
  page += "Escolha o horario: <input type=\"time\" name=\"alarmtime\">";
  page += "<input type=\"submit\" value=\"Setar\">";
  page += "</form>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}

void handleSetTime() {
  if (server.hasArg("alarmtime")) {
    String t = server.arg("alarmtime");
    int hh = t.substring(0, 2).toInt();
    int mm = t.substring(3, 5).toInt();
    horarioProgramado = hh * 60 + mm;
    alarmeDesativadoAposAbrir = false;
    alarmeTocando = false;

    // Salva no banco
    String sql = "INSERT INTO alarmes (hora) VALUES ('" + t + "');";
    sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    Serial.printf("Horario salvo no banco: %s\n", t.c_str());

    String page = "<html><body>";
    page += "<h1>Horario salvo: " + t + "</h1>";
    page += "<a href=\"/\">Voltar</a>";
    page += "</body></html>";
    server.send(200, "text/html", page);
  } else {
    server.send(400, "text/plain", "Horario nao informado");
  }
}
