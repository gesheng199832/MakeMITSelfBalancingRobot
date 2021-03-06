/*
Using a single directional imput from an IMU, convert the string data into a usable int, then use the 
value of the angle and PID control to drive motors to stabilize the system.
Future Steps:
Experiment with PID library and AUTOTUNEPID function
Tuning of the PID Constants is necessary
Scale the Output
*/

// Clockwise and counter-clockwise definitions.
// Depending on how you wired your motors, you may need to swap.
#define CW  0
#define CCW 1

// Motor definitions to make life easier:
#define MOTOR_A 0
#define MOTOR_B 1

// Pin Assignments //
// Don't change these! These pins are statically defined by shield layout
const byte PWMA = 3;  // PWM control (speed) for motor A
const byte PWMB = 11; // PWM control (speed) for motor B
const byte DIRA = 12; // Direction control for motor A
const byte DIRB = 13; // Direction control for motor B

//Serial Comm
#define DATABUFFERSIZE 10
char dataBuffer[DATABUFFERSIZE + 1]; //Add 1 for NULL terminator
byte dataBufferIndex = 0; // I think this can be deleted.
char startChar = '#'; 
char endChar = '\r';
boolean storeString = false; //This will be our flag to put the data in our buffer

int angle;

int scale = 5;

//PID values, parameters, and Constants
int settime = 20; //loop time in milliseconds
float Kp = 1;
float Ki = 0;
float Kd = 0;
float Pout;
float Iout;
float Dout;
int Output;
float now;
float lasttime = 0;
float timechange;
float Input;
float lastinput;
float Setpoint = 0;
int calibAngle;
float error;
float errorsum = 0;
float Derror;

void setup() {
  Serial.begin(57600); // Begins talk to the sensor
  setupArdumoto(); // Set all pins as outputs
  boolean lp = true;
  while (lp) {
    if(getSerialString()) {
      calibAngle = getAngleMeasure();
      lp = false;
    }
  }
}

boolean getSerialString() {
  static byte dataBufferIndex = 0;
  while(Serial.available() > 0) {
    char incomingbyte = Serial.read();
    
    if(incomingbyte == startChar) {
      dataBufferIndex = 0;  //Initialize our dataBufferIndex variable
      storeString = true;
      continue; //return to loop
    }
      
    if(storeString) {
      //Let's check our index here, and abort if we're outside our buffer size
      //We use our define here so our buffer size can be easily modified
      if(dataBufferIndex == DATABUFFERSIZE) {
        //Oops, our index is pointing to an array element outside our buffer.
        dataBufferIndex = 0;
        storeString = false;
        break;
      }
          
      else if(incomingbyte == endChar) {
        dataBuffer[dataBufferIndex] = 0; //null terminate the C string
        //Our data string is complete.  return true
        storeString = false;
        return true;
      }
      
      // If the byte needs to be recorded
      else {
        dataBuffer[dataBufferIndex++] = incomingbyte;
        dataBuffer[dataBufferIndex] = 0; //null terminate the C string
      }
    }
  }  
  return false;  
}  

int getAngleMeasure() {
  char delimiters[] = ",";
  char* valPosition;  
 
  valPosition = strtok(dataBuffer, delimiters);
   
  if (valPosition) {
    angle = atoi(valPosition);
    angle = angle + calibAngle;
    valPosition = strtok(NULL, delimiters);
    angle = -angle; //make angle positive. depends on orientation of IMU
  }
  else {
    angle = calibAngle; //in case of garbage data
  }
  return angle;
}
  
void driveWithPID() {
  now = millis();
  timechange = (now - lasttime);
  
  if (timechange >= settime) {
    Input = (angle); 
    error = (Setpoint) - Input;
    errorsum = errorsum + error;
    Derror = Input - lastinput;
    Pout = Kp * error;
    Iout = Ki * errorsum;
    Dout = Kd * Derror;
    
      if (Iout > 25)
        Iout = 25;
      if (Iout < -25)
        Iout = -25;
        
    Output = (int)(Pout + Iout + Dout);
    
      if (Output > 255)
        Output = 255;
      if (Output < -255)
        Output = -255;
      
      int dir = CW;
      if (Output < 0){
        dir = CCW;
        Output = -Output;
    }    
        
    lastinput = Input;
    lasttime = now;
    
    if (error == 0) {
      errorsum = 0;
        stopArdumoto(MOTOR_A);  // STOP motor A 
        stopArdumoto(MOTOR_B);  // STOP motor B 
    }
      
    else {
      driveArdumoto(MOTOR_A, dir, Output);
      driveArdumoto(MOTOR_B, dir, Output);
    }
    
    /*
    Serial.print("Angle Measure: ");
    Serial.print(angle);
    Serial.print(". Drive Direction: ");
    Serial.print(dir);
    Serial.print(". Drive Amount: ");
    Serial.println(Output);
    */
    

// Print values
// Keep this format to use in processing (i.e. comment lines 4,5,8,11,14,15,16,17,20,23)
Serial.print ("angle"); //line 1
Serial.print (":");
Serial.print (Input);
//Serial.print (",");
//Serial.print ("error : ");
Serial.print (error);
Serial.print (", ");
//Serial.print ("errorsum : ");
Serial.print (errorsum);
Serial.print (",");
//Serial.print ("Pout : ");
Serial.print (Pout);
Serial.print (", ");
//Serial.print ("Derror : ");
//Serial.print (Derror);
//Serial.print (", ");
//Serial.print ("Iout : ");
Serial.print (Iout);
Serial.print (", ");
//Serial.print ("Dout : ");
Serial.print (Dout);
Serial.print (", ");
//Serial.print ("Output : ");
Serial.println (Output);

  }
}
  
// driveArdumoto drives 'motor' in 'dir' direction at 'spd' speed
void driveArdumoto(byte motor, byte dir, byte spd) {
  if (motor == MOTOR_A) {
    digitalWrite(DIRA, (dir + 1) % 2);
    analogWrite(PWMA, spd);
  }
  else if (motor == MOTOR_B) {
    digitalWrite(DIRB, dir);
    analogWrite(PWMB, spd);
  }  
}
  
// setupArdumoto initialize all pins
void setupArdumoto() {
  // All pins should be setup as outputs:
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(DIRB, OUTPUT);
  
  // Initialize all pins as low:
  digitalWrite(PWMA, LOW);
  digitalWrite(PWMB, LOW);
  digitalWrite(DIRA, LOW);
  digitalWrite(DIRB, LOW);
}

// stopArdumoto makes a motor stop
void stopArdumoto(byte motor) {
  driveArdumoto(motor, 0, 0);
}

void loop() {
  //Parse string to get angle as int
  if(getSerialString()) {
    getAngleMeasure();  
  }
  driveWithPID();
}
 



