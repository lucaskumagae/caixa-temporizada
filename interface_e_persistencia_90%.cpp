#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include "SPIFFS.h"
#include <vector>
#include <algorithm>

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
std::vector<String> logMessages; // vetor para armazenar mensagens do log

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
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #f4f7f8;
            margin: 0;
            padding: 0;
            color: #333;
        }
        #error-message {
            font-weight: bold;
        }
        .content {
            width: 100vw;
            height: 100vh;
            margin: 0 auto;
            background: white;
            padding: 2rem;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.1);
            display: flex;
            gap: 2rem;
            overflow-x: hidden;
            overflow-y: hidden;
            box-sizing: border-box;
        }
        body, html {
            overflow: hidden;
            margin: 0;
            padding: 0;
            height: 100%;
            width: 100%;
        }
        .medicines-container, .log-container {
            background: #fff;
            border-radius: 8px;
            box-shadow: 0 1px 6px rgba(0,0,0,0.1);
            padding: 1rem 1.5rem;
            flex: 1;
            display: flex;
            flex-direction: column;
            max-height: 600px;
            overflow-y: auto;
        }
        .medicines-container {
            max-width: 600px;
        }
        .log-container {
            max-width: 500px;
            background-color: #fff;
        }
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1.5rem;
        }
        .log-container h2 {
            margin: 0 0 1rem 0;
            font-size: 2rem;
            color: #2c3e50;
        }
        button.add-remedio {
            background-color: #3498db;
            border: none;
            color: white;
            padding: 0.6rem 1.2rem;
            font-size: 1rem;
            border-radius: 5px;
            cursor: pointer;
            transition: background-color 0.3s ease;
        }
        button.add-remedio:hover {
            background-color: #2980b9;
        }
        section {
            margin-bottom: 1.5rem;
        }
        form label {
            font-weight: 600;
            display: block;
            margin-bottom: 0.3rem;
            color: #34495e;
        }
        form input[type="text"],
        form input[type="time"] {
            width: 100%;
            padding: 0.5rem;
            margin-bottom: 1rem;
            border: 1px solid #ccc;
            border-radius: 4px;
            font-size: 1rem;
            box-sizing: border-box;
        }
        form button {
            background-color: #27ae60;
            border: none;
            color: white;
            padding: 0.5rem 1rem;
            font-size: 1rem;
            border-radius: 5px;
            cursor: pointer;
            margin-right: 0.5rem;
            transition: background-color 0.3s ease;
        }
        form button:hover {
            background-color: #1e8449;
        }
        form button#cancelar-adicionar,
        form button#cancelar-editar {
            background-color: #e74c3c;
        }
        form button#cancelar-adicionar:hover,
        form button#cancelar-editar:hover {
            background-color: #c0392b;
        }
        .lista-remedios .remedio {
            display: flex;
            align-items: center;
            justify-content: space-between;
            background-color: #ecf0f1;
            padding: 0.8rem 1rem;
            border-radius: 6px;
            margin-bottom: 0.8rem;
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
        }
        .lista-remedios .remedio .info {
            flex-grow: 1;
            margin-left: 1rem;
        }
        .lista-remedios .remedio strong {
            font-size: 1.1rem;
            color: #2c3e50;
        }
        .lista-remedios .remedio p {
            margin: 0.2rem 0 0 0;
            color: #7f8c8d;
            font-size: 0.9rem;
        }
        .lista-remedios .remedio button.excluir {
            background-color: #e74c3c;
            border: none;
            color: white;
            padding: 0.4rem 0.8rem;
            font-size: 0.9rem;
            border-radius: 5px;
            cursor: pointer;
            transition: background-color 0.3s ease;
        }
        .lista-remedios .remedio button.excluir:hover {
            background-color: #c0392b;
        }
        #confirm-excluir {
            border-radius: 8px;
            text-align: center;
        }
        #confirm-excluir p {
            font-size: 1.1rem;
            margin-bottom: 1rem;
            color: #2c3e50;
        }
        #confirm-excluir button {
            padding: 0.5rem 1rem;
            font-size: 1rem;
            border-radius: 5px;
            border: none;
            cursor: pointer;
            margin: 0 0.5rem;
            transition: background-color 0.3s ease;
        }
        #confirmar-excluir {
            background-color: #27ae60;
            color: white;
        }
        #confirmar-excluir:hover {
            background-color: #1e8449;
        }
        #cancelar-excluir {
            background-color: #e74c3c;
            color: white;
        }
        #cancelar-excluir:hover {
            background-color: #c0392b;
        }
        .log-messages {
            font-family: monospace;
            font-size: 0.9rem;
            color: #2c3e50;
            background-color: #dfe6e9;
            padding: 1rem;
            border-radius: 6px;
            height: 100%;
            overflow-y: auto;
            white-space: pre-line;
        }
    </style>
</head>
<body>
    <div id="error-message" style="display:none; position: fixed; top: 0; left: 0; right: 0; background-color: #f8d7da; color: #721c24; padding: 1em; border-bottom: 2px solid #f5c6cb; text-align: center; z-index: 1000;"></div>

    <main class="content">
        <div class="medicines-container">
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
        </div>

        <div class="log-container">
            <h2>Log</h2>
            <div class="log-messages">
                <!-- Log messages dinamicamente inseridas aqui -->
            </div>
        </div>
    </main>

    <div id="confirm-excluir" style="display:none; position: fixed; top: 50%; left: 50%; transform: translate(-50%, -50%);
        background: white; border: 1px solid #ccc; padding: 1em; z-index: 2000; box-shadow: 0 0 10px rgba(0,0,0,0.5); border-radius: 8px; text-align: center;">
        <p>Deseja realmente excluir este remédio?</p>
        <button id="confirmar-excluir">Confirmar</button>
        <button id="cancelar-excluir">Cancelar</button>
    </div>

    <script>
      let remedios = [];

      function renderRemedios() {
          const lista = document.getElementById('lista-remedios');
          lista.innerHTML = '';
          remedios.forEach(remedio => {
              const div = document.createElement('div');
              div.className = 'remedio';
              div.setAttribute('data-id', remedio.id);
              div.setAttribute('data-nome', remedio.nome);
              div.setAttribute('data-horario', remedio.horarios.join(", "));

              div.innerHTML = `
                  <div class="icon-placeholder"></div>
                  <div class="info">
                      <strong>${remedio.nome}</strong>
                      <p>Horários: ${remedio.horarios.join(", ")}</p>
                  </div>
                  <button class="excluir">Excluir</button>
              `;
              lista.appendChild(div);
          });
          attachEventListeners();
      }

      function attachEventListeners() {
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
                  // Opcional: implementar DELETE via fetch aqui
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
              fetch('/medicines', {
                  method: 'POST',
                  headers: {
                      'Content-Type': 'application/json'
                  },
                  body: JSON.stringify({
                      nome: nome,
                      horarios: [horario]
                  })
              })
              .then(response => {
                  if (response.ok) {
                      alert("Remédio adicionado com sucesso!");
                      document.getElementById('form-adicionar-remedio').style.display = 'none';
                      document.getElementById('novo-remedio-form').reset();
                      carregarRemedios();
                  } else {
                      alert("Erro ao adicionar remédio.");
                  }
              })
              .catch(() => {
                  alert("Erro de rede.");
              });
          }
      });

      function carregarRemedios() {
          fetch('/medicines')
              .then(res => res.json())
              .then(data => {
                  remedios = data;
                  renderRemedios();
              });
      }

      // Função para carregar o log do backend e atualizar a área de log
      function carregarLog() {
          fetch('/log')
            .then(res => res.json())
            .then(data => {
                const logMessagesDiv = document.querySelector('.log-messages');
                logMessagesDiv.innerHTML = data.join('<br>');
                // Scroll automático para o final ao atualizar o log
                logMessagesDiv.scrollTop = logMessagesDiv.scrollHeight;
            })
            .catch(() => {
                // Pode tratar erros aqui
            });
      }

      // Atualiza o log a cada 3 segundos
      setInterval(carregarLog, 3000);

      window.onload = () => {
          carregarRemedios();
          carregarLog();
      };
    </script>
</body>
</html>
)rawliteral";

void addLogMessage(const String& message) {
  logMessages.push_back(message);
  Serial.println(message);
  // Limitar tamanho máximo do vetor (exemplo 100 entradas)
  if (logMessages.size() > 100) {
    logMessages.erase(logMessages.begin());
  }
}

void salvarMedicines() {
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();
  for (auto &med : medicines) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = med.id;
    obj["nome"] = med.nome;
    JsonArray horarios = obj.createNestedArray("horarios");
    for (int h : med.horarios) {
      horarios.add(h);
    }
  }
  File file = SPIFFS.open("/medicines.json", "w");
  if (!file) {
    addLogMessage("Erro ao abrir /medicines.json para escrita");
    return;
  }
  serializeJson(doc, file);
  file.close();
  addLogMessage("Medicines salvos!");
}

void carregarMedicines() {
  if (!SPIFFS.exists("/medicines.json")) {
    addLogMessage("Nenhum arquivo de medicines encontrado.");
    return;
  }
  File file = SPIFFS.open("/medicines.json", "r");
  if (!file) {
    addLogMessage("Erro ao abrir /medicines.json para leitura");
    return;
  }
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    addLogMessage("Erro ao carregar JSON de medicines");
    return;
  }
  JsonArray arr = doc.as<JsonArray>();
  medicines.clear();
  for (JsonObject obj : arr) {
    Medicine med;
    med.id = obj["id"];
    med.nome = obj["nome"].as<String>();
    JsonArray horarios = obj["horarios"];
    for (int h : horarios) {
      med.horarios.push_back(h);
    }
    medicines.push_back(med);
  }
  addLogMessage("Medicines carregados!");
}

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  if (!SPIFFS.begin(true)) {
    addLogMessage("Erro ao montar SPIFFS");
  } else {
    addLogMessage("SPIFFS montado com sucesso");
  }

  carregarMedicines();

  WiFi.begin(ssid, password);
  addLogMessage("Conectando WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  addLogMessage("\nWiFi conectado!");
  addLogMessage("IP: " + WiFi.localIP().toString());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", main_html);
  });

  server.on("/main.html", HTTP_GET, []() {
    server.send(200, "text/html", main_html);
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

  server.on("/medicines", HTTP_GET, []() {
    DynamicJsonDocument doc(2048);
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
    salvarMedicines();
    server.send(200, "text/plain", "Medicine added");
  });

  // Endpoint para enviar as mensagens do log para o frontend
  server.on("/log", HTTP_GET, []() {
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& msg : logMessages) {
      arr.add(msg);
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });

  server.begin();
  addLogMessage("Servidor iniciado");
}

void loop() {
  server.handleClient();

  int estado = digitalRead(SENSOR_PIN);
  if (estado != ultimoEstado) {
    if (estado == LOW) {
      addLogMessage("Caixa fechada!");
    } else {
      addLogMessage("Caixa aberta!");
      if (alarmeTocando) {
        noTone(buzzerPin);
        alarmeTocando = false;
        alarmeDesativadoAposAbrir = true;
        addLogMessage("Alarme desativado apos abrir a caixa.");
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
              addLogMessage("Alarme tocando.");
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
