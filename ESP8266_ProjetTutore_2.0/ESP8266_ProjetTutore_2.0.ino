#include <ArduinoOTA.h>
#include <RTC_DS1302.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define ssid "DESKTOP-8NRTH60 va"
#define password "12345678900"

#define RTC_CLK D5
#define RTC_DAT D6
#define RTC_RST D7

#define pin_relais D8

ESP8266WebServer server(80);
DS1302 rtc(RTC_RST, RTC_CLK, RTC_DAT);

uint8_t rtc_secondes;
uint8_t rtc_minutes;
uint8_t rtc_heures;

unsigned long present_time = 0;

// ====== DÉCLARATIONS : PLAGES & OUTILS TEMPS ======
struct PlageHoraire {
  uint8_t heure_debut;
  uint8_t minute_debut;
  uint8_t heure_fin;
  uint8_t minute_fin;
};

const uint8_t MAX_PLAGES = 8;
uint8_t nPlages = 2;  // nombre réel de plages actives
PlageHoraire plages[MAX_PLAGES] = {
  {12, 26, 12, 27},
  {12, 29, 12, 30}
};

// Override manuel (optionnel)
bool overrideActive = false;
uint16_t overrideStartMin = 0;
uint16_t overrideEndMin   = 0;

// LittleFS
const char* fichierPlages = "/plages.json";

// Petits utilitaires
inline uint16_t toMinutes(uint8_t h, uint8_t m) { return h * 60 + m; }

bool inWindow(uint16_t now, uint16_t start, uint16_t end) {
  if (start <= end) return now >= start && now <= end;
  else return (now >= start) || (now <= end);
}

// ====== LOGIQUE PRINCIPALE : ACTIVER LE RELAIS ? ======

void handleOverride() {
  if (server.hasArg("minutes")) {
    int duree = server.arg("minutes").toInt();
    if (duree > 0) {
      uint16_t now = toMinutes(rtc_heures, rtc_minutes);
      overrideStartMin = now;
      overrideEndMin   = now + duree;
      overrideActive   = true;

      // 🔄 Redirection vers la page d'accueil
      server.sendHeader("Location", "/");
      server.send(303);
    } else {
      server.send(400, "text/plain", "Durée invalide !");
    }
  } else {
    server.send(400, "text/plain", "Argument 'minutes' manquant !");
  }
}


bool activerRelais() {
  if (rtc_heures > 23 || rtc_minutes > 59) return false;

  uint16_t now = toMinutes(rtc_heures, rtc_minutes);

  if (overrideActive) {
    if (inWindow(now, overrideStartMin, overrideEndMin)) return true;
    else overrideActive = false;
  }

  for (uint8_t i = 0; i < nPlages; i++) {
    uint16_t start = toMinutes(plages[i].heure_debut, plages[i].minute_debut);
    uint16_t end   = toMinutes(plages[i].heure_fin,   plages[i].minute_fin);
    if (inWindow(now, start, end)) return true;
  }
  return false;
}

void test() {
  if (activerRelais()) {
    digitalWrite(pin_relais, true);
    Serial.println("Allumer");
  } else {
    digitalWrite(pin_relais, false);
    Serial.println("Fermer");
  }
  Serial.printf("\n%02d:%02d:%02d\n", rtc_heures, rtc_minutes, rtc_secondes);
}

// ====== LITTLEFS : SAUVEGARDE & CHARGEMENT ======
void sauvegarderPlages() {
  StaticJsonDocument<512> doc;
  JsonArray arr = doc.to<JsonArray>();

  for (uint8_t i = 0; i < nPlages; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["debut_heure"] = plages[i].heure_debut;
    obj["debut_minute"] = plages[i].minute_debut;
    obj["fin_heure"] = plages[i].heure_fin;
    obj["fin_minute"] = plages[i].minute_fin;
  }

  File file = LittleFS.open(fichierPlages, "w");
  if (!file) {
    Serial.println("Erreur ouverture fichier pour écriture");
    return;
  }
  serializeJson(doc, file);
  file.close();
}

bool chargerPlages() {
  if (!LittleFS.exists(fichierPlages)) return false;

  File file = LittleFS.open(fichierPlages, "r");
  if (!file) return false;

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) return false;

  JsonArray arr = doc.as<JsonArray>();
  nPlages = 0;
  for (JsonObject obj : arr) {
    if (nPlages >= MAX_PLAGES) break;
    plages[nPlages].heure_debut  = obj["debut_heure"];
    plages[nPlages].minute_debut = obj["debut_minute"];
    plages[nPlages].heure_fin    = obj["fin_heure"];
    plages[nPlages].minute_fin   = obj["fin_minute"];
    nPlages++;
  }
  return true;
}

// ====== HANDLERS WEB ======
void handleAdd() {
  if (nPlages >= MAX_PLAGES) {
    server.send(400, "text/plain", "Nombre maximum de plages atteint !");
    return;
  }
  if (server.hasArg("h") && server.hasArg("m") && server.hasArg("hf") && server.hasArg("mf")) {
    plages[nPlages].heure_debut = server.arg("h").toInt();
    plages[nPlages].minute_debut = server.arg("m").toInt();
    plages[nPlages].heure_fin   = server.arg("hf").toInt();
    plages[nPlages].minute_fin  = server.arg("mf").toInt();
    nPlages++;
    sauvegarderPlages();
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(400, "text/plain", "Arguments manquants !");
  }
}

void handleDel() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < nPlages) {
      for (int i = id; i < nPlages - 1; i++) plages[i] = plages[i + 1];
      nPlages--;
      sauvegarderPlages();
      server.sendHeader("Location", "/"); 
      server.send(303);
    } else server.send(400, "text/plain", "ID invalide !");
  } else server.send(400, "text/plain", "Argument manquant !");
}

void handleEdit() {
  if (server.hasArg("id") && server.hasArg("h") && server.hasArg("m") && server.hasArg("hf") && server.hasArg("mf")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < nPlages) {
      plages[id].heure_debut = server.arg("h").toInt();
      plages[id].minute_debut = server.arg("m").toInt();
      plages[id].heure_fin   = server.arg("hf").toInt();
      plages[id].minute_fin  = server.arg("mf").toInt();
      sauvegarderPlages();
      server.sendHeader("Location", "/");
      server.send(303);
    } else server.send(400, "text/plain", "ID invalide !");
  } else server.send(400, "text/plain", "Arguments manquants !");
}

// ====== PAGE WEB AVEC AFFICHAGE HEURE RTC ======
void page_d_accueil() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<title>Contrôle Télévision</title>
<style>
  body { font-family: Arial, sans-serif; background: #f0f0f0; padding: 20px; }
  h1, h2 { color: #333; }
  ul { list-style-type: none; padding: 0; }
  li { margin: 5px 0; }
  .time { font-weight: bold; font-size: 1.2em; }
  form input { margin: 2px; }
</style>
</head>
<body>
<h1>Plages horaires actuelles</h1>
<ul>
)rawliteral";

  for (uint8_t i = 0; i < nPlages; i++) {
    page += "<li>";
    page += String(plages[i].heure_debut) + "h" + String(plages[i].minute_debut) + " → ";
    page += String(plages[i].heure_fin)   + "h" + String(plages[i].minute_fin);
    page += " <a href='/del?id=" + String(i) + "'>❌ Supprimer</a>";
    page += "</li>";
  }

  page += R"rawliteral(
</ul>
<h2>Ajouter une plage</h2>
<form action='/add' method='GET'>
Début : <input type='number' name='h' min='0' max='23'>:
<input type='number' name='m' min='0' max='59'> Fin :
<input type='number' name='hf' min='0' max='23'>:
<input type='number' name='mf' min='0' max='59'>
<input type='submit' value='Ajouter'>
</form>

<h2>Heure actuelle RTC</h2>
<p class="time">)rawliteral";

  page += String(rtc_heures) + ":" + (rtc_minutes < 10 ? "0" : "") + String(rtc_minutes) + ":" + (rtc_secondes < 10 ? "0" : "") + String(rtc_secondes);

  page += R"rawliteral(
</p>
<form action='/override' method='GET'>
Allumer la TV pendant (minutes) :
<input type='number' name='minutes' min='1' max='180'>
<input type='submit' value='Activer'>
</form>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", page);
}


// ====== SETUP & LOOP ======
void setup() {
  Serial.begin(115200);
  rtc.init();
  pinMode(pin_relais, OUTPUT);

  WiFi.begin(ssid, password);
  if (WiFi.status() != WL_CONNECTED) {
    // delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connecté !");
  Serial.print("Adresse IP : "); Serial.println(WiFi.localIP());

  // Initialisation LittleFS et lecture des plages
  if (!LittleFS.begin()) Serial.println("Erreur LittleFS");
  else if (!chargerPlages()) Serial.println("Utilisation des plages par défaut");

  server.on("/", page_d_accueil);
  server.on("/add", handleAdd);
  server.on("/del", handleDel);
  server.on("/edit", handleEdit);
  server.on("/override", handleOverride);
  server.begin();
  ArduinoOTA.begin();
}

void loop() {
  present_time = millis();
  ArduinoOTA.handle();
  server.handleClient();
  rtc.getDateTime();
  rtc_secondes = rtc.second;
  rtc_minutes  = rtc.minute;
  rtc_heures   = rtc.hour;
  test();
  if (WiFi.status() != WL_CONNECTED) {
    // delay(500);
    Serial.print(".");
  }
}
