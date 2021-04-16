//Connor Glasgow

#include <commandline.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart0.h"

static uint8_t count = 0;
bool enterPressed = false;

void getsUart0(USER_DATA* userdata)
{
    {
        char c = getcUart0();
        if (c == 8 || c == 127)
        {
            if (count > 0)
                count--;
        }
        else if (c == 10 || c == 13)
        {
            userdata->buffer[count] = 0;
            count = 0;
            enterPressed = true;
            return;
        }
        else if (c >= 32)
        {
            userdata->buffer[count++] = c;
            if (count == MAX_CHARS)
            {
                userdata->buffer[count] = '\0';
                count = 0;
                enterPressed = true;
                return;
            }
        }
    }
}

void parseFields(USER_DATA* userdata)
{
    userdata->fieldCount = 0;
    bool newTok = false;
    uint8_t i = 0;
    for (i = 0; (userdata->buffer[i] != '\0') && (userdata->fieldCount < MAX_FIELDS);
            i++)
    {
        if (((userdata->buffer[i] >= 'A' && userdata->buffer[i] <= 'Z')
                || (userdata->buffer[i] >= 'a' && userdata->buffer[i] <= 'z')
                || (userdata->buffer[i] >= '0' && userdata->buffer[i] <= '9'))
                && !newTok)
        {
            newTok = true;
            userdata->fieldPosition[userdata->fieldCount] = i;
            if (userdata->buffer[i] >= '0' && userdata->buffer[i] <= '9')
                userdata->fieldType[userdata->fieldCount++] = 'n';
            else
                userdata->fieldType[userdata->fieldCount++] = 'a';
        }
        else if (!(userdata->buffer[i] >= 'A' && userdata->buffer[i] <= 'Z')
                && !(userdata->buffer[i] >= 'a' && userdata->buffer[i] <= 'z')
                && !(userdata->buffer[i] >= '0' && userdata->buffer[i] <= '9'))
        {
            newTok = false;
            userdata->buffer[i] = '\0';
        }
    }
    return;
}

bool isCommand(USER_DATA* userdata, const char strCommand[],
               uint8_t MinArgs)
{
    if (userdata->fieldCount - 1 < MinArgs)
        return false;
    if (stringcomp(userdata->buffer, strCommand))
        return true;
    return false;
}

int32_t getFieldInteger(USER_DATA* userdata, uint8_t fieldNumber)
{
    int32_t temp = 0;
    if ((fieldNumber < MAX_FIELDS)
            && (fieldNumber < userdata->fieldCount)
            && (userdata->fieldType[fieldNumber] == 'n'))
    {
        uint8_t pos = userdata->fieldPosition[fieldNumber];
        while (userdata->buffer[pos] != '\0')
            temp = (temp * 10) + (userdata->buffer[pos++] - '0');
    }
    return temp;
}

char* getFieldString(USER_DATA* userdata, uint8_t fieldNumber)
{
    if ((fieldNumber < MAX_FIELDS) && (fieldNumber < userdata->fieldCount)
            && (userdata->fieldType[fieldNumber] == 'a'))
        return userdata->buffer + userdata->fieldPosition[fieldNumber];
    return 0;
}

bool stringcomp(const char str1[], const char str2[])
{
    uint8_t i = 0;
    while((str1[i] != '\0') && (str2[i] != '\0') && i<MAX_CHARS)
    {
        if(str1[i] != str2[i])
        {
            return false;
        }
        i++;
    }
    bool result = !str1[i] && !str2[i];
    return result;
}

