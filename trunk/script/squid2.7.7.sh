#!/bin/sh
version=`uname -r | sed 's/\([0-9]\).*/\1/'`
if [ "$version" = 7 ]; then
echo "Downloading squid2.7.7 for freebsd7"
fetch http://pfsense-cacheboy.googlecode.com/files/freebsd7.X.squid.tar.gz
elif [ "$version" = 8 ]; then
echo "Downloading squid2.7.7 for freebsd8"
fetch http://pfsense-cacheboy.googlecode.com/files/freebsd8.X.squid.tar.gz
else
echo "Your OS is not FreeBSD 7.X nor FreeBSD 8.X your using"
echo "`uname -s` `uname -r`"
exit
fi
kill `ps -auxw | grep proxy_monitor.sh | grep -v grep | awk '{print $2}'`
/usr/local/sbin/squid -k shutdown
while [ `ps auxw | grep "squid -D" | grep -v grep |awk '{print $2}'| wc -l | awk '{ print $1 }'` -gt 0 ] ; do
	echo 'please wait...squid still shutting down'
	sleep 5
done
/etc/rc.conf_mount_rw
/etc/rc.filter_configure_sync
tar -C / -xzvf freebsd*.X.squid.tar.gz
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/script/squidsync && chmod +x squidsync
sed 's:\((\$down_limit \* 1024) \. \" \)allow all:\1deny all:g' /usr/local/pkg/squid.inc > squid.inc.tmp
mv squid.inc.tmp /usr/local/pkg/squid.inc
./squidsync
#/usr/local/etc/rc.d/squid.sh start
/usr/local/etc/rc.d/proxy_monitor.sh start &
/etc/rc.filter_configure_sync
/etc/rc.conf_mount_ro