#!/bin/sh
fetch http://pfsense-cacheboy.googlecode.com/files/srg.tar.gz
tar -xvf srg.tar.gz -C /
echo "Working dir"
echo "/usr/local/www/srg_reports"
read path
if [ $path = "\n" ]
then
path=/usr/local/www/srg_srgports
fi
dir=`echo $path | sed 's+.*/++'`
sed -e 's+^output_dir.*+output_dir "'$path'"+' -e 's+^output_url .*+output_url "/'$dir'/"+' /usr/local/etc/srg/srg.conf > srg.conf.tmp
mv srg.conf.tmp /usr/local/etc/srg/srg.conf
if [ $path != /usr/local/www/$dir ]
then
	ln -s $path /usr/local/www/$dir
fi
echo "Your SRG configurations is save here"
echo "/usr/local/etc/srg/srg.conf"
srg --help