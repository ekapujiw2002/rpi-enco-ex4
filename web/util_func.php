<?php

/* get hostname */

function get_hostname() {
    @exec("hostname", $out);
    return $out[0];
}

/**
 * Timezones list with GMT offset
 * http://www.pontikis.net/tip/?id=24
 * @return array
 * @link http://stackoverflow.com/a/9328760
 */
function tz_list() {
    $zones_array = array();
    $timestamp = time();
    foreach (timezone_identifiers_list() as $key => $zone) {
        date_default_timezone_set($zone);
        $zones_array[$key]['zone'] = $zone;
        $zones_array[$key]['diff_from_GMT'] = 'UTC/GMT ' . date('P', $timestamp);
    }
    return $zones_array;
}

/* generate timezone list */

function generate_option_timezone_list() {
    $res = "";

    foreach (tz_list() as $t) {
        $res .= "<option value=\"{$t['zone']}\">{$t['diff_from_GMT']} - {$t['zone']}</option>";
    }

    return $res;
}

/* generate port number */

function generate_port_lists() {
    $res = "";
    for ($i = 0; $i <= 65535; $i++) {
        $res .= "<option value=$i>$i</option>";
    }

    return $res;
}

/* get network info */

function get_network_info() {
    $res = array();

    /* current server datetime */
    $res['datetime'] = date("d-m-Y H:i:s");

    /* public ip */
    @exec("curl --connect-timeout 10  -s bot.whatismyipaddress.com", $out);
    $res['public_ip'] = empty($out[0]) ? "0.0.0.0" : $out[0];

    /* local ip */
    $out = NULL;
    @exec("ifconfig eth0 | grep inet | awk '{print $2}'", $out);
    $res['local_ip'] = empty($out[0]) ? "0.0.0.0" : $out[0];

    /* gateway */
    $out = NULL;
    @exec("route -n | grep 'UG[ \t]' | head -n 1 | awk '{print $2}'", $out);
    $res['gateway'] = empty($out[0]) ? "0.0.0.0" : $out[0];

    /* dns */
    $out = NULL;
    @exec("cat /etc/resolv.conf | grep nameserver | head -n 1 | awk '{print $2}'", $out);
    $res['dns'] = empty($out[0]) ? "0.0.0.0" : $out[0];

    /* web port */
    $out = NULL;
    @exec("cat /etc/httpd/conf/httpd.conf | grep '^Listen' | awk '{print $2}'", $out);
    $res['web_port'] = intval($out[0]);

    return $res;
}

/* get ftp server setting */

function get_ftp_service() {
    $res = array();

    $WEBDAV_CFG_FILE = "/etc/enstreamer/enstreamer.conf";

    /* current server datetime */
    $res['datetime'] = date("d-m-Y H:i:s");

    $out = NULL;
    @exec("grep -i 'host' $WEBDAV_CFG_FILE | awk -F= '{print $2}'", $out);
    $res['server'] = empty($out[0]) ? "0.0.0.0" : $out[0];

    $out = NULL;
    @exec("grep -i 'port' $WEBDAV_CFG_FILE | awk -F= '{print $2}'", $out);
    $res['port'] = intval($out[0]);

    $out = NULL;
    @exec("grep -i 'user' $WEBDAV_CFG_FILE | awk -F= '{print $2}'", $out);
    $res['username'] = empty($out[0]) ? "" : $out[0];

    $out = NULL;
    @exec("grep -i 'password' $WEBDAV_CFG_FILE | awk -F= '{print $2}'", $out);
    $res['password'] = empty($out[0]) ? "" : $out[0];

    $out = NULL;
    @exec("grep -i 'interval' $WEBDAV_CFG_FILE | awk -F= '{print $2}'", $out);
    $res['interval'] = intval($out[0]);

    return $res;
}

/* get dev name and timezone */

function get_device_name_timezone() {
    $res = array();

    @exec("hostname", $out);
    $res['name'] = $out[0];

    $out = NULL;
    @exec("readlink /etc/localtime | sed \"s/..\/usr\/share\/zoneinfo\///\"", $out);
    $res['timezone'] = $out[0];

    $res['password'] = trim(get_web_password());

    return $res;
}

/* get player status */

function get_player_status() {
    $sts = file_get_contents("/dev/shm/enstreamer-status");

    if ($sts !== FALSE) {
        return $sts;
    } else {
        return json_encode(array());
    }
}

/* get password */

function get_web_password() {
    //return file_get_contents("/data/media/www/password.cfg");
    $res = get_ftp_service();
    return trim($res['password']);
}

/* get dyndns setting */

function get_dyn_dns_settings() {
    $DYN_DNS_CFG_FILE = "/etc/ddclient/ddclient.conf";
    $res = array();

    //user
    $outx = null;
    @exec("sudo grep '^login=' $DYN_DNS_CFG_FILE | awk -F= '{print $2}'", $outx);
    $res['username'] = $outx[0];

    //pass
    $outx = null;
    @exec("sudo grep '^password=' $DYN_DNS_CFG_FILE | awk -F= '{print $2}'", $outx);
    $res['password'] = $outx[0];

    //url
    $outx = null;
    @exec("sudo grep -1 '^password=' $DYN_DNS_CFG_FILE | tail -n 1", $outx);
    $res['url'] = $outx[0];

    return $res;
}

/* get cpustatus */

function get_cpu_status() {
    @exec("sudo cpustatus", $out);
    $arr_data = explode(":", implode(":", $out));
    return array(
        'temp' => trim($arr_data[1]),
        'volt' => trim($arr_data[3]),
        'min_speed' => trim($arr_data[5]),
        'max_speed' => trim($arr_data[7]),
        'curr_speed' => trim($arr_data[9]),
        'governor' => trim($arr_data[11])
    );
}

/* set system root ke rw mode */

function root_rw_mode() {
    $partnum = 5;
    @exec("if [[ $(mount | grep -i -w -c \"/dev/mmcblk0p$partnum.*(rw.*\") == 0 ]] ; then sudo mount / -o remount,rw > /dev/null 2>&1 ; echo $? ; else echo 0 ; fi", $outx);
    return (intval($outx[0]) == 0);
}

/* set system root ke ro mode */

function root_ro_mode() {
    $partnum = 5;
    @exec("if [[ $(mount | grep -i -w -c \"/dev/mmcblk0p$partnum.*(ro.*\") == 0 ]] ; then sudo mount / -o remount,r0 > /dev/null 2>&1 ; echo $? ; else echo 0 ; fi", $outx);
    return (intval($outx[0]) == 0);
}

/* set hostname */

function set_hostname($devname) {
    @exec("sudo hostnamectl set-hostname $devname > /dev/null 2>&1 ; echo $?", $outx);
    $result = intval($outx[0]);
//    if($result==0){
//        //generate new dyndns url and username
//        $old_dyndns = get_dyn_dns_settings();
//        $new_dyndns_url = "$devname.encoip.com";
//        $new_dyndns_username = "$devname:encoip";
//        
//        //update it
//        $result = set_dyndns_settings($new_dyndns_username, $old_dyndns['password'], $new_dyndns_url);
//    }
    return ($result==0);
}

/* set timezone */

function set_timezone($timezone) {
    @exec("sudo timedatectl set-timezone \"$timezone\" > /dev/null 2>&1 ; echo $?", $outx);
    return (intval($outx[0]) == 0);
}

/* set local ip */

function set_local_ip($loc_ip) {
    $IP_CFG_FILE = "/etc/systemd/network/eth0.network";
    @exec("sudo sed -i \"s|^Address=.*|Address=$loc_ip/24|\" $IP_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
    return (intval($outx[0]) == 0);
}

/* set dns */

function set_dns($dns_old, $dns_new) {
    $IP_CFG_FILE = "/etc/systemd/network/eth0.network";
    @exec("sudo sed -i \"s|^DNS=$dns_old.*|DNS=$dns_new|\" $IP_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
    return (intval($outx[0]) == 0);
}

/* set gateway */

function set_gateway($gw_new) {
    $IP_CFG_FILE = "/etc/systemd/network/eth0.network";
    @exec("sudo sed -i \"s|^Gateway=.*|Gateway=$gw_new|\" $IP_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
    return (intval($outx[0]) == 0);
}

/* set web port */

function set_web_port($web_port_new) {
    $HTTP_CFG_FILE = "/etc/httpd/conf/httpd.conf";
    @exec("sudo sed -i \"s|^Listen.*|Listen $web_port_new|\" $HTTP_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
    return (intval($outx[0]) == 0);
}

/* set ftp setting */

function set_ftp_setting($ftp_server, $ftp_port, $ftp_user, $ftp_pass, $ftp_interval) {
    $WEBDAV_CFG_FILE = "/etc/enstreamer/enstreamer.conf";

    $ftp_old_set = get_ftp_service();

    //update server
    if ($ftp_server != $ftp_old_set['server']) {
        $outx = null;
        @exec("sudo sed -i \"s|^host=.*|host=$ftp_server|\" $WEBDAV_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
        if (intval($outx[0]) != 0) {
            return 1;
        }
    }

    //update port
    if ($ftp_port !== $ftp_old_set['port']) {
        $outx = null;
        @exec("sudo sed -i \"s|^port=.*|port=$ftp_port|\" $WEBDAV_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
        if (intval($outx[0]) != 0) {
            return 2;
        }
    }

    //update username
    if ($ftp_user != $ftp_old_set['username']) {
        $outx = null;
        @exec("sudo sed -i \"s|^user=.*|user=$ftp_user|\" $WEBDAV_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
        if (intval($outx[0]) != 0) {
            return 3;
        }
    }

    //update password
    if ($ftp_pass != $ftp_old_set['password']) {
        $outx = null;
        @exec("sudo sed -i \"s|^password=.*|password=$ftp_pass|\" $WEBDAV_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
        if (intval($outx[0]) != 0) {
            return 4;
        }
    }

    //update interval
    if ($ftp_interval !== $ftp_old_set['interval']) {
        $outx = null;
        @exec("sudo sed -i \"s|^interval=.*|interval=$ftp_interval|\" $WEBDAV_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
        if (intval($outx[0]) != 0) {
            return 5;
        }
    }

    return 0;
}

/* set dyndns setting */

function set_dyndns_settings($username, $password, $url) {
    $DDCLIENT_CFG_FILE = "/etc/ddclient/ddclient.conf";

    $dyndns_old = get_dyn_dns_settings();

    //update user
    if ($username !== $dyndns_old['username']) {
        $outx = NULL;
        @exec("sudo sed -i \"s|^login=.*|login=$username|\" $DDCLIENT_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
        if (intval($outx[0]) != 0) {
            return 1;
        }
    }

    //update pass
    if ($password !== $dyndns_old['password']) {
        $outx = NULL;
        @exec("sudo sed -i \"s|^password=.*|password=$password|\" $DDCLIENT_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
        if (intval($outx[0]) != 0) {
            return 2;
        }
    }

    //update url
    if ($url !== $dyndns_old['url']) {
        $outx = NULL;
        @exec("sudo sed -i \"s|^" . $dyndns_old['url'] . "|$url|\" $DDCLIENT_CFG_FILE > /dev/null 2>&1 ; echo $?", $outx);
        if (intval($outx[0]) != 0) {
            return 3;
        }
    }

    return 0;
}
