# ⚠️ Une meilleur documentation est en cour de redaction
  __vous pouvez quand meme utiliser celle ci comme reference.__
  
**________________________________________________________________________________________________________________________________________________**
# Système de contrôle programmer d’alimentation des appareil via et interface web

## Canevas du projet tutoré

1. ### Contexte et justification

    L’objectif principal de ce projet est de concevoir et de mettre en œuvre un système intelligent, basé sur
    un microcontrôleur ESP8266, permettant de contrôler l’alimentation électrique des appareils branchés à
    une prise. Grâce à une interface web intégrée, l’utilisateur peut définir des plages horaires durant
    lesquelles un appareil donné sera alimenté ou non. Ce système se veut simple, pratique et adaptable à
    plusieurs contextes, notamment dans le domaine domestique, agricole, industriel et bien plus.

    Dans une ère où l’optimisation énergétique et l’automatisation des tâches prennent une place centrale,
    ce projet vise à offrir une solution peu coûteuse, facilement déployable et accessible, aussi bien pour les
    particuliers que pour les petites entreprises. L’idée principale repose sur la combinaison d’un
    microcontrôleur ESP8266 (doté d’une connectivité Wi-Fi),d'un module d'horloge(RTC DS1302), d’un systheme de stockage (LittleFS) et
    d’une interface web conviviale.

2. ### Problématique

**Comment concevoir un système basé sur ESP8266, simple et accessible, qui permette de**
**contrôler l’alimentation d’appareils électriques selon des plages horaires définies par l’utilisateur,**
**via une page web intuitive et sans connexion internet externe ?**

3. #### __Objectifs du projet__

- _Objectif principal_
    Développer une prise intelligente contrôlée par ESP8266, permettant à l’utilisateur de
    programmer les horaires d’allumage et d’extinction des appareils électriques via une interface
    web.

- _Objectifs spécifiques_

    - Concevoir une prise connectée sécurisée, intégrant ESP8266 et un relais
    - Offrir à l’utilisateur une interface web intuitive pour configurer et gérer les horaires de
        fonctionnement des appareils.
    - Intégrer un module d’horloge temps réel (RTC) pour garantir le respect des horaires même en
        cas de redémarrage ou de coupure d’alimentation.
    - Mettre en place un système de stockage local (LittleFS) afin de sauvegarder les
        configurations de l’utilisateur.
    - Prévoir une flexibilité dans les applications possibles (particulièrement en milieu agricole ou industriel,...).
    - Ajouter des fonctionnalités secondaires comme la commande manuelle (override), les
        statistiques d’utilisation et éventuellement une extension vers le contrôle via smartphone
    - Tester le système dans un contexte réel (par exemple un arroseur automatique ou un
    éclairage).

4. ### Analyse des besoins

- __Les utilisateurs ciblés par notre système sont :__
    - Les agriculteurs souhaitant automatiser des arroseurs, séchoirs ou éclairages.
    - Les ménages pour gérer la télévision, le ventilateur, ou le chargeur électrique.
    - Les communautés pour gérer un éclairage collectif (ex. lampes solaires).

5. ### Contraintes

- __ce projet doit avoir :__
    - Accessibilité hors ligne (fonctionne sans internet, via WiFi local).
    - Sécurité électrique : relais et prise protégés dans un boîtier isolé.
    - Interface légère et intuitive : adaptée aux téléphones et ordinateurs.
    - Fiabilité : l’heure doit rester correcte même en cas de coupure (RTC).

 ### Conception 

- __La conception repose sur une logique d'algorithmique simple:__

    - Programmation horaire :
        - Définir plusieurs plages horaires par jour.
        - Exemple : activer de 6h à 7h30 et de 18h à 19h.
    - Commande manuelle (Override) :
        - Possibilité d’allumer/éteindre immédiatement l’appareil indépendamment un temps définis.
        - Utile en cas d’imprévu.
    - Interface web intuitive :
        - Accessible depuis un smartphone ou un PC connecté au même réseau Wi-Fi.
        - Interface simple avec boutons, champs de saisie et affichage en temps réel du RTC.
    - Sauvegarde persistante :
        - Les plages horaires et configurations restent disponibles même après un redémarrage.
        - Cela grace a la bibliotheque __littleFS.h__
    - Statut en temps réel :
        - Affichage de l’état actuel (ON/OFF).
        - Heure courante synchronisée avec l’horloge RTC

7. ### Exemples d’utilisation

    1. Agriculture et irrigation
        - Arroseurs automatiques pour champs de maïs ou de manioc : déclenchement à l’aube (5h-6h) et au crépuscule (18h-19h) pour éviter  l’évaporation excessive de l’eau.
        - Pompes d’irrigation : activation uniquement pendant certaines périodes pour éviter la surcharge électrique et optimiser la consommation de carburant ou d’énergie.
        - Séchage des récoltes (maïs, arachides, haricots) : gestion d’un ventilateur ou d’un système de chauffage qui fonctionne seulement à des heures précises de la journée.

    2. Gestion des installations
        - Éclairage communautaire : lampadaires de la place du marché s’allument automatiquement entre 18h et 22h.
        - Forages et distribution d’eau : pompes électriques activées selon un horaire défini pour éviter le gaspillage et permettre une meilleure organisation de l’approvisionnement en eau.

    3. Domestique
        - Éclairage domestique : activation automatique des ampoules dans les maisons ou fermes à certaines heures.
        - gestion des appareils : allumer ou etteindre certain appareils(television, four electique, prises) selon des plages predefinie pour limiterla consomation en electricite.

8. ### Avantages du projet

    1. Économique : composants à faible coût, autonomie énergétique et réduction du gaspillage.
    2. Accessibiliter : disponible sur toutes les platforms avec navigateur et ceux sans connexion internet.
    3. Polyvalent : utilisable en agriculture, domestique, communautaire, pour ne citer que ceux la.
    4. Évolutif : possibilité d’ajouter de nouvelles fonctionnalités

9. ### Déroulement du projet

    1. #### Étude et conception
        - Analyse des besoins spécifiques dans un contexte (agriculture, pompes, arrosage, éclairage).
        - Rédaction d’un cahier des charges adapté.
        - Sélection des composants nécessaires.

    2. #### Développement matériel
        - Montage de l’ESP8266, du relais et du RTC sur une breadboard.
        - Tests de fonctionnement de base (activation du relais).

    3. #### Développement logiciel
        - Programmation de l’ESP8266 via l’IDE Arduino.
        - Implémentation du serveur web embarqué.
        - Développement des fonctions de gestion du RTC et des horaires.
        - Ajout de la logique de sauvegarde (LittleFS).

    4. #### Tests et validation
        - Vérification du respect des horaires programmés.
        - Tests de memoire(coupure de courant, redémarrage, etc.).
        - Validation de l’interface web.

    5. #### Intégration et déploiement
        - Conception d’un boîtier sécurisé pour loger l’ESP8266 et le relais.
        - Presentation pour defense .

10. ### Perspectives d’évolution

    1. #### Contrôle via Internet (IoT) 
        - possibilité de commander les appareils à distance via une application mobile ou une plateforme cloud.
    2. #### Ajout de capteurs intelligents 
        - capteur d’humidité pour déclencher automatiquement l’arrosage, capteur de niveau d’eau pour gérer les pompes, capteur de consomation de courant, etc.
    3. #### Gestion multi-appareils 
        - extension du système pour contrôler plusieurs prises simultanément, par exemple une série arroseur electique.
    4. #### Compatibilité avec panneaux solaires 
        - gestion intelligente de l’énergie solaire pour optimiser la consommation et éviter les pertes.
    5. #### Collecte de données agricoles 
        - ajout de statistiques d’utilisation et de rapports pour aider les cultivateurs à mieux planifier leurs travaux.

11. ### Conclusion
    Le projet de contrôle d’alimentation via ESP8266 constitue une solution flexible et économique pour
    répondre aux besoins d’automatisation et d’optimisation énergétique.
    Grâce à sa conception modulaire, il peut être appliqué dans la gestion des appareils industriels,
    l’éclairage communautaire, d’autres installations agricoles,...

L’utilisation de l’ESP8266 permet une connectivité Wi-Fi intégrée, rendant le système accessible et
configurable via une simple page web, sans dépendre de dispositifs externes. Avec des perspectives
d’évolution vers l’IoT, l’intégration de capteurs intelligents et l’utilisation de l’énergie solaire,
ce projet se positionne comme une base solide pour transformer les pratiques agricoles, domestiques dans des
zones agricoles et semi-urbains.
