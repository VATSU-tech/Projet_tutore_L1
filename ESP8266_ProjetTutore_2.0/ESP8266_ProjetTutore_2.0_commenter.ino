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
#define pin_relais LED_BUILTIN
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
    body,
   a {
    margin: 0 10px;
    opacity: 0.9;
    text-decoration: none;
    color: var(--hot);
    font-size: 1.5rem;
    transition: .2s;
  }

   a:hover { 
    text-shadow: 0 0 1rem var(--bg),0 0 2rem var(--hot);  
    font-size: 1.7rem;
  }

    .club,
    .gallery .ph {
      overflow: hidden;
    }

    input,
    legend {
      color: var(--pri);
      font-weight: 700;
    }

    .btn,
    input,
    legend {
      font-weight: 700;
    }

    :root {
      --bg: #0b1020;
      --bg2: #0f1733;
      --pri: #00e5ff;
      --acc: #00ffa8;
      --hot: #ff3e7f;
      --ink: #e6f7ff;
      --mut: #89a8b3;
      --card: #111a31cc;
      --glass: rgba(255, 255, 255, 0.08);
      --glow: 0 0 18px rgba(0, 229, 255, 0.55), 0 0 38px rgba(0, 229, 255, 0.25);
    }

    * {
      box-sizing: border-box;
    }

    ::-webkit-scrollbar {
      display: none;
    }

    body,
    html {
      height: 100%;
    }

    body {
      margin: 0;
      font-family: system-ui, -apple-system, Segoe UI, Roboto, Ubuntu, Cantarell,
        Inter, Arial, sans-serif;
      background: radial-gradient(1200px 700px at 70% -10%,
          #12224a 0,
          transparent 60%),
        linear-gradient(180deg, var(--bg), var(--bg2) 35%, var(--bg) 100%);
      overflow-x: hidden;
    }

    header {
      position: fixed;
      inset: 0 0 auto 0;
      height: 64px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 0 18px;
      background: linear-gradient(#0b1020cc, #0b102000);
      border-bottom: 1px solid #ffffff12;
      z-index: 50;
      backdrop-filter: blur(6px);
    }

    .club,
    .gallery .ph,
    .hero,
    footer,
    section {
      position: relative;
    }

    .brand {
      display: flex;
      gap: 12px;
      align-items: center;
    }

    .brand .logo {
      width: 34px;
      height: 34px;
      border-radius: 10px;
      background: conic-gradient(from 180deg,
          var(--pri),
          var(--acc),
          var(--hot),
          var(--pri));
      box-shadow: var(--glow);
    }

    .brand h1 {
      font-size: 18px;
      margin: 0;
    }

    nav a {
      margin: 0 10px;
      opacity: 0.9;
    }

    nav a:hover {
      color: #fff;
      text-shadow: 0 0 10px #fff3;
    }

    .hero {
      min-height: 100svh;
      display: grid;
      place-items: center;
      padding: 92px 18px 120px;
    }

    .hero-inner {
      max-width: 1100px;
      width: 100%;
      display: grid;
      grid-template-columns: 1.2fr 0.8fr;
      gap: 28px;
    }

    .cta,
    .stat,
    .stats {
      display: flex;
    }

    .card {
      background: var(--card);
      border: 1px solid #ffffff1a;
      border-radius: 22px;
      padding: 26px;
      box-shadow: 0 8px 30px #0008;
      backdrop-filter: saturate(1.2) blur(4px);
    }

    h2 {
      font-size: clamp(28px, 3vw, 44px);
      margin: 0 0 12px;
    }

    .alert,
    input,
    legend {
      font-size: 1.2rem;
    }

    p {
      color: var(--mut);
      line-height: 1.6;
    }

    .cta {
      gap: 12px;
      flex-wrap: wrap;
      margin-top: 18px;
    }

    .btn {
      appearance: none;
      border: none;
      border-radius: 16px;
      padding: 12px 18px;
      cursor: pointer;
      transition: transform 0.25s, box-shadow 0.25s;
      box-shadow: 0 6px 14px #0008;
    }

    .btn.primary {
      background: linear-gradient(135deg, var(--pri), #00b0ff);
      color: #001217;
      box-shadow: 0 0 0 2px #00c7ff55, var(--glow);
    }

    .btn.ghost {
      background: 0 0;
      color: var(--ink);
      border: 1px solid var(--mut);
    }

    .btn:hover {
      background: linear-gradient(135deg, #00b0ff, var(--pri));
      transform: translateY(-2px);
    }

    .btn:active {
      transform: scale(0.95);
    }

    .stats {
      justify-content: space-around;
      margin-top: 22px;
    }

    .stat {
      min-width: 150px;
      flex-direction: column;
      align-items: center;
      padding: 14px;
      border: 1px dashed #3dd9ff38;
      border-radius: 14px;
    }

    .stat b {
      display: block;
      font-size: 1.35em;
      color: #fff;
    }

    section {
      padding: 80px 18px;
    }

    .wrap {
      max-width: 1100px;
      margin: auto;
    }

    .grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 16px;
    }

    .override,
    .social {
      display: flex;
      gap: 12px;
    }

    .override {
      flex-direction: column;
      margin-top: 22px;
    }

    .override span label {
      margin-left: 10px;
      color: var(--ink);
      font-weight: 700;
      font-size: 1.1rem;
    }

    input {
      background-color: transparent;
      border: none;
      width: 40px;
      outline: 0;
      text-align: center;
    }

    input[type="number"]::-webkit-inner-spin-button,
    input[type="number"]::-webkit-outer-spin-button {
      -webkit-appearance: none;
      margin: 0;
    }

    .club h3 {
      margin: 0 0 6px;
    }

    footer {
      padding: 60px 18px 90px;
      background: linear-gradient(180deg,
          transparent,
          #0b1020cc 40%,
          #05070e 100%);
      border-top: 1px solid #ffffff0f;
    }

    .social {
      flex-wrap: wrap;
    }

    .social a {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 10px 14px;
      border-radius: 12px;
      border: 1px solid #ffffff22;
      color: var(--ink);
      background: var(--glass);
    }

    .social svg {
      width: 18px;
      height: 18px;
    }

    .gallery {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 12px;
    }

    @media (max-width: 900px) {
      .hero-inner {
        grid-template-columns: 1fr;
      }

      .gallery {
        grid-template-columns: repeat(2, 1fr);
      }
    }

    .gallery .ph {
      aspect-ratio: 16/10;
      border-radius: 16px;
      background: #ffffff0f;
      border: 1px solid #ffffff12;
    }

    .gallery .ph::after {
      content: "Ajouter image";
      position: absolute;
      inset: auto 6px 6px auto;
      font-size: 12px;
      color: #fff9;
      background: #0006;
      padding: 4px 8px;
      border-radius: 10px;
    }

    .sparkle {
      text-shadow: 0 0 20px rgba(0, 229, 255, 0.6),
        0 0 40px rgba(0, 229, 255, 0.2);
      animation: 5s ease-in-out infinite sparkle;
      display: inline-block;
    }

    @keyframes sparkle {

      0%,
      100% {
        filter: drop-shadow(0 0 0 rgba(0, 229, 255, 0));
      }

      50% {
        filter: drop-shadow(0 0 14px rgba(0, 229, 255, 0.7));
      }
    }

    .pcb {
      position: fixed;
      inset: 0;
      z-index: -2;
      opacity: 0.8;
      pointer-events: none;
      background: radial-gradient(800px 400px at 20% 10%,
          #0c1b38 0,
          transparent 60%);
      mask: linear-gradient(180deg, #000 50%, transparent 100%);
    }

    .wave,
    .waves {
      inset: auto 0 0 0;
      height: 220px;
    }

    .pcb svg {
      width: 1800px;
      height: auto;
      position: absolute;
      left: 50%;
      top: 50%;
      transform: translate(-50%, -50%) scale(1.2);
      opacity: 0.9;
    }

    .data-rain::after,
    .data-rain::before,
    .waves,
    canvas.snow {
      position: fixed;
      pointer-events: none;
      z-index: -1;
    }

    .trace {
      fill: none;
      stroke: #1bd3ff;
      stroke-width: 2;
      stroke-linecap: round;
      filter: drop-shadow(0 0 3px #00e5ff88);
    }

    .trace.dim {
      stroke: #1086a8;
    }

    .pulse {
      stroke-dasharray: 6 12;
      animation: 3s linear infinite flow;
    }

    @keyframes flow {
      to {
        stroke-dashoffset: -200;
      }
    }

    .node {
      fill: #00ffd0;
      filter: drop-shadow(0 0 6px #00ffd0aa);
    }

    .node.blink {
      animation: 1.8s ease-in-out infinite blink;
    }

    @keyframes blink {
      50% {
        opacity: 0.2;
      }
    }

    .wave {
      position: absolute;
      opacity: 0.6;
    }

    .wave svg {
      width: 200%;
      height: 100%;
    }

    .wave:first-child {
      animation: 14s linear infinite drift;
    }

    .wave:nth-child(2) {
      animation: 22s linear infinite reverse drift;
      opacity: 0.45;
    }

    .wave:nth-child(3) {
      animation: 38s linear infinite drift;
      opacity: 0.3;
    }

    @keyframes drift {
      to {
        transform: translateX(-50%);
      }
    }

    canvas.snow {
      inset: 0;
      mix-blend-mode: screen;
      opacity: 1;
      box-shadow: 0 0 1px initial;
    }

    .data-rain::after,
    .data-rain::before {
      content: "";
      inset: 0;
      opacity: 0.35;
      background: repeating-linear-gradient(to bottom,
          transparent 0 28px,
          rgba(0, 255, 200, 0.06) 28px 30px);
    }

    .data-rain::after {
      opacity: 0.2;
      background: radial-gradient(2px 2px at 20% 10%,
          #00ffd0 50%,
          transparent 60%),
        radial-gradient(2px 2px at 70% 30%, #00e5ff 50%, transparent 60%),
        radial-gradient(2px 2px at 40% 80%, #00ffd0 50%, transparent 60%);
      filter: blur(0.5px);
    }

    .card.glow {
      box-shadow: 0 0 0 1px #00e5ff33, 0 0 80px #00e5ff12 inset;
    }

    .alert {
      color: var(--hot);
    }

    fieldset {
      border: 1px solid var(--pri);
      border-radius: 16px;
      padding: 12px;
    }

    legend {
      margin: 1rem;
    }

    .ajouter {
      display: flex;
      flex-direction: column;
      gap: 2rem;
    }

    .ajouter .debut,
    .ajouter .fin {
      margin-left: 3rem;
    }

    .ajouter .fin {
      margin-left: 4.4rem;
    }

    .ajouter .debut input,
    .ajouter .fin input {
      border-bottom: 1px solid var(--pri);
    }

    .modifierPlageHoraire {
      position: absolute;
      cursor: pointer;
      right: 20px;
      top: 20px;
    }

    .edit_plage_zone {
      margin-left: 2rem;
      display: flex;
      align-items: center;
      gap: 2rem;
    }

    .plage {
      display: flex;
      flex-direction: column;
    }
  </style>

 <body class="data-rain">
  <!-- ====== PCB BACKGROUND ====== -->
  <div class="pcb" aria-hidden="true">
    <svg viewBox="0 0 1200 800">
      <!-- Traces -->
      <path class="trace dim" d="M40 120 C 240 60, 380 240, 560 140 S 840 40, 1140 120" />
      <path class="trace pulse" d="M60 280 C 240 200, 420 360, 600 260 S 900 200, 1140 260" />
      <path class="trace" d="M100 420 C 180 380, 260 520, 360 460 S 540 360, 760 440 S 980 560, 1120 520" />
      <path class="trace pulse" d="M80 600 C 220 540, 380 700, 560 620 S 820 520, 1100 620" />

      <!-- Nodes -->
      <circle class="node blink" cx="120" cy="120" r="3" />
      <circle class="node" cx="360" cy="170" r="3" />
      <circle class="node blink" cx="560" cy="140" r="4" />
      <circle class="node" cx="920" cy="90" r="3" />
      <circle class="node blink" cx="180" cy="420" r="4" />
      <circle class="node" cx="360" cy="460" r="3" />
      <circle class="node blink" cx="760" cy="440" r="4" />
      <circle class="node" cx="980" cy="560" r="3" />
      <circle class="node blink" cx="220" cy="600" r="4" />
      <circle class="node" cx="560" cy="620" r="3" />
      <circle class="node blink" cx="980" cy="620" r="4" />
    </svg>
  </div>

  <!-- ====== WAVES ====== -->
  <div class="waves" aria-hidden="true">
    <div class="wave">
      <svg viewBox="0 0 1440 320" preserveAspectRatio="none">
        <path fill="#0c1a35" fill-opacity="0.8"
          d="M0,160L60,144C120,128,240,96,360,117.3C480,139,600,213,720,229.3C840,245,960,203,1080,170.7C1200,139,1320,117,1380,106.7L1440,96L1440,320L1380,320C1320,320,1200,320,1080,320C960,320,840,320,720,320C600,320,480,320,360,320C240,320,120,320,60,320L0,320Z">
        </path>
      </svg>
    </div>
    <div class="wave">
      <svg viewBox="0 0 1440 320" preserveAspectRatio="none">
        <path fill="#0a1530" fill-opacity="0.7"
          d="M0,64L80,69.3C160,75,320,85,480,117.3C640,149,800,203,960,208C1120,213,1280,171,1360,149.3L1440,128L1440,320L1360,320C1280,320,1120,320,960,320C800,320,640,320,480,320C320,320,160,320,80,320L0,320Z">
        </path>
      </svg>
    </div>
    <div class="wave">
      <svg viewBox="0 0 1440 320" preserveAspectRatio="none">
        <path fill="#071026" fill-opacity="0.65"
          d="M0,32L48,69.3C96,107,192,181,288,202.7C384,224,480,192,576,165.3C672,139,768,117,864,112C960,107,1056,117,1152,128C1248,139,1344,149,1392,154.7L1440,160L1440,320L1392,320C1344,320,1248,320,1152,320C1056,320,960,320,864,320C768,320,672,320,576,320C480,320,384,320,288,320C192,320,96,320,48,320L0,320Z">
        </path>
      </svg>
    </div>
  </div>

  <!-- ====== SNOW ====== -->
  <canvas class="snow" id="snow"></canvas>
      <header>
    <div class="brand">
      <div class="logo" aria-hidden="true"></div>
      <h1>GACI • <span class="mono">Module embarque</span></h1>
    </div>
  </header>

  <!-- ====== HERO ====== -->
  <section id="acceuil" class="hero">
    <div class="hero-inner">
      <div class="card glow">
        <h2>
          Controler vos appareil avec l'<span class="sparkle">IoT</span>
        </h2>
        <p>
          Economiser de l'énergie, de l'<b>argent</b> en
          <b>controlant</b> l'allimentation de vos
          <b>appareils</b> électroménagers avec l'<b>IoT</b> et l'Arduino.
        </p>
        <div class="stats">
          <div class="stat">
            <b id="currentTime"><span class="alert">⁉️ </span>•• : •• : ••</b>
          </div>
          <div class="stat">
            <b id="date"><span class="alert">⁉️ </span>•• / •• / ••••</b>
          </div>
          <div class="stat" style="flex-direction: row; justify-content: center; gap: 3px">
            -<b id="stat-3">0 </b> min
          </div>
        </div>
        <div class="plages">
          <br />
          <h2>Plages autoriser</h2>
          <div class="edit_plage_zone">
            <div class="plage">
              )rawliteral"; for (uint8_t i = 0; i < nPlages; i++) { page +="<span>" ;
               page +=String(plages[i].heure_debut) + "h" + String(plages[i].minute_debut) + " → " ;
               page +=String(plages[i].heure_fin) + "h" + String(plages[i].minute_fin); page +="</span>" ;
                } page +=R"rawliteral( </div>
                <button class="btn primary" id="openEdit" style="height: 50px">
                  🖊
                </button>
            </div>
          </div>
          <form action="/override" method="GET" class="override">
            <span><label for="override">Allumer l'appareil pendant
                <input type="number" value="0" name="minutes" id="minutesInput" min="1" max="180" />
                minutes</label></span>
            <button class="btn primary">Allumez</button>
          </form>
        </div>
      </div>
  </section>
  
  <section id="edit_plages" class="hero" style="display: none">
    <div class="hero-inner">
      <div class="card glow">
        <h2><b>Modifier</b> les plages autorisées</h2>
        <button class="modifierPlageHoraire btn ghost" id="closeEdit">
          Fermer
        </button>
        <div id="editPlages">
          <div class="plageHoraireEdit">
            <div class='plage'>
                )rawliteral"; for (uint8_t i = 0; i < nPlages; i++) { page +="<span>" ;
                page +=String(plages[i].heure_debut) + "h" + String(plages[i].minute_debut) + " → " ; 
                page +=String(plages[i].heure_fin) + "h" + String(plages[i].minute_fin); 
                page +="<a href='/del?id=" + String(i) + "'>🗑</a>" ; page +="</span>" ; } 
                page +=R"rawliteral( 
                  </div>
                <form action="/add" method="GET">
              <fieldset>
                <legend>ajouter plage</legend>
                <div class="ajouter">
                  <div class="debut">
                    <span>Debut :</span>
                    <input type="number" name="h" min="0" max="23" placeholder="H" />
                    <span>:</span>
                    <input type="number" name="m" min="0" max="59" placeholder="M" />
                    <br />
                  </div>
                  <div class="fin">
                    <span>Fin : </span>
                    <input type="number" name="hf" min="0" max="23" placeholder="H" />
                    <span>:</span>
                    <input type="number" name="mf" min="0" max="59" placeholder="M" /><br />
                  </div>
                  <button class="btn primary">Ajouter</button>
                </div>
              </fieldset>
              </form>
          </div>
        </div>
      </div>
    </div>
  </section>
</body>
<script>
  // ===== UTIL =====
  const qs = (s) => document.querySelector(s);
  const qsa = (s) => Array.from(document.querySelectorAll(s));

  // ===== SNOW =====
  const snow = (() => {
    const canvas = document.getElementById("snow");
    const ctx = canvas.getContext("2d");
    let W = (canvas.width = innerWidth);
    let H = (canvas.height = innerHeight);
    let flakes = [];
    const baseCount = Math.min(
      220,
      Math.max(100, Math.floor((W * H) / 25000))
    );

    function reset() {
      W = canvas.width = innerWidth;
      H = canvas.height = innerHeight;
      const count = Math.floor(baseCount * 4); //quantite des flocons
      flakes = new Array(count).fill(0).map(() => ({
        x: Math.random() * W,
        y: Math.random() * H,
        r: Math.random() * 2.2 + 0.6,
        v: Math.random() * 0.6 + 0.2,
        w: Math.random() * 0.7 + 0.1,
        o: Math.random() * 0.5 + 0.3,
      }));
    }

    function draw() {
      ctx.clearRect(0, 0, W, H);
      ctx.globalCompositeOperation = "lighter";
      for (const f of flakes) {
        f.y += f.v * 1; //vitesse de tomber des flocons
        f.x += Math.sin(f.y * 0.02) * f.w;
        if (f.y > H + 10) {
          f.y = -10;
          f.x = Math.random() * W;
        }
        ctx.globalAlpha = f.o;
        ctx.beginPath();
        ctx.arc(f.x, f.y, f.r, 0, Math.PI * 2);
        ctx.fillStyle = "#bff7ff";
        ctx.fill();
      }
      requestAnimationFrame(draw);
    }
    addEventListener("resize", reset);
    reset();
    draw();
  })();

  const hours = document.getElementById("currentTime");
  const date = document.getElementById("date");
  const timer = setInterval(() => {
    const currentDate = new Date();
    hours.textContent = currentDate.getHours() + " : " + currentDate.getMinutes() + " : " + currentDate.getSeconds();
    date.textContent = currentDate.getDate() + " / " + (currentDate.getMonth() + 1) + " / " + currentDate.getFullYear();
  }, 1000);

  const openEdit = document.getElementById("openEdit");
  const closeEdit = document.getElementById("closeEdit");
  const editDisplay = document.getElementById("edit_plages");
  const homeDisplay = document.getElementById("acceuil");
 
  // function updateTime() {
  //   fetch("/time")
  //     .then((response) => response.text())
  //     .then((data) => {
  //       document.getElementById("currentTime").textContent = data;
  //     })
  //     .catch((err) => console.error("Erreur maj heure :", err));
  // }
  // setInterval(updateTime, 1000);

  function updateText() {
    fetch("/allumer")
      .then((response) => response.text())
      .then((data) => {
        document.getElementById("powerBtn").textContent = data;
      })
      .catch((err) => console.error("Erreur text :", err));
  }
  setInterval(updateText, 1000);

  // Gestion de l’edition des plages
  openEdit.addEventListener("click", () => {
    editDisplay.style.display = "block";
    homeDisplay.style.display = "none";
  });

  closeEdit.addEventListener("click", () => {
    editDisplay.style.display = "none";
    homeDisplay.style.display = "block";
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
