//version med easy start switch

// tomgång = 450rpm
// maxvarv = 1600rpm
// oljetryck = 1,5 - 2,5 kg/cm2
// termostat öppning = 77 g C
// termostat helt öppet = 86,5 g C



#include <TimerOne.h>
#include <Wire.h>  
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 
volatile unsigned long int Lap_time;
volatile unsigned long int Old_time;
volatile unsigned long int New_time;
volatile byte RPM;
volatile byte RPM_old;
volatile unsigned long int En_grad;
volatile unsigned long int Timer_count_value;
volatile int Fortandningsvinkel;
volatile unsigned long int old_micros;    //for att mata tid i koden
volatile byte update_lcd_flag;
volatile byte easy_start;
volatile byte easy_start_old;

//----------------------------------------------------------------------------------------------------------------------------------

void setup() 
{
 Serial.begin(9600);
 lcd.begin(20,4); 
 lcd.backlight(); 
 delay(1000);
 Serial.println("serial port initialized");
 delay(1000);
 pinMode(2, INPUT);                                              //magnetsensor
 digitalWrite(2, HIGH);                                          //pull up resistor
 pinMode(13, OUTPUT);                                             // styr trissan: 
 pinMode(12, INPUT);                                               //easy start brytaren
 Timer1.initialize(8000000);
 Timer1.stop();
 Timer1.detachInterrupt(); 
 Old_time = 0;
 RPM = 0;
 Fortandningsvinkel = 0;  
 update_lcd_flag = 0;
 Serial.print("RPM= ");
 Serial.println(RPM);
 lcd.setCursor(0,0); //Start at character 0 on line 0
 lcd.print("Albin O-11 Computer");
 delay(200);
 lcd.setCursor(0,1);
 lcd.print("Starting up...");
 delay(200);  

 easy_start = digitalRead(12);   

 if (easy_start==1)
      {
         lcd.setCursor(0,2);
         lcd.print("EASY START OFF");
      }
 if (easy_start ==0) 
      {
       lcd.setCursor(0,2);
       lcd.print("EASY START ON");
      }
 delay(5000);     
 lcd.setCursor(0,2);
 lcd.print("Please turn flyweel!");
 delay(500);
 attachInterrupt(0, Magnet_sensad, FALLING); 
 
}
//----------------------------------------------------------------------------------------------------------------------------

void lcd_display()
{
 lcd.clear();
 lcd.setCursor(0,0); //Start at character 0 on line 0
 lcd.print("RPM: ");
 lcd.setCursor(15,0);
 lcd.print(RPM);
 lcd.setCursor(0,1);
 lcd.print("PRE-IGNITION: ");
 lcd.setCursor(17,1);
 lcd.print(Fortandningsvinkel);
 
 if (easy_start==1)
      {
         lcd.setCursor(0,2);
         lcd.print("EASY START OFF");
      }
 if (easy_start ==0) 
      {
       lcd.setCursor(0,2);
       lcd.print("EASY START ON");
      }
}

//----------------------------------------------------------------------------------------------------------------------------

void loop() 
{
 easy_start_old = easy_start;
 easy_start = digitalRead(12);
 if (easy_start != easy_start_old) 
      {
       update_lcd_flag = 1;
      }
  
 if (update_lcd_flag == 1)
        {
          lcd_display();
          update_lcd_flag = 0;
        }
        

      
}
//-------------------------------------------------------------------------------------------------------------------------------
void Magnet_sensad() //magneten sitter på övre dödpunkt 
{
 New_time = micros();
 Lap_time = New_time - Old_time;                        //i microsecunder
 Old_time = New_time;
 RPM = 60000000 / Lap_time;
 En_grad = Lap_time / 360;                              //i microsekunder                          
 unsigned long int Timer_count_micros;                   //starta timer1   antag spolens laddtid 3,5ms    antag "magnet sensad"-rutinen tar 280 microsecunder
 if (RPM < 1800)                                         //overvarvningsskydd
              {                                    
               if (easy_start ==0)
                    {
                     Fortandningsvinkel = 0;
                     Timer_count_micros = 14000;            //baktändningsskydd ca 5 graders fördröjning . (antagit ca 60 RPM)
                     Timer1.setPeriod(Timer_count_micros);
                     Timer1.attachInterrupt(Timer_function);
                    }
               if (easy_start == 1)
                    {                                                                 
                     if (RPM < 450)                                                         // tänd direkt
                          {
                           EIMSK = 0;
                           digitalWrite(13, HIGH);                                          // spänning på spolen                                               
                           delayMicroseconds(3500);
                           digitalWrite(13, LOW);                                           // bryt strömmen till spolen
                           int i = 0;
                           for(int i=0; i<=33; i++)                                         //vänta 33 millisek = 90grader på 1800RPM
                                {
                                  delayMicroseconds(1000);
                                }                                                              
                           update_lcd_flag = 1;                                    //visar att LCD ska updateras 
                           EIFR = 1;                                 //rensar flaggan 
                           EIMSK = 1;                              //återaktiverar interrupt troligen onödig åtgärd
                          }
                     
                     if (RPM >= 450)                                   //tänd på nästa varv
                          {
                           Fortandningsvinkel = 0.017 * RPM - 7.65;                         //tomgång = 450 RPM och fortandning 0grader, maxvarv 1600 RPM och fortandning 20 grader
                           Timer_count_micros = ((360 - Fortandningsvinkel) * En_grad ) - 3500 - 280; 
                           Timer1.setPeriod(Timer_count_micros);
                           Timer1.attachInterrupt(Timer_function);                      
                          }
                     
                    }                    
               
               }
 else 
               {
                int i = 0;
                for(int i=0; i<=33; i++)                        //vänta 33 millisek = 90grader på 1800RPM
                             {
                              delayMicroseconds(1000);
                             }                                              
                EIFR = 1;                                //rensar flaggan för säkerhets skull 
                EIMSK = 1;                              //återaktiverar interrupt troligen onödig åtgärd                                              
                }                
}

//------------------------------------------------------------------------------------------------------------------

void Timer_function(void)                                        // timer compare interrupt service routine
{                                                                                                                 
 EIMSK = 0;                                                      //stänger av interrupt
 Timer1.stop();
 Timer1.detachInterrupt();
 digitalWrite(13, HIGH);                                          // spänning på spolen                                               
 delayMicroseconds(3500);
 digitalWrite(13, LOW);                                           // bryt strömmen till spolen
 int i = 0;
 for(int i=0; i<=33; i++)                                         //vänta 33 millisek = 90grader på 1800RPM
        {
         delayMicroseconds(1000);
        }                                                              
 update_lcd_flag = 1;                                    //visar att LCD ska updateras 
 EIFR = 1;                                               //rensar flaggan i EIFR (sätter en 1:a!!)
 EIMSK =1;                                               //återaktiverar interrupt                            
 }
 //----------------------------------------------------------------------------------------------------------------- 
