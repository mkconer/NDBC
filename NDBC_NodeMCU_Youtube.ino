// NDBC Wind & Wave Watcher - Designed by Mark Conner
// This Sketch is designed for the ESP8266 or NodeMCU
// The National Data Buoy Center or NDBC is part of NOAA and the NWS
// The NDBC designs, develops, operates, and maintains a network of data collecting buoys and coastal stations.
// The stations measure wind speed, wind direction, wind gust, atmospheric pressure, and air temperature
// Some stations also measure sea surface temperature, wave height and period.
// This program downloads the Buoy Data from the NDBC website line by line and parses that information so that calculations can be made
// and the information is displayed to the Serial Monitor and an LCD.
// Each line of the information is sent from the NDBC in the form of a String, some of this must be converted to Integers or Floats
// so that calculations can be made; for instance, to convert Knots to MPH or Celcius into Fahrenheit.
// The display will blink when the Wind Speed reaches a set Speed (windSpeedAlert), alerting the Person to favorable conditions for Wind Surfing, or other sports.
// You will need a NodeMCU and a 4 x 20 LCD display, Check the I2C address of Your LCD mine is 0x3F - use an I2C scanner tool to determine yours
// The LCD works on 5v where as the NodeMCU is 3.3v - I have seen no ill effects after many days with this setup; however, a logic level converter may be used
// The LCD power will be hooked to to the VIN and Ground terminals closest to the power plug, SCL goes to D1 and SDA to D2
// Not all information downloaded from the NDBC station is displayed to the Serial Monitor and LCD, you may wish to change or add to the program selecting which fields you prefer; for instance,
// the buoy provides it Longitude and Latitude, I see no reason to display this information, but you may wish to do so.
// Also, be aware the not all buoys provide the same information; for instance, some may not have wave direction or height. Sometimes, buoys may go offline altogether.


#include <ESP8266WiFi.h>                                // Include the ESP8266 Library
#include <LiquidCrystal_I2C.h>                          // Include the I2C Liquid Crystal Display Library

LiquidCrystal_I2C lcd(0x3F,20,4);                       // Set the LCD I2C address to 0x3F and screen size to 20 chars and 4 line display
                                                        // The I2C address may be different for your display
                                                        // Use an I2C Scanner sketch to determine the address of your LCD
                                                        // SCL is D1, and SDA is D2 on the NodeMCU

const char* ssid     = "yourrouter";                    // Enter Your Router SSID
const char* password = "yourpassword";                  // Enter Your Router Password
const char* host     = "www.ndbc.noaa.gov";             // URL of National Data Buoy Center - Do Not change
const char* buoyID   = "41008";                         // Enter the Buoy Station ID # of the Buoy Station you want to monitor
float windSpeedAlert = 20;                              // Set the minimum wind speed (in mph) you wish to be alerted (flash lcd)
int Refresh = 30;                                       // Refresh is the Refresh Rate of how often the NDBC weather data is requested, 25 = approx every 5 minutes
                                                        // The Buoy data is usually updated once per hour, so downloading once every five minutes is more than enough
                                                        // DO NOT change this to less than 25, the NDBC may block your network address 

String url = "http://www.ndbc.noaa.gov/data/latest_obs/latest_obs.txt";
int LED = D0;                                           // NodeMCU-12e pin D0 is the Red led

String StationID;                                       // global variable to store the Station ID
int    windDirInt;                                      // gloabl variable to store the Wind Direction
String resultCD;                                        // global variable to store the Cardinal Direction (wind, wave)
float  windSpdFlt;                                      // global variable to store the Wind Speed
float  windGstFlt;                                      // global variable to store the Wind Gust
String waveHeight;                                      // global variable to store the Wave Height
int    waveDirInt;                                      // global variable to store the Wave Direction
float  airTempFlt;                                      // global variable to store the Air Temperature
float  waterTempFlt;                                    // global variable to store the Water Temperature

void setup() {
  Serial.begin(115200);                                 // Set the Serial Monitor baud rate
  pinMode(LED, OUTPUT);                                 // Initialize the LED_BUILTIN pin as an output 
  lcd.init();                                           // Initialize the lcd 
  lcd.backlight();
  
  lcd.clear();                                          // Clear LCD display  
  lcd.setCursor(0,1);                                   // Set cursor position
  lcd.print(" National Data Buoy");  
  lcd.setCursor(0,2);                                   // Set cursor position
  lcd.print("    Center (NDBC)");    
  delay(1500);                                          // Pause X seconds for Opening Screen   
  
  Serial.print(" Connecting to ");
  Serial.println(ssid);  
  WiFi.begin(ssid, password);                           // Connect to WiFi network
  
  Serial.print("Connecting to NDBC");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);    
    }
  Serial.println();
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  

  lcd.clear();                                          // Clear LCD    
  lcd.setCursor(0,1);                                   // Set cursor position
  lcd.print(" NDBC Wind & Wave");
  lcd.setCursor(0,2);                                   // Set cursor position
  lcd.print("    Watcher");
  
}

void loop() {                                           // Main Loop
    
  digitalWrite(LED, LOW);                               // Turn the onboard LED on (Note that LOW is the voltage level                                     
  delay(1000);                      
  digitalWrite(LED, HIGH);                              // Turn the onboard LED off by making the voltage HIGH  
  delay(1000);    
  
  Serial.println();
  Serial.print("                 Connecting to ");
  Serial.println(host);  

  lcd.clear();                                          // Clear LCD display
  lcd.setCursor(0,1);                                   // Set cursor position
  lcd.print(" Downloading Latest");
  lcd.setCursor(0,2);                                   // Set cursor position
  lcd.print("     Buoy Data");  
  
  
  WiFiClient client;                                    // Create TCP connection
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection failed"); 
    WiFi.disconnect();                                  // If connection failed then disconnect and try again
    reconnect();                                        // run procedure to establish WIFI connection
    return;

  lcd.clear();                                          // Clear lcd 
  lcd.setCursor(0,0);                                   // Set cursor position
  lcd.print("Connection Failed");    
  }  
      
  Serial.println(); 
  Serial.println("                Requesting NDBC Observation Data  ");
  Serial.print("_________________________________________________________________");
  
  //Serial.println(url);                                // Used for Debugging
 
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +   // Send request to the server 
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");               
  delay(2000);                                          // Delay after request
     
  char databuffer[117];                                 // Create buffer of size 117   
  
  while(client.available()){                            // Parse each line of the file for Station ID and data
    String text = client.readStringUntil('\n');         // Create a String called text and read each line of the NDBC txt file
    //Serial.println(text);                             // Used for debugging
    text.toCharArray(databuffer,117);                   // convert string.toCharArray(buf, len)
    //Serial.println(databuffer);                       // Uncomment to Print all Station ID's and raw Data to Serial Monitor (use for debugging)
  
     if (databuffer == strstr(databuffer,buoyID))       // strstr searches for a string inside a string
                                                        // search for the selected Station ID Number
     {
      //Serial.println();
      //Serial.println("Parsing Data");      
      //Serial.println(databuffer);                     // Print complete line of Raw Data of the selected Staion ID - buffer string
      Serial.println();  

    /*************************************** Parse & Display the Data *******************************************/  

    Serial.print("            Station ID: ");     
    StationID = (text.substring(0, 6));
    //Serial.print(text.substring(0, 6));               // Station ID   
    Serial.print(StationID);                            // Print Station ID to Serial Monitor        
    
    //Serial.print(" Latitude: ");                      // Uncomment to display these values to the Serial Monitor
    //Serial.print(text.substring(7, 14));              // Latitude of Station
    //Serial.print(" Longitude: ");
    //Serial.print(text.substring(15, 22));             // Longitude of Station
    //Serial.print(" Year: ");
    //Serial.print(text.substring(23, 27));             // Year
    //Serial.print(" Month: ");
    //Serial.print(text.substring(28, 30));             // Month
    //Serial.print(" Day: ");
    //Serial.print(text.substring(31, 33));             // Day
    
    Serial.print(" Last Update: ");    
    Serial.print(text.substring(34, 36));               // Hour   
    Serial.print(":"); 
    Serial.print(text.substring(37, 39));               // Minute
    Serial.print(" (GMT) ");                            // Greenwich Mean Time (GMT) 
    Serial.println();
    
    Serial.print("Wind Direction: ");    
    //Serial.print(text.substring(40, 43));             // Print Wind Direction
    String windDir(text.substring(40, 43));             // Create a String called windDir from the substring of our data      
    windDirInt = (windDir.toInt());                     // Converts string (windDir) to an Integer (windDirInt)
    Serial.print(windDirInt);
    Serial.print(char(176));                            // Degree Symbol ASCII symbol
    resultCD = cardinalDirection(windDirInt);           // Call function to Calculate Cardinal Wave Direction
    Serial.print(resultCD);      
        
    //Serial.print(" Wind Speed: ");                    // Uncomment to display these values to the Serial Monitor
    //Serial.print(text.substring(44, 49));             // Wind Speed in Knots
    //Serial.print(" Knots ");
    
    String windSpdStr(text.substring(44, 49));          // Create a String called windSpdStr from the substring of our data      
    windSpdFlt = (windSpdStr.toFloat());                // Converts string (windSpdStr) to a Float (windSpdFlt)        
    windSpdFlt = (windSpdFlt * 1.15078);                // Calculate MPH value from Knots
    Serial.print(" Wind Speed: ");
    Serial.print(windSpdFlt, 1);                        // Print float Wind Speed to 1 decimal place Value    
    Serial.print(" mph");
    
    //Serial.print(" Wind Gust: ");                     // Uncomment to display these values to the Serial Monitor
    //Serial.print(text.substring(50, 55));             // Wind Gust in Knots
    //Serial.print(" Knots ");        

    String windGstStr(text.substring(50, 55));          // Create a String called windGstStr from the substring of our data      
    windGstFlt = (windGstStr.toFloat());                // Converts string (windGstStr) to a Float (windGstFlt)        
    windGstFlt = (windGstFlt * 1.15078);                // Calculate MPH value from Knots
    Serial.print(" Wind Gust: ");
    Serial.print(windGstFlt, 1);                        // Print float Wind Gust to 1 decimal place Value    
    Serial.print(" mph");
    Serial.println(); 
        
    Serial.print("            Wave Height: ");
    //Serial.print(text.substring(56, 60));             // Wave Height
    waveHeight = (text.substring(56, 60));              // Wave Height
    Serial.print(waveHeight);    
    
    //Serial.print(" Wave Period: ");                   // Uncomment to display these values to the Serial Monitor
    //Serial.print(text.substring(61, 64));             // DPD Dominant Wave Period
    //Serial.print(" Avg Period: ");
    //Serial.print(text.substring(65, 69));             // APD Average Period
    
    Serial.print("   Wave Direction: ");
    Serial.print(text.substring(70, 73));               // MWD Mean Wave Direction
    String waveDir = (text.substring(70, 73));          // Create a String called waveDir from the substring of our data
    waveDirInt = (waveDir.toInt());                     // Converts string (waveDir) to an Integer (waveDirInt)     
    Serial.print(char(176));                            // Print Degree Symbol ASCII symbol
    resultCD = cardinalDirection(waveDirInt);           // Call function to Calculate the Cardinal Direction of the wave
    Serial.print(resultCD);                             // Print result of function for Cardinal Direction
    Serial.println();            
    
    //Serial.print("Pressure: ");                       // Uncomment to display these values to the Serial Monitor
    //Serial.print(text.substring(74, 80));             // Atmospheric Pressure
    //Serial.print(" PTDY: ");
    //Serial.print(text.substring(81, 86));             // PTDY Pressure Tendency  

    String airTempStr(text.substring(87, 92));          // Create a String called airTempStr from the substring of our data      
    airTempFlt = (airTempStr.toFloat());                // Converts string (airTempStr) to a Float (airTempFlt)        
    airTempFlt = ((airTempFlt * 1.8)+ 32);              // Calculate Farenheit value
    Serial.print("               Air Temp: ");
    Serial.print(airTempFlt, 1);                        // Print float Air temp to 1 decimal Value
    Serial.print(char(176));                            // Degree Symbol ASCII symbol
    Serial.print("F");     
     
    //Serial.print(" Air Temp: ");                      // Uncomment to display these values to the Serial Monitor
    //Serial.print(text.substring(87, 92));             // Air Temperature in Celcius
    //Serial.print(char(176));
    //Serial.print("C");

    String waterTempStr(text.substring(93, 98));        // Create a String called waterTempStr from the substring of our data      
    waterTempFlt = (waterTempStr.toFloat());            // Converts string (waterTempStr) to a Float (waterTempFlt)        
    waterTempFlt = ((waterTempFlt * 1.8)+ 32);          // Calculate Farenheit value
    Serial.print("   Water Temp: ");
    Serial.print(waterTempFlt, 1);                      // Print float Water temp to 1 decimal Value
    Serial.print(char(176));                            // Degree Symbol ASCII symbol
    Serial.print("F");    
        
    //Serial.print(" Water Temp: ");                    // Uncomment to display these values to the Serial Monitor
    //Serial.print(text.substring(93, 98));             // Water Temperature in Celcius
    //Serial.print(char(176));
    //Serial.print("C");
    
    //Serial.print(" Dew Point: ");                     // Uncomment to display these values to the Serial Monitor
    //Serial.print(text.substring(99, 104));            // Dew Point
    //Serial.print(" Visibility: ");
    //Serial.print(text.substring(105, 109));           // Visibility
    //Serial.print(" Tide: ");
    //Serial.print(text.substring(110, 116));           // Tide      
      }           
   }     
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +   // Close the connection to the NDBC server
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  Serial.println();              
  //Serial.println("end of file");                  
  Serial.println("_________________________________________________________________");
  Serial.println("                      Close connection");
  Serial.print("                         Pausing.."); 
  
  delay(1000);                                        // Pause X Seconds 

                                                      // Display weather data to the LCD
 for (int i=0; i <= Refresh; i++)                     // Display several times before requesting update from NDBC
 {      
  displayStationID (StationID);                       // Call function to display the Station ID to the LCD
  delay(2000);
  resultCD = cardinalDirection(windDirInt);           // Call function to Calcula the Cardinal Wind Direction
  displayWindDir(resultCD);                           // Call function to display the Cardinal Direction of the wind
  displayWindGst(windGstFlt);                         // Call function to display the wind gust to the LCD
  displayWindSpd(windSpdFlt);                         // Call function to display the wave height to the LCD  
  delay(4000); 
  displayWaveHt(waveHeight);                          // call function to display the wave height to the LCD
  resultCD = cardinalDirection(waveDirInt);           // Call function to Calcula the Cardinal Wave Direction
  displayWaveDir(resultCD);                           // Call function to display the wave direction to the LCD 
  displayAirTmp(airTempFlt);                          // Call function to display the air temperature to the LCD  
  displayWaterTmp(waterTempFlt);                      // Call function to display the water temp to the LCD
  Serial.print(".");
  yield();                                            // Yield, hopefully to keep the Watch Dog Timer from resetting the ESP8266
  delay(4000); 
 }
               
}                                                     // End of Main Loop

void reconnect()
{ 
 Serial.print(" Connecting to ");
  Serial.println(ssid);  
  WiFi.begin(ssid, password);                         // Connect to WiFi network
  
  Serial.print("Connecting to NDBC");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);    
    }
  Serial.println();
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

void displayStationID(String ID)                      // Display the NDBC buoy station ID to the LCD
{ 
    lcd.clear();                                      // Clear LCD display  
    lcd.setCursor(0,0);                               // Set cursor position
    lcd.print("   Latest Weather");
    lcd.setCursor(0,1);                               // Set cursor position
    lcd.print("   Conditions at");
    lcd.setCursor(0,2);                               // Set cursor position
    lcd.print("   Station ");    
    lcd.print(ID);       
}
void displayWindDir(String resultCD)                  // Display the Cardinal Direction (N,S,E,W etc.) to the LCD
{  
    lcd.clear();                                      // Clear LCD display     
    lcd.setCursor(0,0);                               // Set cursor position
    lcd.print("Wind Dir  :");     
    lcd.print(resultCD); 
}    

void displayWindSpd(int windSpd)                      // Display the wind speed to the LCD
{
    lcd.setCursor(0,1);                               // Set cursor position
    lcd.print("Wind Speed: ");      
    lcd.print(windSpd, 1);                            // Print float Wind Speed to 1 decimal place Value    
    lcd.print(" mph"); 
    if (windSpdFlt >= windSpeedAlert) { 
          lcd.noBacklight();                          // Turn off backlight
          delay(200); 
          lcd.backlight();                            // Turn On backlight 
          delay(200);
          lcd.noBacklight();                          // Turn off backlight
          delay(200); 
          lcd.backlight();                            // Turn On backlight 
          delay(200);   
      
      for (int i=0; i <= 5; i++) 
        {           
          lcd.setCursor(0,1);                         // Set cursor position
          lcd.print("                   ");
          delay(500);
          lcd.setCursor(0,1);                         // Set cursor position
          lcd.print("Wind Speed: ");      
          lcd.print(windSpd);                         // Print Wind Speed Value    
          lcd.print(" mph"); 
          delay(500);
        }                                             // end of for loop
    }                                                 // end of "if" windSpeedAlert
}    

void displayWindGst(int windGst)                      // Display the wind gust to the LCD
{    
    lcd.setCursor(0,2);                               // Set cursor position
    lcd.print("Wind Gust : ");    
    lcd.print(windGst);                               // Print Wind Gust Value    
    lcd.print(" mph");         
}

void displayWaveHt(String waveHt)                     // Display the wave height to the LCD
{
    lcd.clear();                                      // Clear LCD lcd
    lcd.setCursor(0,0);                               // Set cursor position
    lcd.print("Wave  Height: ");                        
    lcd.print(waveHt);                                // Wave Height
    lcd.print("ft");   
}

void displayWaveDir(String resultCD)                  // Display the wave direction to the LCD
{
    lcd.setCursor(0,1);                               // Set cursor position
    lcd.print("Wave  Dir   :");         
    lcd.print(resultCD);      
}

void displayAirTmp(float airTmp)                      // Display the air temperature to the LCD
{
    lcd.setCursor(0,2);                               // Set cursor position
    lcd.print("Air   Temp  : ");           
    lcd.print(airTmp, 0);                             // Print float Air temp to 0 decimal Value
    lcd.print(char(223));                             // Degree Symbol in ASCII
    lcd.print("F");    
}
void displayWaterTmp(float waterTmp)                  // Display the water temperature to the LCD
{    
    lcd.setCursor(0,3);                               // Set cursor position
    lcd.print("Water Temp  : ");    
    lcd.print(waterTmp, 0);                           // Print float Water temp to 0 decimal Value
    lcd.print(char(223));                             // Degree Symbol in ASCII
    lcd.print("F");        
}

                                                      // The NDBC uses 0 to 360 degrees when displaying wind and wave direction 
                                                      // This function converts the degrees to easier to read cardinal directions 
String cardinalDirection(int cardDir)                 // Determine Cardinal Direction from degrees and return the value
{          
    if ((cardDir >= 0) && (cardDir <= 12))    {return (" North");} 
    if ((cardDir >= 11) && (cardDir <= 34))   {return (" NNE");}
    if ((cardDir >= 33) && (cardDir <= 57))   {return (" NE");}
    if ((cardDir >= 56) && (cardDir <= 79))   {return (" ENE");}
    if ((cardDir >= 78) && (cardDir <= 102))  {return (" NNE");}
    if ((cardDir >= 101) && (cardDir <= 124)) {return (" ESE");}
    if ((cardDir >= 123) && (cardDir <= 147)) {return (" SE");}
    if ((cardDir >= 146) && (cardDir <= 169)) {return (" SSE");}
    if ((cardDir >= 168) && (cardDir <= 192)) {return (" South");}
    if ((cardDir >= 191) && (cardDir <= 214)) {return (" SSW");}    
    if ((cardDir >= 213) && (cardDir <= 237)) {return (" SW");} 
    if ((cardDir >= 236) && (cardDir <= 259)) {return (" WSW");}
    if ((cardDir >= 258) && (cardDir <= 282)) {return (" West");}
    if ((cardDir >= 281) && (cardDir <= 304)) {return (" WNW");}
    if ((cardDir >= 303) && (cardDir <= 327)) {return (" NW");}
    if ((cardDir >= 326) && (cardDir <= 349)) {return (" NNE");}
    if ((cardDir >= 348) && (cardDir <= 361)) {return (" North");}
    if (cardDir == 0)                         {return (" North");}    
}
