//global var
var REMOTE_API_URL = 'api.php';

//get player status
function get_player_status() {
    // just clear it first
    clearInterval(idTmrRefresh);

    // create new timer interval
    var isLoading = false;
    var idTmrRefresh = setInterval(
            function () {
                if (!isLoading) {
                    $.getJSON(
                            REMOTE_API_URL,
                            {
                                'c': 2
                            },
                    function (datax) {
                        //                                                    console.log(typeof(datax.p));
                        if (typeof (datax.p) !== 'undefined') {
                            $('#lbl-now-src').html(datax.p.local);
                            $('#lbl-now-ttl').html(datax.p.name);
                            $('#lbl-now-dur').html(datax.p.dur);

                            $('#lbl-next-src').html(datax.n.local);
                            $('#lbl-next-ttl').html(datax.n.name);
                            $('#lbl-next-dur').html(datax.n.dur);
                        }
                    }
                    )
                            .error(
                                    function (jqXHR, textStatus, errorThrown) {
                                        isLoading = false;
                                    }
                            );
                }
            },
            2000 //per 2000ms
            );
}

//get cpu status
function get_cpu_status() {
    // just clear it first
    clearInterval(idTmrCPUStatus);

    // create new timer interval
    var isLoadingCPUStatus = false;
    var idTmrCPUStatus = setInterval(
            function () {
                if (!isLoadingCPUStatus) {
                    $.getJSON(
                            REMOTE_API_URL,
                            {
                                'c': 8
                            },
                    function (datax) {
                        $('#lbl-cpu-status-temp').html('Temp: ' + datax.temp);
                        $('#lbl-cpu-status-volt').html('Volt: ' + datax.volt);
                        $('#lbl-cpu-status-min-speed').html('Min. Speed: ' + datax.min_speed);
                        $('#lbl-cpu-status-max-speed').html('Max. Speed: ' + datax.max_speed);
                        $('#lbl-cpu-status-curr-speed').html('Curr. Speed: ' + datax.curr_speed);
                        $('#lbl-cpu-status-gov').html('Governor: ' + datax.governor);
                    }
                    )
                            .error(
                                    function (jqXHR, textStatus, errorThrown) {
                                        isLoadingCPUStatus = false;
                                    }
                            );
                }
            },
            2000 //per 2000ms
            );
}

//init spin control
function init_spin_control() {
    $("#txt-web-port, #txt-ftp-port").TouchSpin(
            {
                initval: 46789,
                min: 1,
                max: 65535,
                step: 1,
                decimals: 0,
                boostat: 5,
                maxboostedstep: 10
            }
    );

    $("#txt-ftp-interval").TouchSpin(
            {
                initval: 1,
                min: 1,
                max: 60,
                step: 1,
                decimals: 0,
                boostat: 5,
                maxboostedstep: 10
            }
    );
}

// get server config status
function get_server_info_action() {
    //reset the form
    $('#frm-general')[0].reset();
    $('#frm-network')[0].reset();
    $('#frm-ftp')[0].reset();
    $('#frm-dyn-dns')[0].reset();
	$('#frm-video-stream')[0].reset();
	$('#frm-xkey')[0].reset();

    //disable btn
    $('#btn-reboot, #btn-save').removeClass('disabled').addClass('disabled');

    $('#sp-config-status').html('<span class="glyphicon glyphicon-refresh"></span> Get server info. Please wait...')
            .removeClass('label-success')
            .addClass('label-warning');
    $.getJSON(
            REMOTE_API_URL,
            {
                'c': 1
            },
    function (datax) {
        //enable btn
        $('#btn-reboot, #btn-save').toggleClass('disabled');

//                            console.log(datax);
        $('#sp-config-status').html('<span class="glyphicon glyphicon-ok"></span> Server info received')
                .removeClass('label-warning')
                .addClass('label-success');

        //device info
        if (typeof (datax.device) !== 'undefined') {
//                                console.log(datax.device);
            $obj = datax.device;
            $('#txt-device-name').val($obj.name);
            $('#sel-timezone').val($obj.timezone);
            $('#txt-web-password').val($obj.password);
			$('#cb-limited-services').prop("checked", ($obj.limit_svc==0) ? false:true);
        }

        //network info
        if (typeof (datax.network) !== 'undefined') {
//                                console.log(datax.device);
            $obj = datax.network;
            $('#txt-public-ip').val($obj.public_ip);
            $('#txt-local-ip').val($obj.local_ip);
            $('#txt-dns').val($obj.dns);
            $('#txt-gateway').val($obj.gateway);
            $('#txt-web-port').val($obj.web_port);
        }

        //ftp info
        if (typeof (datax.ftp) !== 'undefined') {
//                                console.log(datax.device);
            $obj = datax.ftp;
            $('#txt-ftp-server').val($obj.server);
            $('#txt-ftp-port').val($obj.port);
            $('#txt-ftp-username').val($obj.username);
            $('#txt-ftp-password').val($obj.password);
            $('#txt-ftp-interval').val($obj.interval);
        }

        //dyndns info
        if (typeof (datax.dyndns) !== 'undefined') {
//                                console.log(datax.device);
            $obj = datax.dyndns;
            $('#txt-dyndns-username').val($obj.username);
            $('#txt-dyndns-password').val($obj.password);
            $('#txt-dyndns-url').val($obj.url);
        }
		//videostreaming info
        if (typeof (datax.videostreaming) !== 'undefined') {
//                                console.log(datax.device);
            $obj = datax.videostreaming;
            $('#txt-video-stream-url').val($obj.video_stream_url);
            $('#cb-video-stream-on').prop("checked" , ($obj.video_stream_on==0) ? false:true);
        }
		//xkey cmd info
        if (typeof (datax.xkey) !== 'undefined') {
//                                console.log(datax.device);
            $obj = datax.xkey;
            $('#txt-xkey-btn1').val($obj[0]);
            $('#txt-xkey-btn2').val($obj[1]);
			$('#txt-xkey-btn3').val($obj[2]);
			$('#txt-xkey-btn4').val($obj[3]);
        }
    }
    )
            .error(
                    function () {
                        //enable btn
                        $('#btn-reboot, #btn-save').toggleClass('disabled');
                        $('#sp-config-status').html('<span class="glyphicon glyphicon-remove"></span> Error getting server info')
                                .removeClass('label-warning')
                                .addClass('label-danger');
                    }
            );
}

function get_server_config_info() {
    $("#win-configuration").on(
            'shown.bs.modal',
            function () {
                get_server_info_action();
            }
    );
}

//btn config action
function config_action_handler() {
    $('#btn-config').click(
            function () {
                bootbox.prompt({
                    size: 'small',
                    title: 'Password',
                    inputType: "password",
                    buttons: {
                        confirm: {
                            label: 'Submit'
                        }
                    },
                    callback: function (result) {
                        if ((result != null) && (result != '')) {
                            bootbox.alert({
                                message: '<span class="glyphicon glyphicon-refresh"></span> Verify, please wait...',
                                size: 'small'
                            });

                            $.get(
                                    REMOTE_API_URL,
                                    {
                                        'c': 6,
                                        'p': encodeURI(result)
                                    },
                            function (datax) {
                                bootbox.hideAll();
                                if (parseInt(datax) == 1) {
                                    $('#win-configuration').modal('show');
                                }
                                else {
                                    bootbox.alert({
                                        message: '<span class="glyphicon glyphicon-warning-sign"></span> Invalid password!!!',
                                        size: 'small'
                                    });
                                }
                            }
                            );
                        }
                    }
                }
                );
            }
    );
}

//reboot action
function reboot_handler() {
    $('#btn-reboot').click(
            function () {
                bootbox.confirm(
                        {
                            title: 'ENCO',
                            message: 'Reboot the system?',
                            size: 'small',
                            callback:
                                    function (result) {
                                        if (result) {
                                            $.post(
                                                    REMOTE_API_URL,
                                                    {
                                                        'c': 7
                                                    }
                                            );
                                        }
                                    }
                        }
                );
            }
    );
}

//save config action
function save_config_handler() {
    $('#btn-save').click(
            function () {
                //form the data
                var $data_general = $('#frm-general').serialize();
                var $data_network = $('#frm-network').serialize();
                var $data_ftp = $('#frm-ftp').serialize();
                var $data_dyn_dns = $('#frm-dyn-dns').serialize();
				var $data_video_streaming = $('#frm-video-stream').serialize();
				var $data_xkey = $('#frm-xkey').serialize();
                var $data = 'c=5&' + $data_general + '&' + $data_network + '&' + $data_ftp + '&' + $data_dyn_dns + '&' + $data_video_streaming + '&' + $data_xkey;

//                console.log($data_general);
//                console.log($data_network);
//                console.log($data_ftp);
//                console.log($data);

                //disable btn
                $('#btn-save').removeClass('disabled').addClass('disabled');

                $('#sp-config-status').html('<span class="glyphicon glyphicon-refresh"></span> Saving setting. Please wait...')
                        .removeClass('label-success')
                        .addClass('label-warning');

                //send the data
                $.getJSON(
                        REMOTE_API_URL,
                        $data,
                        function (datax) {
                            //enable btn
                            $('#btn-save').removeClass('disabled');

                            $('#sp-config-status').html('<span class="glyphicon glyphicon-ok"></span> Settings saved')
                                    .removeClass('label-warning')
                                    .addClass('label-success');

                            if (typeof (datax.err) !== 'undefined') {
                                if (parseInt(datax.err) === 0) {
                                    bootbox.alert({
                                        message: '<span class="glyphicon glyphicon-ok"></span> Setting save successfully',
                                        size: 'small',
                                        callback: function () {
                                            get_server_info_action();
                                        }
                                    });
                                }
                                else {
                                    bootbox.alert({
                                        message: '<span class="glyphicon glyphicon-warning-sign"></span> Error saving settings :<br>' + datax.msg,
                                        size: 'small'
                                    });
                                }
                            }
                            else {
                                bootbox.alert({
                                    message: '<span class="glyphicon glyphicon-warning-sign"></span> Invalid respons!!!',
                                    size: 'small'
                                });
                            }
                        }
                )
                        .error(
                                function () {
                                    //enable btn
                                    $('#btn-save').toggleClass('disabled');
                                    $('#sp-config-status').html('<span class="glyphicon glyphicon-remove"></span> Error saving settings')
                                            .removeClass('label-warning')
                                            .addClass('label-danger');
                                }
                        );
            }
    );
}

//main program
$(document).ready(
        function () {
            //all ajax timeout is set to 30secs
            $.ajaxSetup({timeout: 30000});

            // welcome message
//            bootbox.alert({
//                message: 'Welcome to ENCO System',
//                title: 'ENCO',
//                size: 'small'
//            });

            // init control
            init_spin_control();

            //get player status
            get_player_status();

            //get cpu stat
            get_cpu_status();

            //get config status
            get_server_config_info();

            //action config handler
            config_action_handler();

			//handler to reboot system
            reboot_handler();

			//handler to button save config
            save_config_handler();
        }
);