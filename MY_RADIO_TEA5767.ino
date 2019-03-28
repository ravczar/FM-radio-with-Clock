// Ze strony https://www.electronicsblog.net/arduino-fm-receiver-with-tea5767/
#include <Wire.h>
#include <RTClib.h>
#include <TM1637.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h> 
//LiquidCrystal_I2C lcd (0x27, 16, 2); // NOWA BIBLIOTEKA BEZ BLEDÓW KOMPILACJI  -> http://arduino.idsl.pl/index.php/biblioteki-arduino-ide/13-liquidcrystal-i2c-h
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Definiujemy piny pod 4 digit display
#define CLK 2
#define DIO 3

// Do obliczeń czestotliwości
unsigned int frequencyB;
unsigned char frequencyH=0;
unsigned char frequencyL=0;
unsigned char search_mode=0;
unsigned int signal_level=0;
unsigned int signal_percent;
unsigned char buffer[5];

// Przyciski flagi
boolean state0 = 0;
boolean state1 = 0;
boolean state2 = 0;
boolean state3 = 0;
boolean state4 = 0;

// Zmienne sfiksowanych stacji FM
boolean choice = -1;  // next 1, previous 0;
double fixedStations[] = {88.40, 88.90, 89.50, 92.00, 93.40, 
                          93.90, 94.60, 95.70, 96.40, 97.80, 
                          98.40, 99.90, 101.70, 103.00, 103.70, 
                          104.40, 105.0};
String fixedNames[]    = {"RMF CLASSIC", "RADIO MARYJA", "POLSKA DWOJKA", "ANTYRADIO", "POLSKIE RADIO 24",
                          "MUZO FM", "RADIO ESKA", "POLSKA JEDYNKA", "RMF MAX", "TOK FM",
                          "RMF FM", "POLSKA TROJKA", "PLUS GDANSK", "ZLOTE PRZEBOJE", "RADIO GDANSK", "VOX FM", "RADIO ZET"};     
                     
int arrayElementsCount = sizeof(fixedStations)/ sizeof(fixedStations[0]);                          
int stationIndex = 0;

double freq_available; // raczej nie bedzie konieczne - readfromTea można sie tego pozbyc i zostrawic samo actual FREQUENCY
double actualFrequency;

// Tablica gdzie wrzucamy aktulany czas i z niej wyświetlamy w loopie
int8_t TimeDisp[] = {0x06,0x06,0x06,0x06};
unsigned long current_millis = millis(); // tu zapisujemy czas do wyświetlania

//  Tiny RTC i wyświetlacz 4digit-7segment
RTC_DS1307 rtc;
TM1637 tm1637(CLK,DIO);

/*
 * -----------------------------------------------------> SETUP <-------------------------------------------------
 */
void setup() {
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);

  pinMode(8, OUTPUT); //AMP na tranzystorze ( lcd już nie )
  pinMode(4, OUTPUT); // DIODA
  pinMode(A3, OUTPUT); // pin zasilania wyświetlacza LCD 1602

  // Start wyświwtlacza -> BRIGHT_DARKEST(3.5 mA), BRIGHT_TYPICAL (12.5 mA), BRIGHTEST (39 mA)
  tm1637.set(BRIGHT_TYPICAL,0x40, 0xc0); 
  tm1637.init();
  tm1637.point(POINT_ON); // włącza dwukropek // POINT_OFF wyłąca dwukropek
  
  //Serial.begin(9600);
  Wire.begin(); // koniecznie zaczynamy Wire 
  rtc.begin(); // zegarek biblioteka start
 // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 // rtc.adjust(DateTime(2019, 2, 9, 22, 40, 00)); // ustawia sztywno datę i godzinę.
  //while(!Serial);  // tylko dla arduino leonardo!
  Serial.println("Wykryto serial okno. zaczynamy.");
  setFrequency (actualFrequency);
}

/*
 * ----------------------------------------------------> LOOP <---------------------------------------------------------
 */
void loop() {

  /*  ---------- POWER ON/OFF z zapisem   ------------------ */
    if (digitalRead(9) == LOW){
      delay(20);
      //Serial.println("#### NACISNIETO 9 PIN");
      state0 = !state0;

      if (state0){
        digitalWrite(A3, state0);  // LCD power on
        digitalWrite(4, state0);  // Signal LED on
        delay (50); 
        
        actualFrequency = readFromEEprom(); // wczytanie czestotliwosci z pamięci
        setFrequency (actualFrequency);
        readFromTEA();
        welcomeMessageOnLCD();
        delay(1000);
        digitalWrite(8, state0); // AMP power on
      }
      else{
        writeToEEprom ();
        setStandByMode(true);
        setGoodByeScreen (2000);
        digitalWrite(8, state0); // AMP power on
        digitalWrite(4, state0); 
        digitalWrite(A3, state0);
      }
      
      while(digitalRead(9)==LOW);
      delay(20);
     }
  /*  ---------- SCAN DOWN   ----------------------------- */
    if (digitalRead(10) == LOW & state0){
      delay(20);
        state1 = !state1;
        scanDown();
        scannDownInProgressLCD(300);
        readFromTEA();
        roundFreqAfterSearchComplete (); // zaokrągla i ustawia dobrą częstotliwość
        printActFreq (false);
      while(digitalRead(10)==LOW);
      delay(20);
     }
 /*  ---------- SCAN UP   ----------------------------- */
    if (digitalRead(11) == LOW & state0){
      delay(20);
        state2 = !state2;
        scanUp();
        scannUpInProgressLCD(300);
        readFromTEA();
        roundFreqAfterSearchComplete (); // zaokrągla i ustawia już dobrą częstotliwość
        printActFreq(false);
      while(digitalRead(11)==LOW);
      delay(20);
     }
 /*  ---------- PREV STATION FROM ARRAY   ----------------------------- */
     if (digitalRead(12) == LOW & state0){
      delay(20);
        state3 = !state3;
        previousStationFromArray();
        printActFreq(true);
      while(digitalRead(12)==LOW);
      delay(20);
     }
 /*  ---------- NEXT STATION FROM ARRAY   ----------------------------- */
    if (digitalRead(13) == LOW & state0){
      delay(20);
        state4 = !state4;
        nextStationFromArray();
        printActFreq(true);
      while(digitalRead(13)==LOW);
      delay(20);
     }
 /*  ---------- HOUR UP (press both buttons 2 & 3)    ----------------------------- */
    if (!state0 & digitalRead(10) == LOW ){
       delay(20);
       hourDown();
       while(digitalRead(10)==LOW); // wyrzucając to i wkladając delay (1000) zrobisz ciągłą zmianę czasu co sekundę trzymając przycisk
       delay(20);
    }
 /*  ---------- HOUR UP (press both buttons 2 & 3)    ----------------------------- */
    if (!state0 & digitalRead(11) == LOW ){
       delay(20);
       hourUp();
       while(digitalRead(11)==LOW); // wyrzucając to i wkladając delay (1000) zrobisz ciągłą zmianę czasu co sekundę trzymając przycisk
       delay(20);
    }
    
 /*  ---------- MINUTE UP (press both buttons 2 & 3)    ----------------------------- */
    if (!state0 & digitalRead(12) == LOW ){
       delay(50);
       minuteDown();
       while(digitalRead(12)==LOW); // wyrzucając to i wkladając delay (1000) zrobisz ciągłą zmianę czasu co sekundę trzymając przycisk
       delay(20);
    }
 /*  ---------- MINUTE UP (press both buttons 2 & 3)    ----------------------------- */
    if (!state0 & digitalRead(13) == LOW ){
       delay(50);
       minuteUp();
       while(digitalRead(13)==LOW); // wyrzucając to i wkladając delay (1000) zrobisz ciągłą zmianę czasu co sekundę trzymając przycisk
       delay(20);
    }
 /*  ---------- LOOP ON CLOCK DISPLAY   ----------------------------- */
    if ( millis() > current_millis + 1000 ) {
      // uzyskaj czas do tablicy czasu, w zmiennych
      setNewTimeForClock();
      // wyświetl czas na zegarku
      tm1637.display(TimeDisp);
      current_millis = millis();
    }

    // Ciągłę loopowanie po odczytywaniu częstotliwości. Powinno byc zmienione na jednokrotne odczytywanie gdy nacisnięto jakis przycisk,
    /*readFromTEA();
    Serial.println(" ");
    Serial.print("Potwierdzenie czestotliwosci ustawionej:");
    Serial.println(actualFrequency);
    Serial.print("->BIT H: ");
    Serial.print(frequencyH, BIN);
    Serial.print("  ->BIT L: ");
    Serial.print(frequencyL, BIN);
    Serial.print(" ->actualFrequency: ");
    Serial.println(actualFrequency);
    Serial.println("->#############################################################<-");
    delay (1000); */
} // loop - end mark

/*
 * ------> MY FUN <---------
 */


 // ----------------------   TEA5767 FUNKCJE ------------
void roundFreqAfterSearchComplete (){
  double rounded_freq;
  rounded_freq = floor (freq_available / 100000 + .5) / 10;
  setFrequency (rounded_freq);
  readFromTEA();
}

void countDoubleFreqToBytes(double frequency) {
  frequencyB=4*(frequency*1000000+225000)/32768; //calculating PLL word
  frequencyH=frequencyB>>8;
  frequencyL=frequencyB&0XFF;
}
 
void setFrequency( double frequency ) {
  frequencyB= round(4*(frequency*1000000+225000)/32768); //calculating PLL word
  frequencyH=(frequencyB>>8);
  frequencyL=(frequencyB&0XFF);

  //actualFrequency = frequency;  // przypisuje zmienną do pamiętania aktualnej freq - nie jest to konieczne bo zapisuje to nam funkcja READ
  delay(100);
  writeFrequencyByWire();
}

void writeFrequencyByWire( ){
  Wire.beginTransmission(0x60);   //writing TEA5767 0x60
  Wire.write(frequencyH);
  Wire.write(frequencyL);
  Wire.write(0xB0);
  Wire.write(0x10);
  Wire.write(0x00);
  Wire.endTransmission();
 // Serial.println("Zapisano nową częstotliwość w TEA");
}

void readFromTEA(){
  // Bajt 1 i 2 czestotliwosc, Bajt 3 - streo czy ono, Bajt 4 - Sygnał poziom, Bajt 5 - zarezerwowoany
  Wire.requestFrom(0x60,5); //reading TEA5767 0x60, 5 bitów zczytujemy
  if (Wire.available()) 
  {
    for (int i=0; i<5; i++) {
      buffer[i]= Wire.read();
    }
    freq_available=(((buffer[0]&0x3F)<<8)+buffer[1])*32768.00/4.00-225000.00;
    actualFrequency = freq_available/1000000.00;
  //Serial.print("actualFrequency -> : ");
  //Serial.println(actualFrequency);
  //Serial.print("freq_available -> : ");
  //Serial.println(freq_available);
  //Serial.println("Signal_level: ");
  //Serial.println(signal_level);
    
    frequencyH=((buffer[0]&0x3F)); // czyści 7 i 8 bit ta operacja {00}10 1011 & 0011 1111
    frequencyL=buffer[1];
    
    signal_level = buffer[3]>>4;
    signal_percent = signal_level/0.15;

    // dwa Warunki do skanowania  (ograniczenia górne i dolne)
    if (actualFrequency < 87.49){
      setFrequency(108.0);
      readFromTEA();
      delay(40);
    }
    if (actualFrequency > 108.00){
      setFrequency(87.50);
      readFromTEA();
      delay(40);
    }
    
    if (search_mode) {
      if(buffer[0]&0x80) {
        search_mode=0;
        //Serial.println("ustawiono search_mode na FALSE");
      }  
    }
    if (search_mode==1) {
      //Serial.println (" SCANNING!");
    }
    
    else {
      //Serial.println ("Station found!");
    }

    //Serial.print("Signal level: ");
    //Serial.print((buffer[3]>>4));
    //Serial.println("/16 ");
    
    if (buffer[2]&0x80) {
      //Serial.print("Stereo mode");
    }
      
    else {
      //Serial.print("Mono mode");
    }
  }
}

void setStandByMode ( boolean state ){
  if (state){
    Wire.beginTransmission(0x60);  
     
        Wire.write(buffer[0]); 
        Wire.write(buffer[1]);
        Wire.write(buffer[2]);
        Wire.write(0x5F);      // bit 7 na jedynke - standby on 0101 11111
        Wire.write(buffer[4]); 

    Wire.endTransmission();
  }
  else{
    Wire.beginTransmission(0x60);  
     
        Wire.write(buffer[0]); 
        Wire.write(buffer[1]);
        Wire.write(buffer[2]);
        Wire.write(0x1F);      // bit 7 na zero - standby off 0001 1111
        Wire.write(buffer[4]); 

    Wire.endTransmission();
  }
}

void scanUp (){
    double pll = (4 * (((freq_available + 98304) / 1000000) * 1000000 + 225000)) / 32768;
    frequencyH=(int)pll>>8;
    frequencyL=(int)pll&0XFF;

    search_mode=1;
    Wire.beginTransmission(0x60);  
     
        Wire.write(frequencyH+0x40);  // ustawia 1 na siódmym bicie pierwszego bajtu! 
        Wire.write(frequencyL);
        Wire.write(0xF0); // D0 level 7 skanowania 1101 0000, F0 level 10 skanowania 1111 0000
        Wire.write(0x1F);
        Wire.write(0x00); 

    Wire.endTransmission();

    /*    // SPRAWDZENIE ADRESÓW scan up.
      Serial.println("####----------------------####");
      Serial.println("BAJT-0 ADRESU  ");
      Serial.println(0x60, BIN);
      Serial.println("BAJT-1 FreqH  ");
      Serial.println((frequencyH+0x40), BIN);
      Serial.println("BAJT-2 FreqL  ");
      Serial.println(frequencyL, BIN);
      Serial.println("BAJT-3 LO_INJECT  ");
      Serial.println(0xD0, BIN);
      Serial.println("BAJT-4 X-TAL  ");
      Serial.println(0x1F, BIN);
      Serial.println("BAJT-5 REZERW  ");
      Serial.println(0x00, BIN);
      Serial.println("####----------------------####"); */
      //delay (200);
    // koniec
}

void scanDown () {
    double pll = (4 * (((freq_available - 98304) / 1000000) * 1000000 + 225000)) / 32768; // step 98304 przy kwarcu 32768! = 3x 32768
    frequencyH=(int)pll>>8;
    frequencyL=(int)pll&0XFF;
    
    search_mode=1;

    Wire.beginTransmission(0x60);   

        Wire.write(frequencyH+0x40);
        Wire.write(frequencyL); 
        Wire.write(0x70); // 50 level 7 skanowania 0101 0000, 70 level 10 skanowania 0111 0000(próg stop)
        Wire.write(0x1F);
        Wire.write(0x00);
        Wire.endTransmission(); 
    
  /*  // SPRAWDZENIE ADRESÓW scan down.
      Serial.println("####----------------------####");
      Serial.println("BAJT-0 ADRESU  ");
      Serial.println(0x60, BIN);
      Serial.println("BAJT-1 FreqH  ");
      Serial.println((frequencyH+0x40), BIN);
      Serial.println("BAJT-2 FreqL  ");
      Serial.println(frequencyL, BIN);
      Serial.println("BAJT-3 LO_INJECT ");
      Serial.println(0x50, BIN);
      Serial.println("BAJT-4 X-TAL  ");
      Serial.println(0x50, BIN);
      Serial.println("BAJT-5 REZERW  ");
      Serial.println(0x00, BIN);
      Serial.println("####----------------------####"); */
      //delay (200);
    // koniec
}

void littleForward (){
    actualFrequency = actualFrequency + 0.1;

    frequencyB=4*(actualFrequency*1000000+225000)/32768;

    frequencyH=frequencyB>>8;
    frequencyL=frequencyB&0XFF;   

    Wire.beginTransmission(0x60);   

    Wire.write(frequencyH);
    Wire.write(frequencyL);
    Wire.write(0xB0);
    Wire.write(0x1F);
    Wire.write(0x00); 

    Wire.endTransmission(); 
}

void littleBackward () {
    actualFrequency = actualFrequency - 0.1;

    frequencyB=4*(actualFrequency*1000000+225000)/32768;

    frequencyH=frequencyB>>8;
    frequencyL=frequencyB&0XFF;

    Wire.beginTransmission(0x60);   

    Wire.write(frequencyH);
    Wire.write(frequencyL);
    Wire.write(0xB0);
    Wire.write(0x1F);
    Wire.write(0x00); 

    Wire.endTransmission(); 
}


void nextStationFromArray (){
  // korekta indexu
  if (!choice) {
    stationIndex ++;
  }
  if (stationIndex >= 0  ){
     setFrequency (fixedStations[stationIndex]);
     stationIndex ++;
    }
  if (stationIndex == arrayElementsCount + 1) {
    stationIndex = 0;
    setFrequency (fixedStations[stationIndex]);
    stationIndex ++;
  }
  
  delay(30);
  readFromTEA();

  choice = 1;  //  opisuje aktualnie dokonany wybór - next.
}


void previousStationFromArray (){
  // korekta indexu
  if (choice) { 
    stationIndex --;
  }

  if(stationIndex < arrayElementsCount & stationIndex > 0){
    stationIndex --;
    setFrequency (fixedStations[stationIndex]);
    //Serial.println("OBCNY INDEX");
    //Serial.println(stationIndex);
  }
  else if (stationIndex == 0){
    stationIndex = arrayElementsCount - 1;
    setFrequency (fixedStations[stationIndex]);
    //Serial.println("OBCNY INDEX");
    //Serial.println(stationIndex);
  }

  delay(30);
  readFromTEA();

  choice = 0; //  opisuje aktualnie dokonany wybór - previous.
}

// ----------------------  EEPROM FUNKCJE ------------ 
void writeToEEprom (){ // sprawdza co jest w pamięci by jej nie zuzywac ciągłymi zapisami bezsensownych wartosci. Wczytuje z pamięci przy uruchomieniu raz i ustawia actualFrequency.
  double whatsInMemory = readFromEEprom(); 
  if(whatsInMemory != actualFrequency ){
     //Serial.println(frequencyH, BIN);
     //Serial.println(frequencyL, BIN);
     // Ustawia dla pewności frequencyHigh oraz frequencyLow, z czego zapisujemy do pamięci częstotliwość do zapamiętania.
     frequencyB=4*(actualFrequency*1000000+225000)/32768; //calculating PLL word
     frequencyH=frequencyB>>8;
     frequencyL=frequencyB&0XFF;
     EEPROM.write(1, frequencyH);
     EEPROM.write(2, frequencyL);
     //Serial.println("Zapisano w pamięci nową czestotliwość.");
  }
  else if (whatsInMemory == actualFrequency){
    //Serial.println("W pamięci jest ta sama stacja więc nie nadpisano wartości");
    //Serial.println(whatsInMemory);
  }
  
}


double readFromEEprom (){  // odczytuje z pamięci komórek 1 oraz 2 wartości częstotliwości (2 bajty) i zwraca nam double, co ustawiamy przy uruchomieniu
  byte high = EEPROM.read(1);
  byte low = EEPROM.read(2);
  //Serial.println("Z pamięci odczytałem kolejne dane:");
  //Serial.println(high);
  //Serial.println(low);
  
  int binaryFrequency = ((high & 0x3F)<<8) + low; // dobrze działa
  double properFrequency = (((high & 0x3F)<<8) + low )*32768/4-225000;
  double lastFrequency = properFrequency/1000000; // 91.99 - działa
  //Serial.println("Obecnie w pamięci mamy odczyt:");
  //Serial.println(binaryFrequency,BIN);
  //Serial.println(lastFrequency);
  //Serial.println("ActualFrequency:");
  //Serial.println(actualFrequency);

  //actualFrequency = lastFrequency; // nadpisanie obecnej czestotlwiosci odczytana z pamieci
  return lastFrequency;
  
}

// ----------------------   LCD FUNKCJE ------------

void welcomeMessageOnLCD(){
  //lcd.clear();
  lcd.begin(16,2);   
  lcd.setCursor(2,0); 
  lcd.print("RADIO PIOTRA");
  lcd.setCursor(0,1); 
  lcd.print(actualFrequency);
  lcd.setCursor(6,1); 
  lcd.print("MHz"); 
  lcd.setCursor(10,1);
    lcd.print(signal_percent);
  if (signal_percent < 10){
    lcd.setCursor(11,1);
  }
  else if(signal_percent > 9 && signal_percent < 100){
    lcd.setCursor(12,1);
  }
  else if (signal_percent == 100){
    lcd.setCursor(13,1);
  }
  lcd.print("%");
}

void scannUpInProgressLCD( int time ){
  lcd.clear();
  lcd.begin(16,2);   
  lcd.backlight(); // zalaczenie podwietlenia 
  lcd.setCursor(3,0); 
  lcd.print("SKANOWANIE");
  lcd.setCursor(6,1); 
  lcd.print("--->");
  delay(time);
}

void byteValues(){
  lcd.clear();
  lcd.begin(16,2);   
  lcd.backlight(); // zalaczenie podwietlenia 
  lcd.setCursor(0,0); 
  lcd.print( ((buffer[0]&0x3F)<<8)+buffer[1] );
  lcd.setCursor(6,1); 
  lcd.print(actualFrequency);
}

void scannDownInProgressLCD( int time ){
  lcd.clear();
  lcd.begin(16,2);   
  lcd.backlight(); // zalaczenie podwietlenia 
  lcd.setCursor(3,0); 
  lcd.print("SKANOWANIE");
  lcd.setCursor(6,1); 
  lcd.print("<---");
  delay(time);
}

void printActFreq (boolean displayName){
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print(actualFrequency);
  lcd.setCursor(6,0); 
  lcd.print("MHz");
  lcd.setCursor(10,0);
  lcd.print(signal_percent);
  if (signal_percent < 10){
    lcd.setCursor(11,0);
  }
  else if(signal_percent > 9 && signal_percent < 100){
    lcd.setCursor(12,0);
  }
  else if (signal_percent == 100){
    lcd.setCursor(13,0);
  }
  lcd.print("%");
  lcd.setCursor(14,0);
  if (buffer[2]&0x80){
    lcd.print("ST");
  }
  else {
    lcd.print("MO");
  }

  lcd.setCursor(0,1);

  if(displayName){
    lcd.setCursor(0,1);
    
    if(!choice ){
      lcd.print(fixedNames[stationIndex]);
    }
    
    if(choice ){
      lcd.print(fixedNames[(stationIndex -1)]);
    }
    
  }

  
}

void setGoodByeScreen ( int time ){
  lcd.clear();
  lcd.begin(16,2);   
  lcd.backlight(); // zalaczenie podwietlenia 
  lcd.setCursor(2,0); 
  lcd.print("DO WIDZENIA!");
  lcd.setCursor(6,1); 
  lcd.print("####");
  delay(time);
 }

// ----------------------   ZEGARKOWE FUNKCJE ------------


void setNewTimeForClock(){
    DateTime now = rtc.now();
    TimeDisp[0] = now.hour() / 10;  
    TimeDisp[1] = now.hour() % 10; 
    TimeDisp[2] = now.minute() / 10;
    TimeDisp[3] = now.minute() % 10;
}

void displayCurrentTimeAndDateToSerial(){
    DateTime now = rtc.now();
    //Serial.print("Y/M/D: "); 
    //Serial.print(now.year(), DEC);
    //Serial.print('/');
    //Serial.print(now.month(), DEC);
    //Serial.print('/');
    //Serial.print(now.day(), DEC);
    //Serial.print(' ');
    //Serial.print(now.hour(), DEC);
    //Serial.print(':');
    //Serial.print(now.minute(), DEC);
    //Serial.print(':');
    //Serial.println(now.second(), DEC); 
    delay(1000);
}

void hourUp (){ 
  //rtc.adjust(DateTime(2019, 12, 30, 22, 40, 00));
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() + 1, now.minute(), 00));
  setNewTimeForClock();
  tm1637.display(TimeDisp);
}

void hourDown (){ 
  //rtc.adjust(DateTime(2019, 12, 30, 22, 40, 00));
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() - 1, now.minute(), 00));
  setNewTimeForClock();
  tm1637.display(TimeDisp);
}

void minuteUp (){
  //rtc.adjust(DateTime(2019, 12, 30, 22, 40, 00));
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() , now.minute() + 1, 00));
  setNewTimeForClock();
  tm1637.display(TimeDisp);
}

void minuteDown (){
  //rtc.adjust(DateTime(2019, 12, 30, 22, 40, 00));
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() , now.minute() - 1, 00));
  setNewTimeForClock();
  tm1637.display(TimeDisp);
}

 
