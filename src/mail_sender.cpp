/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2016 Javier Carnero

 This file is part of DunaWatchdog.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of DunaWatchdog and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/
#include "mail_sender.h"

#include <iostream>
#include <vector>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <sstream>

using namespace std;

MailSender::MailSender(const char * _muttrc_path) {
  int i;
  for (i = 0; i < 1024 && _muttrc_path[i] != '\0'; ++i)
    muttrc_path[i] = _muttrc_path[i];
  if (i < 1024)
    muttrc_path[i] = '\0';
  else
    cout << "ERROR, la ruta de muttrc es demasiado larga (1024 caracteres max)."
         << endl;
}

MailSender::~MailSender() {

}

void MailSender::send(const char * recipient, const char * subject,
                      const char * message, unsigned int msg_size) {
  char mutt[2048 + msg_size];
  mutt[0] = '\0';
  strcat(mutt, "echo \"");
  strcat(mutt, message);
  strcat(mutt, "\" | mutt -n -F ");
  strcat(mutt, muttrc_path);
  strcat(mutt, " -s \"");
  strcat(mutt, subject);
  strcat(mutt, "\" ");
  strcat(mutt, recipient);
  system(mutt);
}
