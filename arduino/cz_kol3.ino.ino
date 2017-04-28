#include <Servo.h>

#define s0 12
#define s1 11
#define s2 10
#define s3 9
#define out 8
#define servoYpin 7
#define servoXpin 6
#define resetSD 5
#define clockSD 4
#define dataSD 3
#define busySD 2
#define NUMBER_OF_MEASUREMENTS 50
#define DEBUG 0

Servo servoX;
Servo servoY;

int _ready = 0;
int z = 0;

struct stats_t{
  float tab[NUMBER_OF_MEASUREMENTS][4]; 
  float sX[4] = {0,0,0,0};  //reads average
  float sigmum[4] = {0,0,0,0};  //sum (read - reads average)^2
  float variance[4] = {0,0,0,0};
  float sd[4] = {0,0,0,0}; //standard deviation
};

struct rgb_t{
  volatile float R;
  volatile float G;
  volatile float B;
  float cct, cct2;  //CCT CCT2
};

struct scaled_t{
  float R_min_scaled;
  float G_min_scaled;
  float B_min_scaled;
  float L_min_scaled;
  float R_max_scaled;
  float G_max_scaled;
  float B_max_scaled;
  float L_max_scaled;


  float saturation_irradiance = 12000.0;  //czestotliwosc pracy fotodiody sylikonowej dla s0 = LOW, s1 = HIGH

  float B_max = 0; //dla
  float G_max = 0; //funkcji
  float R_max = 0; //
  float L_max = 0; //scaleMAX
  float B_min = 100000000;
  float G_min = 100000000;
  float R_min = 100000000;
  float L_min = 100000000;

};

struct reads_t{
  struct stats_t stats;
  struct scaled_t min_max;
  struct rgb_t rgb;

  volatile float kolor[4] = {
                              0,  // red
                              0,  // green
                              0,  // blue
                              0   // clear photodiode
                            };
  volatile long duration;

  void scaleMAX(void){  //for white
    if(min_max.B_max < kolor[2])
      min_max.B_max = kolor[2];
    Serial.print("B_max = ");
    Serial.print(min_max.B_max);
    Serial.print(" ");
    if(min_max.G_max < kolor[1])
      min_max.G_max = kolor[1];
    Serial.print("G_max = ");
    Serial.print(min_max.G_max);
    Serial.print(" ");
    if(min_max.R_max < kolor[0])
      min_max.R_max = kolor[0];
    Serial.print("R_max = ");
    Serial.print(min_max.R_max);
    Serial.print(" ");
    if(min_max.L_max < kolor[3])
      min_max.L_max = kolor[3];
    Serial.print("L_max = ");
    Serial.print(min_max.L_max);
    Serial.println(" ");
  
    return;
  }
  
  void scaleMIN(void){  //for black
    if(min_max.B_min > kolor[2])
      min_max.B_min = kolor[2];
    Serial.print("B_min = ");
    Serial.print(min_max.B_min);
    Serial.print(" ");
    if(min_max.G_min > kolor[1])
      min_max.G_min = kolor[1];
    Serial.print("G_min = ");
    Serial.print(min_max.G_min);
    Serial.print(" ");
    if(min_max.R_min > kolor[0])
      min_max.R_min = kolor[0];
    Serial.print("R_min = ");
    Serial.print(min_max.R_min);
    Serial.print(" ");
    if(min_max.L_min > kolor[3])
      min_max.L_min = kolor[3];
    Serial.print("L_min = ");
    Serial.print(min_max.L_min);
    Serial.println(" ");
  
    return;
  }  
};

void setup(){
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  pinMode(out, INPUT);
  Serial.begin(9600); 
  digitalWrite(s0, LOW);  //czestotliwosc na wyjsciu:
  digitalWrite(s1, HIGH); //2%
  servoY.attach(servoYpin);
  servoX.attach(servoXpin);
}

void loop(){
  struct reads_t whatreads;
  float Hue = 0, Saturation = 0;
//  if(z==0){
//    collect(&whatreads);
//    z++;
//  }   
//    if(DEBUG==1){
//      for(int j = 0; j < 4; ++j){
//        for(int i = 0; i < NUMBER_OF_MEASUREMENTS; ++i){
//          if(whatreads.stats.tab[i][j] != 0){
//            Serial.print(whatreads.stats.tab[i][j]);
//            Serial.print(" ");
//          }
//        }
//      if(DEBUG==1)
//        Serial.println();
//      }
//    }
//  }
  if(!_ready){
    scale(&whatreads, &_ready);
  }

  while(1){
    color(&whatreads);
    FREQ_to_RGB(&whatreads);

    Serial.print("R = "); Serial.print(whatreads.rgb.R);
    Serial.print("G = "); Serial.print(whatreads.rgb.G);
    Serial.print("B = "); Serial.println(whatreads.rgb.B);
  
  //  CIE_tristimulus(&whatreads);      // nie wiem
  //  Serial.print(whatreads.rgb.cct);  // czy tego
  //  Serial.println("K");              // potrzebuje
    
 //   float Luminance; //= map(whatreads.kolor[3], whatreads.min_max.L_min_scaled, whatreads.min_max.L_max_scaled, 0, 100);
   // Luminance = constrain(whatreads.kolor[3], 0, 100);
  
//    convert_RGB_to_0_to_100_scale(&whatreads);
//    RGB_to_HSL(&whatreads, Hue, Saturation, Luminance);
  
//    Serial.print("Hue: "); Serial.println(Hue*360);
//    Serial.print("Saturation: "); Serial.println(Saturation);
//    Serial.print("Luminance: "); Serial.println(Luminance);
  }
  //distance();
}

/********************************************************************
 *                      TCS 2300                                    *
 *  |s2  s3  | Fotodioda|     |s0  s1  | Czestotliwosc na wyjsciu|  *
 *  |--------+----------|     |--------+-------------------------|  *
 *  |0   0   | Czerwony |     |0   0   | wylaczenie zasilania    |  *
 *  |0   1   | Niebieski|     |0   1   | 2%                      |  *
 *  |1   0   | Czysta   |     |1   0   | 20%                     |  *
 *  |1   1   | Zielony  |     |1   1   | 100%                    |  *
 ********************************************************************/

void color(struct reads_t *r){
  digitalWrite(s2, LOW);           //R
  digitalWrite(s3, LOW);
  r->kolor[0] = pulseIn(out, LOW);
  r->kolor[0] = r->min_max.saturation_irradiance / r->kolor[0];
//  Serial.print("R = "); Serial.print(r->kolor[0]); Serial.print(" ");

  digitalWrite(s2, HIGH);          //G
  digitalWrite(s3, HIGH);
  r->kolor[1] = pulseIn(out, LOW);
  r->kolor[1] = r->min_max.saturation_irradiance / r->kolor[1];
//  Serial.print("G = "); Serial.print(r->kolor[1]); Serial.print(" ");
  
  digitalWrite(s2, LOW);           //B
  digitalWrite(s3, HIGH);
  r->kolor[2] = pulseIn(out, LOW);
  r->kolor[2] = r->min_max.saturation_irradiance / r->kolor[2];
//  Serial.print("B = "); Serial.print(r->kolor[2]); Serial.print(" ");

  digitalWrite(s2, HIGH);          //C
  digitalWrite(s3, LOW);
  r->kolor[3] = pulseIn(out, LOW);
  r->kolor[3] = r->min_max.saturation_irradiance / r->kolor[3];
//  Serial.print("C = "); Serial.print(r->kolor[3]); Serial.println("");
  delay(20);

  return;
}

void FREQ_to_RGB(struct reads_t *r){
  r->rgb.R = (r->kolor[0] - r->min_max.R_min_scaled) * (255 - 0) / (r->min_max.R_max_scaled - r->min_max.R_min_scaled) + 0;  //skalowanie do formatu RGB
  r->rgb.R = constrain(r->rgb.R, 0, 255);
  if(DEBUG==1){
    Serial.print("R = "); Serial.println(r->kolor[0]);
  }

  r->rgb.G = (r->kolor[1] - r->min_max.G_min_scaled) * (255 - 0) / (r->min_max.G_max_scaled - r->min_max.G_min_scaled) + 0;  //skalowanie do formatu RGB
  r->rgb.G = constrain(r->rgb.G, 0, 255);
  if(DEBUG==1){
    Serial.print("G = "); Serial.println(r->kolor[1]);
  }

  r->rgb.B = (r->kolor[2] - r->min_max.B_min_scaled) * (255 - 0) / (r->min_max.B_max_scaled - r->min_max.B_min_scaled) + 0;  //skalowanie do formatu RGB
  r->rgb.B = constrain(r->rgb.B, 0, 255);
  if(DEBUG==1){
    Serial.print("B = "); Serial.println(r->kolor[2]);
  }

  return;
}

void convert_RGB_to_0_to_100_scale(struct reads_t *r){
  r->rgb.R = (((float) r->kolor[0]) / 65536) * 100;
  r->rgb.G = (((float) r->kolor[1]) / 65536) * 100;
  r->rgb.B = (((float) r->kolor[2])/ 65536) * 100;

  return;
}

void CIE_tristimulus(struct reads_t *r){
  
/************************ CIE_tristimulus_XYZ ********************/
  const float X = (-0.14282 * r->rgb.R) + (1.54924 * r->rgb.G) + (-0.95641 * r->rgb.B);
  const float Y = (-0.32466 * r->rgb.R) + (1.57837 * r->rgb.G) + (-0.73191 * r->rgb.B);
  const float Z = (-0.68202 * r->rgb.R) + (0.77073 * r->rgb.G) + (0.56332 * r->rgb.B);
/*****************************************************************/

/********************** CHROMATICLY COORTINATES ******************/
  const float x = X / (X + Y + Z);
  const float y = Y / (X + Y + Z);
/*****************************************************************/

/************************ McCamy's Formula ***********************/
  const float n = (x - 0.332) / (0.1858 - y);
  r->rgb.cct = (449 * n * n * n) + (3525 * n * n) + (6823.3 * n) + 5520.33; //CCT [Kelvin] <- for 'absolute' black object
/*****************************************************************/

  return;
}

//void RGB_to_HSL(struct reads_t *r, float& Hue, float& Saturation, float& Luminance){
//  float fmin, fmax;
//  fmax = max(max(r->rgb.R, r->rgb.G), r->rgb.B);
//  fmin = min(min(r->rgb.R, r->rgb.G), r->rgb.B);
//
//  //Luminance = fmax;
//  Luminance = map(Luminance, r->min_max.L_min_scaled, r->min_max.L_max_scaled, 0, 100);
//  Luminance = constrain(r->kolor[3], 0, 100);
//  if (fmax > 0)
//    Saturation = (fmax - fmin) / fmax;
//  else
//    Saturation = 0;
//
//  if (Saturation == 0)
//    Hue = 0;
//  else{
//    if(fmax == r->rgb.R)
//      Hue = (r->rgb.G - r->rgb.B) / (fmax - fmin);
//    else if (fmax == r->rgb.G)
//      Hue = 2 + (r->rgb.B - r->rgb.R) / (fmax - fmin);
//    else
//      Hue = 4 + (r->rgb.R - r->rgb.G) / (fmax - fmin);
//    Hue = Hue / 6;
//    if (Hue < 0) Hue += 1;
//  }
//  return;
//}

void collect(struct reads_t *r){
  int flag = 1;
  for(int i = 0; i < NUMBER_OF_MEASUREMENTS; ++i){
    color(r);
    for(int j = 0; j < 4; ++j){ 
      if(i < NUMBER_OF_MEASUREMENTS){
        r->stats.tab[i][j] = r->kolor[j];
      }
    }
  }
  statistical_processing(r);
  return;
}

void statistical_processing(struct reads_t *r){
  average(r);
  variance_deviation(r);
  if(cleaning_data(r))
    average(r);
}

void average(struct reads_t *r){
  for(int i = 0; i < 4; ++i){
    for(int j = 0; j < NUMBER_OF_MEASUREMENTS; ++j){
      if(r->stats.tab[j][i] != 0){
        //Serial.print(r->stats.tab[j][i]); Serial.print(" ");
        r->stats.sX[i] += r->stats.tab[j][i];
      }
    }
    if(DEBUG==1){
      Serial.println();
    }
    r->stats.sX[i] /= NUMBER_OF_MEASUREMENTS;
    if(DEBUG==1){
      Serial.print("sX = "); Serial.println(r->stats.sX[i]);
    }
  }
}

void variance_deviation(struct reads_t *r){
    for(int i = 0; i < 4; ++i){
    for(int j = 0; j < NUMBER_OF_MEASUREMENTS; ++j){
      if(r->stats.tab[j][i] != 0){
        r->stats.variance[i] += ( (r->stats.tab[j][i] - r->stats.sX[i]) * (r->stats.tab[j][i] - r->stats.sX[i]) ) / NUMBER_OF_MEASUREMENTS;
      }
    }
    if(DEBUG==1){
      Serial.print("variance = "); Serial.println(r->stats.variance[i], 8);
    }
    r->stats.sd[i] = sqrt(r->stats.variance[i]);
    if(DEBUG==1){
      Serial.println(r->stats.sd[i], 8);
    }
  }
 
  for(int i = 0; i < 4; ++i){
    r->stats.variance[i] = 0;
  }
}

bool cleaning_data(struct reads_t *r){
  int flag = 0;
  for(int i = 0; i < 4; ++i){
    for(int j = 0; j < NUMBER_OF_MEASUREMENTS; ++j){
      if( r->stats.sX[i] - 2 * r->stats.sd[i] > r->stats.tab[j][i] || r->stats.sX[i] + 2 * r->stats.sd[i] < r->stats.tab[j][i] ){
        if(DEBUG==1)
          Serial.println("clearing...");
        r->stats.tab[j][i] = 0;
        flag = 1;
      }
      if(flag)
        return true;
    }
  }
  return false;
}

void scale(struct reads_t *r, int *_ready){
  servo_scale_black();
  scale_black(r);
  delay(4000);
  
  servo_scale_white();
  scale_white(r);
  delay(4000);

  if(DEBUG==1){
      Serial.print("R_min = "); Serial.print(r->min_max.R_min_scaled, 8); Serial.print("   ");
      Serial.print("R_max = "); Serial.println(r->min_max.R_max_scaled, 8);
      Serial.print("G_min = "); Serial.print(r->min_max.G_min_scaled, 8); Serial.print("   ");
      Serial.print("G_max = "); Serial.println(r->min_max.G_max_scaled, 8);
      Serial.print("B_min = "); Serial.print(r->min_max.B_min_scaled, 8); Serial.print("   ");
      Serial.print("B_max = "); Serial.println(r->min_max.B_max_scaled, 8);
      Serial.print("L_min = "); Serial.print(r->min_max.L_min_scaled, 8); Serial.print("   ");
      Serial.print("L_max = "); Serial.println(r->min_max.L_max_scaled, 8);
  }

  *_ready = servo_start_position();

  return;
}

int servo_start_position(){
  servoX.write(90);
  servoY.write(110);

  return 1;
}

void servo_scale_black(){
  servoY.write(30);
  servoX.write(0);

  return;
}

void scale_black(struct reads_t *r){
  collect(r);
  choose_black(r);

  return;
}

void choose_black(struct reads_t *r){
    r->min_max.R_min_scaled = r->stats.sX[0];
    r->min_max.G_min_scaled = r->stats.sX[1];
    r->min_max.B_min_scaled = r->stats.sX[2];
    r->min_max.L_min_scaled = r->stats.sX[3];

  return;
}

void servo_scale_white(){
  servoX.write(180);

  return;
}

void scale_white(struct reads_t *r){
  collect(r);
  choose_white(r);

  return;
}

void choose_white(struct reads_t *r){
    r->min_max.R_max_scaled = r->stats.sX[0];
    r->min_max.G_max_scaled = r->stats.sX[1];
    r->min_max.B_max_scaled = r->stats.sX[2];
    r->min_max.L_max_scaled = r->stats.sX[3];

  return;
}

//void build_cloud(void){
//  for(int i = 0; i < 25; ++i){
//    Serial.print("[ "); Serial.print(tab[i]); Serial.print(", "); Serial.print(tab[i+25]); Serial.print(", "); Serial.print(tab[i+50]); Serial.print(", "); Serial.print(tab[i+75]); Serial.println(" ]");
//  }
//}
