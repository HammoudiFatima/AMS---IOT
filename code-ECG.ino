#include <TheThingsNetwork.h>
#include <math.h>

// =============================================================
// 1) CONFIGURATION LORA (Arduino Leonardo + module RN2483/RN2903)
// =============================================================
#define loraSerial Serial1
#define debugSerial Serial
#define freqPlan TTN_FP_EU868

const char *appEui = "0000000000000000";
const char *appKey = "63C219DD35CC8518F4C0F07EA7765429";

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

// =============================================================
// 2) PARAMETRES (ALERTES / ENVOI)
// =============================================================
const double HR_MIN = 50.0;
const double HR_MAX = 120.0;       // seuil d'alerte
const double HRV_LOW = 20.0;       // seuil d'alerte HRV (quand HRV est "ready")
const double HRV_HIGH = 200.0;     // garde-fou (signal foireux / faux pics)

// Pour filtrer les RR (anti faux battements) :
const double HR_MAX_PHYS = 180.0;  // filtre RR (anti doubles détections)

// Envois
const unsigned long SUMMARY_PERIOD  = 180000UL; // 3 min
const unsigned long ALERT_COOLDOWN  = 60000UL;  // 1 min
const unsigned long ALERT_REPEAT    = 60000UL;  // 1 min

unsigned long lastSummary = 0;
unsigned long lastAlert = 0;
unsigned long lastAlertRepeat = 0;

enum AlertState { NORMAL, ALERT_ACTIVE };
AlertState state = NORMAL;

// =============================================================
// 3) ECG SIGNAL (A0)
// =============================================================
const int PIN_SIGNAL = A0;
const unsigned long SAMPLE_INTERVAL = 10UL;  // ~100 Hz

// ⚡ OPTIMISATION 1 : Période réfractaire ajustée pour arythmie
const int REFRACTORY_MS = 150;  // Réduit de 350 → 150 ms (permet RR min ~200ms)

// --- Lissage du signal (moyenne glissante) ---
// ⚡ OPTIMISATION 2 : Lissage réduit pour plus de réactivité
const int NUM_READINGS = 3;  // Réduit de 5 → 3 (moins de délai)
int readings[NUM_READINGS];
int readIndex = 0;
long total = 0;
int averageSignal = 0;

// --- Détection adaptative ---
float baseline = 0;
float dynamicThreshold = 20;
float maxSignal = -1000, minSignal = 1000;
unsigned long lastSample = 0;
unsigned long lastBeatTime = 0;

// =============================================================
// 4) BUFFERS HR / RR + CALCUL HRV
// =============================================================
#define RR_BUF 10
#define HR_BUF 6

double rr_buffer[RR_BUF];
int rr_index = 0, rr_count = 0;

double hr_buffer[HR_BUF];
int hr_index = 0, hr_count = 0;

double hr = 0;       // HR instantanée (bpm)
double hr_avg = 0;   // HR moyenne glissante (bpm)
double hrv = 0;      // HRV lissée (ms) = SDNN approx sur RR
double hrv_raw = 0;  // HRV brute (ms)

static double meanBuf(const double* buf, int n) {
  if (n <= 0) return 0;
  double s = 0;
  for (int i = 0; i < n; i++) s += buf[i];
  return s / n;
}

// HRV = écart-type des RR (SDNN) en ms
double computeHRV() {
  if (rr_count < 2) return 0;

  double mean = meanBuf(rr_buffer, rr_count);

  double variance = 0;
  for (int i = 0; i < rr_count; i++) {
    double d = rr_buffer[i] - mean;
    variance += d * d;
  }
  variance /= rr_count;
  return sqrt(variance);
}

// =============================================================
// 5) ENVOI LORAWAN
// =============================================================
void sendPayload(double hr_val, double hrv_val, AlertState alert) {
  uint16_t hr_enc  = (uint16_t)(hr_val  * 100.0);
  uint16_t hrv_enc = (uint16_t)(hrv_val * 100.0);
  byte alert_level = (alert == ALERT_ACTIVE) ? 1 : 0;

  byte payload[5] = {
    highByte(hr_enc), lowByte(hr_enc),
    highByte(hrv_enc), lowByte(hrv_enc),
    alert_level
  };

  ttn.sendBytes(payload, sizeof(payload));

  debugSerial.print("[LORA] HR:");
  debugSerial.print(hr_val, 2);
  debugSerial.print(" bpm | HRV:");
  debugSerial.print(hrv_val, 2);
  debugSerial.print(" ms | Etat:");
  debugSerial.println(alert_level ? "ALERTE" : "NORMAL");
}

// =============================================================
// 6) DETECTION DU PIC R (adaptatif + seuil amplitude)
// =============================================================
bool detectPeak(int val) {
  static float prev = 0;
  unsigned long now = millis();

  // Baseline (filtre passe-bas)
  baseline = 0.997f * baseline + 0.003f * (float)val;
  float signal = (float)val - baseline;

  // pente (diff)
  float slope = signal - prev;
  prev = signal;

  // tracking amplitude locale
  if (signal > maxSignal) maxSignal = signal;
  if (signal < minSignal) minSignal = signal;

  float amplitude = maxSignal - minSignal;
  if (amplitude < 5) amplitude = 5;

  // ⚡ OPTIMISATION 3 : Seuil plus réactif
  float adaptivePart = fabs(slope) * 1.5f + amplitude * 0.15f;  // Ajusté
  dynamicThreshold = 0.85f * dynamicThreshold + 0.15f * adaptivePart;  // Plus réactif

  // seuil minimal lié à l'amplitude
  float minThresh = amplitude * 0.12f + 3.0f;  // Réduit légèrement
  if (dynamicThreshold < minThresh) dynamicThreshold = minThresh;

  dynamicThreshold = constrain(dynamicThreshold, 2, 80);  // Plafond réduit

  // condition beat (anti double-détection via refractory)
  bool beat = false;
  if (slope > dynamicThreshold && signal > 0 && (now - lastBeatTime) > (unsigned long)REFRACTORY_MS) {
    beat = true;
    lastBeatTime = now;

    // reset local amplitude trackers
    maxSignal = signal;
    minSignal = signal;
  }

  // ⚡ OPTIMISATION 4 : Décroissance plus rapide
  maxSignal *= 0.993f;  // Plus rapide que 0.997
  minSignal *= 0.993f;
  dynamicThreshold *= 0.995f;

  return beat;
}

// =============================================================
// 7) SETUP
// =============================================================
void setup() {
  loraSerial.begin(57600);
  debugSerial.begin(9600);
  while (!debugSerial && millis() < 3000);

  debugSerial.println("=== ECG LORA PRO (A0 -> R-peak -> RR -> HR/HRV) ===");
  debugSerial.println("Version optimisee pour arythmie");

  // init moyenne glissante
  total = 0;
  int init = analogRead(PIN_SIGNAL);
  baseline = init;
  for (int i = 0; i < NUM_READINGS; i++) {
    readings[i] = init;
    total += init;
    delay(2);
  }
  averageSignal = (int)(total / NUM_READINGS);

  debugSerial.println("Connexion TTN...");
  ttn.join(appEui, appKey);
  debugSerial.println("TTN OK");
  
  // ⚡ MODIFICATION : Premier envoi immédiat après 30 sec
  lastSummary = millis() - (SUMMARY_PERIOD - 30000UL);  // Force envoi dans 30 sec
}

// =============================================================
// 8) LOOP
// =============================================================
void loop() {
  unsigned long now = millis();

  // --- échantillonnage ECG à ~100Hz ---
  if (now - lastSample >= SAMPLE_INTERVAL) {
    lastSample = now;

    // moyenne glissante sur signal
    total -= readings[readIndex];
    readings[readIndex] = analogRead(PIN_SIGNAL);
    total += readings[readIndex];
    readIndex = (readIndex + 1) % NUM_READINGS;
    averageSignal = (int)(total / NUM_READINGS);

    // détection du pic
    if (detectPeak(averageSignal)) {
      static unsigned long lastBeat = 0;
      unsigned long rr = now - lastBeat;
      lastBeat = now;

      // filtre RR cohérent (anti faux battements)
      unsigned long rrMin = (unsigned long)(60000.0 / HR_MAX_PHYS); // ex: 333 ms si 180 bpm
      unsigned long rrMax = (unsigned long)(60000.0 / HR_MIN);      // ex: 1200 ms si 50 bpm

      if (rr >= rrMin && rr <= rrMax) {
        // HR instantanée
        hr = 60000.0 / (double)rr;

        // buffer HR (moyenne glissante)
        hr_buffer[hr_index] = hr;
        hr_index = (hr_index + 1) % HR_BUF;
        if (hr_count < HR_BUF) hr_count++;
        hr_avg = meanBuf(hr_buffer, hr_count);

        // buffer RR pour HRV
        rr_buffer[rr_index] = (double)rr;
        rr_index = (rr_index + 1) % RR_BUF;
        if (rr_count < RR_BUF) rr_count++;

        // HRV brute + lissage exponentiel
        hrv_raw = computeHRV();
        // ⚡ OPTIMISATION 5 : HRV plus réactif pour arythmie
        hrv = 0.70 * hrv + 0.30 * hrv_raw;  // Plus réactif que 0.90/0.10

        // DEBUG (plotter friendly)
        debugSerial.print(averageSignal); debugSerial.print(",");
        debugSerial.print((int)baseline); debugSerial.print(",");
        debugSerial.print((int)dynamicThreshold); debugSerial.print(",");
        debugSerial.print(hr_avg, 2); debugSerial.print(",");
        debugSerial.print(hrv, 2); debugSerial.print(",");
        debugSerial.println(rr_count);
      }
    }
  }

  // attendre HR valide
  if (hr_avg < 30) return;

  // HRV "ready" seulement quand la fenêtre RR est remplie
  bool hrv_ready = (rr_count >= RR_BUF);

  // anomalie: HR hors bornes, ou HRV hors borne quand prêt
  bool anomaly = (hr_avg < HR_MIN || hr_avg > HR_MAX ||
                 (hrv_ready && (hrv < HRV_LOW || hrv > HRV_HIGH)));

  // ⚡ DEBUG : Afficher l'état toutes les 10 secondes
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 10000) {
    lastDebug = now;
    debugSerial.print("[DEBUG] State: ");
    debugSerial.print(state == NORMAL ? "NORMAL" : "ALERT");
    debugSerial.print(" | Anomaly: ");
    debugSerial.print(anomaly ? "OUI" : "NON");
    debugSerial.print(" | Prochain envoi dans: ");
    debugSerial.print((SUMMARY_PERIOD - (now - lastSummary)) / 1000);
    debugSerial.println(" sec");
  }

  // ---------- LOGIQUE ENVOI ----------
  if (state == NORMAL && now - lastSummary >= SUMMARY_PERIOD) {
    sendPayload(hr_avg, hrv, NORMAL);
    lastSummary = now;
  }

  if (anomaly && state == NORMAL) {
    debugSerial.println("⚠️ ALERTE ACTIVEE !");
    sendPayload(hr_avg, hrv, ALERT_ACTIVE);
    state = ALERT_ACTIVE;
    lastAlert = now;
    lastAlertRepeat = now;
  }

  if (state == ALERT_ACTIVE && anomaly && now - lastAlertRepeat >= ALERT_REPEAT) {
    debugSerial.println("🔁 Alerte persistante...");
    sendPayload(hr_avg, hrv, ALERT_ACTIVE);
    lastAlertRepeat = now;
    lastAlert = now;
  }

  if (state == ALERT_ACTIVE && !anomaly && now - lastAlert >= ALERT_COOLDOWN) {
    debugSerial.println("✅ Retour a la normale");
    sendPayload(hr_avg, hrv, NORMAL);
    state = NORMAL;
    lastSummary = now;
  }
}