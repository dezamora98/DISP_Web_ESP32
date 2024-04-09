#define DEBUG

#ifdef DEBUG
#define debugPrintln(msg) Serial.println(msg)
#else
#define debugPrintln(msg)
#endif

#ifdef DEBUG
#define debugPrint(msg) Serial.print(msg)
#else
#define debugPrint(msg)
#endif