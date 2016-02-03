Duna Watchdog
=====

Brief description
-----------
It opens the apps configured in *etc/duna.xml*, each one inside a new *screen* session. Then it checks if these apps are alive, warning by mail when one of them crash/end/close. When that happens, **DunaWatchdog** try to restart the app.

If one app cannot start/restart, **DunaWatchdog** also warns about it by *email*. Additionally you can tell **DunaWatchdog** to restart an app if it have reached a certain memory percentage.

*DunaWatchdog* logs are generated automatically and *screen* logs can be enabled for each app inside *etc/duna.xml*.

Install
-----------
* Install software:
mutt msmtp screen

* git clone https://github.com/emepetres/dunaWatchdog.git
* ./install.sh
* move/rename the portable folder wherever you want. It is all you need.
* Edit *portable/etc/msmtp.conf* with your gmail user and password. Any mail configuration can be used configuring msmtp.conf properly (see man msmtp)
* Edit duna.xml adding the apps you want to watch. Also you can change the global time parameters at top for your own needs.
* Edit *portable/etc/muttrc* writing the full path of msmtp.conf on the same folder. Must be done every time the portable folder is moved/renamed.

How to use it
-----------
```
$ portable/dunaWatchdog [--verbose] [--no_exe] [--no_recovery] [--no_mail]
```
* Can be executed fron anywhere
* Options are useful for debug purpouses


How to colaborate
-----------
* *sample.project* and *sample.cproject* can be used removing the "sample" part to import easily the project into eclipse.

TODO
-----------
* Translate code comments and outputs to English.
* E-Mail message to config file.
* Use Tmux instead of screen to run each app.
* CMake/Autotools configuration to build project.
