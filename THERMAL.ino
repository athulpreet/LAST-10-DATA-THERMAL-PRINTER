#include <SPI.h>
#include <SD.h>
#include "Adafruit_Thermal.h"

// Pin definitions for SPI1
#define SD_CS   PA4    // CS
#define SD_SCK  PA5    // SCK
#define SD_MISO PA6    // MISO
#define SD_MOSI PA7    // MOSI

// Initialize Thermal Printer on Serial1
Adafruit_Thermal printer(&Serial1);

void setup() {
  // Start serial for debug
  Serial.begin(115200);
  
  // Initialize Serial1 for printer
  Serial1.begin(9600);
  printer.begin();
  delay(500);
  
  Serial.println("GPS Data Printer - Reading from SD Card");
  
  // Configure SPI pins for SD card
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH); // Deselect SD card
  
  // Initialize SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  
  // Power-up sequence for SD card
  for(int i = 0; i < 10; i++) {
    SPI.transfer(0xFF);
  }
  delay(100);
  
  // Initialize SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    printer.println("SD CARD ERROR");
    printer.feed(1);
    while(1);
  }
  Serial.println("SD card initialized successfully!");

  // Print the last 10 entries from the GPS log file
  printLast10GPSEntries();
}

void loop() {
  // Nothing to do
}

void printLast10GPSEntries() {
  const char* filename = "gps_log.txt";
  Serial.print("Opening file: ");
  Serial.println(filename);
  
  // Open the file
  File dataFile = SD.open(filename);
  if (dataFile) {
    // Create a circular buffer to store the last 10 lines
    char lastTenEntries[10][64];
    int entryCount = 0;
    int currentEntry = 0;
    
    // Variables for reading the file line by line
    char line[64];
    int lineIndex = 0;
    
    // Read the file byte by byte
    while (dataFile.available()) {
      char c = dataFile.read();
      
      if (c == '\n' || c == '\r') {
        if (c == '\r' && dataFile.peek() == '\n') {
          dataFile.read(); // Skip the \n part of \r\n
        }
        // End of line - process this line
        if (lineIndex > 0) { // Only process non-empty lines
          line[lineIndex] = '\0'; // Null terminate
          
          // Store in circular buffer
          strcpy(lastTenEntries[currentEntry], line);
          currentEntry = (currentEntry + 1) % 10; // Circular buffer
          if (entryCount < 10) entryCount++;
          
          // Reset for next line
          lineIndex = 0;
        }
      } 
      else if (lineIndex < sizeof(line) - 1) {
        // Add character to current line
        line[lineIndex++] = c;
      }
    }
    
    dataFile.close();
    
    Serial.print("Found ");
    Serial.print(entryCount);
    Serial.println(" GPS entries to print");
    
    // Print entries in chronological order
    int startIndex = (entryCount < 10) ? 0 : currentEntry;
    
    // Print to thermal printer with small font
    printer.setSize('S');  // Small font
    printer.justify('C');  // Center align
    printer.println("GPS DATA LOG");
    printer.println("Last 10 Entries");
    printer.println("------------");
    printer.justify('L');  // Left align
    
    for (int i = 0; i < entryCount; i++) {
      int index = (startIndex + i) % 10;
      // Format and print the GPS data entry
      char* entry = lastTenEntries[index];
      
      // Print to thermal printer with minimal formatting
      printer.print(i+1);
      printer.print(": ");
      printer.println(entry);
      
      // Also echo to serial for debug
      Serial.print(i+1);
      Serial.print(": ");
      Serial.println(entry);
    }
    
    printer.println("------------");
    printer.println("End of log");
    printer.feed(1);  // Feed only 1 line at the end
    
    Serial.println("Printing complete");
  } 
  else {
    Serial.print("Error opening file: ");
    Serial.println(filename);
    
    printer.println("ERROR!");
    printer.println("No GPS log file");
    printer.feed(1);
  }
}
