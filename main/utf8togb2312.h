#ifndef UTF8TOGB2312_H
#define UTF8TOGB2312_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// === 关键部分开始 ===
#ifdef __cplusplus
extern "C" {
#endif
// ===================

int  UTF_8ToGB2312(char*pOut, char *pInput, int pLen);
int UTF_8ToUnicode(char* pOutput, char *pInput);
void UnicodeToGB2312(char*pOut, char *pInput);

// === 关键部分结束 ===
#ifdef __cplusplus
}
#endif
// ===================

#endif

