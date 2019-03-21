#ifndef _LCD12864_H_
#define _LCD12864_H_

void LcdInit(void);
void ClrScreen(void);
void PutString(unsigned char x,unsigned char y,unsigned char *p);
void FontSet(unsigned char Font_NUM,unsigned char Color);
void FontSet_cn(unsigned char Font_NUM,unsigned char Color);
void PutString_cn(unsigned char x,unsigned char y,unsigned char *p);
void ShowFloat(unsigned char x, unsigned char y, float num, unsigned char DecNum);
void PutChar(unsigned char x,unsigned char y,unsigned char a);
void ShowShort(unsigned char x,unsigned char y,unsigned short a,unsigned char type);

#endif
