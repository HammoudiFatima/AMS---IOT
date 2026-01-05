Holter Connecté – ECG Simulé
Architecture IoT & Edge Computing (AMS)
Projet réalisé dans le cadre de l’UE AMS – Application IoT & Edge Computing
Master 2 SYRIUS – Janvier 2026
Auteurs
Hammoudi Fatima
Berkai Yanis
Encadrants
Marc Silanus
Eric Zouania
1. Présentation du projet
Ce projet implémente un prototype pédagogique de Holter connecté, destiné à illustrer une chaîne complète IoT + Edge Computing appliquée à la surveillance cardiaque.
Le signal ECG est simulé à partir de profils réalistes (CSV) et non acquis sur un patient réel.
Le traitement principal est effectué en local sur microcontrôleur (Edge) afin de réduire la quantité de données transmises et permettre une réaction rapide en cas d’anomalie.
⚠️ Ce projet n’est pas un dispositif médical.
2. Fonctionnalités principales
Génération de signaux ECG simulés (profils normal, sportif, pathologique)
Acquisition analogique du signal ECG
Traitement Edge Computing :
détection des battements (pics R),
calcul de la fréquence cardiaque (HR),
estimation de la variabilité de la fréquence cardiaque (HRV),
détection d’anomalies (tachycardie, bradycardie, HRV faible)
Transmission LoRaWAN via The Things Network (TTN)
Supervision temps réel avec Node-RED
Stockage des données et export CSV
3. Architecture globale
Simulateur ECG (CSV + carte)
        |
        v
Raspberry Pi 2
        |
        v  (signal analogique)
Arduino Leonardo (Edge Computing)
        |
        v  (LoRaWAN)
The Things Network (TTN)
        |
        v  (MQTT)
Node-RED
        |
        v
Dashboard + Base de données
4. Architecture matérielle
Composants utilisés
Composant	Rôle
Arduino Leonardo	Nœud Edge (acquisition + traitement + LoRa)
Raspberry Pi 2	Plateforme hôte du simulateur ECG
Carte simulateur ECG	Génération du signal analogique
Module LoRa	Communication LoRaWAN
Passerelle TTN	Réception radio
Câblage principal
Simulateur ECG	Arduino
Vout_Sim	A0
GND	GND
Une masse commune est indispensable pour une acquisition stable.
5. Architecture logicielle
5.1 Traitement Edge (Arduino)
Fréquence d’échantillonnage : ~100 Hz
Pré-traitement :
moyenne glissante,
suppression de la dérive de baseline
Détection de battements :
seuil adaptatif,
période réfractaire (150 ms)
Calcul :
HR = 60000 / RR
HRV = écart-type des RR (SDNN approximé)
Machine à états :
NORMAL
ALERTE
Encodage LoRaWAN compact (5 octets)
5.2 Communication LoRaWAN
Réseau : The Things Network
Mode NORMAL : envoi périodique (60 s)
Mode ALERTE : envoi immédiat + répétition si persistance
5.3 Supervision Node-RED
Ingestion MQTT depuis TTN
Décodage du payload binaire
Visualisation :
HR (bpm)
HRV (ms)
état NORMAL / ALERTE
courbes temporelles
Stockage MariaDB
Export CSV
6. Prérequis
Matériel
Arduino Leonardo
Raspberry Pi 2 (Ethernet recommandé)
Carte simulateur ECG
Module LoRa
Câbles USB et Dupont
Logiciels
Arduino IDE (≥ 1.8.x)
Bibliothèque Arduino :
TheThingsNetwork
Node-RED
Accès TTN
7. Mise en œuvre (tutoriel rapide)
Étape 1 – Montage matériel
Éteindre le Raspberry Pi 2
Monter la carte simulateur ECG sur le GPIO
Réaliser le câblage Vout_Sim → A0 et GND → GND
Démarrer le Raspberry Pi
Étape 2 – Arduino
Ouvrir le code Arduino
Configurer les clés TTN (DevEUI, AppEUI, AppKey)
Téléverser sur l’Arduino Leonardo
Vérifier les logs via Serial Monitor
Étape 3 – TTN
Vérifier la réception des uplinks dans la console TTN (Live Data)
Étape 4 – Node-RED
Lancer Node-RED
Importer le flow fourni
Déployer
Accéder au dashboard via /ui
8. Tests et validation
Tests unitaires avec profils ECG CSV
Tests d’intégration Edge → TTN → Node-RED
Scénario de démonstration de 10 minutes :
passage NORMAL → ALERTE → NORMAL
vérification des messages LoRaWAN
validation du dashboard et de l’historique
9. Limites
Signal ECG simulé uniquement
Algorithmes basés sur des seuils simples
Pas de diagnostic médical
HRV simplifiée (pas d’analyse fréquentielle)
10. Perspectives
Filtrage ECG plus avancé (passe-bande)
Détection QRS type Pan–Tompkins
Configuration dynamique des seuils (downlink LoRa)
Stockage time-series (InfluxDB)
Détection automatique d’arythmies par ML
11. Licence et avertissement
Projet fourni à des fins pédagogiques uniquement.
Il ne doit en aucun cas être utilisé pour un usage médical ou clinique.
Version : 1.1
Date : Janvier 2026
