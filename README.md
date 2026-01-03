# Holter Connecté (ECG Simulé) - Tutoriel Complet

## 📋 Vue d'ensemble

Ce projet implémente un prototype de **Holter connecté** à vocation pédagogique, permettant la surveillance continue d'un signal ECG simulé via une architecture IoT complète. Le système combine **Edge Computing** (Arduino), **communication LoRaWAN** (The Things Network), et **supervision temps réel** (Node-RED).

### Objectif Principal
Concevoir une chaîne fonctionnelle ECG simulé → traitement local → transmission IoT → visualisation, démontrant l'intégration de technologies IoT modernes pour la santé connectée.

---

## 🎯 Caractéristiques Principales

✅ **Acquisition ECG simulée** - Signal généré à partir de profils CSV (sportif, pathologique, normal)  
✅ **Traitement Edge Computing** - Détection de battements, calcul HR/HRV sur microcontrôleur  
✅ **Communication LoRaWAN** - Transmission compacte via The Things Network (TTN)  
✅ **Dashboard en temps réel** - Visualisation Node-RED avec état NORMAL/ALERTE  
✅ **Stockage historique** - Base de données MariaDB pour analyse a posteriori  
✅ **Reproductibilité** - Dossier-tutoriel avec recette de tests complète  

---

## 🔧 Architecture Matérielle

### Composants Principaux

| Composant | Rôle | Caractéristiques |
|-----------|------|------------------|
| **Arduino Leonardo** | Nœud Edge | Acquisition ECG (A0), traitement, LoRa UART |
| **Raspberry Pi 3** | Plateforme hôte | Pilotage simulateur ECG via GPIO/SPI |
| **Simulateur ECG** | Génération signal | Carte HAT + fichiers profils CSV |
| **Module LoRa** | Communication | Connexion UART à Arduino |
| **Passerelle TTN** | Récepteur radio | Infrastructure LoRaWAN communautaire |

### Schéma de Connexion

```
┌─────────────────────────────────────────────────────────────┐
│ Raspberry Pi 3                                              │
│ ┌────────────────────────┐                                  │
│ │ Simulateur ECG (HAT)   │─── Vout_Sim (sortie analogique) │
│ │ - Profils CSV          │                                  │
│ └────────────────────────┘                                  │
└──────────────────┬──────────────────────────────────────────┘
                   │ Signal analogique
                   │ (Fil jaune)
                   ▼
         ┌──────────────────┐
         │ Arduino Leonardo │
         │ ┌──────────────┐ │
         │ │ A0: ECG      │ │◄─── Masse commune (Fil noir)
         │ │ Serial1: LoRa│ │
         │ └──────────────┘ │
         └────────┬─────────┘
                  │ LoRaWAN
                  ▼
         ┌──────────────────┐
         │ Passerelle TTN   │
         │ (CERI)           │
         └────────┬─────────┘
                  │ MQTT
                  ▼
         ┌──────────────────┐
         │ The Things       │
         │ Network (TTN)    │
         └────────┬─────────┘
                  │ MQTT
                  ▼
         ┌──────────────────┐
         │ Node-RED         │
         │ Dashboard + BDD  │
         └──────────────────┘
```

---

## 💻 Architecture Logicielle

### Couches du Système

**1. Edge (Arduino Leonardo)**
- Échantillonnage : 100 Hz (10 ms)
- Pré-traitement : moyenne glissante, estimation baseline
- Détection battements : seuil adaptatif + période réfractaire (150 ms)
- Calcul indicateurs : HR (bpm), HRV (ms)
- Machine à états : NORMAL ↔ ALERTE
- Encodage : payload compact 5 octets

**2. Réseau (TTN + LoRaWAN)**
- Protocole : LoRaWAN (longue portée, bas débit)
- Infrastructure : The Things Network (communautaire)
- Stratégie d'envoi :
  - Mode NORMAL : résumé toutes les 60 s
  - Mode ALERTE : message immédiat + répétition si persistance

**3. Supervision (Node-RED)**
- Ingestion MQTT depuis TTN
- Décodage binaire → JSON
- Dashboard : HR, HRV, état, courbes temporelles
- Stockage : MariaDB

---

## 📦 Pré-requis

### Matériel
- Arduino Leonardo
- Raspberry Pi 3 avec accès réseau
- Simulateur ECG (carte HAT)
- Module LoRa (connexion UART)
- Câbles : USB (Arduino), Ethernet/Wi-Fi (Raspberry), fils de liaison
- Ordinateur pour IDE Arduino

### Logiciels
- Arduino IDE (version 1.8.15+)
- Bibliothèques Arduino :
  - `TheThingsNetwork` (LoRaWAN)
  - Dépendances radio (compatibles LoRa)
- Node-RED installé sur Raspberry Pi
- Accès internet (TTN, GitHub)

### Accès Réseau
- Connexion TTN (accès console avec identifiants)
- Accès passerelle CERI (infrastructure locale)
- Adresse IP stable du Raspberry Pi

---

## 🚀 Guide de Mise en Place

### Étape 1 : Montage Matériel et Câblage

#### 1.1 Encapsulation du Simulateur ECG

```bash
# Éteindre le Raspberry Pi
sudo shutdown -h now

# Enfoncer la carte simulateur ECG sur le connecteur GPIO
# (Respecter l'orientation, aligner les broches)

# Réalimenter le Raspberry Pi
```

#### 1.2 Câblage du Signal ECG

**Connexions à réaliser :**

| Simulateur | → | Arduino |
|-----------|---|---------|
| Vout_Sim (jaune) | → | A0 |
| GND (noir) | → | GND |

⚠️ **Important** : Une masse commune (GND) est **indispensable** pour une lecture stable de la tension analogique.

### Étape 2 : Configuration The Things Network (TTN)

#### 2.1 Accès à la Console TTN

```
URL : https://www.thethingsnetwork.org/
1. Cliquer : "Login to The Things Network"
2. Identifiants : m2-sicom-iot
3. Sélectionner l'app : hammoudi-berkai-ecg-app
4. Sélectionner l'équipement : yan-fat
```

#### 2.2 Vérification de la Réception

Après téléversement du code Arduino :

```
TTN Console → Live data
- Observer les uplinks reçus
- Vérifier le timestamp et le payload
- Chaque message doit afficher "received" avec un RSSI/SNR
```

### Étape 3 : Configuration Arduino

#### 3.1 Installation des Dépendances

Dans Arduino IDE :
```
Sketch → Include Library → Manage Libraries
- Rechercher "TheThingsNetwork"
- Installer la dernière version
```

#### 3.2 Téléversement du Code

```
1. Ouvrir le fichier : src/arduino/holter_lorawan.ino
2. Sélectionner la carte : Tools → Board → "Arduino Leonardo"
3. Sélectionner le port : Tools → Port → /dev/ttyACM0 (ou COM...)
4. Cliquer : Upload
5. Ouvrir Serial Monitor (9600 baud) pour vérifier les logs
```

### Étape 4 : Lancement du Simulateur ECG

Sur le Raspberry Pi :

```bash
# Naviguer vers le répertoire du simulateur
cd ~/simulateur-ecg

# Lancer le simulateur avec sélection de profil
python3 ecg_generator.py --profile normal

# Profils disponibles : normal, sportif, tachycardie, bradycardie, arythmie
```

### Étape 5 : Configuration de Node-RED

#### 5.1 Démarrage de Node-RED

```bash
# Sur Raspberry Pi
node-red-start

# Attendre le message : "[info] Server now running at http://..."
# Interface accessible : http://<ip_raspberry>:1880
```

#### 5.2 Import du Flow

```
1. Ouvrir navigateur : http://<ip_raspberry>:1880
2. Menu (☰) → Import
3. Sélectionner le fichier : flows/node-red-flow.json
4. Cliquer : Import
5. Cliquer : Deploy (bouton rouge en haut à droite)
```

#### 5.3 Accès au Dashboard

```
URL : http://<ip_raspberry>:1880/ui

Affichage :
- Jauge HR (bpm)
- Jauge HRV (ms)
- Indicateur état (NORMAL / ALERTE)
- Courbes temporelles HR/HRV
- Historique des alertes
```

---

## 📊 Explication des Programmes

### Programme Arduino : Chaîne de Traitement

#### 1. Acquisition (100 Hz)

```cpp
const int SAMPLE_INTERVAL = 10;  // 10 ms → 100 Hz

if (now - lastSample >= SAMPLE_INTERVAL) {
    value = analogRead(A0);        // Lecture 0-1023
    averaged = movingAverage(value); // Lissage
}
```

#### 2. Détection des Battements

Algorithme **seuil adaptatif** :

```cpp
// 1. Estimation de la baseline (ligne de base)
baseline = 0.997 * baseline + 0.003 * valeur;

// 2. Centrage du signal
signal = valeur - baseline;

// 3. Calcul du seuil dynamique
seuil = 0.85 * seuil + 0.15 * (|pente| × 1.5 + amplitude × 0.15);

// 4. Détection + période réfractaire
if (pente > seuil && signal > 0 && temps - dernier_beat > 150 ms) {
    battement détecté ✓
}
```

**Avantages** :
- Adaptatif aux variations d'amplitude entre profils
- Robustesse au bruit
- Évite le double comptage via la période réfractaire

#### 3. Calcul HR et HRV

**Intervalle RR** (entre deux battements) :

```cpp
RR = temps_actuel - temps_dernier_battement  // ms
HR = 60000 / RR                                // bpm
```

**Variabilité (HRV)** - écart-type des RR sur fenêtre de 10 battements :

```cpp
HRV = √(moyenne((RRᵢ - moyenne_RR)²))  // SDNN approximé
```

Lissage exponentiel pour stabiliser :

```cpp
HRV = 0.70 × HRV_précédent + 0.30 × HRV_brut
```

#### 4. Détection d'Anomalies

Machine à états :

```cpp
if (HR < 60 || HR > 100 || HRV < seuil_bas) {
    anomalie = true
    État → ALERTE
} else {
    anomalie = false
}
```

**Seuils définis** :
- HR_MIN = 60 bpm
- HR_MAX = 100 bpm
- HRV_LOW = 20 ms
- HRV_HIGH = 200 ms

#### 5. Stratégie d'Envoi LoRaWAN

```
Mode NORMAL
  └─► Envoyer résumé tous les 60 s

Mode ALERTE (anomalie détectée)
  └─► Envoyer immédiatement
  └─► Répéter tous les 15 s si persistance
  
Retour NORMAL
  └─► Attendre 30 s de stabilité
  └─► Envoyer message retour à NORMAL
  └─► Reprendre résumés périodiques
```

#### 6. Encodage Compact (5 octets)

```cpp
Payload = [HR_high, HR_low, HRV_high, HRV_low, state]
          [  octet0     octet1    octet2   octet3  octet4]

Exemple : HR=72 bpm, HRV=45 ms, NORMAL
  → 7200 → [0x1C, 0x20]
  → 4500 → [0x11, 0x94]
  → 0 (NORMAL)
  → Payload = [0x1C, 0x20, 0x11, 0x94, 0x00]
```

### Node-RED : Décodage et Visualisation

#### Nœud de Décodage (Function)

```javascript
// Recevoir payload MQTT depuis TTN
let msg_in = msg.payload.uplink_message.frm_payload;

// Décoder en base64
let bytes = Buffer.from(msg_in, 'base64');

// Extraire les valeurs
let hr = ((bytes[0] << 8) | bytes[1]) / 100.0;
let hrv = ((bytes[2] << 8) | bytes[3]) / 100.0;
let state = bytes[4] === 0 ? "NORMAL" : "ALERTE";

// Structurer en JSON
msg.payload = {
    hr: hr,
    hrv: hrv,
    state: state,
    timestamp: new Date().toISOString()
};

return msg;
```

#### Dashboard - Affichage

- **Jauge HR** : 0-180 bpm, code couleur (vert normal, rouge alerte)
- **Jauge HRV** : 0-300 ms
- **Indicateur État** : badge NORMAL (vert) / ALERTE (rouge clignotant)
- **Courbes** : historique 10 min en graphiques temps réel
- **Table des alertes** : liste horodatée des événements

---

## 🧪 Tests et Validation

### Tests Unitaires

**Profils de Test CSV** :

| Profil | HR (bpm) | RR (ms) | Résultat Attendu |
|--------|----------|---------|------------------|
| Normal | 70-80 | 750-850 | NORMAL |
| Sportif | 50-65 | 950-1200 | NORMAL |
| Tachycardie | 130-150 | 400-460 | ALERTE |
| Bradycardie | 35-50 | 1200-1700 | ALERTE |
| Arythmie | Variable | Irrégulier | ALERTE |

**Exécution** :

```bash
python3 tests/test_profiles.py
# Résultat : ✓ Normal / ✓ Pathologique / ✓ Edge cases
```

### Tests d'Intégration

| Test | Étapes | Critère de Succès |
|------|--------|------------------|
| **T1 : Uplink TTN** | Lancer Arduino → vérifier TTN Console | Reçu avec RSSI/SNR ✓ |
| **T2 : Décodage Node-RED** | Uplink → Node-RED Function | Valeurs décodées cohérentes ✓ |
| **T3 : Dashboard** | Mise à jour automatique | Jauges et courbes réagissent ✓ |
| **T4 : Alerte** | Profil pathologique | Changement d'état en <5s ✓ |

### Scénario de Validation (10 min)

```
1. [0 min]   Démarrer profil NORMAL
             ✓ État NORMAL affiché
             ✓ Résumés toutes les 60 s sur TTN
             ✓ Dashboard met à jour HR/HRV

2. [3 min]   Basculer vers profil TACHYCARDIE
             ✓ Alerte déclenchée <5s
             ✓ Message alerte visible sur TTN
             ✓ Dashboard affiche ALERTE en rouge

3. [5 min]   Alerte se répète tous les 15 s
             ✓ Persistance de l'anomalie confirmée

4. [7 min]   Revenir à profil NORMAL
             ✓ Après 30 s, retour à NORMAL
             ✓ Message "retour normal" sur TTN
             ✓ Dashboard repasse au vert

5. [10 min]  Arrêt
             ✓ Historique des alertes sauvegardé en MariaDB
             ✓ Export CSV possible
```

---

## 📁 Structure du Dépôt

```
AMS-IOT/
├── README.md                          # Ce fichier
├── src/
│   ├── arduino/
│   │   ├── holter_lorawan.ino         # Programme principal Arduino
│   │   ├── detectBeat.cpp             # Détection de pics
│   │   └── hrv_calc.cpp               # Calcul HRV
│   ├── raspberry/
│   │   └── ecg_generator.py           # Générateur ECG (profils CSV)
│   └── nodered/
│       └── flows.json                 # Flow Node-RED complet
├── data/
│   ├── profiles/
│   │   ├── normal.csv                 # Profil ECG normal
│   │   ├── tachycardie.csv            # Profil tachycardie
│   │   ├── bradycardie.csv            # Profil bradycardie
│   │   └── arythmie.csv               # Profil arythmie
│   └── test_results/
│       └── validation_log.txt          # Résultats de tests
├── docs/
│   ├── AMS-IOT.pdf                    # Documentation technique complète
│   ├── INSTALLATION.md                 # Guide installation détaillé
│   └── TROUBLESHOOTING.md             # Dépannage courant
└── tests/
    ├── test_profiles.py               # Tests unitaires profils
    ├── test_integration.sh            # Tests intégration
    └── test_data/                     # Jeux de données de test
```

---

## 🔍 Dépannage Courant

### Problème : Aucune lecture analogique (A0 reste à 0)

**Causes possibles** :
- Masse commune (GND) non connectée
- Carte simulateur mal encochée sur Raspberry Pi
- Arduino Leonardo sur mauvais port COM

**Solution** :
```bash
# Vérifier la connexion physique GND
# Relancer Arduino IDE, sélectionner le bon port COM
# Tester avec sketch simple : analogRead() et Serial.println()
```

### Problème : Aucun uplink reçu sur TTN

**Causes possibles** :
- Module LoRa non alimenté ou non initialisé
- Clés de sécurité TTN (DevEUI/AppEUI/AppKey) incorrectes
- Passerelle hors de portée

**Solution** :
```cpp
// Dans Arduino : afficher dans Serial Monitor
Serial.println(DevEUI);
Serial.println(AppKey);

// Vérifier sur TTN Console que DevEUI/AppKey correspondent
// Vérifier la distance à la passerelle (<5 km en ligne de vue)
```

### Problème : Dashboard Node-RED vide

**Causes possibles** :
- Flow non déployé
- Connexion MQTT à TTN non établie
- Payload mal décodé

**Solution** :
```
Node-RED :
1. Vérifier la présence du nœud MQTT (couleur bleue = connecté)
2. Ajouter des debug nodes pour tracer les messages
3. Vérifier les logs : Node-RED console
4. Déployer à nouveau le flow (bouton Deploy)
```

### Problème : Lectures HR/HRV instables ou nulles

**Causes possibles** :
- Bruit électrique sur l'entrée A0
- Seuil de détection trop élevé ou trop bas
- Baseline mal estimée

**Solution** :
```cpp
// Augmenter la fenêtre de lissage
const int NUM_READINGS = 5;  // au lieu de 3

// Ajuster dynamiquement le seuil adaptatif
dynamicThreshold = 0.90 * dynamicThreshold + 0.10 * adaptivePart;

// Vérifier la stabilité avec Serial Plotter
```

---

## 📈 Perspectives d'Améliorations

**Court terme** :
- Filtrage passe-bande (butterworth) pour réduire le bruit 60 Hz
- Détecteur QRS plus robuste (transformée de Pan-Tompkins)
- Interface web pour configuration des seuils en temps réel

**Moyen terme** :
- Stockage time-series (InfluxDB) pour analyse avancée
- Historique avancé avec statistiques (min/max/moyenne)
- Notifications par email/SMS en cas d'alerte critique

**Long terme** :
- Intégration ML (détection patterns arythmies)
- Synchronisation multi-patients (plusieurs Holter)
- Certification/validation clinique (conformité MDD 93/42/CEE)

---

## 📞 Support et Ressources

**Dépôt GitHub** :  
https://github.com/HammoudiFatima/AMS---IOT

**Documentation Externe** :
- [The Things Network Docs](https://www.thethingsnetwork.org/docs/)
- [Arduino Reference](https://www.arduino.cc/reference/)
- [Node-RED Documentation](https://nodered.org/docs/)
- [Pan & Tompkins QRS Detection](https://ieeexplore.ieee.org/document/4122029)

**Contacts (Projet)** :
- **Étudiants** : Hammoudi Fatima & Berkai Yanis
- **Encadrants** : Marc Silanus & Eric Zouania
- **Formation** : Master 2 SYRIUS, Janvier 2026

---

## 📝 Licence et Mentions Légales

Ce projet est fourni à titre pédagogique. **Il ne constitue pas un dispositif médical** et ne doit pas être utilisé pour le diagnostic ou le suivi clinique.

**Cadre pédagogique** : Les signaux ECG sont simulés ; l'objectif est la démonstration de l'architecture IoT/Edge, pas la validation médicale.

**Attribution** : Ce travail s'appuie sur les standards ECG (référence [1-6] dans la documentation PDF).

---

**Version** : 1.0 | **Date** : Janvier 2026 | **État** : Production✓
