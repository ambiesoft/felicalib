﻿/*
  felicalib - FeliCa access wrapper library

  Copyright (c) 2007, Takuya Murakami, All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer. 

  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution. 

  3. Neither the name of the project nor the names of its contributors
  may be used to endorse or promote products derived from this software
  without specific prior written permission. 

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**
   @file nanaco.c

   nanaco ダンプ
*/

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <locale.h>

#include "felicalib.h"

static void nanaco_dump(uint8 *data);
static int read4b(uint8 *p);
static int read2b(uint8 *p);

int _tmain(int argc, _TCHAR *argv[])
{
    pasori *p;
    felica *f;
    int i;
    uint8 data[16];

        setlocale( LC_ALL, "Japanese");

    p = pasori_open(NULL);
    if (!p) {
        _ftprintf(stderr, _T("PaSoRi open failed.\n"));
        exit(1);
    }
    pasori_init(p);
    
    f = felica_polling(p, 0xfe00, 0, 0);
    if (!f) {
        _ftprintf(stderr, _T("Polling card failed.\n"));
        exit(1);
    }

    if (felica_read_without_encryption02(f, 0x558b, 0, 0, data)) {
        _ftprintf(stderr, _T("Can't read nanaco ID.\n"));
        exit(1);
    }

    _tprintf(_T("nanaco id: "));
    for (i = 0; i < 8; i++) {
        _tprintf(_T("%02x"), data[i]);
    }
    _tprintf(_T("\n"));

    for (i = 0; ; i++) {
        if (felica_read_without_encryption02(f, 0x564f, 0, (uint8)i, data)) {
            break;
        }
        nanaco_dump(data);
    }
    felica_free(f);
    pasori_close(p);

    return 0;
}

static void nanaco_dump(uint8 *data)
{
    int value, yy, mm, dd, hh, min;

    switch (data[0]) {
    case 0x47:
        _tprintf(_T("支払     "));
        break;
    case 0x6f:
        _tprintf(_T("チャージ "));
        break;
    default:
        _tprintf(_T("不明     "));
        break;
    }

    value = read4b(data + 1);
    _tprintf(_T("金額:%-6d円 "), value);

    value = read4b(data + 5);
    _tprintf(_T("残高:%-6d円 "), value);

    value = read4b(data + 9);
    yy = value >> 21;
    mm = (value >> 17) & 0xf;
    dd = (value >> 12) & 0x1f;
    hh = (value >> 6) & 0x3f;
    min = value & 0x3f;
    _tprintf(_T("%02d/%02d/%02d %02d:%02d "), yy, mm, dd, hh, min);

    value = read2b(data + 13);
    _tprintf(_T("連番: %d\n"), value);
}

static int read4b(uint8 *p)
{
    int v;
    v = (*p++) << 24;
    v |= (*p++) << 16;
    v |= (*p++) << 8;
    v |= *p;
    return v;
}

static int read2b(uint8 *p)
{
    int v;
    v = (*p++) << 8;
    v |= *p;
    return v;
}
