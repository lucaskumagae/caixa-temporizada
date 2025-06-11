#include <WiFi.h>
#include <WebServer.h>
#include "time.h"

const char* ssid = "VIVOFIBRA-7AA1";
const char* password = "Dd13492131";

#define SENSOR_PIN 23
const int buzzerPin = 19;

int ultimoEstado = HIGH;

WebServer server(80);

int horarioProgramado = -1;

bool alarmeTocando = false;
bool alarmeDesativadoAposAbrir = false;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/settime", HTTP_GET, handleSetTime);

  server.begin();
  Serial.println("Servidor iniciado");
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
      } else {
        if (alarmeTocando) {
          noTone(buzzerPin);
          alarmeTocando = false;
        }
      }
    }
  }

  delay(100);
}

#include <WiFi.h>
#include <WebServer.h>
#include "time.h"

const char* ssid = "VIVOFIBRA-7AA1";
const char* password = "Dd13492131";

#define SENSOR_PIN 23
const int buzzerPin = 19;

int ultimoEstado = HIGH;

WebServer server(80);

int horarioProgramado = -1;

bool alarmeTocando = false;
bool alarmeDesativadoAposAbrir = false;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;

const char* main_html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Meus Remédios</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div id="error-message" style="display:none; position: fixed; top: 0; left: 0; right: 0; background-color: #f8d7da; color: #721c24; padding: 1em; border-bottom: 2px solid #f5c6cb; text-align: center; z-index: 1000;"></div>

    <!-- Navigation -->
    <div id="nav-placeholder"></div>

    <main class="content">
        <header class="header">
            <h1>Meus remédios</h1>
            <button class="add-remedio">📅 Adicionar novo remédio</button>
        </header>

        <section id="form-adicionar-remedio" style="display:none; padding: 1em; border: 1px solid #ccc; margin: 1em 0;">
            <h2>Adicionar novo remédio</h2>
            <form id="novo-remedio-form" method="POST" action="#">
                <input type="hidden" name="acao" value="adicionar">
                <label for="nome-remedio">Nome do remédio:</label><br>
                <input type="text" id="nome-remedio" name="nome-remedio" required><br><br>
                <label for="horario-remedio">Horário:</label><br>
                <input type="time" id="horario-remedio" name="horario-remedio" required><br><br>
                <button type="submit">Adicionar</button>
                <button type="button" id="cancelar-adicionar">Cancelar</button>
            </form>
        </section>

        <section id="form-editar-remedio" style="display:none; padding: 1em; border: 1px solid #ccc; margin: 1em 0;">
            <h2>Editar remédio</h2>
            <form id="editar-remedio-form" method="POST" action="#">
                <input type="hidden" name="acao" value="editar">
                <input type="hidden" id="id-medicamento" name="id-medicamento" value="">
                <label for="nome-remedio-editar">Nome do remédio:</label><br>
                <input type="text" id="nome-remedio-editar" name="nome-remedio-editar" required><br><br>
                <label for="horario-remedio-editar">Horário:</label><br>
                <input type="time" id="horario-remedio-editar" name="horario-remedio-editar" required><br><br>
                <button type="submit">Confirmar</button>
                <button type="button" id="cancelar-editar">Cancelar</button>
            </form>
        </section>

        <section class="lista-remedios" id="lista-remedios">
            <!-- Medicine list will be populated here -->
        </section>
    </main>

    <div id="confirm-excluir" style="display:none; position: fixed; top: 50%; left: 50%; transform: translate(-50%, -50%);
        background: white; border: 1px solid #ccc; padding: 1em; z-index: 2000; box-shadow: 0 0 10px rgba(0,0,0,0.5);">
        <p>Deseja realmente excluir este remédio?</p>
        <button id="confirmar-excluir">Confirmar</button>
        <button id="cancelar-excluir">Cancelar</button>
    </div>

    <script>
        // Simulated data (replace with real data or API calls if available)
        let remedios = [
            {id: 1, nome: "Remédio A", horario: "08:00, 20:00"},
            {id: 2, nome: "Remédio B", horario: "12:00"}
        ];

        function renderRemedios() {
            const lista = document.getElementById('lista-remedios');
            lista.innerHTML = '';
            remedios.forEach(remedio => {
                const div = document.createElement('div');
                div.className = 'remedio';
                div.setAttribute('data-id', remedio.id);
                div.setAttribute('data-nome', remedio.nome);
                div.setAttribute('data-horario', remedio.horario);

                div.innerHTML = `
                    <div class="icon-placeholder"></div>
                    <div class="info">
                        <strong>${remedio.nome}</strong>
                        <p>Horários: ${remedio.horario}</p>
                    </div>
                    <button class="editar">Editar</button>
                    <button class="excluir">Excluir</button>
                `;
                lista.appendChild(div);
            });
            attachEventListeners();
        }

        function attachEventListeners() {
            document.querySelectorAll('.editar').forEach(button => {
                button.addEventListener('click', function() {
                    const remedioDiv = this.closest('.remedio');
                    const id = remedioDiv.getAttribute('data-id');
                    const nome = remedioDiv.getAttribute('data-nome');
                    const horario = remedioDiv.getAttribute('data-horario');

                    document.getElementById('id-medicamento').value = id;
                    document.getElementById('nome-remedio-editar').value = nome;
                    const horarioPrimeiro = horario.split(',')[0].trim();
                    document.getElementById('horario-remedio-editar').value = horarioPrimeiro;

                    document.getElementById('form-editar-remedio').style.display = 'block';
                });
            });

            let remedioToDeleteId = null;
            document.querySelectorAll('.excluir').forEach(button => {
                button.addEventListener('click', function() {
                    const remedioDiv = this.closest('.remedio');
                    remedioToDeleteId = remedioDiv.getAttribute('data-id');
                    document.getElementById('confirm-excluir').style.display = 'block';
                });
            });

            document.getElementById('cancelar-excluir').addEventListener('click', function() {
                remedioToDeleteId = null;
                document.getElementById('confirm-excluir').style.display = 'none';
            });

            document.getElementById('confirmar-excluir').addEventListener('click', function() {
                if (remedioToDeleteId) {
                    remedios = remedios.filter(r => r.id != remedioToDeleteId);
                    renderRemedios();
                    remedioToDeleteId = null;
                    document.getElementById('confirm-excluir').style.display = 'none';
                }
            });
        }

        document.querySelector('.add-remedio').addEventListener('click', function() {
            document.getElementById('form-adicionar-remedio').style.display = 'block';
        });

        document.getElementById('cancelar-adicionar').addEventListener('click', function() {
            document.getElementById('form-adicionar-remedio').style.display = 'none';
        });

        document.getElementById('cancelar-editar').addEventListener('click', function() {
            document.getElementById('form-editar-remedio').style.display = 'none';
        });

        document.getElementById('novo-remedio-form').addEventListener('submit', function(event) {
            event.preventDefault();
            const nome = document.getElementById('nome-remedio').value;
            const horario = document.getElementById('horario-remedio').value;
            if (nome && horario) {
                const newId = remedios.length ? Math.max(...remedios.map(r => r.id)) + 1 : 1;
                remedios.push({id: newId, nome: nome, horario: horario});
                renderRemedios();
                document.getElementById('form-adicionar-remedio').style.display = 'none';
                this.reset();
            }
        });

        document.getElementById('editar-remedio-form').addEventListener('submit', function(event) {
            event.preventDefault();
            const id = parseInt(document.getElementById('id-medicamento').value);
            const nome = document.getElementById('nome-remedio-editar').value;
            const horario = document.getElementById('horario-remedio-editar').value;
            const remedio = remedios.find(r => r.id === id);
            if (remedio && nome && horario) {
                remedio.nome = nome;
                remedio.horario = horario;
                renderRemedios();
                document.getElementById('form-editar-remedio').style.display = 'none';
            }
        });

        // Load navigation from nav.html
        fetch('nav.html')
            .then(response => response.text())
            .then(data => {
                document.getElementById('nav-placeholder').innerHTML = data;
            });

        // Initial render
        renderRemedios();
    </script>
</body>
</html>
)rawliteral";

const char* nav_html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="UTF-8">
  <title>Painel</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css">
  <link rel="stylesheet" href="style.css">
</head>
<body>

  <div class="sidebar">
    <img src="imagens/logo caixa-temporizada-sem-fundo.png" alt="Logo Caixa Temporizada" class="sidebar-logo" />
    <div class="nav-buttons-container">
      <a href="main.html" class="nav-button">
        <i class="fa-solid fa-capsules"></i>
        <span>Meus remédios</span>
      </a>

      <a href="log.html" class="nav-button">
        <i class="fa-solid fa-notes-medical"></i>
        <span>Log</span>
      </a>

      <a href="configuracoes.html" class="nav-button">
        <i class="fa-solid fa-sliders"></i>
        <span>Configurações</span>
      </a>
    </div>
  </div>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", main_html);
  });

  server.on("/main.html", HTTP_GET, []() {
    server.send(200, "text/html", main_html);
  });

  server.on("/nav.html", HTTP_GET, []() {
    server.send(200, "text/html", nav_html);
  });

  server.on("/style.css", HTTP_GET, []() {
    File file = SPIFFS.open("/style.css", "r");
    if (!file) {
      server.send(404, "text/plain", "File not found");
      return;
    }
    server.streamFile(file, "text/css");
    file.close();
  });

  server.on("/settime", HTTP_GET, handleSetTime);

  server.begin();
  Serial.println("Servidor iniciado");
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
      } else {
        if (alarmeTocando) {
          noTone(buzzerPin);
          alarmeTocando = false;
        }
      }
    }
  }

  delay(100);
}

void handleSetTime() {
  if (server.hasArg("alarmtime")) {
    String t = server.arg("alarmtime");
    int hh = t.substring(0, 2).toInt();
    int mm = t.substring(3, 5).toInt();
    horarioProgramado = hh * 60 + mm;

    alarmeDesativadoAposAbrir = false;
    alarmeTocando = false;

    Serial.printf("Horario programado: %02d:%02d\n", hh, mm);

    String page = "<html><body>";
    page += "<h1>Horario salvo: " + t + "</h1>";
    page += "<a href=\"/\">Voltar</a>";
    page += "</body></html>";
    server.send(200, "text/html", page);
  } else {
    server.send(400, "text/plain", "Horario nao informado");
  }
}

void handleSetTime() {
  if (server.hasArg("alarmtime")) {
    String t = server.arg("alarmtime");
    int hh = t.substring(0, 2).toInt();
    int mm = t.substring(3, 5).toInt();
    horarioProgramado = hh * 60 + mm;

    alarmeDesativadoAposAbrir = false;
    alarmeTocando = false;

    Serial.printf("Horario programado: %02d:%02d\n", hh, mm);

    String page = "<html><body>";
    page += "<h1>Horario salvo: " + t + "</h1>";
    page += "<a href=\"/\">Voltar</a>";
    page += "</body></html>";
    server.send(200, "text/html", page);
  } else {
    server.send(400, "text/plain", "Horario nao informado");
  }
}
