#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Fuzzy.h>
#define ONE_WIRE_BUS 2 //sensor suhu
#define RELAY1 8 // Relay Kipas
#define RELAY2 9 //Relay Pompa Down
#define RELAY3 10 //Relay Pompa Up
#define RELAY4 11 // Relay Heater

//koneksi pin LCD 16x2 ke Arduino UNO
LiquidCrystal_I2C lcd(0x27,16,2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature Suhu(&oneWire);

#define SensorPin A0          //pH meter Analog output to Arduino Analog Input 0
#define Offset 0.00            //deviation compensate
unsigned long int avgValue; 
float phValue;
String NilaiPH;

float Error1, Error, dError, sp=7.2, tair;
int relayON = LOW; //relay nyala
int relayOFF = HIGH; //relay mati



//-------------  FUZZY DAN SET NILAI TIAP VARIABEL -------------
Fuzzy *fuzzy = new Fuzzy();
// FuzzyInput
FuzzySet *negatif       = new FuzzySet(-6.8, -6.8, -1, 0);
FuzzySet *netral        = new FuzzySet(-1, 0, 0.7, 1.7);
FuzzySet *positif       = new FuzzySet(0.7, 1.7, 6.8, 6.8);

// FuzzyInput
FuzzySet *dnegatif       = new FuzzySet(-6.8, -6.8, -1, 0);
FuzzySet *dnetral        = new FuzzySet(-1, 0, 0.7, 1.7);
FuzzySet *dpositif       = new FuzzySet(0.7, 1.7, 6.8, 6.8);


// FuzzyOutput
FuzzySet *singkat        = new FuzzySet(0, 0, 0, 200);
FuzzySet *sedang         = new FuzzySet(100, 200, 200, 300);
FuzzySet *lama           = new FuzzySet(200, 400, 400, 400);


void fuzzySet () {
  // FuzzyInput PH
  FuzzyInput *error = new FuzzyInput(1);

  error->addFuzzySet(negatif);
  error->addFuzzySet(netral);
  error->addFuzzySet(positif);
  
  fuzzy->addFuzzyInput(error);


  // FuzzyInput KELEMBABAN
  FuzzyInput *derror = new FuzzyInput(2);

  derror->addFuzzySet(dnegatif);
  derror->addFuzzySet(dnetral);
  derror->addFuzzySet(dpositif);
  fuzzy->addFuzzyInput(derror);


  // FuzzyOutput RELAY (POMPA)
  FuzzyOutput *pompa = new FuzzyOutput(1);

  pompa->addFuzzySet(singkat);
  pompa->addFuzzySet(sedang);
  pompa->addFuzzySet(lama);
  fuzzy->addFuzzyOutput(pompa);
}

void fuzzyRule () {
  // Building FuzzyRule////////////////////////////////////////////////////////////////// 1
  FuzzyRuleAntecedent *negatif_dnegatif = new FuzzyRuleAntecedent();
  negatif_dnegatif ->joinWithAND(negatif, dnegatif);

  FuzzyRuleConsequent *o_lama1 = new FuzzyRuleConsequent();
  o_lama1->addOutput(lama);
  
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, negatif_dnegatif, o_lama1);
  fuzzy->addFuzzyRule(fuzzyRule1);

  // Building FuzzyRule////////////////////////////////////////////////////////////////// 2
  FuzzyRuleAntecedent *negatif_dnetral = new FuzzyRuleAntecedent();
  negatif_dnetral ->joinWithAND(negatif, dnetral);

  FuzzyRuleConsequent *o_sedang1 = new FuzzyRuleConsequent();
  o_sedang1->addOutput(sedang);
  
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, negatif_dnetral, o_sedang1);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // Building FuzzyRule////////////////////////////////////////////////////////////////// 3
  FuzzyRuleAntecedent *negatif_dpositif = new FuzzyRuleAntecedent();
  negatif_dpositif ->joinWithAND(negatif, dpositif);

  FuzzyRuleConsequent *o_sedang2 = new FuzzyRuleConsequent();
  o_sedang2->addOutput(sedang);
  
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, negatif_dpositif, o_sedang2);
  fuzzy->addFuzzyRule(fuzzyRule3);

  // Building FuzzyRule////////////////////////////////////////////////////////////////// 4
  FuzzyRuleAntecedent *netral_dnegatif = new FuzzyRuleAntecedent();
  netral_dnegatif ->joinWithAND(netral, dnegatif);

  FuzzyRuleConsequent *o_singkat1 = new FuzzyRuleConsequent();
  o_singkat1->addOutput(singkat);
  
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, netral_dnegatif, o_singkat1);
  fuzzy->addFuzzyRule(fuzzyRule4);

  // Building FuzzyRule////////////////////////////////////////////////////////////////// 5
  FuzzyRuleAntecedent *netral_dnetral = new FuzzyRuleAntecedent();
  netral_dnetral ->joinWithAND(netral, dnetral);

  FuzzyRuleConsequent *o_singkat2 = new FuzzyRuleConsequent();
  o_singkat2->addOutput(singkat);
  
  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, netral_dnetral, o_singkat2);
  fuzzy->addFuzzyRule(fuzzyRule5);

  // Building FuzzyRule////////////////////////////////////////////////////////////////// 6
  FuzzyRuleAntecedent *netral_dpositif = new FuzzyRuleAntecedent();
  netral_dpositif ->joinWithAND(netral, dpositif);

  FuzzyRuleConsequent *o_singkat3 = new FuzzyRuleConsequent();
  o_singkat3->addOutput(singkat);
  
  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, netral_dpositif, o_singkat3);
  fuzzy->addFuzzyRule(fuzzyRule6);

  // Building FuzzyRule////////////////////////////////////////////////////////////////// 7
  FuzzyRuleAntecedent *positif_dnegatif = new FuzzyRuleAntecedent();
  positif_dnegatif ->joinWithAND(positif, dnegatif);

  FuzzyRuleConsequent *o_sedang3 = new FuzzyRuleConsequent();
  o_sedang3->addOutput(sedang);
  
  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, positif_dnegatif, o_sedang3);
  fuzzy->addFuzzyRule(fuzzyRule7);

  // Building FuzzyRule////////////////////////////////////////////////////////////////// 8
  FuzzyRuleAntecedent *positif_dnetral = new FuzzyRuleAntecedent();
  positif_dnetral ->joinWithAND(positif, dnetral);

  FuzzyRuleConsequent *o_sedang4 = new FuzzyRuleConsequent();
  o_sedang4->addOutput(sedang);
  
  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, positif_dnetral, o_sedang4);
  fuzzy->addFuzzyRule(fuzzyRule8);

  // Building FuzzyRule////////////////////////////////////////////////////////////////// 9
  FuzzyRuleAntecedent *positif_dpositif = new FuzzyRuleAntecedent();
  positif_dpositif ->joinWithAND(positif, dpositif);

  FuzzyRuleConsequent *o_lama2 = new FuzzyRuleConsequent();
  o_lama2->addOutput(lama);
  
  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, positif_dpositif, o_lama2);
  fuzzy->addFuzzyRule(fuzzyRule9);
 }

 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  digitalWrite(RELAY1, relayOFF);
  digitalWrite(RELAY2, relayOFF);
  digitalWrite(RELAY3, relayOFF);
  digitalWrite(RELAY4, relayOFF);
  lcd.begin();
  Suhu.begin();
  lcd.setBacklight(255);
  
  fuzzySet ();
  fuzzyRule ();

  
}

void loop() {
  // put your main code here, to run repeatedly:
  Suhu.requestTemperatures();
  tair= Suhu.getTempCByIndex(0);

  int buf[10];                //buffer for read analog
  for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(SensorPin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        int temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue=3.5*phValue+Offset;                      //convert the millivolt into pH value
 
  NilaiPH = String(phValue,2);
  
  Error1=Error;
  Error=sp-phValue;
  dError= Error-Error1;

  fuzzy->setInput(1, Error);
  fuzzy->setInput(2, dError);
  fuzzy->fuzzify();

  float pompa = fuzzy->defuzzify(1);

    Serial.print("pH value:   ");
    Serial.println(phValue, 2);
    Serial.print("Suhu Air:   ");
    Serial.println(tair);
    Serial.print("Error   :   ");
    Serial.println(Error);
    Serial.print("dError  :   ");
    Serial.println(dError);
    Serial.print("Pompa   :   ");
    Serial.println(pompa);
    Serial.println();
    delay(2000);

    lcd.setCursor(0,0);
    lcd.print("Suhu : ");
    lcd.print(tair);
    lcd.setCursor(0,1);
    lcd.print("PH   :  "); 
    lcd.print(phValue);

    if (phValue <6.5){
      digitalWrite(RELAY2, LOW);
      delay(pompa);
      digitalWrite(RELAY2, HIGH);
      delay(1000);
    }
    else if (phValue >7.2){
      digitalWrite(RELAY3, LOW);
      delay(pompa);
      digitalWrite(RELAY3, HIGH);
      delay(1000);
    }

    if (tair > 30){
      digitalWrite(RELAY1, LOW);
    }
    else if(tair <= 30 && tair >=27){
      digitalWrite(RELAY1, HIGH);
    }
    
    if(tair < 27){
      digitalWrite(RELAY4, LOW);
    }
    else if(tair <= 30 && tair >=27){
      digitalWrite(RELAY4, HIGH);
    }
}
