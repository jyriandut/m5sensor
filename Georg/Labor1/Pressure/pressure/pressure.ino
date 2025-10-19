int analogPin = 32;  // можно использовать 32, 33, 36, 39 и другие входы ESP32
int val = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  val = analogRead(analogPin);  // значение 0-4095 (12 бит)
  Serial.println(val);
  delay(500);  // полсекунды пауза
}