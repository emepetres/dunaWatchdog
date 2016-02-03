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

#include <vector>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <time.h>
#include <iostream>
#include <fstream>

#include "tinyxml2.h"
#include "mail_sender.h"

using namespace std;
using namespace tinyxml2;

#define DTTMFMT "%Y-%m-%d %H:%M:%S "
#define DTTMSZ 21
static char *getDtTm (char *buff) {
    time_t t = time (0);
    strftime (buff, DTTMSZ, DTTMFMT, localtime (&t));
    return buff;
}

char buff[DTTMSZ];
#define toLog(msg) { cout << getDtTm (buff) <<  msg << endl; log << getDtTm (buff) <<  msg << endl; }

bool verbose;
bool no_exe;
bool no_recovery;
bool no_mail;
fstream log;

struct _app_t
{
	const char * name;
	const char * exe;
	char * shell_exe;
	const char * working_directory;
	string pid;
	float max_mem;
	bool screen_log;
	vector<const char *> alarm_mails;
	int fail_tries;
	int success_tries;
};
typedef struct _app_t app_t;

string exec(char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}

const char* toString(int n)
{
	stringstream ss;
	ss << n;
	return ss.str().c_str();
}

int toInt(const char * s)
{
	stringstream strValue;
	strValue << s;

	int intValue;
	strValue >> intValue;

	return intValue;
}

float toFloat(const char * s)
{
	stringstream strValue;
	strValue << s;

	float floatValue;
	strValue >> floatValue;

	return floatValue;
}

const char * formatInteger(const char * s)
{
	return toString(toInt(s));
}

void formatPath(string & str)
{
	for (int i = 1; str[str.length()-i] == '\n' || str[str.length()-i] == '\r'; ++i)
		str[str.length()-i] = '\0';
}

bool executeApp(app_t& app)
{
	if (verbose)
		toLog("Ejecutando app " << app.name << "...")

	char kill_screens[1024] = "for session in $(screen -ls | grep -o '[0-9]*\\.";
	strcat(kill_screens, app.name);
	strcat(kill_screens, "'); do screen -S \"${session}\" -X quit; done");
	system(kill_screens);

	chdir(app.working_directory);
	if (verbose)
		toLog("--> Directorio de trabajo: " << app.working_directory);

	char screen[1024] = "screen -dmS";
	if (app.screen_log)
		strcat(screen, "L ");
	else strcat(screen, " ");
	strcat(screen, app.name);
	strcat(screen, " ");
	strcat(screen, app.exe);
	if (verbose)
		toLog("--> Comando screen: " << screen)
	system(screen);

	char screen_ls[1024] = "screen -ls | grep ";
	strcat(screen_ls, app.name);
	strcat(screen_ls, " | awk -F\".\" '{print $1}' | awk -F\"\\t\" '{print $2}'");
	if (verbose)
		toLog("--> Comando screen pid: " << screen_ls)
	string screen_pid_str = exec(screen_ls);
	formatPath(screen_pid_str);
	if (verbose)
		toLog("--> Screen pid: " << screen_pid_str)
	if (screen_pid_str.length() == 0)
	{
		toLog("ERROR: no se pudo obtener el pid del screen al intentar ejecutar la aplicación " << app.name);
		return false;
	}

	char ps[1024] = "ps --ppid ";
	strcat(ps, screen_pid_str.c_str());
	strcat(ps, " -o pid=");
	if (verbose)
		toLog("--> Comando app pid: " << ps)

	string app_pid_str = exec(ps);
	formatPath(app_pid_str);
	app.pid.assign(app_pid_str);
	if (app.pid[0] == '\0')
	{
		usleep(500000);
		app_pid_str = exec(ps);
		formatPath(app_pid_str);
		app.pid.assign(app_pid_str);
		if (app.pid.length() == 0)
		{
			toLog("ERROR: no se pudo obtener el pid de la aplicación al intentar ejecutarla (" << app.name << ").")
			return false;
		}
	}
	if (verbose)
	{
		toLog("--> App pid: " << app.pid)
		toLog("...app ejecutada.")
	}

	return true;
}

void subsec(const char AString[], char Substring[], int start, int length) {
	int b;
    for (b = start; b <= length; ++b) {
        Substring[b] = AString[b];
    }
    Substring[b] = '\0';
}

bool isAppRunning(app_t app)
{
	if (verbose)
		toLog("Comprobando ejecución app " << app.name << "...")
	char ps[1024] = "ps -p ";
	strcat(ps, app.pid.c_str());
	strcat(ps, " -o command=");
	if (verbose)
		toLog("--> Obtener el comando asociado al pid: " << ps)
	string cmd_str = exec(ps);
	formatPath(cmd_str);
	if (verbose)
	{
		toLog("\t--> app.exe-> " << app.exe << " ==? " << cmd_str << " <-comando asociado o\n\t\t\t--> app.shell_exe-> " << app.shell_exe << " ==? " << cmd_str << " <-comando asociado")
		string res;

		if (strcmp(app.exe, cmd_str.c_str())==0 || strcmp(app.shell_exe, cmd_str.c_str())==0)
			res = "EN EJECUCIÓN";
		else res = "PARADA";
		toLog("...resultado de la comprobación: " << res)
	}

	return strcmp(app.exe, cmd_str.c_str())==0 || strcmp(app.shell_exe, cmd_str.c_str())==0;
}

bool isMemLow(app_t app)
{
	if (app.max_mem == 0) return true;

	if (verbose)
		toLog("Comprobando memoria app " << app.name << "...")
	char ps[1024] = "ps -p ";
	strcat(ps, app.pid.c_str());
	strcat(ps, " -o pmem=");
	if (verbose)
		toLog("--> Obtener la memoria asociada al pid: " << ps)
	string mem_str = exec(ps);
	formatPath(mem_str);
	float mem = toFloat(mem_str.c_str());
	if (verbose)
	{
		toLog("--> mem-> " << mem << " <? " << app.max_mem << " <-max mem")
		string res;
		if (mem<app.max_mem)
			res = "POR DEBAJO DEL LIMITE";
		else res = "POR ENCIMA DEL LIMITE";
		toLog("...resultado de la comprobación: " << res)
	}

	return mem < app.max_mem;
}

int main(int argc, char *argv[])
{
	verbose = false;
	no_exe = false;
	no_recovery = false;
	no_mail = false;

	//leemos parametros de entrada
	for (int i=1; i<argc; ++i)
	{
		if (strcmp(argv[i],"--verbose")==0)
		{
			verbose = true;
		}
		else if (strcmp(argv[i],"--no_exe")==0)
		{
			no_exe = true;
			no_recovery = true;
		}
		else if (strcmp(argv[i],"--no_recovery")==0)
		{
			no_recovery = true;
		}
		else if (strcmp(argv[i],"--no_mail")==0)
		{
			no_mail = true;
		}
		else
		{
			toLog("ERROR: Argumento " << argv[i] << " no válido.")
			toLog("USO: ./dunaWatchdog [--verbose] [--no_exe] [--no_recovery] [--no_mail]")
			return 1;
		}
	}

	//cambiamos el directorio de trabajo al nuestro
	char buffer[1024];
	int len = readlink("/proc/self/exe", buffer, 1024);
	if (len  != -1)
	{
		buffer[len] = '\0';
	}
	else
	{
		cout <<  "ERROR leyendo el directorio de trabajo del ejecutable dunaWatchdog." << endl;
		return -1;
	}
	string full_path_str(buffer);
	unsigned found = full_path_str.find_last_of("/\\");
	string full_path = full_path_str.substr(0,found);
  if (verbose)
    toLog("Directorio de trabajo: " << full_path.c_str())
  chdir(full_path.c_str());

	//backup del archivo de log anterior
	system("mv dunaWatchdog.log dunaWatchdog.log.backup");

	//creamos el nuevo archivo de log
	log.open("dunaWatchdog.log", fstream::out|fstream::app);

	char muttrc_path[1024]; muttrc_path[0] = '\0';
	strcat(muttrc_path, full_path.c_str());
	strcat(muttrc_path, "/etc/muttrc");
	if (verbose)
		toLog("Archivo muttrc: " << muttrc_path)
	MailSender sender(muttrc_path);

	char xml_path[1024]; xml_path[0] = '\0';
	strcat(xml_path, full_path.c_str());
	strcat(xml_path, "/etc/duna.xml");
	if (verbose)
		toLog("Archivo xml: " << xml_path)



	toLog("Iniciando DunaWatchdog...");
	///////////////////////////
	//leemos la configuración
	///////////////////////////
	XMLDocument doc;
	doc.LoadFile( xml_path );
	if (!doc.RootElement())
	{
		char error_str[2048]; error_str[0] = '\0';
		strcat(error_str, "ERROR: No se ha encontrado el archivo de configuración ");
		strcat(error_str, xml_path);
		toLog(error_str)
		return 1;
	}

	toLog("Leyendo configuracion...")
	vector<app_t> apps;

	int check_timeout_uS = toInt(doc.FirstChildElement("check_timeout_s")->GetText())*1000000;
	int retries_to_warn = toInt(doc.FirstChildElement("retries_to_warn")->GetText());
	int warn_reminder = toInt(doc.FirstChildElement("warn_reminder")->GetText());
	int recover_hysteresis = toInt(doc.FirstChildElement("recover_hysteresis")->GetText());

	XMLElement * app_element = doc.FirstChildElement("app");
	while (app_element)
	{
		app_t app;
		app.name = app_element->FirstChildElement("name")->GetText();
		toLog(app.name)

		app.exe = app_element->FirstChildElement("exe")->GetText();
		app.shell_exe = new char[1024];
		strcpy(app.shell_exe, "/bin/sh ");
		strcat(app.shell_exe, app.exe);
		toLog(app.exe)

		app.working_directory = app_element->FirstChildElement("working_directory")->GetText();
		toLog(app.working_directory)

		if (app_element->FirstChildElement("max_mem_percentage"))
		{
			app.max_mem = toFloat(app_element->FirstChildElement("max_mem_percentage")->GetText());
			if (app.max_mem <= 0 || app.max_mem > 100)
			{
				toLog("ERROR: Porcentage máximo de memoria incorrecto.")
				return 1;
			}
		}
		else app.max_mem = 0;

		app.screen_log = (bool)toInt(app_element->FirstChildElement("screen_log")->GetText());
		if (app.screen_log)
		{
			toLog("Log de screen activado en directorio de trabajo.");
		}
		else
			toLog("Log de screen no activado.");

		if (app_element->FirstChildElement("alarm"))
		{
			XMLElement * mail_element = app_element->FirstChildElement("alarm")->FirstChildElement("mail");
			while (mail_element)
			{
				app.alarm_mails.push_back(mail_element->GetText());
				toLog(mail_element->GetText())
				mail_element = mail_element->NextSiblingElement("mail");
			}
		}

		apps.push_back(app);
		app_element = app_element->NextSiblingElement("app");

		toLog("---------------------");
	}

	if (apps.size()==0)
	{
		toLog("No se pudo leer ninguna aplicación en la configuración.")
		return 0;
	}
	else
	{
		toLog("...OK.")
	}

	toLog("DunaWatchdog Iniciado.")

	/////////////////////////////
	//ejecutamos cada aplicacion en su screen correspondiente
	/////////////////////////////
	for (uint i=0; i<apps.size(); ++i)
	{
		if (!no_exe) executeApp(apps[i]);
		apps[i].fail_tries = 0;
		apps[i].success_tries = recover_hysteresis;
	}

    /////////////////////////////
    //monitorizamos
    /////////////////////////////
	while (true)
	{
		usleep(check_timeout_uS);

		bool failed = false;
		bool send_message = false;
		char msg[1024], subject[256];

		for (uint i=0; i<apps.size(); ++i)
		{
			if (!isAppRunning(apps[i]))
			{
				if (!no_recovery) executeApp(apps[i]);

				subject[0] = '\0';
				strcat(subject, apps[i].name);
				strcat(subject, ": FALLO INESPERADO.");

				msg[0] = '\0';
				strcat(msg, "La aplicación ");
				strcat(msg, apps[i].name);
				strcat(msg, " falló inesperadamente, se reinicia.");

				failed = true;
			}
			else if (!isMemLow(apps[i]))
			{
				if (!no_recovery) executeApp(apps[i]);

				subject[0] = '\0';
				strcat(subject, apps[i].name);
				strcat(subject, ": MEMORIA LLENA.");

				msg[0] = '\0';
				strcat(msg, "La aplicación ");
				strcat(msg, apps[i].name);
				strcat(msg, " ocupó el límite máximo de memoria permitido, se reinicia.");

				failed = true;
			}
			else
			{
				if (apps[i].success_tries<recover_hysteresis)
				{
					++apps[i].success_tries;
				}
				else if (apps[i].success_tries==recover_hysteresis)
				{
					if (apps[i].fail_tries > retries_to_warn)
					{
						send_message = true;
						subject[0] = '\0';
						strcat(subject, apps[i].name);
						strcat(subject, ": OK.");

						msg[0] = '\0';
						strcat(msg, "La aplicación ");
						strcat(msg, apps[i].name);
						strcat(msg, " consiguió reiniciarse.");
					}

					++apps[i].success_tries;
					apps[i].fail_tries = 0;
				}
			}

			if (failed)
			{
				if (apps[i].fail_tries==0)
				{
					send_message = true;
				}
				else
				{
					if (apps[i].success_tries>0 && apps[i].success_tries<=recover_hysteresis)
						apps[i].fail_tries += apps[i].success_tries;

					if (apps[i].fail_tries==retries_to_warn)
					{
						send_message = true;

						subject[0] = '\0';
						strcat(subject, apps[i].name);
						strcat(subject, ": ERROR GRAVE.");

						msg[0] = '\0';
						strcat(msg, "La aplicación ");
						strcat(msg, apps[i].name);
						strcat(msg, " no es capaz de reiniciarse, se requiere acción INMEDIATA.");
					}
					else if (apps[i].fail_tries%warn_reminder==0)
					{
						send_message = true;

						subject[0] = '\0';
						strcat(subject, apps[i].name);
						strcat(subject, ": RECORDATORIO DE ERROR GRAVE.");

						msg[0] = '\0';
						strcat(msg, "La aplicación ");
						strcat(msg, apps[i].name);
						strcat(msg, " no es capaz de reiniciarse, se requiere acción INMEDIATA.");
					}
				}

				++apps[i].fail_tries;
				apps[i].success_tries = 0;
			}

			if (send_message)
			{
				toLog(subject)

				if (!no_mail)
				{
					for (unsigned int j = 0; j < apps[i].alarm_mails.size(); ++j)
					{
						const char * mail = apps[i].alarm_mails[j];
						sender.send(mail, subject, msg, 1024);
					}
				}
			}
		}
	}

	log.close();
	return 0;
}
