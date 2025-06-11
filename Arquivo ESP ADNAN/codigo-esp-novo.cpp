#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include "SPIFFS.h"

const char* ssid = "TP-LINK_0A24";
const char* password = "96813195";

#define SENSOR_PIN 23
const int buzzerPin = 19;

int ultimoEstado = HIGH;

WebServer server(80);

struct Medicine {
  int id;
  String nome;
  std::vector<int> horarios; // store alarm times in minutes from midnight
};

std::vector<Medicine> medicines;

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
        let medicines = [];

        function renderMedicines() {
            const lista = document.getElementById('lista-remedios');
            lista.innerHTML = '';
            medicines.forEach(med => {
                const div = document.createElement('div');
                div.className = 'remedio';
                div.setAttribute('data-id', med.id);
                div.setAttribute('data-nome', med.nome);
                div.setAttribute('data-horario', med.horarios.join(', '));

                div.innerHTML = `
                    <div class="icon-placeholder"></div>
                    <div class="info">
                        <strong>${med.nome}</strong>
                        <p>Horários: ${med.horarios.join(', ')}</p>
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
                    const horarios = remedioDiv.getAttribute('data-horario').split(',').map(h => h.trim());

                    document.getElementById('id-medicamento').value = id;
                    document.getElementById('nome-remedio-editar').value = nome;
                    document.getElementById('horario-remedio-editar').value = horarios[0] || '';

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
                    fetch('/medicines/' + remedioToDeleteId, { method: 'DELETE' })
                    .then(response => {
                        if (response.ok) {
                            loadMedicines();
                        }
                    });
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
                fetch('/medicines', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ nome: nome, horarios: [horario] })
                }).then(response => {
                    if (response.ok) {
                        loadMedicines();
                        document.getElementById('form-adicionar-remedio').style.display = 'none';
                        this.reset();
                    }
                });
            }
        });

        document.getElementById('editar-remedio-form').addEventListener('submit', function(event) {
            event.preventDefault();
            const id = parseInt(document.getElementById('id-medicamento').value);
            const nome = document.getElementById('nome-remedio-editar').value;
            const horario = document.getElementById('horario-remedio-editar').value;
            if (nome && horario) {
                fetch('/medicines/' + id, {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ nome: nome, horarios: [horario] })
                }).then(response => {
                    if (response.ok) {
                        loadMedicines();
                        document.getElementById('form-editar-remedio').style.display = 'none';
                    }
                });
            }
        });

        function loadMedicines() {
            fetch('/medicines')
                .then(response => response.json())
                .then(data => {
                    medicines = data;
                    renderMedicines();
                });
        }

        // Load navigation from nav.html
        fetch('nav.html')
            .then(response => response.text())
            .then(data => {
                document.getElementById('nav-placeholder').innerHTML = data;
            });

        // Initial load
        loadMedicines();
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

  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS");
  } else {
    Serial.println("SPIFFS montado com sucesso");
  }

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

  // API endpoints for medicines
  server.on("/medicines", HTTP_GET, []() {
    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.to<JsonArray>();
    for (auto &med : medicines) {
      JsonObject obj = arr.createNestedObject();
      obj["id"] = med.id;
      obj["nome"] = med.nome;
      JsonArray horariosArr = obj.createNestedArray("horarios");
      for (int h : med.horarios) {
        int hh = h / 60;
        int mm = h % 60;
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
        horariosArr.add(String(buf));
      }
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });

  server.on("/medicines", HTTP_POST, []() {
    if (server.hasArg("plain") == false) {
      server.send(400, "text/plain", "Body not received");
      return;
    }
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      server.send(400, "text/plain", "Invalid JSON");
      return;
    }
    String nome = doc["nome"];
    JsonArray horariosArr = doc["horarios"].as<JsonArray>();
    std::vector<int> horariosVec;
    for (JsonVariant v : horariosArr) {
      String t = v.as<String>();
      int hh = t.substring(0, 2).toInt();
      int mm = t.substring(3, 5).toInt();
      horariosVec.push_back(hh * 60 + mm);
    }
    int newId = medicines.empty() ? 1 : medicines.back().id + 1;
    medicines.push_back({newId, nome, horariosVec});
    server.send(200, "text/plain", "Medicine added");
  });

  server.on("/medicines", HTTP_OPTIONS, []() {
    server.send(200);
  });

  server.on("/medicines/", HTTP_DELETE, []() {
    // This will not be called because path is /medicines/:id, so we handle below
    server.send(400, "text/plain", "ID not specified");
  });

  server.on("/medicines/", HTTP_PUT, []() {
    // This will not be called because path is /medicines/:id, so we handle below
    server.send(400, "text/plain", "ID not specified");
  });

  // Handle dynamic routes for PUT and DELETE with ID
  server.onNotFound([]() {
    String uri = server.uri();
    if (uri.startsWith("/medicines/")) {
      String idStr = uri.substring(strlen("/medicines/"));
      int id = idStr.toInt();
      if (server.method() == HTTP_DELETE) {
        auto it = std::find_if(medicines.begin(), medicines.end(), [id](const Medicine& m) { return m.id == id; });
        if (it != medicines.end()) {
          medicines.erase(it);
          server.send(200, "text/plain", "Medicine deleted");
        } else {
          server.send(404, "text/plain", "Medicine not found");
        }
      } else if (server.method() == HTTP_PUT) {
        if (server.hasArg("plain") == false) {
          server.send(400, "text/plain", "Body not received");
          return;
        }
        String body = server.arg("plain");
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
          server.send(400, "text/plain", "Invalid JSON");
          return;
        }
        String nome = doc["nome"];
        JsonArray horariosArr = doc["horarios"].as<JsonArray>();
        std::vector<int> horariosVec;
        for (JsonVariant v : horariosArr) {
          String t = v.as<String>();
          int hh = t.substring(0, 2).toInt();
          int mm = t.substring(3, 5).toInt();
          horariosVec.push_back(hh * 60 + mm);
        }
        auto it = std::find_if(medicines.begin(), medicines.end(), [id](const Medicine& m) { return m.id == id; });
        if (it != medicines.end()) {
          it->nome = nome;
          it->horarios = horariosVec;
          server.send(200, "text/plain", "Medicine updated");
        } else {
          server.send(404, "text/plain", "Medicine not found");
        }
      } else {
        server.send(405, "text/plain", "Method not allowed");
      }
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });

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

  if (!medicines.empty()) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      int agora = timeinfo.tm_hour * 60 + timeinfo.tm_min;

      bool alarmTriggered = false;
      for (const auto& med : medicines) {
        for (int h : med.horarios) {
          if (agora == h && !alarmeDesativadoAposAbrir) {
            if (estado == LOW && !alarmeTocando) {
              tone(buzzerPin, 1000);
              alarmeTocando = true;
              Serial.println("Alarme tocando.");
              alarmTriggered = true;
              break;
            }
          }
        }
        if (alarmTriggered) break;
      }
      if (!alarmTriggered && alarmeTocando) {
        noTone(buzzerPin);
        alarmeTocando = false;
      }
    }
  }

  delay(100);
}
