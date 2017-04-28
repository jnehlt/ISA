/**********************
* TIMER v1.5          *
* SEBASTIAN BĄKAŁA    *
**********************/

#include "Buttons.h"
#include "ISA7SegmentDisplay.h"
#include "DueTimer.h"
#include "LiquidCrystal.h"
#include "ISALedControl.h"
#include <SPI.h>
#include "SH1106_SPI.h"

#include "obraz.h"
SH1106_SPI_FB lcd2;


//pin24 - speaker
ISA7SegmentDisplay seg;
Buttons set;
LiquidCrystal lcd(26, 28, 29, 30, 31, 32);
ISALedControl osiem;

float f_interwal;
int x=0, y=0, pomocnicza_do_wygaszania=1;
int pos, i, __time, kolejnaNiepotrzebnaZmienna;
bool seg_blink, what;

bool gameOver = false;
int score = 0;
const int width = 8, height = 8;
int fruitX, fruitY;
int fX[100];
int fY[100];
enum direct {STOP = 0, LEFT, RIGHT, UP, DOWN};
direct dir;

bool dark = true;
const byte bez_kropki[10] = { //tablica bez kropek (dla 7segmentowego)+
                              B11101101, // 0                         |
                              B00001001, // 1                         |
                              B10110101, // 2                         |
                              B10011101, // 3                         |
                              B01011001, // 4                         |
                              B11011100, // 5                         |
                              B11111100, // 6                         |
                              B00001101, // 7                         |
                              B11111101, // 8                         |
                              B11011101  // 9                         |
                            };  //------------------------------------+

const byte z_kropka[10] = { //tablica z kropkami (dla 7segmentowego)--+
                            B11101111, // 0.                          |
                            B00001011, // 1.                          |
                            B10110111, // 2.                          |
                            B10011111, // 3.                          |
                            B01011011, // 4.                          |
                            B11011110, // 5.                          |
                            B11111110, // 6.                          |
                            B00001111, // 7.                          |
                            B11111111, // 8.                          |
                            B11011111  // 9.                          |
                          };  //--------------------------------------+

void hello(){ //powitanie (Tylko pierwsze uruchomienie) (pikanie)---+
  int i = 2;                                                      //|
  while(i)                                                        //|
  {                                                               //|
    digitalWrite(24, HIGH);                                       //|
    delay(60);                                                    //|
    digitalWrite(24, LOW);                                        //|
    delay(60);                                                    //|
    i--;                                                          //|
  }                                                               //|
  return;                                                         //|
}//-----------------------------------------------------------------+

void osiem_na_osiem(bool indeks_stanu_wyswietlacza, int etap){
  static const int stan_wyswietlacza[2]={0,255};
  switch (etap)
  {
    case 0: //zapal lub zgas wszystkie diody na wyswietlaczu
      for(int zapalaj = 0; zapalaj < 8; zapalaj++)
        osiem.setRow(zapalaj, stan_wyswietlacza[indeks_stanu_wyswietlacza]);
    break;

    case 1: //oblicz z jaka gestoscia maja gasnac diody
      if(__time)
        f_interwal = 1000000.0 * (__time )/ 64.0;
    break;
  }
  return;
}

void wygaszaj_diody(){
  static int tablica_stanow[8][8]={
                                    1,1,1,1,1,1,1,1,
                                    1,1,1,1,1,1,1,1,
                                    1,1,1,1,1,1,1,1,
                                    1,1,1,1,1,1,1,1,
                                    1,1,1,1,1,1,1,1,
                                    1,1,1,1,1,1,1,1,
                                    1,1,1,1,1,1,1,1,
                                    1,1,1,1,1,1,1,1
                                  };
  
  do{
    x = random(0, 8);
    y = random(0, 8);
    if(tablica_stanow[x][y])
    {
      osiem.setLed(x, y, 0);
      tablica_stanow[x][y] = 0;
      break;
    }
  }while(1);
  pomocnicza_do_wygaszania++;
  if(pomocnicza_do_wygaszania == 65){
    Timer6.stop();
    Timer6.detachInterrupt();
  }

}

void startowa(){ //przypisz od nowa wartosci startowe zmiennych globalnych, oraz odłącz uchwyty Timerów-------------------------------+
/////// STAN STARTOWY WYŚWIETLACZA ///////                                                                                            |
  seg.setLed(237, 0);              ///////                                                                                            |
  seg.setLed(237, 1);              ///////                                                                                            |
  seg.setLed(239, 2);              ///////                                                                                            |
  seg.setLed(237, 3);              ///////                                                                                            |
/////// STAN STARTOWY WYŚWIETLACZA ///////                                                                                            |
  Timer4.stop();                                                                                                                    //|
  Timer5.stop();                                                                                                                    //|
  Timer4.detachInterrupt(); //odlaczenie poprzedniego uchwytu (w przypadku rozpoczecia programu kolejny raz)                          |
  Timer5.detachInterrupt(); //odlaczenie poprzedniego uchwytu (w przypadku rozpoczecia programu kolejny raz)                          |
  Timer4.attachInterrupt(set_tHrs_tMins); //timer odpowiedzialny za wlaczenie funkcji odpowiedzialnej za wprowadzanie danych          |
  Timer5.attachInterrupt(mryga);  //timer odpowiedzialny za miganie aktualnie wprowadzanej wartosci na panelu liczbowym    |          |
  Timer4.start(25000);  //--------------------------------------------------------------------------------------------|----+          |
  Timer5.start(500000); //--------------------------------------------------------------------------------------------+               |
  osiem_na_osiem(0, 0);                                                                                                             //|
  lcd.setCursor(0,0);                                                                                                               //|
  lcd.print("Dziesiatki");                                                                                                          //|
  lcd.setCursor(0,1);                                                                                                               //|
  lcd.print("minut");                                                                                                              //|
  pos = 3;  // ustawienie pozycji migajacego segmentu na segment 'dziesiatki minut'                                                   |
  __time = 0; // dla pewnosci ustaw wartosc czasu na 0                                                                                |
  kolejnaNiepotrzebnaZmienna = 0; //zmienna pomocnicza uzywana w kilku funcjach                                                       |
  seg_blink = true;                                                                                                                 //|
  what = true;                                                                                                                      //|
  return;                                                                                                                           //|
}//-----------------------------------------------------------------------------------------------------------------------------------+

void rozpocznij(){ //funkcja rozpoczynajaca odliczanie
  seg.setLed(bez_kropki[(__time / 600)], 3);
  seg.setLed(bez_kropki[(__time % 60) / 10], 1);
  seg.setLed(bez_kropki[(__time % 10)], 0);

  if(__time-1 <= 0){
      Timer5.stop();
      Timer5.detachInterrupt();
      Timer5.attachInterrupt(alarm);
      Timer5.start(250000);
      return;
  }
  __time--;
}

void alarm(){ //funkcja do wydania sygnalu dzwiekowego i swietlnego w momencie skonczenia czasu odliczania

  if(kolejnaNiepotrzebnaZmienna){
    int sign[3]={0,237,239};
    lcd.clear();
    if(what){
      seg.setLed(sign[what], 0);
      seg.setLed(sign[what], 1);
      seg.setLed(sign[what], 3);
      seg.setLed(sign[what+1], 2);
      lcd.setCursor(0,0);
      lcd.print("ALARM");
      digitalWrite(24, sign[what]);
    }
    else{
      seg.setLed(sign[what], 0);
      seg.setLed(sign[what], 1);
      seg.setLed(sign[what], 2);
      seg.setLed(sign[what], 3);
      digitalWrite(24, sign[what]);
    }
    what = !what;
    kolejnaNiepotrzebnaZmienna--;
  }
  else{
    startowa();
  }
}

void mryga(){ //funkcja odpowiedzialna za mruganie (kropki przy odliczaniu lub aktualnie ustawianej cyfry)
  if(seg_blink){
    if(pos == 2){
      if(what){
        seg.setLed(z_kropka[i], pos);
      }
      else
        seg.setLed(z_kropka[(__time / 60) % 10], 2);
    }
    else
      seg.setLed(bez_kropki[i], pos);
  }
  else{
    if(what)
      seg.setLed(0, pos);
    else
      seg.setLed(bez_kropki[(__time / 60) % 10], 2);
  }
  seg_blink = !seg_blink;
}

void enter(){
  lcd.setCursor(0,0);
  lcd.clear();
  switch (pos)
  {
    case 3:
      lcd.print("Minuty");
      __time += 600 * i;
      seg.setLed(bez_kropki[i], pos);
      break;
    case 2:
      lcd.print("Dziesiatki");
      lcd.setCursor(0,1);
      lcd.print("sekund");
      __time += 60 * i;
      seg.setLed(z_kropka[i], pos);
      break;
    case 1:
      lcd.print("Sekundy");
      __time += 10 * i;
      seg.setLed(bez_kropki[i], pos);
      break;
    case 0:
      seg.setLed(bez_kropki[i], pos);
      if(__time > 1)
        __time += i - 1;
      else
        __time += i;
      osiem_na_osiem(0, 1); //oblicz interwal wygaszania diod
      i = 0;
      lcd.print("ODLICZANIE...");
      seg_blink = 0;
      kolejnaNiepotrzebnaZmienna = 10;
      what = false; //do funkcji mryga()
      pos = 2;  //do migajacej kropki
      if(__time == 0){
        osiem_na_osiem(0,0);
        Timer4.detachInterrupt();
        Timer5.detachInterrupt();
        snake();
        return;
      }
      osiem_na_osiem(1, 0); //zapal wszystkie diody na wyswietlaczu 8x8
      Timer4.stop();
      Timer4.detachInterrupt();
      Timer5.detachInterrupt();
      Timer4.attachInterrupt(mryga);
      Timer5.attachInterrupt(rozpocznij);
      Timer6.attachInterrupt(wygaszaj_diody);
      Timer4.start(500000);
      Timer5.start(1000000);
      Timer6.start(f_interwal);
      return;
      break;
  }
  pos--;
  i = 0;
  kolejnaNiepotrzebnaZmienna = 0;
}

void set_tHrs_tMins(){//ustawianie czasu odliczania-------------------------------------------+
  static int x;                                                                             //|
  static const int przyciski[11] = {    //tablica z numeracją przycisków                      |
                                    14, // 0 przyciski[0]                                     |
                                    8,  // 1 przyciski[1]                                     |
                                    9,  // 2 przyciski[2]                                     |
                                    10, // 3 przyciski[3]                                     |
                                    7,  // 4 przyciski[4]                                     |
                                    6,  // 5 przyciski[5]                                     |
                                    5,  // 6 przyciski[6]                                     |
                                    0,  // 7 przyciski[7]                                     |
                                    1,  // 8 przyciski[8]                                     |
                                    2,  // 9 przyciski[9]                                     |
                                    13  // enter przyciski[10]                                |
                                   };                                                       //|
  for(x = 0; x < 11; x++){                                                                  //|
    if(set.buttonPressed(przyciski[x])){                                                    //|
      if(x == 10)                                                                           //|
        enter();                                                                            //|
      else{                                                                                 //|
        if(pos == 2)//wartosc z kropka                                                        |
          seg.setLed(z_kropka[i = x], pos);                                                 //|
        else{                                                                               //|
          if(pos == 0)                                                                      //|
            seg.setLed(bez_kropki[i = x], pos);                                             //|
          else{                                                                             //|
            if(x <= 5)//maksymalna wartosc dla liczb na pozycjach 3 oraz 1                    |
              seg.setLed(bez_kropki[i = x], pos);                                           //|
          }                                                                                 //|
        }                                                                                   //|
      }                                                                                     //|
    }                                                                                       //|
  }                                                                                         //|
  kolejnaNiepotrzebnaZmienna++;                                                             //|
}//-------------------------------------------------------------------------------------------+

void snake()
{
  dir = STOP;
  x = width / 2;
  y = height / 2;
  fruitX = 6;
  fruitY = 6;
  osiem.setLed(8 - y, x - 1, true);
  txt(1);
  Timer5.attachInterrupt(input);
  Timer5.start(5);
  Timer4.attachInterrupt(movment);
  sSpeed();
  fruit();
}

  void movment() {
  osiem.clearDisplay();
  switch (dir)
  {
    case LEFT:
      x--;
      if (x < 1)x = 8;
      break;
      
    case RIGHT:
      x++;
      if (x > 8)x = 1;
      break;
      
    case UP:
      y++;
      if (y > 8)y = 1;
      break;
      
    case DOWN:
      y--;
      if (y < 1)y = 8;
      break;
      
    default:
      break;
  }
  osiem.setLed(8 - y, x - 1, true);

  tail();
  Timer5.start(5);
  sSpeed();
  fruit();

  if(dir!=STOP)
    txt(3);
  else
    txt(4);
  collision();
}

void sSpeed() {
  int value = analogRead(A9) / 128;
  //value++;
  Timer4.stop();

  for (int i = 2; i < 10; i++)
    digitalWrite(i, HIGH);

  for (int i = 8-value ; i >= 2; i--) 
    digitalWrite(i, LOW);
  

  Timer4.start((8-value) * 100000);
}

void fruit(){
  dark = !dark;
  if(fruitX==x&&fruitY==y){
    score++;
    points(score);
    digitalWrite(24, HIGH);
        //////////////////////////////////dodać tutaj do-while X/y fruit będzie rózne od ogona
    do{
      fruitX = random(width)+1;
      fruitY = random(height)+1;
    }while(fruitCollision());
  }
  osiem.setLed(8 - fruitY, fruitX - 1, dark);
  
  
}

void tail(){
  for(int i=score;i>0;i--){
    fX[i]=fX[i-1];
    fY[i]=fY[i-1];
  }
  
  fX[0]=x;
  fY[0]=y;
  for(int i=0;i<=score+1;i++){
    osiem.setLed(8-fY[i],fX[i]-1,true);
  }
}

void collision(){
    for(int i=1;i<score+1;i++)
      if(fX[i]==x&&fY[i]==y)
        gameOver=true;

    if(gameOver==true){
      txt(2);
      Timer5.stop();
      Timer4.stop();
    }
}

void txt(int tx){
  lcd.clear();
  switch(tx)
  {
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Zjedz Bugi");
      lcd.setCursor(0, 1);
      lcd.print("Wcisnij przycisk");
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Przegrales stary");
      lcd.setCursor(0, 1);
      lcd.print("Wcisnij reset ;(");
      break;
    case 3:
      lcd.clear();
      break;
    case 4:
      lcd.setCursor(0, 0);
      lcd.print("Zjedz Bugi");
      lcd.setCursor(0, 1);
      lcd.print("Wcisnij przycisk");
      break;
    default:
      lcd.clear();
      break;  
  }
}

bool fruitCollision(){
  for(int i=1;i<score+1;i++)
    if(fX[i]==fruitX&&fY[i]==fruitY)
      return true;
  return false;  
}

void points(int score){
  seg.displayDigit(score%10,0);
  seg.displayDigit(score/10,1);
}

void input() {
  if ((set.buttonPressed(1) || analogRead(A10) > 900 )&& dir != DOWN) {
    dir = UP; Timer5.stop();
  }
  if ((set.buttonPressed(7) || analogRead(A11) < 100 )&& dir != RIGHT) {
    dir = LEFT; Timer5.stop();
  }
  if ((set.buttonPressed(5) || analogRead(A11) > 900) && dir != LEFT) {
    dir = RIGHT; Timer5.stop();
  }
  if ((set.buttonPressed(6) || analogRead(A10) < 100 )&& dir != UP) {
    dir = DOWN; Timer5.stop();
  }
  if (set.buttonPressed(0)) {
    dir = STOP; Timer5.stop();
  }
  digitalWrite(24, LOW);
}

void setup() {
  //lcd.begin();
  lcd2.begin();
  for (int i = 2; i <= 9; i++){
    pinMode(i, OUTPUT);
  }
  pinMode(24, OUTPUT); //speaker signal ON
  Serial.begin(9600);
  osiem.init();
  hello();
  lcd.begin(16,2);//inicjalizacja rozmiaru wyświetlacza lcd
  seg.init();//initializacja wyświetlacza 7io segmentowego
  set.init();//initializacja przycisków
  startowa();
  randomSeed(analogRead(0));

    bitmapa(obraz::width,obraz::height,obraz::header_data);
    lcd2.renderAll();
    //delay(5000);
}

void bitmapa(int wid, int hei, char bitmap[]){
  for(int i=hei;i>0;i--){
    for(int j=wid;j>0;j--){
      lcd2.setPixel(j,i,bitmap[(i - 1)*wid + (j - 1)]);
    }
  }
}

void loop() {
}
