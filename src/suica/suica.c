/*
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
  @file suica.c

  suica �����_���v (���o�ꗚ���͖��Ή�)
*/

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <locale.h>
#include "felicalib.h"

static void suica_dump_history(uint8 *data);
static LPCTSTR consoleType(int ctype);
static LPCTSTR procType(int proc);
static int read4b(uint8 *p);
static int read2b(uint8 *p);

// �T�[�r�X�R�[�h
#define SERVICE_SUICA_INOUT     0x108f
#define SERVICE_SUICA_HISTORY   0x090f


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
    
    f = felica_polling(p, POLLING_SUICA, 0, 0);
    if (!f) {
        _ftprintf(stderr, _T("Polling card failed.\n"));
        exit(1);
    }

    _tprintf(_T("IDm: "));
    for (i = 0; i < 8; i++) {
        _tprintf(_T("%02x"), f->IDm[i]);
    }
    _tprintf(_T("\n"));

    for (i = 0; ; i++) {
        if (felica_read_without_encryption02(f, SERVICE_SUICA_HISTORY, 0, (uint8)i, data)) {
            break;
        }
        suica_dump_history(data);
    }

    felica_free(f);
    pasori_close(p);

    return 0;
}

static void suica_dump_history(uint8 *data)
{
    int ctype, proc, date, time, balance, seq, region;
    int in_line, in_sta, out_line, out_sta;
    int yy, mm, dd;

    ctype = data[0];            // �[����
    proc = data[1];             // ����
    date = read2b(data + 4);    // ���t
    balance = read2b(data + 10);// �c��
    balance = N2HS(balance);
    seq = read4b(data + 12);
    region = seq & 0xff;        // Region
    seq >>= 8;                  // �A��

    out_line = -1;
    out_sta = -1;
    time = -1;

    switch (ctype) {
    case 0xC7:  // ����
    case 0xC8:  // ���̋@          
        time = read2b(data + 6);
        in_line = data[8];
        in_sta = data[9];
        break;

    case 0x05:  // �ԍڋ@
        in_line = read2b(data + 6);
        in_sta = read2b(data + 8);
        break;

    default:
        in_line = data[6];
        in_sta = data[7];
        out_line = data[8];
        out_sta = data[9];
        break;
    }

    _tprintf(_T("�[����:%s "), (LPCTSTR)consoleType(ctype));
    _tprintf(_T("����:%s "), (LPCTSTR)procType(proc));

    // ���t
    yy = date >> 9;
    mm = (date >> 5) & 0xf;
    dd = date & 0x1f;
    _tprintf(_T("%02d/%02d/%02d "), yy, mm, dd);

    // ����
    if (time > 0) {
        int hh = time >> 11;
        int min = (time >> 5) & 0x3f;

        _tprintf(_T(" %02d:%02d "), hh, min);
    }
    
    _tprintf(_T("��:%x/%x "), in_line, in_sta);
    if (out_line != -1) {
        _tprintf(_T("�o:%x/%x "), out_line, out_sta);
    }

    _tprintf(_T("�c��:%d "), balance);
    _tprintf(_T("�A��:%d\n"), seq);
}

static LPCTSTR consoleType(int ctype)
{
    switch (ctype) {
    case 0x03: return (LPCTSTR)_T("���Z�@");
    case 0x05: return (LPCTSTR)_T("�ԍڒ[��");
    case 0x08: return (LPCTSTR)_T("�����@");
    case 0x12: return (LPCTSTR)_T("�����@");
    case 0x16: return (LPCTSTR)_T("���D�@");
    case 0x17: return (LPCTSTR)_T("�ȈՉ��D�@");
    case 0x18: return (LPCTSTR)_T("�����[��");
    case 0x1a: return (LPCTSTR)_T("���D�[��");
    case 0x1b: return (LPCTSTR)_T("�g�ѓd�b");
    case 0x1c: return (LPCTSTR)_T("��p���Z�@");
    case 0x1d: return (LPCTSTR)_T("�A�����D�@");
    case 0xc7: return (LPCTSTR)_T("����");
    case 0xc8: return (LPCTSTR)_T("���̋@");
    }
    return (LPCTSTR)_T("???");
}

static LPCTSTR procType(int proc)
{
    switch (proc) {
    case 0x01: return (LPCTSTR)_T("�^���x��");
    case 0x02: return (LPCTSTR)_T("�`���[�W");
    case 0x03: return (LPCTSTR)_T("���w");
    case 0x04: return (LPCTSTR)_T("���Z");
    case 0x07: return (LPCTSTR)_T("�V�K");
    case 0x0d: return (LPCTSTR)_T("�o�X");
    case 0x0f: return (LPCTSTR)_T("�o�X");
    case 0x14: return (LPCTSTR)_T("�I�[�g�`���[�W");
    case 0x46: return (LPCTSTR)_T("����");
    case 0x49: return (LPCTSTR)_T("����");
    case 0xc6: return (LPCTSTR)_T("����(�������p)");
    }
    return (LPCTSTR)_T("???");
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


