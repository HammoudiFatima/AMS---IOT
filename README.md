# Holter connecté (ECG simulé) — IoT & Edge Computing (LoRaWAN/TTN + Node-RED + MariaDB)

Prototype pédagogique de Holter connecté : un simulateur ECG (sur Raspberry Pi) génère un signal analogique, un Arduino (Edge) détecte les battements et calcule HR/HRV, envoie des uplinks LoRaWAN vers TTN, puis Node-RED décode, affiche un dashboard et historise en base MariaDB.

> ⚠️ Projet pédagogique : ce système **n’est pas un dispositif médical**.

---

## Sommaire
- [Architecture](#architecture)
- [Matériel](#matériel)
- [Pré-requis logiciels](#pré-requis-logiciels)
- [Arborescence du dépôt](#arborescence-du-dépôt)
- [Mise en œuvre](#mise-en-œuvre)
  - [1) Raspberry Pi + simulateur ECG](#1-raspberry-pi--simulateur-ecg)
  - [2) Arduino (Edge + LoRaWAN)](#2-arduino-edge--lorawan)
  - [3) TTN (The Things Network)](#3-ttn-the-things-network)
  - [4) Node-RED (MQTT + dashboard)](#4-node-red-mqtt--dashboard)
  - [5) MariaDB (historisation)](#5-mariadb-historisation)
- [Recette / Validation](#recette--validation)
- [Dépannage](#dépannage)
- [Sécurité](#sécurité)

---

## Architecture

Chaîne de bout en bout :

1. **Raspberry Pi + simulateur ECG** : lecture de profils ECG (CSV) et génération d’un signal analogique.
2. **Arduino (Edge)** : échantillonnage + détection battements + calcul **HR** / **HRV** + décision **NORMAL/ALERTE**.
3. **LoRaWAN → TTN** : envoi de payloads compacts (uplinks).
4. **Node-RED (MQTT)** : ingestion des uplinks, décodage, affichage dashboard.
5. **MariaDB** : stockage des mesures (historique + alertes).

---

## Matériel

- 1 × **Raspberry Pi** (Pi 3 recommandé) + alimentation + carte microSD
- 1 × **Carte simulateur ECG** (HAT/extension sur GPIO)
- 1 × **Arduino** (modèle utilisé : **Leonardo** avec `Serial1` pour LoRa)
- 1 × **Module LoRa** compatible TTN (EU868) selon votre carte (liaison UART)
- Câblage :
  - 1 fil signal (simulateur → Arduino entrée analogique)
  - 1 fil masse (simulateur → Arduino masse)
- (Pour mise en œuvre) écran HDMI + clavier + souris (ou SSH)

---

## Pré-requis logiciels

### Sur Raspberry Pi
- OS Raspberry Pi (Debian/Raspberry Pi OS)
- Accès réseau (Ethernet ou Wi-Fi)
- Application du **simulateur ECG** fournie par l’enseignant (package `.deb`)
- SPI activé (si requis par la carte)

### Sur PC / environnement dev
- Arduino IDE
- Node-RED (sur Raspberry ou PC)
- MariaDB (sur Raspberry ou serveur)
- Accès au compte/console **TTN** (identifiants fournis par l’enseignant)

---

## Arborescence du dépôt

Exemple recommandé :

