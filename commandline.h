//Connor Glasgow

#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_CHARS 80
#define MAX_FIELDS 8

//-----------------------------------------------------------------------------
// Structs
//-----------------------------------------------------------------------------
typedef struct _USER_DATA
{
    char buffer[MAX_CHARS + 1];
    char fieldType[MAX_FIELDS];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
} USER_DATA;

void getsUart0(USER_DATA* userdata);
void parseFields(USER_DATA* userdata);
bool isCommand(USER_DATA* userdata, const char strCommand[], uint8_t minArgs);
int32_t getFieldInteger(USER_DATA* userdata, uint8_t fieldNumber);
char* getFieldString(USER_DATA* userdata, uint8_t fieldNumber);

#endif /* COMMANDLINE_H_ */
