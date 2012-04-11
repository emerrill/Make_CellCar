#include <SoftwareSerial.h>
#include <Servo.h>
#include <avr/wdt.h>

//Options

int useServo = 0;          //If using the servo, set this to 1
int use2Buttons = 1;       //If you are controlling 2 buttons, set this to one

int button1Delay = 500;    //How long, in milliseconds, to hold down button one
int button2Delay = 2500;   //How long, in milliseconds, to hold down button two if used

//If we are using a servo
int servoNeutral = 90;     //Angle for the servo neutral position
int servoPress = 33;       //Angle for the servo when pressing the button
int servoDelay = 2000;     //How many miliseconds to hold the button down to start


//Phone numbers to accept start requests from.
//You can change the number of slots by changing NUM_OF_NUMS and 
//change the number of elements in numbers
#define NUM_OF_NUMS 6
String numbers[NUM_OF_NUMS] = {String("2123231111"), //Change these numbers to you desired numbers
                               String("2123232222"), 
                               String("2123233333"), 
                               String("2123234444"), 
                               String("2123235555"), 
                               String("2123236666")  
                               };



float voltMulti = 0.0447;  //The multipler for the analog read to battery voltage.
                           // = (((R1+R2)/R2)*5)/1024 = ((24700/2700)*5)/1024 = 0.0447
float cutoffVolt = 11.0;   //The voltage you would like to turn off at



//USE_WDT enables to watchdog timer, making the system more stable, but will only work properly with
//Ardunio Unos or other updated bootloader Arduinos. If you are not sure, leave set to 0.
//
//CAUTION: Set this option to 1 only if you are using a Arduino Uno, or an older Arduino with
//CAUTION: the Adafruit or Optiboot bootloaders installed.
//CAUTION: Enabling this option on a standard Arduino Duemilanove, Diecimila, or otheres will
//CAUTION: cause the Arduino to enter an infinate bootloop that is very hard (but not impossible),
//CAUTION: to exit.
#define USE_WDT 0


//Normally 7 for North America, 4 for Europe. Check online documentation of SMB5100 for more details
#define SBAND 7

//End of options











SoftwareSerial cell(2,3);   //Create a 'fake' serial port. Pin 2 is the Rx pin, pin 3 is the Tx pin.

Servo myServo;

//Buffer
#define LINE_BUF_SZ 256
char buf[LINE_BUF_SZ];
int bufidx = 0;

//Some varbiables for the cell phone
byte ready = 0; //Is the cell shield ready
byte network = 0; //Do we have a network
char mem = 'S';


//Some misc variables we'll use later
char incoming_char=0;      //Will hold the incoming character from the Serial Port.
char a = 0;
char b = 0;
int num = 0;

//Pins
int voltPin = A1;
int killPin = 4;
//If we are using relay buttons
int buttonPin1 = 5;
int buttonPin2 = 6;
//If we are using a servo
int servoPin = 9;


float voltAvg = 12.00;


void setup()
{
  if (USE_WDT) {
    wdt_enable(WDTO_8S); //Start the Watchdog timer
  }
  
  //Initialize serial ports for communication.
  Serial.begin(57600); //For communication with computer
  cell.begin(9600);
  cell.println("AT+CFUN=1,1");
  check_sband();
  
  //Setup the controller pins
  pinMode(voltPin, INPUT);
  pinMode(killPin, OUTPUT);
  digitalWrite(killPin, LOW);
  digitalWrite(buttonPin1, LOW);
  digitalWrite(buttonPin2, LOW);
  
  if (useServo) {
    myServo.attach(servoPin);
    myServo.write(servoNeutral); //The 'default' angle
  } else {
    pinMode(buttonPin1, OUTPUT);
    pinMode(buttonPin2, OUTPUT);
  }
  
  
  Serial.println("Starting CellCar");
}


void loop()
{
  //Check and process the cell shield incoming buffer
  if (cell.available() > 0) {
    while(cell.available() > 0) {
      //Read the buffer until the end off the line
      buf[bufidx] = cell.read();
      if ((buf[bufidx] == 0x0A) && (buf[bufidx-1] == 0x0D)) {
        buf[bufidx-1] = 0;
        process_buf();
      } else {
        bufidx++;
      }

    }
  }
  
  
  //Check the battery voltage
  check_voltage();
  

  
  if (USE_WDT) {
    wdt_reset(); //Pat the watchdog to preven a reset
    
    //After a few days, reboot the system.
    if (millis() > 172800000) { //48 hours
      Serial.println("Reboot");
      while(1) { //Force reset
        //We enter an infinate loop, this will cause the watchdog to reset us after 8 seconds.
      }
    }
  }
  
}

//Process the input buffer
void process_buf() {
  if (buf[0] == 0) {
    bufidx = 0;
    return;
  }
  
  String str = String(buf);
  Serial.println(str);
  
  bufidx = 0;
  
  //SIND is a status line from the shield
  if (str.startsWith("+SIND: ")) {
    a = str.charAt(7);
    b = str.charAt(8);
    if (b) {
      num = (b - 48) + ((a - 48) * 10);
    } else {
      num = a - 48;
    }
    
    switch (num) {
       case 4: //GSM Ready
         Serial.println("GSM Ready");
         cell.println("AT+CLIP=1"); //Set caller ID on
         cell.println("AT+CMGF=1"); //Text TXT MODE
         cell.println("AT+CNMI=3,1,0,0"); //Show when we receive SMS
         ready = 1;
         module_ready();
         break;
       case 11: //Connected to network
         Serial.println("Network Found");
         network = 1;
         break;
       case 8: //No network
         Serial.println("Network Lost");
         network = 0;
         break;
       
    }
  } else if (str.startsWith("+CMTI: ")) { //SMS received
    a = str.charAt(12);
    b = str.charAt(13);
    if (b) {
      num = (b - 48) + ((a - 48) * 10);
    } else {
      num = a - 48;
    }
    
    a = str.charAt(8);
    
    String sms;
    String body;
    
    if (a == 'B') { //SMS can be stored in various memory locations. 
      if (mem == 'S') {
        get_command("AT+CPMS=\"BM\",\"SM\"");
        mem = 'B';
        
        sms = get_sms_header(num);
        body = get_sms_body();
        Serial.print(body);
        
        get_command("AT+CPMS=\"SM\",\"SM\"");
      }
    } else {
      if (mem == 'B') {
        get_command("AT+CPMS=\"SM\",\"SM\"");
        mem = 'S';
        
        sms = get_sms_header(num);
        body = get_sms_body();
        Serial.print(body);
      }
    }
    
    Serial.print("Text header: ");
    Serial.println(sms);
    
    if (check_number(sms)) {
      start_car();
    }
    
  } else if (str.startsWith("+CLIP: ")) { //Incoming call
    String tmp = str.substring(str.indexOf('"')+1, str.lastIndexOf('"'));
    get_command("ATH"); //Command to hangup
    
    Serial.print("Call from: ");
    Serial.println(tmp);
    
    if (check_number(tmp)) {
      start_car();
    }

  }
  
  
  

  
  
}

//See if the incoming number is in the valid list.
byte check_number(String str) {
  int i = 0;
  for (i = 0; i <= NUM_OF_NUMS; i++) {
    if (str.indexOf(numbers[i]) > -1) {
      Serial.println("Good Number");
      return 1;
    }
  }
  
  Serial.println("Bad Number");
  return 0;
}

void check_voltage() {
  if ((millis() % 4000)) {
    return;
  }
  
  delay(1);
  
  float volt = ((float)analogRead(voltPin)*voltMulti);  
  
  voltAvg = ((voltAvg*4) + volt)/5;

  if (voltAvg < cutoffVolt) {
    digitalWrite(killPin, HIGH);
  }
}

//Start the car
void start_car() {
  Serial.println("Starting Car");
  
  //If a servo start
  if (useServo) {
    myServo.write(servoPress);
    delay(servoDelay);
    myServo.write(servoNeutral);
  } else {
    if (use2Buttons) {
      digitalWrite(buttonPin1, HIGH);
      delay(button1Delay);
      digitalWrite(buttonPin1, LOW);
      delay(200);
      digitalWrite(buttonPin2, HIGH);
      delay(button2Delay);
      digitalWrite(buttonPin2, LOW);
    } else {
      digitalWrite(buttonPin1, HIGH);
      delay(button1Delay);
      digitalWrite(buttonPin1, LOW);
    }
  }
  Serial.println("Car Started");
}

//Get the header of an SMS
String get_sms_header(int num) {
  String tmp = get_command("AT+CMGR=" + num);
  
  get_command("AT+CMGD=" + num);

  
  return tmp;
  
}

//Get the body of an SMS
String get_sms_body() {
  String body = String();
  String line;
  
  while (1) {
    if (!get_line()) {
      return body;
    }
    line = String(buf);
    
    if ((line.length() == 2) && (line.startsWith("OK"))) {
      return body;
    }
    
    body.concat(line);
    
  }
}

//Get back a line of data
byte get_line() {
  //delay(50);
  unsigned long time = millis();
  
  while (1) {
    //Timeout if we don't get it back fast enough
    if (millis() > (time + 1000)) {
      Serial.println("Line Not Found");
      return 0;
    }
    while(cell.available() > 0) {
        buf[bufidx] = cell.read();
        if ((buf[bufidx] == 0x0A) && (buf[bufidx-1] == 0x0D)) {
          buf[bufidx-1] = 0;
          
          if (buf[0] == 0) {
            bufidx = 0;
          } else {
            Serial.println("Line Found");
            bufidx = 0;
            return 1;
          }
          
          
        } else {
          bufidx++;
        }

    }
  }
}

//Execute command and get results
String get_command(String command) {
  cell.println(command);

  get_line();
  
  String line = String(buf);
  
  return line;
  
}

void check_sband() {
  String str = get_command("AT+SBAND?");
  
  a = str.charAt(8);
  b = str.charAt(9);
  if (b) {
  num = (b - 48) + ((a - 48) * 10);
  } else {
  num = a - 48;
  }
  
  if (num != SBAND) {
    String command = String("AT+SBAND=");
    command.concat(SBAND);
    get_command(command);
  } 
}


//Execute a command on ready.
void module_ready() {
  get_command("AT+CNMI=3,1,0,0");
}
