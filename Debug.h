#define DEBUG

#ifdef DEBUG
    #define PRINTS(s)   { Serial.print(F(s)); }
    #define PRINTSLN(s)   { Serial.println(F(s)); }
    #define PRINTV(s)   { Serial.print(s); }
    #define PRINTVLN(s)   { Serial.println(s); }
    #define PRINT(s,v)  { Serial.print(F(s)); Serial.print(v); }
    #define PRINTLN(s,v)  { Serial.print(F(s)); Serial.println(v); }
    #define PRINTX(s,v) { Serial.print(F(s)); Serial.print(F("0x")); Serial.print(v, HEX); }
    #define PRINTXLN(s,v) { Serial.print(F(s)); Serial.print(F("0x")); Serial.println(v, HEX); }
#else
    #define PRINTS(s)
    #define PRINTSLN(s)
    #define PRINTV(s)
    #define PRINTVLN(s)
    #define PRINT(s,v)
    #define PRINTLN(s,v)
    #define PRINTX(s,v)
    #define PRINTXLN(s,v)
#endif