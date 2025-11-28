//VATSU
//====== DECLARATION : BIBLIOTHEQUES =====
#include <ArduinoOTA.h>           // Permet la mise à jour du code via WiFi (OTA = Over The Air)
#include <RTC_DS1302.h>           // Gère le module RTC DS1302 (horloge temps réel)
#include <ESP8266WiFi.h>          // Gestion du WiFi pour l'ESP8266
#include <ESP8266WebServer.h>     // Serveur web embarqué sur ESP8266
#include <LittleFS.h>             // Système de stockage permanent interne de l’ESP8266
#include <ArduinoJson.h>          // Librairie pour manipuler des fichiers JSON

// Identifiants WiFi (ici inutilisés car tu es en mode AP)
#define ssid "DESKTOP-8NRTH60 va"
#define password "12345678900"

// Broches utilisées par le module RTC DS1302
#define RTC_CLK D5
#define RTC_DAT D6
#define RTC_RST D7

// Broches utilisées pour le relais et la LED
#define pin_relais D8
#define pin_led    D3

ESP8266WebServer server(80);     // Déclaration d’un serveur web sur le port 80
DS1302 rtc(RTC_RST, RTC_CLK, RTC_DAT); // Instance pour piloter l’horloge RTC

// Variables pour stocker l’heure lue depuis le RTC
uint8_t rtc_secondes;
uint8_t rtc_minutes;
uint8_t rtc_heures;

unsigned long present_time = 0;  // Stocke le temps en millisecondes depuis le démarrage

 // Structure pour stocker une plage horaire (heure de début/fin)
struct PlageHoraire {              // Définition d'une structure appelée "PlageHoraire"
  uint8_t heure_debut;             // Heure de début de la plage (nombre entier 0 à 23)
  uint8_t minute_debut;            // Minute de début de la plage (nombre entier 0 à 59)
  uint8_t heure_fin;               // Heure de fin de la plage (nombre entier 0 à 23)
  uint8_t minute_fin;              // Minute de fin de la plage (nombre entier 0 à 59)
};

const uint8_t MAX_PLAGES = 8;      // Nombre maximum de plages horaires  (ici 8)
uint8_t nPlages = 2;               // Nombre actuel de plages configurées (ici 2 plage)
PlageHoraire plages[MAX_PLAGES] = { // Création d’un tableau qui contient toutes les plage
  {12, 26, 12, 27},                // 1ère plage : commence à 12h26 et finit à 12h27
  {12, 29, 12, 30}                 // 2ème plage : commence à 12h29 et finit à 12h30
};                                 // Les 6 autres plages ne sont pas encore utilisées


  // Override manuel (optionnel)
bool overrideActive = false;   // Vrai si une activation manuelle est en cours
uint16_t overrideStartMin = 0; // Début en minutes
uint16_t overrideEndMin   = 0; // Fin en minutes

  // LittleFS
const char* fichierPlages = "/plages.json";  // Nom du fichier JSON où sont stockées les plages

// Convertit une heure et minute en minutes totales (ex: 2h30 => 150 minutes)
inline uint16_t toMinutes(uint8_t h, uint8_t m) { return h * 60 + m; }

// Vérifie si un instant (now) est dans une plage (start -> end)
// Fonction qui vérifie si l’heure actuelle (now) est comprise dans une plage horaire
bool inWindow(uint16_t now, uint16_t start, uint16_t end) { 
  if (start <= end)                        // Cas normal : la plage horaire ne dépasse pas minuit
    return now >= start && now <= end;     // → Vrai si "now" est compris entre "start" et "end"
  else                                     // Cas spécial : la plage horaire traverse minuit (ex : 22h → 02h)
    return (now >= start) || (now <= end); // → Vrai si "now" est après "start" ou avant "end"
}


  // ====== LOGIQUE PRINCIPALE : ACTIVER LE RELAIS ? ======

  void handleOverride() {
  if (server.hasArg("minutes")) {                   // Vérifie si l’utilisateur a donné un temps
    int duree = server.arg("minutes").toInt();      // Convertit en entier
    if (duree > 0) {                                // Durée valide
      uint16_t now = toMinutes(rtc_heures, rtc_minutes);
      overrideStartMin = now;                       // Enregistre l’heure de début
      overrideEndMin   = now + duree;               // Enregistre l’heure de fin
      overrideActive   = true;                      // Active le mode override
      server.sendHeader("Location", "/");           // Redirection vers la page d’accueil
      server.send(303);
    } else {
      // Si la durée est invalide
      server.send(400, "text/html", "<h1>Duree invalide !</h1> <a href='/'>Revenir a la page d'acceuil");
    }
  } else {
    // Si aucun paramètre minutes n’a été fourni
    server.send(400, "text/plain", "Argument 'minutes' manquant !");
  }
}

bool activerRelais() {                                       // Fonction qui détermine si le relais (appareil) doit être activé ou non
  if (rtc_heures > 23 || rtc_minutes > 59) return false;     // Sécurité : si l'heure ou minute est invalide, on n’active pas le relais
  uint16_t now = toMinutes(rtc_heures, rtc_minutes);         // Convertit l’heure actuelle en minutes depuis minuit

  // Cas override
  if (overrideActive) {                                       // Si le mode override est actif
    if (inWindow(now, overrideStartMin, overrideEndMin))      // Vérifie si l’heure actuelle est dans la plage de l’override
      return true;                                           // Si oui, relais activé
    else 
      overrideActive = false;                                 // Sinon, le mode override se termine automatiquement
  }

  // Vérifie si l’heure actuelle est dans une plage horaire configurée
  for (uint8_t i = 0; i < nPlages; i++) {                   // Parcourt toutes les plages configurées
    uint16_t start = toMinutes(plages[i].heure_debut, plages[i].minute_debut); // Début de la plage en minutes
    uint16_t end   = toMinutes(plages[i].heure_fin,   plages[i].minute_fin);   // Fin de la plage en minutes
    if (inWindow(now, start, end)) return true;              // Si l’heure actuelle est dans cette plage, relais activé
  }
  return false;                                              // Si aucune condition n’est remplie, le relais reste éteint
}


// Sauvegarde les plages horaires actuelles dans un fichier JSON
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
    Serial.println("Erreur ouverture fichier pour ecriture");
    return;
  }
  serializeJson(doc, file); // Écrit le JSON dans le fichier
  file.close();
}

// Charge les plages depuis le fichier JSON
bool chargerPlages() {                                     // Fonction pour charger les plages horaires depuis le fichier sur l’ESP8266
  if (!LittleFS.exists(fichierPlages)) return false;      // Vérifie si le fichier existe : si non, retourne false (rien à charger)

  File file = LittleFS.open(fichierPlages, "r");          // Ouvre le fichier en lecture
  if (!file) return false;                                // Si le fichier ne s'ouvre pas, retourne false

  StaticJsonDocument<512> doc;                             // Crée un document JSON en mémoire (512 octets)
  DeserializationError error = deserializeJson(doc, file); // Lit le JSON depuis le fichier et le transforme en structure utilisable
  file.close();                                           // Ferme le fichier après lecture
  if (error) return false;                                // Si erreur pendant la lecture JSON, retourne false

  JsonArray arr = doc.as<JsonArray>();                     // Transforme le document JSON en tableau
  nPlages = 0;                                            // Réinitialise le nombre de plages chargées à zéro
  for (JsonObject obj : arr) {                             // Parcourt chaque objet (chaque plage) dans le tableau JSON
    if (nPlages >= MAX_PLAGES) break;                     // Ne dépasse pas le nombre maximum de plages autorisées
    plages[nPlages].heure_debut  = obj["debut_heure"];    // Récupère l’heure de début de la plage
    plages[nPlages].minute_debut = obj["debut_minute"];   // Récupère la minute de début
    plages[nPlages].heure_fin    = obj["fin_heure"];      // Récupère l’heure de fin
    plages[nPlages].minute_fin   = obj["fin_minute"];     // Récupère la minute de fin
    nPlages++;                                            // Incrémente le compteur de plages chargées
  }
  return true;                                            // Retourne true si tout s'est bien passé
}

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

  void handleTime() {
    String heure = String(rtc_heures) + ":" +
                  (rtc_minutes < 10 ? "0" : "") + String(rtc_minutes) + ":" +
                  (rtc_secondes < 10 ? "0" : "") + String(rtc_secondes);

    server.send(200, "text/plain", heure);
  }

  void handleRun(){
    String allumer = activerRelais() ? "Appareil allumer" : "Allumer l'appareil";
    server.send(200, "text/plain", allumer);
  }

  // ====== PAGE WEB AVEC AFFICHAGE HEURE RTC ======
  void page_d_accueil() {
    String page = R"rawliteral(
  <!DOCTYPE html>
  <html lang='fr'>     
  <head>
      <meta charset='UTF-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>Contrôle Television Reglemente</title> 
  </head>
  <style>
  /* Variables CSS pour les couleurs et animations */
  :root {
    --background: #444444;
    --primary: #9598bf;
    /* --primary: #2b2d42; */
    --secondary: #8d99ae;
    --accent: #9598bf;
    --success: #8d99ae; 
    /* --bg: #edf2f4; */
    --bg: #272828;
    --text: #222;
    --shadow: 0 4px 24px rgba(44, 62, 80, 0.15);
  }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Segoe UI', Arial, sans-serif;
    margin: 0;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    transition: background 0.5s;
  }

  .container { 
    background: var(--background);
    box-shadow:0 0 10px var(--primary);
    border-radius: 20px;
    padding: 2rem 2.5rem;
    margin-top: 3rem;
    width: 100%;
    max-width: 480px;
    animation: fadeIn 1.2s cubic-bezier(.42,0,.58,1); 
  }
  .timePanel{
    display: flex;
    justify-content: space-evenly;
  } 
  #allowedRange{
    display: flex;
    flex-direction: column;
  }

  @keyframes fadeIn {
    from { opacity: 0; transform: translateY(40px); }
    to { opacity: 1; transform: translateY(0); }
  }

  h1 {
    color: var(--primary);
    text-align: center;
    margin-bottom: 1.2rem;
    font-size: 2.2rem;
    letter-spacing: 2px;
    animation: popIn 1s;
  }

  @keyframes popIn {
    0% { transform: scale(0.8); opacity: 0; }
    80% { transform: scale(1.1); opacity: 1; }
    100% { transform: scale(1); }
  }

  .time-section, .control-section, .stats-section {
    margin-bottom: 2rem;
    display: flex;
    flex-direction: column;
    gap: 0.7rem;
  }
  .plageHoraire{
    display: flex;
    justify-content: space-between;
  }
  .modifierPlageHoraire{
    display: flex;
    align-items: center;
    color: var(--primary);
    border: .1px solid var(--primary);
    border-radius: 50%;
    height: 40px;
    padding: 0 .7rem;
    transform: scale(.65);
    transition: .2s;
    user-select: none;
  }

  .modifierPlageHoraire > span {
    transform: scale(1.4);
  }

  .modifierPlageHoraire:hover{
    transform: scale(.8);
    cursor: pointer;
    box-shadow:  0 0 10px var(--primary);
    background-color: var(--primary);
    color: var(--bg);
    font-weight: 700;
  }

  .label {
    font-weight: 600;
    color: var(--secondary);
    margin-bottom: 0.2rem;
  }input[type='number']{
    text-align: center;
    width: 39%;
  }

  input[type='number'], input[type='time'] {
    background-color: var(--background);
    color: var(--primary);
    padding: 0.5rem 1rem;
    border-radius: 8px;
    border: 1px solid var(--secondary);
    font-size: 1.1rem;
    transition: border 0.3s;
  }
  input:focus {
    border-color: var(--text);
    color: var(--text);
    outline: none;
  }

  button {
    width: 100%;
    background: var(--background);
    color: var(--accent);
    border: .5px solid var(--accent);
    border-radius: 8px;
    padding: 0.7rem 1.5rem;
    font-size: 1.1rem;
    font-weight: 600;
    cursor: pointer;
    box-shadow: var(--shadow);
    transition: background 0.5s, transform 0.2s;
    margin: 4% 0 0 0;
  }
  button:hover{
    background-color: var(--success);
    color: var(--background);
  }
  button:active {
    background: var(--primary);
    transform: scale(0.97);
  }

  .status {
    font-size: 1.2rem;
    font-weight: 700;
    color: var(--success);
    margin-top: 0.5rem;
    animation: pulse 2s infinite;
  }

  @keyframes pulse {
    0% { color: var(--success); }
    30% { color: var(--accent); }
    100% { color: var(--success); }
  }

  .countdown, .total-on {
    font-size: 1.5rem;
    font-weight: 700;
    color: var(--primary);
    text-align: center;
    margin-top: 0.5rem;
    letter-spacing: 1px;
    animation: bounce 1.2s infinite alternate;
  }

  .minutes, #powerBtn{
    margin: 1% 5%;
  }

  .plageHoraireEdit{
    display: flex;
    flex-direction: column;
    text-align: center;
  }

  #editPlages{
    display: none;
    color: var(--primary);
    position:absolute;
    top: 48px;
    left: 50%;
    transform: translateX( -50%);
    background-color: var(--background);
    padding: 1rem;
    border-radius: 8px;
    box-shadow:0 0 10px var(--primary);
    transition: 1s;
    animation: fadenIn  1.2s cubic-bezier(.42,0,.58,1);
  }

  #editPlages > div > span{
    font-size: 1.5rem;
    font-weight: 450; 
  }

  #editPlages > div > span a{
    text-decoration: none;
    color: red;
    margin: 0 0 0 1rem;
    font-size: 1.8rem;
    animation: delet 2s infinite forwards cubic-bezier(.42,0,.58,1);
  }

  @keyframes delet {
    0%,100%{ text-shadow:0 0 0 red;}
    50%{text-shadow:0 0 20px red;}
    
  }
  #editPlages > div > form > fieldset{
    padding: 1rem 3rem ;
    display: flex;
    flex-direction: column;
    align-items: end;
    margin: 0 auto;
    gap:.5rem;
  }
  fieldset{ 
    color: var(--primary);
    border: .4px solid var(--primary);
    border-radius: 15px;
  }

  legend{
    margin-left: -36%;
    border: .5px solid var(--primary);
    border-radius: 5px;
    padding: 0 .2rem .2rem .2rem;
  }

  #editPlages > div > form > fieldset > div > span{
    font-size: 1.3rem;
    font-weight: 400;
  }

  #editPlages > div > form > fieldset > div > input[type=number]{
    width: 40px;
    /* padding: auto 0 auto 0; */
    /* text-align: left; */
    height: 10px;
    scrollbar-width: none; 
  }

  #editPlages > div > form > fieldset > input{
    width: 70px;
    background: var(--background);
    color: var(--accent);
    border: .5px solid var(--accent);
    border-radius: 8px;
    padding: 0.3rem 0 ;
    font-size: 1.1rem;
    cursor: pointer;
    box-shadow: var(--shadow);
    transition: background 0.5s, transform 0.2s;
    margin: .1rem auto;  
  }
  #editPlages > div > form > fieldset > input:hover{
    background-color: var(--success);
    color: var(--background);
  }
  #editPlages > div > form > fieldset > input:active {
    background: var(--primary);
    transform: scale(0.97);
  }

  #closeEdit{
    display: flex;
    justify-content: center;
    font-size: 1.3rem;
    border-radius: 15px;
    text-align: center;
    padding: 0 auto;
    height: 50px; 
    margin:0 auto;
  }

  @keyframes bounce {
    0% { transform: translateY(0); }
    100% { transform: translateY(-8px); }
  }

  .range {
    display: flex;
    gap: 1rem;
    align-items: center;
  }

  ::-webkit-scrollbar {
    width: 5px;
    background: var(--bg);
    transition: width .5s;
  }

  ::-webkit-scrollbar-thumb {
    background: var(--accent);
    border-radius: 9px;
  }
  ::-webkit-scrollbar:hover{ 
    width: 12px;
  }
  @media screen and (max-width: 470px) {
    .timePanel{
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    
  }
  </style>

  <body> 
      <div class='container' id="container">
          <div></div>
          <h1>Contrôle Appareil</h1>
          <div class='timePanel'>

              <div class='time-section'>
                  <span class='label'>Heure actuelle :</span>
                  <div id='currentTime' class='countdown'></div>
              </div>
              <div class='control-section'>
                  <span class='label'>Plage horaire autorisee :</span>
                  <div class='plageHoraire'>
                      <div id='allowedRange' style='font-weight:700; color:var(--primary);'>
                          )rawliteral";
                          for (uint8_t i = 0; i < nPlages; i++) {
                            page += "<span>";
                            page += String(plages[i].heure_debut) + "h" + String(plages[i].minute_debut) + " → ";
                            page += String(plages[i].heure_fin)   + "h" + String(plages[i].minute_fin);
                            page += "</span>";
                          }
                          page += R"rawliteral(
                      </div>
                      <span class='modifierPlageHoraire' id='openEdit'><span>🖊</span></span>
                  </div>
              </div>

          </div>
          <form action='/override' method='GET' class='minutes'>
              <span class='label'>Allumer pendant (minutes) :</span>
              <input type='number' name='minutes' id='minutesInput' min='1' max='180' placeholder='Duree en minutes'>
              <button id='powerBtn' type="submit">Allumer la l'appareil</button>
          </form> 
      </div>

      <div id='editPlages'>
          <h2>Modifier les plages horaires</h2>
          <div class='plageHoraireEdit'>)rawliteral";

    for (uint8_t i = 0; i < nPlages; i++) {
      page += "<span>";
      page += String(plages[i].heure_debut) + "h" + String(plages[i].minute_debut) + " → ";
      page += String(plages[i].heure_fin)   + "h" + String(plages[i].minute_fin);
      page += " <a href='/del?id=" + String(i) + "'>🗑</a>";
      page += "</span>";
    }

    page += R"rawliteral(
              <form action='/add' method='GET'>
                  <fieldset>
                      <legend>ajouter plage</legend>

                      <div>
                          <span>Debut :</span> 
                          <input type='number' name='h' min='0' max='23' placeholder="H"> <span>:</span>
                          <input type='number' name='m' min='0' max='59' placeholder="M"> <br>
                      </div>
                      <div>
                          <span>Fin : </span>
                          <input type='number' name='hf' min='0' max='23' placeholder="H"> <span>:</span>
                          <input type='number' name='mf' min='0' max='59' placeholder="M"><br>
                      </div>
                      <input type='submit' value='Ajouter'>
                  </fieldset>
              </form>
          </div>
          <span class='modifierPlageHoraire' id='closeEdit'>Fermer</span>
      </div>
  </body>
  <script>
  const openEdit = document.getElementById('openEdit');
  const closeEdit = document.getElementById('closeEdit');
  const editDisplay = document.getElementById('editPlages');
  const homeDisplay = document.getElementById('container');

  // ⚡ Recuperation heure depuis l’ESP toutes les secondes
  function updateTime() {
    fetch('/time')
      .then(response => response.text())
      .then(data => {
        document.getElementById('currentTime').textContent = data;
      })
      .catch(err => console.error("Erreur maj heure :", err));
  }
  setInterval(updateTime, 1000);

  function updateText() {
    fetch('/allumer')
      .then(response => response.text())
      .then(data => {
        document.getElementById('powerBtn').textContent = data;
      }).catch(err => console.error("Erreur text :", err));
  }
  setInterval(updateText, 1000);

  // Gestion de l’edition des plages
  openEdit.addEventListener('click', ()=>{
    editDisplay.style.display = 'block';
    homeDisplay.style.filter = 'blur(15px)';
    homeDisplay.style.cursor = 'not-allowed';
  });

  closeEdit.addEventListener('click', ()=>{
    editDisplay.style.display = 'none';
    homeDisplay.style.filter = 'none';
  });

  </script>

  </html> 
  )rawliteral";

    server.send(200, "text/html", page);
  }

void setup() {
  Serial.begin(115200);
  rtc.init();
  pinMode(pin_relais, OUTPUT);
  pinMode(pin_led, OUTPUT);
  digitalWrite(pin_relais, false);
  digitalWrite(pin_led, false);


  const char* ap_ssid = "NodeMCU_Control";   // nom du WiFi créé par la carte
  const char* ap_password = "12345678";      // mot de passe min 8 caractères
  
  WiFi.mode(WIFI_AP);                        // mettre la carte en mode Point d’Accès
  WiFi.softAP(ap_ssid, ap_password);         // créer le réseau WiFi
  Serial.println("Point d'accès actif !");
  Serial.print("Nom du WiFi : "); Serial.println(ap_ssid);
  Serial.print("Mot de passe : "); Serial.println(ap_password);
  Serial.print("Adresse IP : "); Serial.println(WiFi.softAPIP());

  // Initialisation LittleFS et lecture des plages
  if (!LittleFS.begin()) Serial.println("Erreur LittleFS");
  else if (!chargerPlages()) Serial.println("Utilisation des plages par defaut");

  // Déclaration des routes
  server.on("/add", handleAdd);
  server.on("/del", handleDel);
  server.on("/edit", handleEdit);
  server.on("/time", handleTime);
  server.on("/", page_d_accueil);
  server.on("/allumer", handleRun);
  server.on("/override", handleOverride);

  server.begin();
  ArduinoOTA.begin();
  ArduinoOTA.setPassword(".");
}

  void loop() {
    present_time = millis();
    ArduinoOTA.handle();
    server.handleClient();
    rtc.getDateTime();
    rtc_secondes = rtc.second;
    rtc_minutes  = rtc.minute;
    rtc_heures   = rtc.hour;
    
    digitalWrite(pin_relais, activerRelais()); 
    digitalWrite(pin_led, activerRelais()); 
  }
