#!/bin/sh
version=`uname -r | sed 's/\([0-9]\).*/\1/'`
if [ "$version" = 8 ]; then
echo "Warning lusca was not compiled for freebsd8"
echo "Lusca may run slow and with some errors"
fi
fetch http://pfsense-cacheboy.googlecode.com/files/pfsense.lusca.tar.gz
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/script/squidsync && chmod +x squidsync
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/lusca/refresh.conf
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/lusca/cachemgr.conf
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/lusca/storeurl.pl
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/lusca/include.conf
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/lusca/dir.conf
mv *.conf /usr/local/etc/squid/
mv storeurl.pl /usr/local/etc/squid/
chmod a+x /usr/local/etc/squid/storeurl.pl
kill `ps -auxw | grep proxy_monitor.sh | grep -v grep | awk '{print $2}'`
squid -k shutdown
while [ `ps auxw | grep "proxy.*squid" | grep -v grep |awk '{print $2}'| wc -l | awk '{ print $1 }'` -gt 0 ] ; do
	echo 'please wait...squid still shutting down'
	sleep 5
done
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/lusca/lusca.diff
patch < lusca.diff -d /usr/local/pkg/
find /usr/local/pkg/ -name '*.rej' -delete
find /usr/local/pkg/ -name '*.orig' -delete
/etc/rc.filter_configure_sync
./squidsync
tar -xvf pfsense.lusca.tar.gz -C /
mkdir /usr/local/www/squid
ln -s /usr/local/libexec/squid/cachemgr.cgi /usr/local/www/squid/cachemgr.cgi
squid -D
./squidsync
echo "cachemgr at http://pfsense/squid/cachemgr.cgi"
