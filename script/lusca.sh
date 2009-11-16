#!/bin/sh
version=`uname -r | sed 's/\([0-9]\).*/\1/'`
if [ "$version" = 8 ]; then
echo "Warning lusca was not compiled for freebsd8"
echo "Lusca may run slow and with some errors"
fi
echo "Downloading Lusca"
fetch http://pfsense-cacheboy.googlecode.com/files/pfsense.lusca.tar.gz
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/script/squidsync && chmod +x squidsync
kill `ps -auxw | grep proxy_monitor.sh | grep -v grep | awk '{print $2}'`
squid -k shutdown
while [ `ps auxw | grep "proxy.*squid" | grep -v grep |awk '{print $2}'| wc -l | awk '{ print $1 }'` -gt 0 ] ; do
	echo 'please wait...squid still shutting down'
	sleep 5
done
/etc/rc.filter_configure_sync
./squidsync
tar -xvf pfsense.lusca.tar.gz -C /
mkdir /usr/local/www/squid
ln -s /usr/local/libexec/squid/cachemgr.cgi /usr/local/www/squid/cachemgr.cgi
squid -D
./squidsync
echo "cachemgr at http://pfsense/squid/cachemgr.cgi"
