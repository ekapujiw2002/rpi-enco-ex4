<?php

include_once './util_func.php';

// command define
define("CMD_GET_SERVER_SETTING", 1);
define("CMD_GET_PLAYER_STATUS", 2);
define("CMD_SET_HOSTNAME", 3);
define("CMD_SET_TIMEZONE", 4);
define("CMD_SAVE_SETTING", 5);
define("CMD_VERIFY_PASSWORD", 6);
define("CMD_REBOOT", 7);
define("CMD_GET_CPU_STATUS", 8);


/* get cmd */
$cmd = intval($_REQUEST['c']);

// process cmd
switch ($cmd) {
    case CMD_GET_SERVER_SETTING:
        echo json_encode(
		array(
		'network' => get_network_info(), 
		'ftp' => get_ftp_service(), 
		'device' => get_device_name_timezone(), 
		'dyndns' => get_dyn_dns_settings(), 
		'videostreaming' => get_video_stream_setting(),
		'xkey' => get_xkey_cmd_setting()
		), 
		JSON_NUMERIC_CHECK);
        break;

    case CMD_GET_PLAYER_STATUS:
        echo get_player_status();
        break;
    
    case CMD_GET_CPU_STATUS:
        echo json_encode(get_cpu_status(), JSON_NUMERIC_CHECK);
        break;

    case CMD_SET_HOSTNAME:
        $hostname = $_REQUEST['h'];
        if (empty($hostname))
            echo 1;
        else {
            @exec("sudo /usr/bin/hostnamectl set-hostname $h > /dev/null 2>&1 ; echo $?", $out);
            echo (empty($out[0]) ? 1 : intval($out[0]));
        }
        break;

    case CMD_SET_TIMEZONE:
        $tz = $_REQUEST['tz'];
        if (empty($tz))
            echo 1;
        else {
            @exec("sudo /usr/bin/timedatectl set-timezone \"$tz\" > /dev/null 2>&1 ; echo $?", $out);
            echo (empty($out[0]) ? 1 : intval($out[0]));
        }
        break;

    case CMD_SAVE_SETTING:
        // get all value
        $dev_name = $_REQUEST['txt-device-name'];
        $tz = $_REQUEST['sel-timezone'];
        $web_password = $_REQUEST['txt-web-password'];
		$limit_svc = ($_REQUEST['cb-limited-services']=="on") ? 1:0;
        $local_ip = $_REQUEST['txt-local-ip'];
        $dns = $_REQUEST['txt-dns'];
        $gateway = $_REQUEST['txt-gateway'];
        $web_port = intval($_REQUEST['txt-web-port']);
        $ftp_server = $_REQUEST['txt-ftp-server'];
        $ftp_port = $_REQUEST['txt-ftp-port'];
        $ftp_user = $_REQUEST['txt-ftp-username'];
        $ftp_pass = $_REQUEST['txt-ftp-password'];
        $ftp_interval = $_REQUEST['txt-ftp-interval'];

//        $dyndns_username = $_REQUEST['txt-dyndns-username'];
//        $dyndns_url = $_REQUEST['txt-dyndns-url'];
        $dyndns_username = "$dev_name%23encoip";
        $dyndns_url = "$dev_name.encoip.com";
        $dyndns_password = $_REQUEST['txt-dyndns-password'];
		// vide streaming
		$vidstream_url = $_REQUEST['txt-video-stream-url'];
		$vidstream_on = ($_REQUEST['cb-video-stream-on']=="on") ? 1:0;
		// command button
		$xkey_btn = array(
		$_REQUEST['txt-xkey-btn1'],
		$_REQUEST['txt-xkey-btn2'],
		$_REQUEST['txt-xkey-btn3'],
		$_REQUEST['txt-xkey-btn4']
		);

        $resx = array(
            'err' => 0,
            'msg' => ''
        );

        //step by step compare and save the config
        //1. make the system rw
        if (root_rw_mode()) {

            //2. update dev name
            $devtz = get_device_name_timezone();
            if ($dev_name != $devtz['name']) {
                if (!set_hostname($dev_name)) {
                    $resx = array(
                        'err' => 2,
                        'msg' => 'Cannot set hostname'
                    );
                }
            }

            //3. set timezone
            if ($resx['err'] == 0) {
                if ($tz != $devtz['timezone']) {
                    if (!set_timezone($tz)) {
                        $resx = array(
                            'err' => 3,
                            'msg' => 'Cannot set timezone'
                        );
                    }
                }
            }

            //4. set local ip
            $netw_info = get_network_info();
            if ($resx['err'] == 0) {
                if ($local_ip != $netw_info['local_ip']) {
                    if (!set_local_ip($local_ip)) {
                        $resx = array(
                            'err' => 4,
                            'msg' => 'Cannot set local ip'
                        );
                    }
                }
            }

            //5. set dns
            if ($resx['err'] == 0) {
                if ($dns != $netw_info['dns']) {
                    if (!set_dns($netw_info['dns'], $dns)) {
                        $resx = array(
                            'err' => 5,
                            'msg' => 'Cannot set DNS'
                        );
                    }
                }
            }

            //6. set gateway
            if ($resx['err'] == 0) {
                if ($gateway != $netw_info['gateway']) {
                    if (!set_gateway($gateway)) {
                        $resx = array(
                            'err' => 6,
                            'msg' => 'Cannot set gateway'
                        );
                    }
                }
            }

            //7. set web port
            if ($resx['err'] == 0) {
                if (intval($web_port) != $netw_info['web_port']) {
                    if (!set_web_port(intval($web_port))) {
                        $resx = array(
                            'err' => 7,
                            'msg' => 'Cannot set web port'
                        );
                    }
                }
            }

            //8. set ftp
            if ($resx['err'] == 0) {
                if (set_ftp_setting($ftp_server, $ftp_port, $ftp_user, $ftp_pass, $ftp_interval) != 0) {
                    $resx = array(
                        'err' => 8,
                        'msg' => 'Cannot update FTP setting'
                    );
                }
            }

            //9. set dyndns
            if ($resx['err'] == 0) {
                if (set_dyndns_settings($dyndns_username, $dyndns_password, $dyndns_url) != 0) {
                    $resx = array(
                        'err' => 9,
                        'msg' => 'Cannot update dynamic dns setting'
                    );
                }
            }
			//10. set limit service
            if ($resx['err'] == 0) {
                if ( set_limit_svc_setting($limit_svc) != 0) {
                    $resx = array(
                        'err' => 10,
                        'msg' => 'Cannot update service limit'
                    );
                }
            }
			//11. set video setting
            if ($resx['err'] == 0) {
                if ( set_video_streaming_setting($vidstream_url, $vidstream_on) != 0) {
                    $resx = array(
                        'err' => 11,
                        'msg' => 'Cannot update video streaming'
                    );
                }
            }
			//12. set xkey cmd button
			if ($resx['err'] == 0) {
                if ( set_xkey_cmd_setting($xkey_btn) != 0) {
                    $resx = array(
                        'err' => 12,
                        'msg' => 'Cannot update command button list'
                    );
                }
            }
        } else {
            $resx['err'] = 1;
            $resx['msg'] = 'Cannot set system to read-write mode';
        }

        //make ro back
        //root_ro_mode();

        echo json_encode($resx, JSON_NUMERIC_CHECK);

        break;

    case CMD_VERIFY_PASSWORD:
        $pw = $_REQUEST['p'];
        $ky = trim(get_web_password());
        echo (!empty($pw)) ? ((md5($pw) == md5($ky)) ? 1 : 0) : 0;
        break;

    case CMD_REBOOT:
        @exec("sudo /usr/bin/reboot now");
        break;

    case 100:
        print_r(get_cpu_status());
        break;

    default:
        echo json_encode($_REQUEST, JSON_NUMERIC_CHECK);
        break;
}