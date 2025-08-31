// // Structure pour stocker une plage horaire
// struct PlageHoraire {
//   uint8_t heure_debut;
//   uint8_t minute_debut;
//   uint8_t heure_fin;
//   uint8_t minute_fin;
// };

// // Exemple : 2 plages horaires
// PlageHoraire plages[] = {
//   {14, 0, 16, 0},   // 14h00 → 16h00
//   {18, 0, 20, 0}    // 18h00 → 20h00
// };

// const int nombrePlages = sizeof(plages) / sizeof(plages[0]);

// bool activerRelais() {
//   for (int i = 0; i < nombrePlages; i++) {
//     uint16_t now = rtc_heures * 60 + rtc_minutes;  // heure actuelle en minutes
//     uint16_t start = plages[i].heure_debut * 60 + plages[i].minute_debut;
//     uint16_t end   = plages[i].heure_fin   * 60 + plages[i].minute_fin;

//     // Cas 1 : plage normale (ex: 14h → 16h)
//     if (start <= end && now >= start && now <= end) {
//       stateRelais = true;
//       return true;
//     }

//     // Cas 2 : plage qui passe minuit (ex: 22h → 02h)
//     if (start > end && (now >= start || now <= end)) {
//       stateRelais = true;
//       return true;
//     }
//   }
//   // Si aucune plage n’est valide
//   stateRelais = false;
//   return false;
// }
// //-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_
// // ====== DÉCLARATIONS : PLAGES & OUTILS TEMPS ======
// struct PlageHoraire {
//   uint8_t heure_debut;
//   uint8_t minute_debut;
//   uint8_t heure_fin;
//   uint8_t minute_fin;
// };

// // Ajuste la capacité si besoin
// const uint8_t MAX_PLAGES = 8;

// // Initialise ici tes plages par défaut
// PlageHoraire plages[MAX_PLAGES] = {
//   {14, 0, 16, 0},   // 14:00 → 16:00
//   {18, 0, 20, 0}    // 18:00 → 20:00
// };
// uint8_t nPlages = 2;  // nombre réel de plages actives

// // Override manuel (optionnel) : allumer pendant N minutes depuis le bouton web
// bool overrideActive = false;
// uint16_t overrideStartMin = 0;  // début (en minutes depuis minuit)
// uint16_t overrideEndMin   = 0;  // fin (en minutes depuis minuit)

// // Petit utilitaire
// inline uint16_t toMinutes(uint8_t h, uint8_t m) { return h * 60 + m; }

// // Teste si un "now" (en minutes) est dans [start, end], en gérant le passage de minuit
// bool inWindow(uint16_t now, uint16_t start, uint16_t end) {
//   if (start <= end) {
//     return now >= start && now <= end;      // fenêtre normale (ex: 14:00 → 16:00)
//   } else {
//     return (now >= start) || (now <= end);  // traverse minuit (ex: 22:00 → 02:00)
//   }
// }

// // ====== LOGIQUE PRINCIPALE : ACTIVER LE RELAIS ? ======
// bool activerRelais() {
//   // sécurise : si l'heure RTC n'est pas encore lue, on considère OFF
//   if (rtc_heures > 23 || rtc_minutes > 59) return false;

//   uint16_t now = toMinutes(rtc_heures, rtc_minutes);

//   // 1) Priorité à l'override manuel s'il est actif
//   if (overrideActive) {
//     if (inWindow(now, overrideStartMin, overrideEndMin)) {
//       return true;                // ON forcé tant que la fenêtre override n'est pas expirée
//     } else {
//       overrideActive = false;     // la fenêtre est terminée → on retombe sur les plages normales
//     }
//   }

//   // 2) Sinon, on teste toutes les plages configurées
//   for (uint8_t i = 0; i < nPlages; i++) {
//     uint16_t start = toMinutes(plages[i].heure_debut, plages[i].minute_debut);
//     uint16_t end   = toMinutes(plages[i].heure_fin,   plages[i].minute_fin);
//     if (inWindow(now, start, end)) {
//       return true;  // trouvé une plage valide
//     }
//   }
//   // 3) Aucune plage valide
//   return false;
// }

// //-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_-_*-_*-*_
//   uint8_t nPlages = 2;  // nombre réel de plages actives
//   //VATSU
//   #include <ArduinoOTA.h>
//   #include <RTC_DS1302.h>
//   #include <ESP8266WiFi.h>
//   #include <ESP8266WebServer.h>

//   #define ssid "DESKTOP-8NRTH60 va"
//   #define password "12345678900"

//   #define hour_close_1 18
//   #define hour_close_2 18

//   #define hour_open_1 18
//   #define hour_open_2 18

//   #define min_close_1 0
//   #define min_close_2 0

//   #define min_open_1 40
//   #define min_open_2 43


//   #define RTC_CLK D5
//   #define RTC_DAT D6
//   #define RTC_RST D7

//   #define pin_relais D8

//   ESP8266WebServer server(80);
//   DS1302 rtc(RTC_RST, RTC_CLK, RTC_DAT);


//   uint8_t rtc_secondes;
//   uint8_t rtc_minutes;
//   uint8_t rtc_heures;

//   int testCompteur = 0;
//   bool stateRelais = 0;

//   unsigned long present_time = 0;
//   unsigned long previus_time_compteur = 0;

//   // ====== DÉCLARATIONS : PLAGES & OUTILS TEMPS ======
//   struct PlageHoraire {
//     uint8_t heure_debut;
//     uint8_t minute_debut;
//     uint8_t heure_fin;
//     uint8_t minute_fin;
//   };

//   // Ajuste la capacité si besoin
//   const uint8_t MAX_PLAGES = 8;

//   // Initialise ici tes plages par défaut
//   PlageHoraire plages[MAX_PLAGES] = {
//     {12, 26, 12, 27},   // 14:00 → 16:00
//     {12, 29, 12, 30}    // 18:00 → 20:00
//   };

//   // Override manuel (optionnel) : allumer pendant N minutes depuis le bouton web
//   bool overrideActive = false;
//   uint16_t overrideStartMin = 0;  // début (en minutes depuis minuit)
//   uint16_t overrideEndMin   = 0;  // fin (en minutes depuis minuit)

//   // Petit utilitaire
//   inline uint16_t toMinutes(uint8_t h, uint8_t m) { return h * 60 + m; }

//   // Teste si un "now" (en minutes) est dans [start, end], en gérant le passage de minuit
//   bool inWindow(uint16_t now, uint16_t start, uint16_t end) {
//     if (start <= end) {
//       return now >= start && now <= end;      // fenêtre normale (ex: 14:00 → 16:00)
//     } else {
//       return (now >= start) || (now <= end);  // traverse minuit (ex: 22:00 → 02:00)
//     }
//   }

//   // ====== LOGIQUE PRINCIPALE : ACTIVER LE RELAIS ? ======
//   bool activerRelais() {
//     // sécurise : si l'heure RTC n'est pas encore lue, on considère OFF
//     if (rtc_heures > 23 || rtc_minutes > 59) return false;

//     uint16_t now = toMinutes(rtc_heures, rtc_minutes);

//     // 1) Priorité à l'override manuel s'il est actif
//     if (overrideActive) {
//       if (inWindow(now, overrideStartMin, overrideEndMin)) {
//         return true;                // ON forcé tant que la fenêtre override n'est pas expirée
//       } else {
//         overrideActive = false;     // la fenêtre est terminée → on retombe sur les plages normales
//       }
//     }

//     // 2) Sinon, on teste toutes les plages configurées
//     for (uint8_t i = 0; i < nPlages; i++) {
//       uint16_t start = toMinutes(plages[i].heure_debut, plages[i].minute_debut);
//       uint16_t end   = toMinutes(plages[i].heure_fin,   plages[i].minute_fin);
//       if (inWindow(now, start, end)) {
//         return true;  // trouvé une plage valide
//       }
//     }
//     // 3) Aucune plage valide
//     return false;
//   }

//   void test() {
//     if (activerRelais()) {
//       digitalWrite(pin_relais, true);
//       Serial.println("Allumer");
//     } else {
//       digitalWrite(pin_relais, false);
//       Serial.println("Fermer");
//     }
//     Serial.printf("\n%02d : %02d : %02d", rtc_heures, rtc_minutes, rtc_secondes);
//   }

//   void setup() {
//     Serial.begin(115200);
//     rtc.init();

//     pinMode(pin_relais, OUTPUT);

//     WiFi.begin(ssid, password);
//     while (WiFi.status() != WL_CONNECTED) {
//       delay(500);
//       Serial.print(".");
//     }

//     Serial.println("\nWiFi connecté !");
//     Serial.print("Adresse IP : ");
//     Serial.println(WiFi.localIP());

// server.on("/", page_d_accueil);
// server.on("/add", handleAdd);
// server.on("/del", handleDel);
// server.on("/edit", handleEdit);

//     server.begin();
//     ArduinoOTA.begin();
//   }

//   void loop() {
//     present_time = millis();
//     ArduinoOTA.handle();
//     server.handleClient();
//     rtc.getDateTime();
//     rtc_secondes = rtc.second;
//     rtc_minutes  = rtc.minute;
//     rtc_heures   = rtc.hour;
//     test();

//   }

// void handleAdd() {
//   if (nPlages >= MAX_PLAGES) {
//     server.send(400, "text/plain", "Nombre maximum de plages atteint !");
//     return;
//   }
//   if (server.hasArg("h") && server.hasArg("m") && server.hasArg("hf") && server.hasArg("mf")) {
//     plages[nPlages].heure_debut = server.arg("h").toInt();
//     plages[nPlages].minute_debut = server.arg("m").toInt();
//     plages[nPlages].heure_fin   = server.arg("hf").toInt();
//     plages[nPlages].minute_fin  = server.arg("mf").toInt();
//     nPlages++;
//     server.sendHeader("Location", "/");
//     server.send(303);  // redirige vers la page d'accueil
//   } else {
//     server.send(400, "text/plain", "Arguments manquants !");
//   }
// }

// void handleDel() {
//   if (server.hasArg("id")) {
//     int id = server.arg("id").toInt();
//     if (id >= 0 && id < nPlages) {
//       // Décale les plages suivantes pour combler le trou
//       for (int i = id; i < nPlages - 1; i++) {
//         plages[i] = plages[i + 1];
//       }
//       nPlages--;
//       server.sendHeader("Location", "/"); 
//       server.send(303);
//     } else {
//       server.send(400, "text/plain", "ID invalide !");
//     }
//   } else {
//     server.send(400, "text/plain", "Argument manquant !");
//   }
// }

// void handleEdit() {
//   if (server.hasArg("id") && server.hasArg("h") && server.hasArg("m") && server.hasArg("hf") && server.hasArg("mf")) {
//     int id = server.arg("id").toInt();
//     if (id >= 0 && id < nPlages) {
//       plages[id].heure_debut = server.arg("h").toInt();
//       plages[id].minute_debut = server.arg("m").toInt();
//       plages[id].heure_fin   = server.arg("hf").toInt();
//       plages[id].minute_fin  = server.arg("mf").toInt();
//       server.sendHeader("Location", "/");
//       server.send(303);
//     } else {
//       server.send(400, "text/plain", "ID invalide !");
//     }
//   } else {
//     server.send(400, "text/plain", "Arguments manquants !");
//   }
// }
// void page_d_accueil() {
//   String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
//   page += "<title>Contrôle Télévision</title></head><body>";
//   page += "<h1>Plages horaires actuelles</h1><ul>";

//   for (uint8_t i = 0; i < nPlages; i++) {
//     page += "<li>";
//     page += String(plages[i].heure_debut) + "h" + String(plages[i].minute_debut) + " → ";
//     page += String(plages[i].heure_fin)   + "h" + String(plages[i].minute_fin);
//     page += " <a href='/del?id=" + String(i) + "'>❌ Supprimer</a>";
//     page += "</li>";
//   }

//   page += "</ul>";
//   page += "<h2>Ajouter une plage</h2>";
//   page += "<form action='/add' method='GET'>Début : <input type='number' name='h' min='0' max='23'>:";
//   page += "<input type='number' name='m' min='0' max='59'> Fin : ";
//   page += "<input type='number' name='hf' min='0' max='23'>:";
//   page += "<input type='number' name='mf' min='0' max='59'>";
//   page += "<input type='submit' value='Ajouter'></form>";

//   page += "</body></html>";
//   server.send(200, "text/html", page);
// }