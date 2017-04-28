#include <Servo.h>

int data;
int i = 0;

void setup() {
  
  Serial1.begin(9600);
  Serial.begin(9600);
  pinMode(7, OUTPUT);

}

void loop() {
  if(Serial1.available() > 0)
   {
      data = Serial1.read();
      i++;
      Serial.print(data-48);
      if(data == '1')
        digitalWrite(7, HIGH);
      if(data == '0')
        digitalWrite(7, LOW);
   }
   
}
