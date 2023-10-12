
#define GPSTX 17           //pin number for TX output from ESP32 - RX into GPS
#define GPSRX 16           //pin number for RX input into ESP32 - TX from GPS
#define GPSserial Serial2  //define GPSserial as ESP32 Serial2
#define SD_CS_PIN 5
#define LED_RED 25
#define LED_YELLOW 33
#define LED_GREEN 32
#define BUTTON_MENU 26
#define BUTTON_UP 35
#define BUTTON_DOWN 34
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#include <TinyGPSPlus.h>
#include <SPI.h>
#include <SD.h>
#include <Arduino.h>
#include <TinyMPU6050.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int BUTTON_PIN = 27;  // test stopera
unsigned long start_time;
unsigned long elapsed_time, old_time_lap;
bool reset_time_lap = false;
int reset_time_loop = 0;
unsigned long best_time = 0;
char filename[30];
MPU6050 mpu(Wire);
TinyGPSPlus gps;
String logData;  // (id_toru, data, czas, nr_okrazenia, czas_okrazenia, szerokość_geograficzna, Dlugosc_geograficzna, predkosc, kierunek, kat_X, kat_Y, numer okrazenia)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int id_track = 1;
int id_track_select = 0;
String name_track;
float lat_left, lng_left, lat_right, lng_right;           // Zmienne lini mety Varibles finish line position
int year, month, day, hour, minute, second, millisecond;  // Zmienne GPS
float lat, lng, speed, course;                            // zmienne GPS
int mpuX, mpuY;                                           // Zmienne MPU6050
int led_value = 0;
int status_LED = 0;
//int status_YELLOW = 0;
int laps = 0;
File fileTrack;
String tracks[20];
int numTracks = 0;
int selectedOption = 1;  // początkowo zaznaczona opcja
String Menu[3] = { "Select track", "Go race", "TEST" };
unsigned long previousMillis = 0;
const unsigned long loopInterval = 100;  // czas wykonywania pętli loop w milisekundach

// Write the sensor readings on the SD card
void logSave(const char *path, String message) {
  message += "\n";
  Serial.print("Save data: ");
  Serial.println(message);
  appendFile(SD, path, message.c_str());
}
// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing: " + file);
    return;
  }
  if (file.print(message)) {
    Serial.println("File written: " + file);
  } else {
    Serial.println("Write failed: " + file);
  }
  file.close();
}
// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending: " + file);
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended: " + file);
  } else {
    Serial.println("Append failed: " + file);
  }
  file.close();
}

void printMPU() {
  mpuX = mpu.GetAngX();
  mpuY = mpu.GetAngY();

  logData += ",";
  logData += mpuX;
  logData += ",";
  logData += mpuY;
}

void printGPS() {
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }
  year = gps.date.year();
  month = gps.date.month();
  day = gps.date.day();
  hour = gps.time.hour();
  minute = gps.time.minute();
  second = gps.time.second();
  millisecond = gps.time.centisecond();
  lat = gps.location.lat();
  lng = gps.location.lng();
  speed = gps.speed.kmph();
  course = gps.course.deg();

  logData += ",";
  logData += year;
  logData += ".";
  if (month < 10) logData += "0";
  logData += month;
  logData += ".";
  if (day < 10) logData += "0";
  logData += day;
  logData += ",";
  if (hour < 10) logData += "0";
  logData += hour;
  logData += ":";
  if (minute < 10) logData += "0";
  logData += minute;
  logData += ":";
  if (second < 10) logData += "0";
  logData += second;
  logData += ":";
  if (millisecond < 10) logData += "0";
  logData += millisecond;
  logData += ",";
  logData += String(lat, 6);
  logData += ",";
  logData += String(lng, 6);
  logData += ",";
  logData += speed;
  logData += ",";
  logData += course;
}

void printTft() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE, BLACK);
  Serial.print("Time: ");
  Serial.print(hour);  // Hour (0-23) (u8)
  Serial.print(":");
  Serial.print(minute);  // Minute (0-59) (u8)
  Serial.print(":");
  Serial.print(second);  // Second (0-59) (u8)
  Serial.print(":");
  Serial.print(millisecond);
  Serial.print(" | Lat: ");
  Serial.print(lat, 6);
  Serial.print(" | LNG: ");
  Serial.print(lng, 6);
  Serial.print(" | Speed: ");
  Serial.print(speed);
  Serial.print(" | Course: ");
  Serial.print(course);
  Serial.print(" | X = ");
  Serial.print(mpuX);  // Wyświetlanie X
  Serial.print(" | Y = ");
  Serial.println(mpuY);  // Wyświetlanie Y
  Serial.print(" | Wartosc diody = ");
  Serial.println(status_LED);
  Serial.print(" | LAps = ");
  Serial.println(laps);  // Wyświetlanie Y
  display.setTextSize(1);

  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  display.print(hour);  // Hour (0-23) (u8)
  display.print(":");
  display.print(minute);  // Minute (0-59) (u8)
  display.print(":");
  display.print(second);  // Second (0-59) (u8)
  display.print(":");
  display.print(millisecond);
  display.print("   ID: ");
  display.println(id_track);
  display.print("N: ");
  display.println(lat, 6);
  display.print("E: ");
  display.println(lng, 6);
  display.println("Speed:");

  display.setTextSize(2);
  display.println(speed);

  display.setTextSize(1);
  display.print("Satellites: ");
  display.println(gps.satellites.value());
  display.print("HDOP:");
  display.print(gps.hdop.hdop());
  display.print(" Course:");
  display.println(course);
  display.display();
}

void printMENU(File fileTrack) {
  display.setTextSize(1);
  display.println("Select track:");
  display.setTextSize(1);
  fileTrack.seek(0);
  // Odczytaj i wypisz kolejne wiersze pliku
  while (fileTrack.available()) {
    String line = fileTrack.readStringUntil('\n');

    // Wyszukaj pozycje przecinków w wierszu
    int firstComma = line.indexOf(",");
    int secondComma = line.indexOf(",", firstComma + 1);

    // Wypisz tekst między przecinkami
    String text = line.substring(firstComma + 1, secondComma);
    display.println(text);
    display.display();
  }
}
void open_file_track() {
  fileTrack = SD.open("/track.txt", FILE_READ);
  if (!fileTrack) {
    Serial.println("Błąd odczytu pliku!");
    return;
  }

  fileTrack.seek(0);
  while (fileTrack.available()) {
    String line = fileTrack.readStringUntil('\n');
    int firstComma = line.indexOf(",");
    int secondComma = line.indexOf(",", firstComma + 1);
    tracks[numTracks] = line.substring(firstComma + 1, secondComma);  // Zapisz tekst między przecinkami
    numTracks++;
  }
  fileTrack.close();
}
void select_Track() {
  int currentTrack = 0;
  // Wygeneruj menu na podstawie danych z pliku
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.setTextColor(WHITE, BLACK);
    display.println("Select track:");
    for (int i = 0; i < (numTracks); i++) {
      if (i == currentTrack) {
        display.setTextColor(BLACK, WHITE);
        display.setCursor(0, 16 + 10 * i);
        display.print("> ");
      } else {
        display.setTextColor(BLACK, BLACK);
        display.setCursor(0, 16 + 10 * i);
        display.print("  ");
        display.setTextColor(WHITE, BLACK);
        display.setCursor(12, 16 + 10 * i);
      }
      display.print(tracks[i]);
    }
    display.display();

    if (digitalRead(BUTTON_UP) == HIGH) {
      currentTrack--;
      if (currentTrack < 0) {
        currentTrack = numTracks - 1;
      }
    } else if (digitalRead(BUTTON_DOWN) == HIGH) {
      currentTrack++;
      if (currentTrack >= numTracks) {
        currentTrack = 0;
      }
    } else if (digitalRead(BUTTON_MENU) == HIGH) {
      delay(800);
      // Uruchom wybraną ścieżkę
      Serial.println("Uruchamiam ścieżkę: " + tracks[currentTrack]);
      id_track_select = currentTrack;
      break;
    }
    delay(150);
  }
  laps = 0;
  select_read_track();
  selectedOption = 1;
}

void select_read_track() {  //odczytywanie danych z wiersza, który został wbrany w select_Track()
  Serial.println("początek funkcji select_read_track");
  // Wybierz wiersz do odczytu
  File fileTrack1 = SD.open("/track.txt", FILE_READ);
  if (!fileTrack1) {
    Serial.println("Błąd odczytu pliku!");
    return;
  }
  // Przejdź do początku pliku
  fileTrack1.seek(0);

  // Odczytaj wybrany wiersz z pliku
  for (int i = 0; i < id_track_select; i++) {
    fileTrack1.readStringUntil('\n');
  }
  String line1 = fileTrack1.readStringUntil('\n');

  // Rozdziel linię na części oddzielone przecinkami
  int commaPos[5] = { 0, 0, 0, 0, 0 };
  int commaCount = 0;
  for (int i = 0; i < line1.length(); i++) {
    if (line1.charAt(i) == ',') {
      commaPos[commaCount] = i;
      commaCount++;
    }
  }

  // Przypisz wartości z wybranego wiersza do zmiennych
  id_track = line1.substring(0, commaPos[0]).toInt();
  name_track = line1.substring(commaPos[0] + 1, commaPos[1]);
  lat_left = line1.substring(commaPos[1] + 1, commaPos[2]).toFloat();
  lng_left = line1.substring(commaPos[2] + 1, commaPos[3]).toFloat();
  lat_right = line1.substring(commaPos[3] + 1, commaPos[4]).toFloat();
  lng_right = line1.substring(commaPos[4] + 1).toFloat();
  fileTrack1.close();
  get_date_gps();
}
void stopwatch_menu() {
  // obsługa przycisku resetującego stoper
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(100);
    reset_time_lap = true;
  }
  logData += ",";
  logData += laps;

  if (reset_time_loop > 0) { reset_time_loop--; }
  if ((lat >= lat_left && lat <= lat_right && lng >= lng_left && lng <= lng_right && reset_time_loop == 0) || (reset_time_lap == true && reset_time_loop == 0)) {  // zapisując linie mety wartoście mniejsze powinny być zapisane pod left
    if (best_time == 0 || best_time > elapsed_time) best_time = elapsed_time;
    if (old_time_lap > elapsed_time) {
      digitalWrite(LED_GREEN, HIGH);
    } else {
      digitalWrite(LED_YELLOW, HIGH);
    }
    old_time_lap = elapsed_time;
    start_time = millis();
    reset_time_lap = false;  // dla przycisku
    laps++;
    reset_time_loop = 50;
    status_LED = 30;
  }
  Serial.println(reset_time_lap);
  Serial.println(reset_time_loop);

  // obliczenie czasu od uruchomienia stopera
  elapsed_time = millis() - start_time;

  // obliczenie minut, sekund i dziesiętnych milisekund
  int minutes = elapsed_time / 60000;
  int seconds = (elapsed_time % 60000) / 1000;
  int milliseconds = (elapsed_time % 1000) / 100;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE, BLACK);
  display.print("Laps: ");
  display.print(laps);
  display.setCursor(65, 0);
  display.println(name_track);
  if (minutes < 10) {
    display.setTextSize(3);
  } else {
    display.setTextSize(2);
  }
  display.setCursor(5, 16);
  display.print(minutes);
  display.print(":");
  if (seconds < 10) {
    display.print("0");
  }
  display.print(seconds);
  display.print(":");
  display.println(milliseconds);

  display.setTextSize(1);
  display.setCursor(0, 50);
  if (best_time != 0) {

    display.print("BEST LAP: ");
    int best_minutes = best_time / 60000;
    int best_seconds = (best_time % 60000) / 1000;
    int best_milliseconds = (best_time % 1000) / 100;
    display.print(best_minutes);
    display.print(":");
    if (best_seconds < 10) {
      display.print("0");
    }
    display.print(best_seconds);
    display.print(":");
    display.println(best_milliseconds);

  } else {
    display.print("BEST LAP: else");
  }

  display.display();
}

void get_date_gps() {
  int year = 0;
  int month = 0;
  int day = 0;
  while (true) {
    while (GPSserial.available()) {
      if (gps.encode(GPSserial.read())) {
        year = gps.date.year();
        month = gps.date.month();
        day = gps.date.day();
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.println("Brak syganału GPS");
        display.display();
        digitalWrite(LED_RED, HIGH);
        Serial.println(year);
        Serial.println(month);
        Serial.println(day);
      }
    }
    if (gps.location.isValid() && gps.time.isValid()) {
      sprintf(filename, "/%s_%04d-%02d-%02d.txt", name_track.c_str(), year, month, day);
      Serial.print("Utworzona plik: ");
      Serial.println(filename);
      digitalWrite(LED_RED, LOW);
      break;
    }
  }
  File fileDATA = SD.open(filename);
  if (!fileDATA) {
    Serial.print("File doens't exist");
    Serial.println(filename);
    Serial.println("Creating file...");
    writeFile(SD, filename, "");
  } else {
    Serial.println("File already exists");
  }
  fileDATA.close();
}

void setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  delay(1000);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 20);
  display.println("Lap Timer");
  display.setTextSize(1);
  display.setCursor(45, 40);
  display.println("v 1.0");
  display.display();

  mpu.Initialize();  // Inicjalizacja MPU6050
  Serial.begin(115200);
  mpu.Calibrate();  // Kalibracja modułu
  Serial.println("GPS_Echo_ESP32 Starting");
  GPSserial.begin(9600, SERIAL_8N1, GPSTX, GPSRX);  //format is baud, mode, UART RX data, UART TX dat
  delay(100);

  // ustawienie GNSS na szykość 10HZ
  GPSserial.println("$PMTK300,100,0,0,0,0*2C\r\n");
  GPSserial.println("$PMTK220,100*2F\r\n");

  // ustawienie GNSS na szykość 5HZ
  // GPSserial.println("$PMTK300,200,0,0,0,02F\r\n");
  // GPSserial.println("$PMTK220,2002C\r\n");
  delay(1000);
  start_time = millis();
  pinMode(BUTTON_PIN, INPUT);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(BUTTON_MENU, INPUT);

  SD.begin(SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;  // init failed
  }

  File fileTRACK = SD.open("/track.txt");
  if (!fileTRACK) {
    Serial.println("File doens't exist /track.txt");
    Serial.println("Creating file...");
    writeFile(SD, "/track.txt", "ESP32 and SD Card \r\n");
  } else {
    Serial.println("File already exists /track.txt");
  }
  fileTRACK.close();
  open_file_track();
  select_Track();
  delay(100);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= loopInterval) {  // sprawdzamy, czy czas wykonywania pętli loop minął
    previousMillis = currentMillis;
    mpu.Execute();  // Aktualizacja danych z modułu
    logData = id_track;
    printGPS();
    printMPU();

    // zaswiecenie diody przy przejezdzie przez linie
    if (status_LED >= 1) {
      status_LED--;
      if (status_LED == 0) {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_YELLOW, LOW);
      }
    }

    if (digitalRead(BUTTON_MENU) == HIGH) {
      delay(150);
      while (true) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.setTextColor(WHITE, BLACK);
        display.println("Select option:");
        for (int i = 0; i < 3; i++) {
          if (i == selectedOption) {
            display.setTextColor(BLACK, WHITE);
            display.setCursor(0, 16 + 10 * i);
            display.print("> ");
          } else {
            display.setTextColor(BLACK, BLACK);
            display.setCursor(0, 16 + 10 * i);
            display.print("  ");
            display.setTextColor(WHITE, BLACK);
            display.setCursor(12, 16 + 10 * i);
          }
          display.print(Menu[i]);
        }
        display.display();

        // przetwarzamy zmiany stanów przycisków up i down
        if (digitalRead(BUTTON_UP) == HIGH) {
          selectedOption--;
          if (selectedOption < 0) {
            selectedOption = 2;
          }
        } else if (digitalRead(BUTTON_DOWN) == HIGH) {
          selectedOption++;
          if (selectedOption > 2) {
            selectedOption = 0;
          }
        } else if (digitalRead(BUTTON_MENU) == HIGH) {
          delay(150);
          break;
        }
        delay(150);
      }
    }
    if (selectedOption == 0) {
      select_Track();
    } else if (selectedOption == 1) {
      stopwatch_menu();
    } else {
      printTft();
    }

    logSave(filename, logData);  // zapisanie danych

    unsigned long elapsedTime = millis() - currentMillis;  // czas, który upłynął od początku iteracji pętli
    if (elapsedTime < loopInterval) {
      delay(loopInterval - elapsedTime);  // opóźnienie, aby zapewnić równomierny czas wykonywania pętli loop
    }
  }
}
