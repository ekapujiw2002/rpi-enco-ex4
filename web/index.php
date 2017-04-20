<?php
include_once './util_func.php';
?>

<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <title>ENCO System - <?php echo $hostname; ?></title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="stylesheet" type="text/css" href="script/bootstrap/css/bootstrap.min.css">
        <link rel="stylesheet" type="text/css" href="script/bootstrap/css/bootstrap-theme.min.css">
        <link rel="stylesheet" type="text/css" href="script/touchspin/jquery.bootstrap-touchspin.min.css">
        <style>
            body{
                background-image: url(images/ENCO.png);
                background-attachment: fixed;
                background-size: cover;
                background-repeat: no-repeat;
                background-position: center center;
                /*                color: #ffffff;*/
            }

            .info-box{
                color: #000000;
            }

            .panel-font{
                font-size: 16px;
            }

            .control-label{
                text-align: left!important;
            }
			.panel-body  {
				word-break:break-all
			}
        </style>
    </head>
    <body>
        <div class="container-fluid">
            <!-- header -->
            <div class="row" style="margin-bottom: 50px;">
                <div class="col-sm-12 text-center">
                    <h1 style="color: #ffffff;">ENCO Systems<br><?php echo get_hostname(); ?></h1>
                </div>    
            </div>

            <!-- info -->
            <div class="row info-box">
                <div class="col-sm-1"></div>
                <div class="col-sm-5">
                    <div class="panel panel-info">
                        <div class="panel-heading text-center"><h3>NOW PLAYING</h3></div>
                        <div class="panel-body panel-font">
                            <div class="row">
                                <label class="col-sm-3">Source</label><label class="col-sm-9" id="lbl-now-src">?</label>
                            </div>
                            <div class="row">
                                <label class="col-sm-3">Title</label><label class="col-sm-9" id="lbl-now-ttl">?</label>
                            </div>
                            <div class="row">
                                <label class="col-sm-3">Duration</label><label class="col-sm-9" id="lbl-now-dur">?</label>
                            </div>
                        </div>
                    </div>
                </div>
                <div class="col-sm-5">
                    <div class="panel panel-info">
                        <div class="panel-heading text-center"><h3>UP NEXT</h3></div>
                        <div class="panel-body panel-font">
                            <div class="row">
                                <label class="col-sm-3">Source</label><label class="col-sm-9" id="lbl-next-src">?</label>
                            </div>
                            <div class="row">
                                <label class="col-sm-3">Title</label><label class="col-sm-9" id="lbl-next-ttl">?</label>
                            </div>
                            <div class="row">
                                <label class="col-sm-3">Duration</label><label class="col-sm-9" id="lbl-next-dur">?</label>
                            </div>
                        </div>
                    </div>
                </div>
                <div class="col-sm-1"></div>
            </div>

            <!-- cpustatus -->
            <div class="row">
                <div class="col-sm-10 col-sm-offset-1">
                    <div class="panel panel-info">
                        <div class="panel-heading text-center"><b>CPU STATUS</b></div>
                        <div class="panel-body panel-font">
                            <span id="lbl-cpu-status-temp" class="label label-primary">Temp: ?</span>
                            <span id="lbl-cpu-status-volt" class="label label-primary">Volt: ?</span>
                            <span id="lbl-cpu-status-min-speed" class="label label-primary">Min. Speed: ?</span>
                            <span id="lbl-cpu-status-max-speed" class="label label-primary">Max. Speed: ?</span>
                            <span id="lbl-cpu-status-curr-speed" class="label label-primary">Curr. Speed: ?</span>
                            <span id="lbl-cpu-status-gov" class="label label-primary">Governor: ?</span>
                        </div>
                    </div>
                </div>
            </div>

            <!-- button-->
            <div class="row">
                <div class="col-sm-5 col-sm-offset-1">
                    <!--
                    <button class="btn btn-primary btn-lg" data-toggle="modal" data-target="#win-configuration"><span class="glyphicon glyphicon-cog"></span></button>     
                    -->
                    <button id="btn-config" class="btn btn-primary btn-lg"><span class="glyphicon glyphicon-cog"></span></button>
                </div>
            </div>
        </div>

        <!-- configuration windows -->
        <div id="win-configuration" class="modal fade" data-backdrop="static">
            <div class="modal-dialog modal-lg">
                <div class="modal-content">
                    <div class="modal-header">
                        <button type="button" class="close" data-dismiss="modal" aria-hidden="true">&times;</button>
                        <h4 class="modal-title"><span class="glyphicon glyphicon-wrench"></span> enStreamer Configuration</h4>
                    </div>
                    <div class="modal-body">

                        <!-- tab nav -->
                        <ul class="nav nav-tabs">
                            <li class="active"><a href="#tab-general" data-toggle="tab"><span class="glyphicon glyphicon-home"></span> General</a></li>
                            <li><a href="#tab-network" data-toggle="tab"><span class="glyphicon glyphicon-random"></span> Network</a></li>
                            <li><a href="#tab-ftp" data-toggle="tab"><span class="glyphicon glyphicon-folder-open"></span> FTP</a></li>
                            <li><a href="#tab-dyn-dns" data-toggle="tab"><span class="glyphicon glyphicon-cloud"></span> Dyn. DNS</a></li>
							<li><a href="#tab-video-stream" data-toggle="tab"><span class="glyphicon glyphicon-film"></span> Video Streaming</a></li>
							<li><a href="#tab-xkey" data-toggle="tab"><span class="glyphicon glyphicon-tasks"></span> Command Keys</a></li>
                        </ul>

                        <!-- tab content -->
                        <div class="tab-content" style="margin-top: 10px;">
                            <div id="tab-general" class="tab-pane active">
                                <form id="frm-general" class="form-horizontal">
                                    <div class="form-group">
                                        <label for="txt-device-name" class="control-label col-xs-2">Name</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-device-name" name="txt-device-name" placeholder="Device name">
                                        </div>
                                    </div>
                                    <div class="form-group">
                                        <label for="txt-timezone" class="control-label col-xs-2">Timezone</label>
                                        <div class="col-xs-10">
                                            <select id="sel-timezone" name="sel-timezone" class="form-control">
                                                <option value="0">Please, select timezone</option>
                                                <?php echo generate_option_timezone_list(); ?>
                                            </select>
                                        </div>
                                    </div> 
                                    <div class="form-group hidden">
                                        <label for="txt-web-password" class="control-label col-xs-2">Password</label>
                                        <div class="col-xs-10">
                                            <input type="password" class="form-control" id="txt-web-password" name="txt-web-password" placeholder="Password">
                                        </div>
                                    </div>
									<div class="form-group">
                                        <div class="col-xs-offset-2 col-xs-10">
                                            <div class="checkbox">
												<label><input type="checkbox" id="cb-limited-services" name="cb-limited-services">Limited services</label>
											</div>
                                        </div>
                                    </div>
                                </form>
                            </div>

                            <div id="tab-network" class="tab-pane">
                                <form id="frm-network" class="form-horizontal">
                                    <div class="form-group">
                                        <label for="txt-public-ip" class="control-label col-xs-2">Public IP</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-public-ip" name="txt-public-ip" placeholder="Public IP" readonly="readonly">
                                        </div>
                                    </div>
                                    <div class="form-group">
                                        <label for="txt-local-ip" class="control-label col-xs-2">Local IP</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-local-ip" name="txt-local-ip" placeholder="Local IP">
                                        </div>
                                    </div>      
                                    <div class="form-group">
                                        <label for="txt-dns" class="control-label col-xs-2">DNS</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-dns" name="txt-dns" placeholder="DNS">
                                        </div>
                                    </div>
                                    <div class="form-group">
                                        <label for="txt-gateway" class="control-label col-xs-2">Gateway</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-gateway" name="txt-gateway" placeholder="Gateway">
                                        </div>
                                    </div>
                                    <div class="form-group">
                                        <label for="txt-web-port" class="control-label col-xs-2">Web Port</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-web-port" name="txt-web-port" placeholder="Web port number">
                                        </div>
                                    </div> 
                                </form>
                            </div>

                            <div id="tab-ftp" class="tab-pane">
                                <form id="frm-ftp" class="form-horizontal">
                                    <div class="form-group">
                                        <label for="txt-ftp-server" class="control-label col-xs-2">Server</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-ftp-server" name="txt-ftp-server" placeholder="Server">
                                        </div>
                                    </div>
                                    <div class="form-group">
                                        <label for="txt-ftp-port" class="control-label col-xs-2">Port</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-ftp-port" name="txt-ftp-port" placeholder="Port" value="21">
                                        </div>
                                    </div>      
                                    <div class="form-group">
                                        <label for="txt-ftp-username" class="control-label col-xs-2">Username</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-ftp-username" name="txt-ftp-username" placeholder="Username">
                                        </div>
                                    </div>
                                    <div class="form-group">
                                        <label for="txt-ftp-password" class="control-label col-xs-2">Password</label>
                                        <div class="col-xs-10">
                                            <input type="password" class="form-control" id="txt-ftp-password" name="txt-ftp-password" placeholder="Password">
                                        </div>
                                    </div>
                                    <div class="form-group">
                                        <label for="txt-ftp-interval" class="control-label col-xs-2">Scan Interval</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-ftp-interval" name="txt-ftp-interval" placeholder="Scanning interval">
                                        </div>
                                    </div> 
                                </form>
                            </div>

                            <div id="tab-dyn-dns" class="tab-pane">
                                <form id="frm-dyn-dns" class="form-horizontal">
                                    <div class="form-group">
                                        <label for="txt-dyndns-username" class="control-label col-xs-2">Username</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-dyndns-username" name="txt-dyndns-username" placeholder="Username" readonly="readonly">
                                        </div>
                                    </div> 
                                    <div class="form-group">
                                        <label for="txt-dyndns-password" class="control-label col-xs-2">Password</label>
                                        <div class="col-xs-10">
                                            <input type="password" class="form-control" id="txt-dyndns-password" name="txt-dyndns-password" placeholder="Password">
                                        </div>
                                    </div>
                                    <div class="form-group">
                                        <label for="txt-dyndns-url" class="control-label col-xs-2">URL</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-dyndns-url" name="txt-dyndns-url" placeholder="URL" readonly="readonly">
                                        </div>
                                    </div>
                                </form>
                            </div>
							<div id="tab-video-stream" class="tab-pane">
                                <form id="frm-video-stream" class="form-horizontal">
                                    <div class="form-group">
                                        <label for="txt-video-stream-url" class="control-label col-xs-2">URL</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-video-stream-url" name="txt-video-stream-url" placeholder="Video streaming URL">
                                        </div>
                                    </div> 
                                    <div class="form-group">
                                        <div class="col-xs-offset-2 col-xs-10">
                                            <div class="checkbox">
												<label><input type="checkbox" id="cb-video-stream-on" name="cb-video-stream-on">Active</label>
											</div>
                                        </div>
                                    </div>
                                </form>
                            </div>
							<div id="tab-xkey" class="tab-pane">
                                <form id="frm-xkey" class="form-horizontal">
                                    <div class="form-group">
                                        <label for="txt-xkey-btn1" class="control-label col-xs-2">Button 1</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-xkey-btn1" name="txt-xkey-btn1" placeholder="Command for button 1">
                                        </div>
                                    </div> 
									<div class="form-group">
                                        <label for="txt-xkey-btn2" class="control-label col-xs-2">Button 2</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-xkey-btn2" name="txt-xkey-btn2" placeholder="Command for button 2">
                                        </div>
                                    </div> 
									<div class="form-group">
                                        <label for="txt-xkey-btn3" class="control-label col-xs-2">Button 3</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-xkey-btn3" name="txt-xkey-btn3" placeholder="Command for button 3">
                                        </div>
                                    </div> 
									<div class="form-group">
                                        <label for="txt-xkey-btn4" class="control-label col-xs-2">Button 4</label>
                                        <div class="col-xs-10">
                                            <input type="text" class="form-control" id="txt-xkey-btn4" name="txt-xkey-btn4" placeholder="Command for button 4">
                                        </div>
                                    </div> 
                                </form>
                            </div>
                        </div>

                    </div>
                    <div class="modal-footer">
                        <span id="sp-config-status" class="label label-success">Status : Ready...</span>
                        <button id="btn-reboot" type="button" class="btn btn-primary disabled">Reboot</button>
                        <button id="btn-save" type="button" class="btn btn-primary disabled">Save</button>
                        <button type="button" class="btn btn-danger" data-dismiss="modal">Close</button>           
                    </div>
                </div>
            </div>            
        </div>

        <!-- script needs -->
        <script src="script/jquery.min.js"></script>
        <script src="script/bootstrap/js/bootstrap.min.js"></script>
        <script src="script/touchspin/jquery.bootstrap-touchspin.min.js"></script>            
        <script src="script/bootbox/bootbox.min.js"></script>
        <script src="script/main.js"></script>
    </body>
</html>