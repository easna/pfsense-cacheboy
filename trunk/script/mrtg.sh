#!/bin/sh
version=`uname -r | sed 's/\([0-9.]\{3\}\).*/\1/'`
if [ "$version" = "7.1" ]; then {
echo "`uname -r` Changing Packagesite..."
export PACKAGESITE="ftp://ftp-archive.freebsd.org/pub/FreeBSD-Archive/old-releases/i386/7.1-RELEASE/packages/Latest/"
}
fi
pkg_add -rf perl
pkg_add -rf mrtg
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/conf/mrtg.cfg
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/script/mrtg_daemon.sh
mv mrtg_daemon.sh /usr/local/etc/rc.d
chmod a+x /usr/local/etc/rc.d/mrtg_daemon.sh
chmod a+rw /var
tae () {
sed 's+\(^WorkDir: \).*+\1'$dir'+' mrtg.cfg > mrtg.cfg.tmp
break
}
while :
do
echo "Mrtg Working Directory path (default)"
echo "/usr/local/mrtg"
read dir
case $dir in
	[/a-zA-Z0-9-]*)
	tae
	;;
esac
done
mv mrtg.cfg.tmp /usr/local/etc/mrtg/mrtg.cfg
chown mrtg:mrtg /usr/local/etc/mrtg/mrtg.cfg
cat mrtg.cfg | grep '^WorkDir'
mkdir "$dir"
chmod a+rw "$dir"
chown -R mrtg:mrtg "$dir"
if [ "$dir" != "/usr/local/www/mrtg" ]; then {
echo "Softlink to /usr/local/www/mrtg"
ln -s "$dir" /usr/local/www/mrtg
}
fi
indexmaker --output="$dir/index.html" /usr/local/etc/mrtg/mrtg.cfg
/usr/local/etc/rc.d/mrtg_daemon.sh stop
/usr/local/etc/rc.d/mrtg_daemon stop
/usr/local/etc/rc.d/mrtg_daemon.sh start
hostname=`hostname | sed 's/\...*//'`
echo "You may access your mrtg at http://$hostname/mrtg"