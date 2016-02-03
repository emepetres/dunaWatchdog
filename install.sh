cd release
make clean
make
cd ../
mkdir -p portable
cp release/dunaWatchdog portable/
mkdir -p portable/etc
cp -n etc/sample.duna.xml portable/etc/duna.xml
cp -n etc/sample.muttrc portable/etc/muttrc
cp -n etc/sample.msmtp.conf portable/etc/msmtp.conf
chmod 600 portable/etc/msmtp.conf
