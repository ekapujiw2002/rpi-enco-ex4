<?php

include_once './util_func.php';

// command define
define("CMD_GET_SERVER_SETTING", 1);
define("CMD_GET_PLAYER_STATUS", 2);

/* get cmd */
$cmd = intval($_REQUEST['c']);

// process cmd
switch ($cmd) {
    case CMD_GET_SERVER_SETTING:
        echo json_encode(array('network' => get_network_info(), 'ftp' => get_ftp_service(), 'device' => get_device_name_timezone()), JSON_NUMERIC_CHECK);
        break;
    
    case CMD_GET_PLAYER_STATUS:
        echo get_player_status();
        break;

    default:
        break;
}