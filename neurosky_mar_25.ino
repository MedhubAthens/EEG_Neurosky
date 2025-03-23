#include <SoftwareSerial.h>

// Pins for SoftwareSerial
#define RX_PIN  10
#define TX_PIN  11
SoftwareSerial BT(RX_PIN, TX_PIN);

#define BAUDRATE    57600
#define DEBUGOUTPUT 0

// -----------------------------------------------------
// Global Variables
// -----------------------------------------------------
byte  generatedChecksum = 0;
byte  checksum          = 0;
int   payloadLength     = 0;
byte  payloadData[64]   = {0};

// ---- EEG metrics (store last known) ----
byte    poorQuality = 200;   // Initially unknown/poor
byte    attention   = 0;
byte    meditation  = 0;
int16_t rawSignal   = 0;     // signed 16-bit raw EEG

// ---- EEG power values (store last known) ----
unsigned long delta     = 0;
unsigned long theta     = 0;
unsigned long lowAlpha  = 0;
unsigned long highAlpha = 0;
unsigned long lowBeta   = 0;
unsigned long highBeta  = 0;
unsigned long lowGamma  = 0;
unsigned long midGamma  = 0;

void setup() {
  Serial.begin(115200);         // USB serial (debug monitor)
  BT.begin(BAUDRATE);           // MindWave on SoftwareSerial at 57600
  Serial.println("MindWave Example: RawSignal + Attention/Meditation");
}

// Read one byte (blocking) from SoftwareSerial
byte readOneByte() {
  while (!BT.available());
  return BT.read();
}

void loop() {
  // Look for sync bytes 0xAA 0xAA
  if (readOneByte() == 0xAA) {
    if (readOneByte() == 0xAA) {
      // Payload length
      payloadLength = readOneByte();
      if (payloadLength > 169) return;  // Invalid length, skip

      // Read payload
      generatedChecksum = 0;
      for (int i = 0; i < payloadLength; i++) {
        payloadData[i] = readOneByte();
        generatedChecksum += payloadData[i];
      }

      // Read checksum
      checksum          = readOneByte();
      generatedChecksum = 255 - generatedChecksum; // 1's complement

      // Validate
      if (checksum == generatedChecksum) {
        parsePayload(payloadData, payloadLength);
      } else {
        Serial.println("Checksum error!");
      }
    }
  }
}

// -----------------------------------------------------
// parsePayload() - Reads each code, updates global variables
// -----------------------------------------------------
void parsePayload(byte* payload, int length) {
  Serial.println("\nParsing Payload...");
  Serial.print("Payload Data: ");
  
  for (int i = 0; i < length; i++) {
    Serial.print(payload[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  for (int i = 0; i < length; i++) {
    byte code = payload[i];

    Serial.print("Code: 0x");
    Serial.println(code, HEX);

    switch (code) {
      case 0x02: {  // Poor Quality
        i++;
        poorQuality = payload[i];
        Serial.print("Poor Quality: ");
        Serial.println(poorQuality);
        break;
      }

      case 0x04: {  // Attention
        i++;
        attention = payload[i];
        Serial.print(">> Attention updated: ");
        Serial.println(attention);
        break;
      }

      case 0x05: {  // Meditation
        i++;
        meditation = payload[i];
        Serial.print(">> Meditation updated: ");
        Serial.println(meditation);
        break;
      }

      case 0x80: {  // Raw EEG
        i++;
        byte vLength = payload[i];  // should be 2
        if (vLength == 2 && (i + 2) < length) {
          i++;
          rawSignal = (int16_t)((payload[i] << 😎 | payload[i + 1]);
          i++;

          // Print raw signal
          Serial.print("RawSignal: ");
          Serial.print(rawSignal);
          Serial.print("  (PoorQuality=");
          Serial.print(poorQuality);
          Serial.print(", Att=");
          Serial.print(attention);
          Serial.print(", Med=");
          Serial.print(meditation);
          Serial.println(")");
        } else {
          i += vLength; // skip incorrectly sized data
        }
        break;
      }

      case 0x83: {  // EEG Power Values
        i++;
        byte vLength = payload[i];
        if (vLength == 24 && (i + 24) < length) {
          delta     = parseEEGValue(payload, &i);
          theta     = parseEEGValue(payload, &i);
          lowAlpha  = parseEEGValue(payload, &i);
          highAlpha = parseEEGValue(payload, &i);
          lowBeta   = parseEEGValue(payload, &i);
          highBeta  = parseEEGValue(payload, &i);
          lowGamma  = parseEEGValue(payload, &i);
          midGamma  = parseEEGValue(payload, &i);

          Serial.println(">> EEG Power updated:");
          Serial.print("   Delta: ");   Serial.println(delta);
          Serial.print("   Theta: ");   Serial.println(theta);
          Serial.print("   LowAlpha: ");Serial.println(lowAlpha);
          Serial.print("   HighAlpha: ");Serial.println(highAlpha);
          Serial.print("   LowBeta: "); Serial.println(lowBeta);
          Serial.print("   HighBeta: ");Serial.println(highBeta);
          Serial.print("   LowGamma: ");Serial.println(lowGamma);
          Serial.print("   MidGamma: ");Serial.println(midGamma);
        } else {
          i += vLength; // skip if invalid
        }
        break;
      }

      default:
        break;
    }
  }

  // Warn if poor quality is high
  if (poorQuality > 50) {
    Serial.println("WARNING: Poor signal quality detected!");
  }
}

// -----------------------------------------------------
// parseEEGValue() - helper for 3-byte EEG power values
// -----------------------------------------------------
unsigned long parseEEGValue(byte* payload, int* index) {
  (*index)++;
  unsigned long value = ((unsigned long)payload[*index]) << 16;  (*index)++;
  value |= ((unsigned long)payload[*index]) << 8;                (*index)++;
  value |=  (unsigned long)payload[*index];
  return value;
}
