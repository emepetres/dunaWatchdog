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

#ifndef MAIL_SENDER_H_
#define MAIL_SENDER_H_

class MailSender {
 public:
  MailSender(const char * muttrc_path = "./muttrc");
  virtual ~MailSender();

  void send(const char * recipient, const char * subject, const char * message,
            unsigned int msg_size);
  //tambien se pueden mantar adjuntos y en html (ver opciones de mutt)
 private:
  char muttrc_path[1024];
};

#endif /* MAIL_SENDER_H_ */
