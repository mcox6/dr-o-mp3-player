
#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_

#define MATCH(a,b)		(strcmp(a,b) == 0)

void getUartLine(char* uartInput);
void uartUI(void *pvParameters);
void getMP3Names(void );
void retGlobals(char c, char *toReturn);

#endif /* USERINTERFACE_H_ */
