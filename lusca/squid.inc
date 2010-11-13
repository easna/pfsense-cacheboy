<?php
/* $Rev$ */
/*
	squid.inc
	Copyright (C) 2006-2009 Scott Ullrich
	Copyright (C) 2006 Fernando Lemos
	Copyright (C) 2008 Martin Fuchs
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
	AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

require_once('globals.inc');
require_once('config.inc');
require_once('util.inc');
require_once('pfsense-utils.inc');
require_once('pkg-utils.inc');
require_once('service-utils.inc');

if(!function_exists("filter_configure"))
	require_once("filter.inc");
	
function is_process_running_full($process) {

	$running = shell_exec("/bin/pgrep -fl \"{$process}\" | grep -v pgrep");
	log_error("process \"{$process}\" running at \"{$running}\"");
        return !empty($running);
}

define('SQUID_CONFBASE', '/usr/local/etc/squid');
define('SQUID_BASE', '/var/squid/');
define('SQUID_ACLDIR', '/var/squid/acl');
define('SQUID_PASSWD', '/var/etc/squid.passwd');

$valid_acls = array();

function squid_get_real_interface_address($iface) {
	global $config;

	$iface = convert_friendly_interface_to_real_interface_name($iface);
	$line = trim(shell_exec("ifconfig $iface | grep inet | grep -v inet6"));
	list($dummy, $ip, $dummy2, $netmask) = explode(" ", $line);

	return array($ip, long2ip(hexdec($netmask)));
}

function squid_chown_recursive($dir, $user, $group) {
	exec("chown -R $user:$group $dir");
}

/* setup cache */
function squid_dash_z() {
	global $config;
	$settings = $config['installedpackages']['squidcache']['config'][0];
	$cachedir = ($settings['harddisk_cache_location'] ? $settings['harddisk_cache_location'] : '/var/squid/cache');
	$cosscachedir = ($settings['coss_harddisk_cache_location'] ? $settings['coss_harddisk_cache_location'] : '/var/squid');
	$hddsystem = ($settings['harddisk_cache_system'] ? $settings['harddisk_cache_system'] : 'null');

	if ($hddsystem == "coss" || $hddsystem == "coss+aufs") {
	$coss = "on";
		if (!file_exists("$cosscachedir/coss")) {
			make_dirs($cosscachedir);
			squid_chown_recursive($cosscachedir, 'proxy', 'proxy');
		}
	}

	if ($hddsystem == "aufs" || $hddsystem == "coss+aufs") {
	$aufs = "on";
		if (!is_dir($cachedir.'/')) {
			make_dirs($cachedir);
			squid_chown_recursive($cachedir, 'proxy', 'proxy');
		}
	}

	if (file_exists("$cachedir/swap.state"))
		exec("chmod a+rw $cachedir/swap.state");
	if (file_exists("$cosscachedir/swap.state"))
		exec("chmod a+rw $cosscachedir/swap.state");

 	if (((!is_dir($cachedir.'/00/')) && $aufs == "on") || ((!file_exists("$cosscachedir/coss")) && $coss == "on")) {
		while (is_process_running_full("squid -D")) {
		log_error("Waiting squid to shutdown before creating squid cache dir");
		sleep(1);
		}
		log_error("Creating squid cache dir and files");
		squid_chown_recursive($cachedir, 'proxy', 'proxy');
		mwexec("/usr/local/sbin/squid -z");
	}

	/* for custom cache system. should be here soon */

}

function squid_is_valid_acl($acl) {
	global $valid_acls;
	if(!is_array($valid_acls))
		return;
	return in_array($acl, $valid_acls);
}

function squid_install_command() {
	global $config;
	/* migrate existing csv config fields */
	$settingsauth = $config['installedpackages']['squidauth']['config'][0];
	$settingscache = $config['installedpackages']['squidcache']['config'][0];
	$settingsnac = $config['installedpackages']['squidnac']['config'][0];

	/* migrate auth settings */
	if (!empty($settingsauth['no_auth_hosts'])) {
		if(strstr($settingsauth['no_auth_hosts'], ",")) {
			$settingsauth['no_auth_hosts'] = base64_encode(implode("\n", explode(",", $settingsauth['no_auth_hosts'])));
			$config['installedpackages']['squidauth']['config'][0]['no_auth_hosts'] = $settingsauth['no_auth_hosts'];
		}
	}

	/* migrate cache settings */
	if (!empty($settingscache['donotcache'])) {
		if(strstr($settingscache['donotcache'], ",")) {
			$settingscache['donotcache'] = base64_encode(implode("\n", explode(",", $settingscache['donotcache'])));
			$config['installedpackages']['squidcache']['config'][0]['donotcache'] = $settingscache['donotcache'];
		}
	}

	/* migrate nac settings */
	if(! empty($settingsnac['allowed_subnets'])) {
		if(strstr($settingsnac['allowed_subnets'], ",")) {
			$settingsnac['allowed_subnets'] = base64_encode(implode("\n", explode(",", $settingsnac['allowed_subnets'])));
			$config['installedpackages']['squidnac']['config'][0]['allowed_subnets'] = $settingsnac['allowed_subnets'];
		}
	}

	if(! empty($settingsnac['banned_hosts'])) {
		if(strstr($settingsnac['banned_hosts'], ",")) {
			$settingsnac['banned_hosts'] = base64_encode(implode("\n", explode(",", $settingsnac['banned_hosts'])));
			$config['installedpackages']['squidnac']['config'][0]['banned_hosts'] = $settingsnac['banned_hosts'];
		}
	}

	if(! empty($settingsnac['banned_macs'])) {
		if(strstr($settingsnac['banned_macs'], ",")) {
			$settingsnac['banned_macs'] = base64_encode(implode("\n", explode(",", $settingsnac['banned_macs'])));
			$config['installedpackages']['squidnac']['config'][0]['banned_macs'] = $settingsnac['banned_macs'];
		}
	}

	if(! empty($settingsnac['unrestricted_hosts'])) {
		if(strstr($settingsnac['unrestricted_hosts'], ",")) {
			$settingsnac['unrestricted_hosts'] = base64_encode(implode("\n", explode(",", $settingsnac['unrestricted_hosts'])));
			$config['installedpackages']['squidnac']['config'][0]['unrestricted_hosts'] = $settingsnac['unrestricted_hosts'];
		}
	}

	if(! empty($settingsnac['unrestricted_macs'])) {
		if(strstr($settingsnac['unrestricted_macs'], ",")) {
			$settingsnac['unrestricted_macs'] = base64_encode(implode("\n", explode(",", $settingsnac['unrestricted_macs'])));
			$config['installedpackages']['squidnac']['config'][0]['unrestricted_macs'] = $settingsnac['unrestricted_macs'];
		}
	}

	if(! empty($settingsnac['whitelist'])) {
		if(strstr($settingsnac['whitelist'], ",")) {
			$settingsnac['whitelist'] = base64_encode(implode("\n", explode(",", $settingsnac['whitelist'])));
			$config['installedpackages']['squidnac']['config'][0]['whitelist'] = $settingsnac['whitelist'];
		}
	}

	if(! empty($settingsnac['blacklist'])) {
		if(strstr($settingsnac['blacklist'], ",")) {
			$settingsnac['blacklist'] = base64_encode(implode("\n", explode(",", $settingsnac['blacklist'])));
			$config['installedpackages']['squidnac']['config'][0]['blacklist'] = $settingsnac['blacklist'];
		}
	}

	update_status("Writing configuration... One moment please...");

	write_config();
	/* make sure pinger is executable */
	if(file_exists("/usr/local/libexec/squid/pinger"))
		exec("/bin/chmod a+x /usr/local/libexec/squid/pinger");
	if(file_exists("/usr/local/etc/rc.d/squid"))
		exec("/bin/rm /usr/local/etc/rc.d/squid");
	$rc = array();
	$rc['file'] = 'squid.sh';
	$rc['start'] = <<<EOD

# fsck and mount before starting squid if
# /etc/fstab configured not to check cache_dir drives for boot time purposes

#if [ ! -d "/cache1/00" ] && [ ! `/bin/pgrep -f fsck` ]; then
#fsck -y /cache0
#fsck -y /cache1
#mount -a
#rm -r /cache0/lost+found
#rm -r /cache1/lost+found
#elif [ `/bin/pgrep -f fsck` ]; then
#exit
#fi

while [ `pgrep -f "squid -D"` ]; do
	sleep 1
	i=$((\$i+1))
	if [ \$i -gt 15 ]; then
		exit
	fi
done
if [ ! `/bin/pgrep -f "squid -D"` ]; then
	/usr/local/sbin/squid -D
fi
/etc/rc.filter_configure_sync
if [ ! `/bin/pgrep -f "proxy_monitor"` ]; then
	/usr/local/etc/rc.d/proxy_monitor.sh &
fi

EOD;
	$rc['stop'] = <<<EOD

/bin/pkill -f proxy_monitor.sh
if [ `/bin/pgrep -f "squid -D"` ]; then
	/usr/local/sbin/squid -k shutdown
fi
while [ `pgrep -f "squid -D"` ]; do
	sleep 1
	i=$((\$i+1))
	if [ \$i -gt 5 ]; then
		exit
	fi
done
/etc/rc.filter_configure_sync &

EOD;

	update_status("Writing rc.d files... One moment please...");
	conf_mount_rw();
	write_rcfile($rc);

	mwexec(`"ps awux | grep proxy_monitor.sh | grep -v grep | awk '{ print $2 }' | xargs kill"`);
	exec("chmod a+rx /usr/local/libexec/squid/dnsserver");

	foreach (array(	SQUID_CONFBASE,
			SQUID_ACLDIR,
			SQUID_BASE ) as $dir) {
			make_dirs($dir);
			squid_chown_recursive($dir, 'proxy', 'proxy');
	}
	$confmgr = "127.0.0.1:80\n";
	$confmgr .= $config['system']['hostname'] . "\n";
	file_put_contents(SQUID_CONFBASE . '/cachemgr.conf', $confmgr);

	if (!file_exists(SQUID_CONFBASE . '/mime.conf') && file_exists(SQUID_CONFBASE . '/mime.conf.default'))
		copy(SQUID_CONFBASE . '/mime.conf.default', SQUID_CONFBASE . '/mime.conf');

//	squid_resync(); //not needed since squid.xml will do squid_resync()
//	squid_dash_z();
//	update_status("Synchronizing configurations... done.");

 	mwexec("sed s/'^kern.ipc.nmbclusters=\"0\"'/#kern.ipc.nmbclusters=\"0\"/ /boot/loader.conf > /boot/loader.conf.tmp");
	mwexec("mv /boot/loader.conf.tmp /boot/loader.conf");
	update_status("lusca-cache Installation complete!");
}

function squid_deinstall_command() {
	global $config, $g;
	$plswait_txt = "This operation may take quite some time, please be patient.  Do not press stop or attempt to navigate away from this page during this process.";
	squid_install_cron(false);
 	$settings = &$config['installedpackages']['squid']['config'][0];
/* 	$cachedir =($settings['harddisk_cache_location'] ? $settings['harddisk_cache_location'] : '/var/squid/cache');
	$cachedir2 =($settings['coss_harddisk_cache_location'] ? $settings['coss_harddisk_cache_location'] : '/var/squid');
	$logdir = ($settings['log_dir'] ? $settings['log_dir'] : '/var/squid/logs'); */
	update_output_window("$plswait_txt");
	$i = 0;
	update_status("Waiting squid to shutdown...");
		while (is_process_running_full("squid\ -D")) {
			if ($i == 5) {
			update_status("Retry shutting down squid...");
			mwexec("/usr/local/sbin/squid -k shutdown");
			sleep(5);
			}
		sleep(1);
		$i++;
			if ($i = 30){
			update_status("shutdown taking too long. now forcing to exit...$i");
			$num_fields = array('proxy_monitor.sh','squid','dnsserver','unlinkd');
			foreach ($num_fields as $field) {
				$value = trim($post[$field]);
				log_error("killing \"{$field}\"");
				exec("pkill -f \"$field\"");
			}
			}
		}
	update_status("Finishing package cleanup.");
	sleep(3);
/* 	mwexec("rm -rf $logdir");
	#this part needs confirmations. this will solve the conflict with other versions
 	$num_fields2 = array( 'squidcache','squid','squidauth','squidnac','squidtraffic','squidupstream','squidusers'
	);
	foreach ($num_fields2 as $field2) {
		$value = trim($post[$field]);
		unset($config['installedpackages']["$field2"]);
	}
	mwexec("rm -rf $cachedir");
	mwexec("rm -rf $cachedir2"); */
	update_status("Reloading filter...");
	filter_configure_sync();
}

function squid_before_form_general($pkg) {
	$values = get_dir(SQUID_CONFBASE . '/errors/');
	// Get rid of '..' and '.'
	array_shift($values);
	array_shift($values);
	$name = array();
	foreach ($values as $value)
		$names[] = implode(" ", explode("_", $value));

	$i = 0;
	foreach ($pkg['fields']['field'] as $field) {
		if ($field['fieldname'] == 'error_language')
			break;
		$i++;
	}
	$field = &$pkg['fields']['field'][$i];

	for ($i = 0; $i < count($values) - 1; $i++)
		$field['options']['option'][] = array('name' => $names[$i], 'value' => $values[$i]);
}

function squid_validate_general($post, $input_errors) {
	global $config;
	$settings = $config['installedpackages']['squid']['config'][0];
	$port = ($settings['proxy_port'] ? $settings['proxy_port'] : 3128);
	$port = $post['proxy_port'] ? $post['proxy_port'] : $port;

	$icp_port = trim($post['icp_port']);
	if (!empty($icp_port) && !is_port($icp_port))
		$input_errors[] = 'You must enter a valid port number in the \'ICP port\' field';

	if (substr($post['log_dir'], -1, 1) == '/')
		$input_errors[] = 'You may not end log location with an / mark';

	if ($post['log_dir']{0} != '/')
		$input_errors[] = 'You must start log location with a / mark';
	if (strlen($post['log_dir']) <= 3)
		$input_errors[] = "That is not a valid log location dir";

	$log_rotate = trim($post['log_rotate']);
	if (!empty($log_rotate) && (!is_numeric($log_rotate) or ($log_rotate < 1)))
		$input_errors[] = 'You must enter a valid number of days \'Log rotate\' field';

	$webgui_port = $config['system']['webgui']['port'];
	if(($config['system']['webgui']['port'] == "") && ($config['system']['webgui']['protocol'] == "http")) {
		$webgui_port = 80;
	}
	if(($config['system']['webgui']['port'] == "") && ($config['system']['webgui']['protocol'] == "https")) {
		$webgui_port = 443;
	}

	if (($post['transparent_proxy'] != 'on') && ($port == $webgui_port)) {
		$input_errors[] = "You can not run squid on the same port as the webgui";
	}

/* 	if (($post['transparent_proxy'] != 'on') && ($post['private_subnet_proxy_off'] == 'on')) {
		$input_errors[] = "You can not bypass traffic to private subnets  without using the transparent proxy.";
	} */

/* 	if (($post['transparent_proxy'] != 'on') && !empty($post['defined_ip_proxy_off'])) {
		$input_errors[] = "You can not bypass traffic from specific IPs without using the transparent proxy.";
	} */

	foreach (array('defined_ip_proxy_off') as $hosts) {
		foreach (explode(";", $post[$hosts]) as $host) {
			$host = trim($host);
			if (!empty($host) && !is_ipaddr($host))
				$input_errors[] = "The entry '$host' is not a valid IP address";
		}
	}
	foreach (array('defined_ip_proxy_off_dist') as $hosts) {
		foreach (explode(";", $post[$hosts]) as $host) {
			$host = trim($host);
			if (!empty($host) && !is_ipaddr($host))
				$input_errors[] = "The entry '$host' is not a valid IP address";
		}
	}
  if(!empty($post['dns_nameservers'])) {
	 $altdns = explode(";", ($post['dns_nameservers']));
	 foreach ($altdns as $dnssrv) {
    if (!is_ipaddr($dnssrv))
      $input_errors[] = 'You must enter a valid IP address in the \'Alternate DNS servers\' field';
	 }}
}

function squid_validate_upstream($post, $input_errors) {
	if ($post['proxy_forwarding'] == 'on') {
		$addr = trim($post['proxy_addr']);
		if (empty($addr))
			$input_errors[] = 'The field \'Hostname\' is required';
		else {
			if (!is_ipaddr($addr) && !is_domain($addr))
				$input_errors[] = 'You must enter a valid IP address or host name in the \'Proxy hostname\' field';
		}

		foreach (array('proxy_port' => 'TCP port', 'icp_port' => 'ICP port') as $field => $name) {
			$port = trim($post[$field]);
			if (empty($port))
				$input_errors[] = "The field '$name' is required";
			else {
					if (!is_port($port))
					$input_errors[] = "The field '$name' must contain a valid port number, between 0 and 65535";
			}
		}
	}
}

function squid_validate_cache($post, $input_errors) {
//	$cache_system = $config['installedpackages']['squidcache']['config'][0]['harddisk_cache_system']
// 'harddisk_cache_size' => 'Hard disk cache size', 				'maximum_object_size' => 'Maximum object size',
	$num_fields = array(	'memory_cache_size' => 'Memory cache size', 'minimum_object_size' => 'Minimum object size', 'mem_object_size' => 'Max memory object size',
				'memory_cache_size' => 'Memory cache size'

	);
	foreach ($num_fields as $field => $name) {
		$value = trim($post[$field]);
		if (!is_numeric($value) || ($value < 0))
			$input_errors[] = "You must enter a valid value for '$name'";
	}

  if (!empty($post['cache_swap_low'])) {
    $value = trim($post['cache_swap_low']);
	  if (!is_numeric($value) || ($value > 100))
	  $input_errors[] = 'You must enter a valid value for \'Low-water-mark\'';
	}

  if (!empty($post['cache_swap_high'])) {
	 $value = trim($post['cache_swap_high']);
	 if (!is_numeric($value) || ($value > 100))
		$input_errors[] = 'You must enter a valid value for \'High-water-mark\'';
  }

	if ($post['donotcache'] != "") {
		foreach (split("\n", $post['donotcache']) as $host) {
			$host = trim($host);
			if (!is_ipaddr($host) && !is_domain($host))
				$input_errors[] = "The host '$host' is not a valid IP or host name";
		}
	}

	squid_dash_z();

}

function squid_validate_nac($post, $input_errors) {
	$allowed_subnets = explode("\n", $post['allowed_subnets']);
	foreach ($allowed_subnets as $subnet) {
		$subnet = trim($subnet);
		if (!empty($subnet) && !is_subnet($subnet))
			$input_errors[] = "The subnet '$subnet' is not a valid CIDR range";
	}

	foreach (array(	'unrestricted_hosts', 'banned_hosts') as $hosts) {
		foreach (explode("\n", $post[$hosts]) as $host) {
			$host = trim($host);
			if (!empty($host) && !is_ipaddr($host))
				$input_errors[] = "The host '$host' is not a valid IP address";
		}
	}

	foreach (array('unrestricted_macs', 'banned_macs') as $macs) {
		foreach (explode("\n", $post[$macs]) as $mac) {
			$mac = trim($mac);
			if (!empty($mac) && !is_macaddr($mac))
				$input_errors[] = "The mac '$mac' is not a valid MAC address";
		}
	}

	foreach (explode(",", $post['timelist']) as $time) {
		$time = trim($time);
		if (!empty($time) && !squid_is_timerange($time))
			$input_errors[] = "The time range '$time' is not a valid time range";
	}

  if(!empty($post['ext_cachemanager'])) {
	 $extmgr = explode(";", ($post['ext_cachemanager']));
	 foreach ($extmgr as $mgr) {
    if (!is_ipaddr($mgr))
      $input_errors[] = 'You must enter a valid IP address in the \'External Cache Manager\' field';
	 }}
}

function squid_validate_traffic($post, $input_errors) {
	$num_fields = array(	'max_download_size' => 'Maximum download size',
				'max_upload_size' => 'Maximum upload size',
	);
/* 	if ($post['enable_delay_pool']) {
	$num_fields['perhost_throttling'] = 'Per-host bandwidth throttling';
	$num_fields['overall_throttling'] = 'Overall bandwidth throttling';
	} */
	foreach ($num_fields as $field => $name) {
		$value = trim($post[$field]);
		if (!is_numeric($value) || ($value < 0))
			$input_errors[] = "The field '$name' must contain a positive number";
	}

  if (!empty($post['quick_abort_min'])) {
	 $value = trim($post['quick_abort_min']);
     if (!is_numeric($value))
	   $input_errors[] = "The field 'Finish when remaining KB' must contain a positive number";
  }

  if (!empty($post['quick_abort_max'])) {
	 $value = trim($post['quick_abort_max']);
     if (!is_numeric($value))
	   $input_errors[] = "The field 'Abort when remaining KB' must contain a positive number";
  }

  if (!empty($post['quick_abort_pct'])) {
	 $value = trim($post['quick_abort_pct']);
     if (!is_numeric($value) || ($value > 100))
	   $input_errors[] = "The field 'Finish when remaining %' must contain a percentaged value";
  }

}

function squid_validate_auth($post, $input_errors) {
	$num_fields = array(	array('auth_processes', 'Authentication processes', 1),
				array('auth_ttl', 'Authentication TTL', 0),
	);
	foreach ($num_fields as $field) {
		$value = trim($post[$field[0]]);
		if (!empty($value) && (!is_numeric($value) || ($value < $field[2])))
			$input_errors[] = "The field '{$field[1]}' must contain a valid number greater than {$field[2]}";
	}

	$auth_method = $post['auth_method'];
	if (($auth_method != 'none') && ($auth_method != 'local')) {
		$server = trim($post['auth_server']);
		if (empty($server))
			$input_errors[] = 'The field \'Authentication server\' is required';
		else if (!is_ipaddr($server) && !is_domain($server))
			$input_errors[] = 'The field \'Authentication server\' must contain a valid IP address or domain name';

		$port = trim($post['auth_server_port']);
		if (!empty($port) && !is_port($port))
			$input_errors[] = 'The field \'Authentication server port\' must contain a valid port number';

		switch ($auth_method) {
			case 'ldap':
				$user = trim($post['ldap_user']);
				if (empty($user))
					$input_errors[] = 'The field \'LDAP server user DN\' is required';
				else if (!$user)
					$input_errors[] = 'The field \'LDAP server user DN\' must be a valid domain name';
				break;
			case 'radius':
				$secret = trim($post['radius_secret']);
				if (empty($secret))
					$input_errors[] = 'The field \'RADIUS secret\' is required';
				break;
			case 'msnt':
				foreach (explode(",", trim($post['msnt_secondary'])) as $server) {
					if (!empty($server) && !is_ipaddr($server) && !is_domain($server))
						$input_errors[] = "The host '$server' is not a valid IP address or domain name";
				}
				break;
		}

		$no_auth = explode("\n", $post['no_auth_hosts']);
		foreach ($no_auth as $host) {
			$host = trim($host);
			if (!empty($host) && !is_subnet($host))
				$input_errors[] = "The host '$host' is not a valid CIDR range";
		}
	}
}

function squid_install_cron($should_install) {
	global $config, $g;
	if($g['booting']==true)
		return;
	$is_installed = false;
	if(!$config['cron']['item'])
		return;
	$x=0;
	foreach($config['cron']['item'] as $item) {
		if(strstr($item['command'], "/usr/local/sbin/squid")) {
			$is_installed = true;
			break;
		}
		$x++;
	}
	switch($should_install) {
		case true:
			if(!$is_installed) {
				$cron_item = array();
				$cron_item['task_name'] = "squid_rotate_logs";
				$cron_item['minute'] = "0";
				$cron_item['hour'] = "0";
				$cron_item['mday'] = "*";
				$cron_item['month'] = "*";
				$cron_item['wday'] = "*";
				$cron_item['who'] = "root";
				$cron_item['command'] = "/usr/local/sbin/squid -k rotate";
				$config['cron']['item'][] = $cron_item;
				parse_config(true);
				write_config("Squid Log Rotation");
				configure_cron();
			}
		break;
		case false:
			if($is_installed == true) {
				if($x > 0) {
					unset($config['cron']['item'][$x]);
					parse_config(true);
					write_config();
				}
				configure_cron();
			}
		break;
	}
}

function squid_resync_general() {
	global $g, $config, $valid_acls;

	$settings = $config['installedpackages']['squid']['config'][0];
	$conf = "# This file is automatically generated by pfSense\n";
	$conf = "# Do not edit manually !\n";

	$port = ($settings['proxy_port']);
	if ($settings['enable_http11'] == 'on') {
	$http_port_options = "http11 ";
	}
	$ifaces = ($settings['active_interface'] ? $settings['active_interface'] : 'lan');
	$real_ifaces = array();
	$http_port_options .= ($settings['transparent_proxy'] == 'on' ? 'transparent ' : '');
	foreach (explode(",", $ifaces) as $i => $iface) {
		$real_ifaces[] = squid_get_real_interface_address($iface);
		if (!empty($port))
		if($real_ifaces[$i][0]) {
			$conf .= "http_port {$real_ifaces[$i][0]}:$port $http_port_options\n";
		}
	}
	if (($settings['transparent_proxy'] == 'on') || (!$port > 0))
		$conf .= "http_port 127.0.0.1:80 $http_port_options\n";
	$icp_port = ($settings['icp_port'] ? $settings['icp_port'] : 0);
	$pidfile = "{$g['varrun_path']}/squid.pid";
	$language = ($settings['error_language'] ? $settings['error_language'] : 'English');
	$errordir = SQUID_CONFBASE . '/errors/' . $language;
	$icondir = SQUID_CONFBASE . '/icons';
	$hostname = ($settings['visible_hostname'] ? $settings['visible_hostname'] : 'localhost');
	$email = ($settings['admin_email'] ? $settings['admin_email'] : 'admin@localhost');

	$logdir = ($settings['log_dir'] ? $settings['log_dir'] : '/var/squid/logs');

	$logdir_cache = $logdir . '/cache.log';
	$logdir_access = ($settings['log_enabled'] == 'on' ? $logdir . '/access.log' : '/dev/null');


	$conf .= <<<EOD
icp_port $icp_port

pid_filename $pidfile
cache_effective_user proxy
cache_effective_group proxy
error_directory $errordir
icon_directory $icondir
visible_hostname $hostname
cache_mgr $email
access_log $logdir_access
cache_log $logdir_cache
cache_store_log none

EOD;

  if (!empty($settings['log_rotate'])) {
    $conf .= "logfile_rotate {$settings['log_rotate']}\n";
    squid_install_cron(true);
    }
  else {
    squid_install_cron(false);
    }

	$conf .= <<<EOD
shutdown_lifetime 0 seconds

EOD;

	if ($settings['allow_interface'] == 'on') {
		$src = '';
		foreach ($real_ifaces as $iface) {
			list($ip, $mask) = $iface;
			$ip = long2ip(ip2long($ip) & ip2long($mask));
			$src .= " $ip/$mask";
		}
		$conf .= "# Allow local network(s) on interface(s)\n";
		$conf .= "acl localnet src $src\n";
		$valid_acls[] = 'localnet';
	}
	if ($settings['disable_xforward']) $conf .= "forwarded_for transparent\n";
	if ($settings['disable_via']) $conf .= "via off\n";
	if ($settings['enable_http11']) $conf .= "server_http11 on\n";
	if ($settings['disable_squidversion']) $conf .= "httpd_suppress_version_string on\n";
	if (!empty($settings['uri_whitespace'])) $conf .= "uri_whitespace {$settings['uri_whitespace']}\n";
  else $conf .= "uri_whitespace strip\n"; //only used for first run

	if(!empty($settings['dns_nameservers'])) {
	  $altdns = explode(";", ($settings['dns_nameservers']));
    $conf .= "dns_nameservers ";
	 foreach ($altdns as $dnssrv) {
    $conf .= $dnssrv." ";
	 }
//  $conf .= "\n";  //Kill blank line after DNS-Servers
   }

  return $conf;
}

function cache_system_changed_javascript() {
	print("<script language=\"JavaScript\">cache_system_changed()</script>\n");
}

function squid_resync_cache() {
	global $config;

	$settings = $config['installedpackages']['squidcache']['config'][0];

	$cosscachedir = ($settings['coss_harddisk_cache_location'] ? $settings['coss_harddisk_cache_location'] : '/var/squid');
	$cachedir = ($settings['harddisk_cache_location'] ? $settings['harddisk_cache_location'] : '/var/squid/cache');
	$coss_disk_cache_size = ($settings['coss_harddisk_cache_size'] ? $settings['coss_harddisk_cache_size'] : 50);
	$disk_cache_size = ($settings['harddisk_cache_size'] ? $settings['harddisk_cache_size'] : 100);
	$disk_cache_system = ($settings['harddisk_cache_system'] ? $settings['harddisk_cache_system'] : 'null');
	$object_size = (($settings['max_min_size'] ? $settings['max_min_size'] : 4) * 1024);
	$level1 = ($settings['level1_subdirs'] ? $settings['level1_subdirs'] : 16);
	$memory_cache_size = ($settings['memory_cache_size'] ? $settings['memory_cache_size'] : 8);
	$max_objsize = ($settings['maximum_object_size'] ? $settings['maximum_object_size'] : 32);
	$min_objsize = ($settings['minimum_object_size'] ? $settings['minimum_object_size'] : 0);
	$max_mem_objsize = ($settings['mem_object_size'] ? $settings['mem_object_size'] : 4);
	$cache_policy = ($settings['cache_replacement_policy'] ? $settings['cache_replacement_policy'] : 'heap LFUDA');
	$memory_policy = ($settings['memory_replacement_policy'] ? $settings['memory_replacement_policy'] : 'heap GDSF');
	$offline_mode = ($settings['enable_offline'] == 'on' ? 'on' : 'off');
	$n = 512;
	$s = 8192;
	while ($coss_disk_cache_size >= $s){$n = $n * 2;$s = $s * 2;}
	if ($n > 8192)
	$n = 8192;
	$blocksize = "block-size=$n";
	
	if ($disk_cache_system == "null") {
	$cache_dir = "cache_dir null /tmp";
	}
	elseif ($disk_cache_system == "custom") {
	$cache_dir = "include /usr/local/etc/squid/dir.conf";
	}
	elseif ($disk_cache_system == "coss+aufs") {
	$cache_coss_dir = "cache_dir coss $cosscachedir/coss $coss_disk_cache_size max-size=$object_size $blocksize";
	$cache_dir = "cache_dir aufs $cachedir $disk_cache_size $level1 256 min-size=$object_size";
	}
	elseif ($disk_cache_system == "aufs") {
	$cache_dir = "cache_dir aufs $cachedir $disk_cache_size $level1 256";
	}
	elseif ($disk_cache_system == "coss") {
	$cache_coss_dir = "cache_dir coss $cosscachedir/coss $coss_disk_cache_size max-size=$object_size $blocksize";
	}

	$conf = <<<EOD

cache_mem $memory_cache_size MB
maximum_object_size_in_memory $max_mem_objsize KB
memory_replacement_policy $memory_policy
cache_replacement_policy $cache_policy
$cache_coss_dir
$cache_dir
minimum_object_size $min_objsize KB
maximum_object_size $max_objsize MB
offline_mode $offline_mode

EOD;

	if (!empty($settings['cache_swap_low'])) $conf .= "cache_swap_low {$settings['cache_swap_low']}\n";
	if (!empty($settings['cache_swap_high'])) $conf .= "cache_swap_high {$settings['cache_swap_high']}\n";

	$donotcache = base64_decode($settings['donotcache']);
	if (!empty($donotcache)) {
		file_put_contents(SQUID_ACLDIR . '/donotcache.acl', $donotcache);
		$conf .= 'acl donotcache dstdomain "' . SQUID_ACLDIR . "/donotcache.acl\"\n";
		$conf .= 'cache deny donotcache';
	}
	elseif (file_exists(SQUID_ACLDIR . '/donotcache.acl')) {
     unlink(SQUID_ACLDIR . '/donotcache.acl');
    }

	return $conf;
}

function cache_system_changed_java_body() {
	global $config;
	$settings = $config['installedpackages']['squidcache']['config'][0];
	$cachedir = ($settings['harddisk_cache_location'] ? $settings['harddisk_cache_location'] : '/var/squid/cache');
	$cachedirsize = ($settings['harddisk_cache_size'] ? $settings['harddisk_cache_size'] : '100');
	$coss_disk_cache_size = ($settings['coss_harddisk_cache_size'] ? $settings['coss_harddisk_cache_size'] : 50);
	$cosscachedir = ($settings['coss_harddisk_cache_location'] ? $settings['coss_harddisk_cache_location'] : '/var/squid');
	$max_min_size = ($settings['max_min_size'] ? $settings['max_min_size'] : 4);

		$javascript = <<<EOD
<script language="JavaScript">
<!--
function cache_system_changed() {
	var field = document.iform.harddisk_cache_system;
	var harddisk_cache_system = field.options[field.selectedIndex].value;
			document.iform.harddisk_cache_location.value = "$cachedir";
			document.iform.harddisk_cache_size.value = "$cachedirsize";
			document.iform.coss_harddisk_cache_size.value = "$coss_disk_cache_size";
			document.iform.coss_harddisk_cache_location.value = "$cosscachedir";
			document.iform.max_min_size.value = "$max_min_size";
	switch (harddisk_cache_system) {
		case 'null':
			document.iform.coss_harddisk_cache_size.disabled = 1;
			document.iform.coss_harddisk_cache_location.disabled = 1;
			document.iform.harddisk_cache_size.disabled = 1;
			document.iform.harddisk_cache_location.disabled = 1;
			document.iform.max_min_size.disabled = 1;
			break;
		case 'aufs':
			document.iform.coss_harddisk_cache_size.disabled = 1;
			document.iform.coss_harddisk_cache_location.disabled = 1;
			document.iform.harddisk_cache_size.disabled = 0;
			document.iform.harddisk_cache_location.disabled = 0;
			document.iform.max_min_size.disabled = 1;
			break;
		case 'coss+aufs':
			document.iform.coss_harddisk_cache_size.disabled = 0;
			document.iform.coss_harddisk_cache_location.disabled = 0;
			document.iform.harddisk_cache_size.disabled = 0;
			document.iform.harddisk_cache_location.disabled = 0;
			document.iform.max_min_size.disabled = 0;
			break;
		case 'coss':
			document.iform.coss_harddisk_cache_size.disabled = 0;
			document.iform.coss_harddisk_cache_location.disabled = 0;
			document.iform.harddisk_cache_size.disabled = 1;
			document.iform.harddisk_cache_location.disabled = 1;
			document.iform.max_min_size.disabled = 0;
			break;
		case 'custom':
			document.iform.coss_harddisk_cache_size.disabled = 1;
			document.iform.coss_harddisk_cache_location.disabled = 1;
			document.iform.harddisk_cache_size.disabled = 1;
			document.iform.harddisk_cache_location.disabled = 1;
			document.iform.max_min_size.disabled = 1;
			break;
	}
}
-->
</script>

EOD;


	print($javascript);
}

function squid_resync_upstream() {
	global $config;
	$settings = $config['installedpackages']['squidupstream']['config'][0];

	$conf = '';
	if ($settings['proxy_forwarding'] == 'on') {
		$conf .= "cache_peer {$settings['proxy_addr']} parent {$settings['proxy_port']} ";
		if ($settings['icp_port'] == '7')
		  $conf .= "{$settings['icp_port']} no-query";
    else
      $conf .= "{$settings['icp_port']}";

		if (!empty($settings['username']))
			$conf .= " login={$settings['username']}";
		if (!empty($settings['password']))
			$conf .= ":{$settings['password']}";
	}

	return $conf;
}

function squid_resync_redirector() {
	global $config;

	$httpav_enabled = ($config['installedpackages']['clamav']['config'][0]['scan_http'] == 'on');
	if ($httpav_enabled) {
		$conf = "url_rewrite_program /usr/local/bin/squirm\n";
	} else {
		$conf = "# No redirector configured\n";
	}
	return $conf;
}

function squid_resync_nac() {
	global $config, $valid_acls;

  $port = ($settings['proxy_port'] ? $settings['proxy_port'] : 3128);
  $settings = $config['installedpackages']['squidnac']['config'][0];
	$webgui_port = $config['system']['webgui']['port'];

	$conf = <<<EOD

# Setup some default acls
acl all src 0.0.0.0/0.0.0.0
acl localhost src 127.0.0.1/255.255.255.255
acl safeports port 21 70 80 210 280 443 488 563 591 631 777 901 $webgui_port $port 1025-65535
acl sslports port 443 563 $webgui_port
acl manager proto cache_object
acl purge method PURGE
acl connect method CONNECT
acl partialcontent_req req_header Range .*
#acl dynamic urlpath_regex cgi-bin \?
include /usr/local/etc/squid/include.conf

EOD;

	$allowed_subnets = explode("\n", base64_decode($settings['allowed_subnets']));
	$allowed = "";
	foreach ($allowed_subnets as $subnet) {
		if(!empty($subnet)) {
			$subnet = trim($subnet);
			$allowed .= "$subnet ";
		}
	}
	if (!empty($allowed)) {
		$conf .= "acl allowed_subnets src $allowed\n";
		$valid_acls[] = 'allowed_subnets';
	}

	$options = array(	'unrestricted_hosts' => 'src',
				'banned_hosts' => 'src',
				'whitelist' => 'dstdom_regex -i',
				'blacklist' => 'dstdom_regex -i',
	);
	foreach ($options as $option => $directive) {
		$contents = base64_decode($settings[$option]);
		if (!empty($contents)) {
			file_put_contents(SQUID_ACLDIR . "/$option.acl", $contents);
			$conf .= "acl $option $directive \"" . SQUID_ACLDIR . "/$option.acl\"\n";
			$valid_acls[] = $option;
		}
		elseif (file_exists(SQUID_ACLDIR . "/$option.acl")) {
      unlink(SQUID_ACLDIR . "/$option.acl");
    }
	}

	$conf .= <<<EOD
#cache deny dynamic
http_access allow manager localhost

EOD;

	if(!empty($settings['ext_cachemanager'])) {
	  $extmgr = explode(";", ($settings['ext_cachemanager']));
    $count = 1;
    $conf .= "\n# Allow external cache managers\n";
//    $conf .= "acl ext_manager src ".$settings['ext_cachemanager']."\n";
	 foreach ($extmgr as $mgr) {
    $conf .= "acl ext_manager_".$count." src ";
    $conf .= $mgr." ";
    $conf .= "\n";
    $conf .= "http_access allow manager ext_manager_".$count."\n";
    $count += 1;
	 }}

  $conf .= <<<EOD

http_access deny manager
http_access allow purge localhost
http_access deny purge
http_access deny !safeports
http_access deny CONNECT !sslports

# Always allow localhost connections
http_access allow localhost

EOD;

	return $conf;
}

function squid_resync_traffic() {
	global $config, $valid_acls;
	if(!is_array($valid_acls))
		return;
	$settings = $config['installedpackages']['squidtraffic']['config'][0];
	$conf = '';

  if (!empty($settings['quick_abort_min']) || ($settings['quick_abort_min']) == "0") $conf .= "quick_abort_min {$settings['quick_abort_min']} KB\n";
  if (!empty($settings['quick_abort_max']) || ($settings['quick_abort_max']) == "0") $conf .= "quick_abort_max {$settings['quick_abort_max']} KB\n";
  if (!empty($settings['quick_abort_pct'])) $conf .= "quick_abort_pct {$settings['quick_abort_pct']}\n";
  if (!empty($settings['range_offset_limit']) || ($settings['range_offset_limit']) == "0") $conf .= "range_offset_limit {$settings['range_offset_limit']} MB\n";

	$up_limit = ($settings['max_upload_size'] ? $settings['max_upload_size'] : 0);
	$down_limit = ($settings['max_download_size'] ? $settings['max_download_size'] : 0);
	$perbody = ($settings['bodymaxsize_throttling'] ? $settings['bodymaxsize_throttling'] : 0);
	$perpartial = ($settings['partialcontent_throttling'] ? $settings['partialcontent_throttling'] : 0);
	if (!empty($up_limit));
	$conf .= 'request_body_max_size ' . ($up_limit * 1024) . " allow all\n";
	if (!empty($down_limit));
	$conf .= 'reply_body_max_size ' . ($down_limit * 1024) . " deny all\n";

	// Only apply throttling past 10MB
	// XXX: Should this really be hardcoded?
	$threshold = 10 * 1024 * 1024;
	$overall = ($settings['overall_throttling'] ? $settings['overall_throttling'] : 0);
	if (!isset($overall) || ($overall == 0))
		$overall = -1;
	else
		$overall *= 1024;
	$perhost = ($settings['perhost_throttling'] ? $settings['perhost_throttling'] : 0);
	if (!isset($perhost) || ($perhost == 0))
		$perhost = -1;
	else {
		$perhost *= 1024;
		}
		$delay_pools = 1;
	if (!isset($perpartial) || ($perpartial == 0))
		$perpartial = -1;
	else {
		$delay_pools = 2;
		$perpartial *= 1024;
		}
	if ($settings['enable_delay_pool'] == 'on') {
	$conf .= <<<EOD

delay_pools $delay_pools
delay_class 1 2
delay_parameters 1 $overall/$overall $perhost/$perhost
delay_initial_bucket_level 100

EOD;
	if ($delay_pools == 2) {
	$conf .= <<<EOD

delay_class 2 1
delay_parameters 2 $perpartial/$perpartial

EOD;
	$bodymaxsize = "!partialcontent_req";
	}
	else 
	$bodymaxsize = "all";

	if(! empty($settings['unrestricted_hosts'])) {
		foreach (array('unrestricted_hosts') as $item) {
			if (in_array($item, $valid_acls))
				$conf .= "# Do not throttle unrestricted hosts\n";
				$conf .= "delay_access 1 deny $item\n";
		}
	}

	if ($settings['throttle_specific'] == 'on') {
		$exts = array();
		$binaries = 'bin,cab,sea,ar,arj,tar,tgz,gz,tbz,bz2,zip,7z,exe,com';
		$cdimages = 'iso,bin,mds,nrg,gho,bwt,b5t,pqi';
		$multimedia = 'aiff?,asf,avi,divx,mov,mp3,mp4,wmv,mpe?g,qt,ra?m';
		foreach (array(	'throttle_binaries' => $binaries,
				'throttle_cdimages' => $cdimages,
				'throttle_multimedia' => $multimedia) as $field => $set) {
			if ($settings[$field] == 'on')
				$exts = array_merge($exts, explode(",", $set));
		}

		foreach (explode(",", $settings['throttle_others']) as $ext) {
			if (!empty($ext)) $exts[] = $ext;
		}

		$contents = '';
		foreach ($exts as $ext)
			$contents .= "\.$ext\$\n";
		file_put_contents(SQUID_ACLDIR . '/throttle_exts.acl', $contents);

		$conf .= "# Throttle extensions matched in the url\n";
		$conf .= "acl throttle_exts urlpath_regex -i \"" . SQUID_ACLDIR . "/throttle_exts.acl\"\n";
		$conf .= "delay_access 1 allow throttle_exts\n";
		$conf .= "delay_access 1 deny all\n";
	}
	else
		$conf .= "delay_access 1 allow all\n";
		if ($perbody > 0)
		$conf .= 'delay_body_max_size ' . ($perbody * 1024) . " 1 allow $bodymaxsize \n";
		if (delay_pools == 2) {
		$conf .= "delay_access 2 allow partialcontent_req\n";
		$conf .= "delay_access 2 deny all\n";
		}
	}
	return $conf;
}

function squid_resync_auth() {
	global $config, $valid_acls;

	$settings = $config['installedpackages']['squidauth']['config'][0];
	$settingsnac = $config['installedpackages']['squidnac']['config'][0];
	$settingsconfig = $config['installedpackages']['squid']['config'][0];
	$conf = '';

	// Deny the banned guys before allowing the good guys
	if(! empty($settingsnac['banned_hosts'])) {
		if (squid_is_valid_acl('banned_hosts')) {
			$conf .= "# These hosts are banned\n";
			$conf .= "http_access deny banned_hosts\n";
		}
	}
	if(! empty($settingsnac['banned_macs'])) {
		if (squid_is_valid_acl('banned_macs')) {
			$conf .= "# These macs are banned\n";
			$conf .= "http_access deny banned_macs\n";
		}
	}

	// Unrestricted hosts take precendence over blacklist
	if(! empty($settingsnac['unrestricted_hosts'])) {
		if (squid_is_valid_acl('unrestricted_hosts')) {
			$conf .= "# These hosts do not have any restrictions\n";
			$conf .= "http_access allow unrestricted_hosts\n";
		}
	}
	if(! empty($settingsnac['unrestricted_macs'])) {
		if (squid_is_valid_acl('unrestricted_macs')) {
			$conf .= "# These hosts do not have any restrictions\n";
			$conf .= "http_access allow unrestricted_macs\n";
		}
	}

	// Whitelist and blacklist also take precendence over other allow rules
	if(! empty($settingsnac['whitelist'])) {
		if (squid_is_valid_acl('whitelist')) {
			$conf .= "# Always allow access to whitelist domains\n";
			$conf .= "http_access allow whitelist\n";
		}
	}
	if(! empty($settingsnac['blacklist'])) {
		if (squid_is_valid_acl('blacklist')) {
			$conf .= "# Block access to blacklist domains\n";
			$conf .= "http_access deny blacklist\n";
		}
	}

	if(!empty($config['installedpackages']['squid']['config'][0]['custom_options'])) {
		$custopts = explode(";", ($config['installedpackages']['squid']['config'][0]['custom_options']));
		$conf .= "# Custom options\n";
		foreach ($custopts as $custopt) {
			$conf .= $custopt."\n";
		}
		$conf .= "\n";
	}

	$transparent_proxy = ($settingsconfig['transparent_proxy'] == 'on');
	$auth_method = (($settings['auth_method'] && !$transparent_proxy) ? $settings['auth_method'] : 'none');
	// Allow the remaining ACLs if no authentication is set
	if ($auth_method == 'none') {
		if ($settingsconfig['allow_interface'] == 'on')
				$allowed = array('localnet', 'allowed_subnets');
		else 	$allowed = array('allowed_subnets');
			$conf .= "# Allow local network(s) on interface(s)\n";
			$allowed = array_filter($allowed, 'squid_is_valid_acl');
			foreach ($allowed as $acl)
				$conf .= "http_access allow $acl\n";
			$conf .= "\n";
	}
	else {
		$noauth = implode(' ', explode("\n", base64_decode($settings['no_auth_hosts'])));
		if (!empty($noauth)) {
			$conf .= "acl noauth src $noauth\n";
			$valid_acls[] = 'noauth';
		}

		// Set up the external authentication programs
		$auth_ttl = ($settings['auth_ttl'] ? $settings['auth_ttl'] : 60);
		$processes = ($settings['auth_processes'] ? $settings['auth_processes'] : 5);
		$prompt = ($settings['auth_prompt'] ? $settings['auth_prompt'] : 'Please enter your credentials to access the proxy');
		switch ($auth_method) {
			case 'local':
				$conf .= 'auth_param basic program /usr/local/libexec/squid/ncsa_auth ' . SQUID_PASSWD . "\n";
				break;
			case 'ldap':
				$port = (isset($settings['auth_port']) ? ":{$settings['auth_port']}" : '');
				$password = (isset($settings['ldap_pass']) ? "-w {$settings['ldap_pass']}" : '');
				$conf .= "auth_param basic program /usr/local/libexec/squid/squid_ldap_auth -v {$settings['ldap_version']} -b {$settings['ldap_basedomain']} -D {$settings['ldap_user']} $password -f \"{$settings['ldap_filter']}\" -u uid -P {$settings['auth_server']}$port\n";
				break;
			case 'radius':
				$port = (isset($settings['auth_port']) ? "-p {$settings['auth_server_port']}" : '');
				$conf .= "auth_param basic program /usr/local/libexec/squid/squid_radius_auth -w {$settings['radius_secret']} -h {$settings['auth_server']} $port\n";
				break;
			case 'msnt':
				$conf .= "auth_param basic program /usr/local/libexec/squid/msnt_auth\n";
				break;
		}
		$conf .= <<<EOD
auth_param basic children $processes
auth_param basic realm $prompt
auth_param basic credentialsttl $auth_ttl minutes
acl password proxy_auth REQUIRED

EOD;

		// Onto the ACLs
		$password = array('localnet', 'allowed_subnets');
		$passwordless = array('unrestricted_hosts');
		if ($settings['unrestricted_auth'] == 'on') {
			// Even the unrestricted hosts should authenticate
			$password = array_merge($password, $passwordless);
			$passwordless = array();
		}
		$passwordless[] = 'noauth';
		$password = array_filter($password, 'squid_is_valid_acl');
		$passwordless = array_filter($passwordless, 'squid_is_valid_acl');

		// Allow the ACLs that don't need to authenticate
		foreach ($passwordless as $acl)
			$conf .= "http_access allow $acl\n";

		// Allow the other ACLs as long as they authenticate
		foreach ($password as $acl)
			$conf .= "http_access allow password $acl\n";
	}

	$conf .= "# Default block all to be sure\n";
	$conf .= "http_access deny all\n";

	return $conf;
}

function squid_resync_users() {
	global $config;

	$users = $config['installedpackages']['squidusers']['config'];
	$contents = '';
	if (is_array($users)) {
		foreach ($users as $user)
			$contents .= $user['username'] . ':' . crypt($user['password'], base64_encode($user['password'])) . "\n";
	}
	file_put_contents(SQUID_PASSWD, $contents);
	chown(SQUID_PASSWD, 'proxy');
	chmod(SQUID_PASSWD, 0600);
}

function squid_resync() {
	global $config;
	conf_mount_rw();
	$conf = squid_resync_general() . "\n";
	$conf .= squid_resync_cache() . "\n";
	$conf .= squid_resync_redirector() . "\n";
	$conf .= squid_resync_upstream() . "\n";
	$conf .= squid_resync_nac() . "\n";
	$conf .= squid_resync_traffic() . "\n";
	$conf .= squid_resync_auth();
	squid_resync_users();

	/* make sure pinger is executable */
	if(file_exists("/usr/local/libexec/squid/pinger"))
		exec("chmod a+x /usr/local/libexec/squid/pinger");

	file_put_contents(SQUID_CONFBASE . '/squid.conf', $conf);

	$log_dir = ($config['installedpackages']['squid']['config'][0]['log_dir'] ? $config['installedpackages']['squid']['config'][0]['log_dir'] : '/var/squid/logs');

	if(!is_dir($log_dir.'/')) {
		log_error("Creating squid log dir $log_dir");
		make_dirs($log_dir);
		squid_chown_recursive($log_dir, 'proxy', 'proxy');
	}

	conf_mount_ro();
	/* if proxy_monitor.sh is not running then this is an .xml startup trying to sync all package */
	/* let the squid.sh starts the squid */
	if (is_process_running_full("proxy_monitor.sh")) {
		if (is_process_running_full("squid -D")) {
			mwexec("/usr/local/sbin/squid -k reconfigure");
			filter_configure();
			log_error("squid config synchronized and squid reconfigured");
		} else
		log_error("proxy_monitor running but no \"squid -D\"");
	} else
	log_error("squid config synchronized");

}

function squid_print_javascript_auth() {
	global $config;
	$transparent_proxy = ($config['installedpackages']['squid']['config'][0]['transparent_proxy'] == 'on');

	// No authentication for transparent proxy
	if ($transparent_proxy) {
		$javascript = <<<EOD
<script language="JavaScript">
<!--
function on_auth_method_changed() {
	document.iform.auth_method.disabled = 1;
	document.iform.auth_server.disabled = 1;
	document.iform.auth_server_port.disabled = 1;
	document.iform.ldap_user.disabled = 1;
	document.iform.ldap_version.disabled = 1;
	document.iform.ldap_filter.disabled = 1;
	document.iform.ldap_password.disabled = 1;
	document.iform.ldap_basedomain.disabled = 1;
	document.iform.radius_secret.disabled = 1;
	document.iform.msnt_secondary.disabled = 1;
	document.iform.auth_prompt.disabled = 1;
	document.iform.auth_processes.disabled = 1;
	document.iform.auth_ttl.disabled = 1;
	document.iform.unrestricted_auth.disabled = 1;
	document.iform.no_auth_hosts.disabled = 1;
}
-->
</script>

EOD;
	}
	else {
		$javascript = <<<EOD
<script language="JavaScript">
<!--
function on_auth_method_changed() {
	var field = document.iform.auth_method;
	var auth_method = field.options[field.selectedIndex].value;

	if (auth_method == 'none') {
		document.iform.auth_server.disabled = 1;
		document.iform.auth_server_port.disabled = 1;
		document.iform.ldap_user.disabled = 1;
		document.iform.ldap_version.disabled = 1;
		document.iform.ldap_filter.disabled = 1;
		document.iform.ldap_password.disabled = 1;
		document.iform.ldap_basedomain.disabled = 1;
		document.iform.radius_secret.disabled = 1;
		document.iform.msnt_secondary.disabled = 1;
		document.iform.auth_prompt.disabled = 1;
		document.iform.auth_processes.disabled = 1;
		document.iform.auth_ttl.disabled = 1;
		document.iform.unrestricted_auth.disabled = 1;
		document.iform.no_auth_hosts.disabled = 1;
	}
	else {
		document.iform.auth_prompt.disabled = 0;
		document.iform.auth_processes.disabled = 0;
		document.iform.auth_ttl.disabled = 0;
		document.iform.unrestricted_auth.disabled = 0;
		document.iform.no_auth_hosts.disabled = 0;
	}

	switch (auth_method) {
		case 'local':
			document.iform.auth_server.disabled = 1;
			document.iform.auth_server_port.disabled = 1;
			document.iform.ldap_user.disabled = 1;
			document.iform.ldap_password.disabled = 1;
			document.iform.ldap_version.disabled = 1;
			document.iform.ldap_filter.disabled = 1;
			document.iform.ldap_basedomain.disabled = 1;
			document.iform.radius_secret.disabled = 1;
			document.iform.msnt_secondary.disabled = 1;
			break;
		case 'ldap':
			document.iform.auth_server.disabled = 0;
			document.iform.auth_server_port.disabled = 0;
			document.iform.ldap_user.disabled = 0;
			document.iform.ldap_password.disabled = 0;
			document.iform.ldap_version.disabled = 0;
			document.iform.ldap_filter.disabled = 0;
			document.iform.ldap_basedomain.disabled = 0;
			document.iform.radius_secret.disabled = 1;
			document.iform.msnt_secondary.disabled = 1;
			break;
		case 'radius':
			document.iform.auth_server.disabled = 0;
			document.iform.auth_server_port.disabled = 0;
			document.iform.ldap_user.disabled = 1;
			document.iform.ldap_password.disabled = 1;
			document.iform.ldap_version.disabled = 1;
			document.iform.ldap_filter.disabled = 1;
			document.iform.ldap_basedomain.disabled = 1;
			document.iform.radius_secret.disabled = 0;
			document.iform.msnt_secondary.disabled = 1;
			break;
		case 'msnt':
			document.iform.auth_server.disabled = 0;
			document.iform.auth_server_port.disabled = 1;
			document.iform.ldap_user.disabled = 1;
			document.iform.ldap_password.disabled = 1;
			document.iform.ldap_version.disabled = 1;
			document.iform.ldap_filter.disabled = 1;
			document.iform.ldap_basedomain.disabled = 1;
			document.iform.radius_secret.disabled = 1;
			document.iform.msnt_secondary.disabled = 0;
			break;
	}
}
-->
</script>

EOD;
	}

	print($javascript);
}

function squid_print_javascript_auth2() {
	print("<script language=\"JavaScript\">on_auth_method_changed()</script>\n");
}

function squid_generate_rules($type) {
	global $config;

	$squid_conf = $config['installedpackages']['squid']['config'][0];
	if (!shell_exec("ps axwu | grep '(squid) -D' | grep -v 'grep'") != '') { /* is_service_running is not that reliable */
		return;
	}

	if ($squid_conf['transparent_proxy'] != 'on') {
		return;
	}

	$ifaces = explode(",", $squid_conf['active_interface']);
	$ifaces = array_map('convert_friendly_interface_to_real_interface_name', $ifaces);
	$port = ($squid_conf['proxy_port'] ? $squid_conf['proxy_port'] : 3128);

	$fw_aliases = filter_generate_aliases();
	if(strstr($fw_aliases, "pptp ="))
		$PPTP_ALIAS = "\$pptp";
	else
		$PPTP_ALIAS = "\$PPTP";
	if(strstr($fw_aliases, "PPPoE ="))
		$PPPOE_ALIAS = "\$PPPoE";
	else
		$PPPOE_ALIAS = "\$pppoe";
	switch($type) {
		case 'nat':
			$rules .= "\n# Setup Squid proxy redirect\n";
			if ($squid_conf['private_subnet_proxy_off'] == 'on') {
				foreach ($ifaces as $iface) {
					$rules .= "no rdr on $iface proto tcp from any to { 192.168.0.0/16, 172.16.0.0/12, 10.0.0.0/8 } port 80\n";
				}
			}
			if (!empty($squid_conf['defined_ip_proxy_off_dist'])) {
				$defined_ip_proxy_off_dist = explode(";", $squid_conf['defined_ip_proxy_off_dist']);
				$exempt_ip = "";
				foreach ($defined_ip_proxy_off_dist as $ip_proxy_off) {
					if(!empty($ip_proxy_off)) {
						$ip_proxy_off = trim($ip_proxy_off);
						$exempt_ip .= ", $ip_proxy_off";
					}
				}
				$exempt_ip = substr($exempt_ip,2);
				foreach ($ifaces as $iface) {
					$rules .= "no rdr on $iface proto tcp from any to { $exempt_ip } port 80\n";
				}
			}
			if (!empty($squid_conf['defined_ip_proxy_off'])) {
				$defined_ip_proxy_off = explode(";", $squid_conf['defined_ip_proxy_off']);
				$exempt_ip = "";
				foreach ($defined_ip_proxy_off as $ip_proxy_off) {
					if(!empty($ip_proxy_off)) {
						$ip_proxy_off = trim($ip_proxy_off);
						$exempt_ip .= ", $ip_proxy_off";
					}
				}
				$exempt_ip = substr($exempt_ip,2);
				foreach ($ifaces as $iface) {
					$rules .= "no rdr on $iface proto tcp from { $exempt_ip } to any port 80\n";
				}
			}
			foreach ($ifaces as $iface) {
			if ($squid_conf['proxy_port'] > 0) {
				$rules .= "rdr on $iface proto tcp from any to !($iface) port 80 -> $iface port $port\n";
			} else {
				$rules .= "rdr on $iface proto tcp from any to !($iface) port 80 -> 127.0.0.1 port 80\n";
			}
			}
			/* Handle PPPOE case */
			if($config['pppoe']['mode'] == "server" && $config['pppoe']['localip']) {
				$rules .= "rdr on $PPPOE_ALIAS proto tcp from any to !127.0.0.1 port 80 -> 127.0.0.1 port 80\n";
			}
			/* Handle PPTP case */
			if($config['pptpd']['mode'] == "server" && $config['pptpd']['localip']) {
				$rules .= "rdr on $PPTP_ALIAS proto tcp from any to !127.0.0.1 port 80 -> 127.0.0.1 port 80\n";
			}
			$rules .= "\n";
			break;
		case 'filter':
		case 'rule':
			foreach ($ifaces as $iface) {
				$rules .= "# Setup squid pass rules for proxy\n";
				$rules .= "#pass in quick on $iface proto tcp from any to !($iface) port 80 flags S/SA keep state\n";
				$rules .= "#pass in quick on $iface proto tcp from any to !($iface) port $port flags S/SA keep state\n";
				$rules .= "\n";
			};
			if($config['pppoe']['mode'] == "server" && $config['pppoe']['localip']) {
				$rules .= "pass in quick on $PPPOE_ALIAS proto tcp from any to !127.0.0.1 port $port flags S/SA keep state\n";
			}
			if($config['pptpd']['mode'] == "server" && $config['pptpd']['localip']) {
				$rules .= "pass in quick on $PPTP_ALIAS proto tcp from any to !127.0.0.1 port $port flags S/SA keep state\n";
			}
			break;
		default:
			break;
	}

	return $rules;
}

?>
