void setup() {                
  // Initialize the digital pin as an output
  pinMode(0, OUTPUT);      // LED on Model B
  pinMode(1, OUTPUT);      // LED on Model A   
}

void loop() {
  digitalWrite(0, HIGH);   // Turn the LED on
  digitalWrite(1, HIGH);
  delay(250);             // Wait for a second
  digitalWrite(0, LOW);    // Turn the LED off
  digitalWrite(1, LOW); 
  delay(300);             // Wait for a second
}
