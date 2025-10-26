
const int ledPin = 2;
// esp8266 pinout https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  digitalWrite(ledPin, HIGH); 
  delay(500);
  digitalWrite(ledPin, LOW); 
  delay(500);
}
