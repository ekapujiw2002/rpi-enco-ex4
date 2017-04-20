<?php
	@exec("hostname", $out);
	$hostname = $out[0];
?>

<!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="utf-8">
        <title>ENCO Systems - enStreamer</title>

        <style>
            * { 
                margin: 0; padding: 0; 
            }

            html { 
                background: url('images/ENCO.png') no-repeat center center fixed; 
                -webkit-background-size: cover;
                -moz-background-size: cover;
                -o-background-size: cover;
                background-size: cover;
            }

            #settings_button {
                position:absolute;
                transition: .5s ease;
                top: 90%;
                left: 10.5%;

                width: 100px;
                height: 44px;
            }
            #settings_button:hover { 
                -webkit-transform: scale(1.18);/*Grows in size like Angry Birds button*/
                -moz-transform: scale(1.18);
                -ms-transform: scale(1.18);
                -o-transform: scale(1.18);
            }      
            #status_button {
                position:absolute;
                transition: .5s ease;
                top: 90%;
                left: 30%;

                width: 100px;
                height: 44px;
            }
            #status_button:hover { 
                -webkit-transform: scale(1.18);/*Grows in size like Angry Birds button*/
                -moz-transform: scale(1.18);
                -ms-transform: scale(1.18);
                -o-transform: scale(1.18);
            }      
        </style>    

    </style>    

</head>
<body>

<center>
    <font face="Arial" color="white" size="5">
    <h1>
        ENCO Systems - <?php echo $hostname; ?>
        <hr width="80%">
    </h1>
</center>


<font face="Arial" color="white" size="6">
<br/> 
<br/>

<table>
    <tr>
        <td colspan="3"><font color="YELLOW">NOW PLAYING</font></td>
    </tr>
    <tr>
        <td width="5%"></td>
        <td>Source:</td>
        <td id="now_play_src">?</td>
    </tr>
    <tr>
        <td></td>
        <td>Title:</td>
        <td id="now_play_file">?</td>
    </tr>
    <tr>
        <td></td>
        <td>Duration:</td>
        <td id="now_play_dur">?</td>
    </tr>

    <tr height="7px"></tr>
    <tr>
        <td colspan="3"><font color="YELLOW">UP NEXT</font></td>
    </tr>
    <tr>
        <td width="5%"></td>
        <td>Source:</td>
        <td id="next_play_src">?</td>
    </tr>
    <tr>
        <td></td>
        <td>Title:</td>
        <td id="next_play_file">?</td>
    </tr>
    <tr>
        <td></td>
        <td>Duration:</td>
        <td id="next_play_dur">?</td>
    </tr>

</table>
</font>

<font face="Arial" size="6">
<a href="settings.html"><input type="button" value="Settings" id="settings_button"></a>
<a href="status.html"><input type="button" value="Status" id="status_button"></a>
</font>

<script type="text/javascript" src="script/jquery.min.js"></script>
<script>
    $(document).ready(
            function () {
                // just clear it first
                clearInterval(idTmrRefresh);

                // create new timer interval
                var isLoading = false;
                var idTmrRefresh = setInterval(
                        function () {
                            if (!isLoading) {
                                $.getJSON(
                                        'api.php',
                                        function (datax) {
                                            $('#now_play_src').html(datax.p.local);
                                            $('#now_play_file').html(datax.p.name);
                                            $('#now_play_dur').html(datax.p.dur);

                                            $('#next_play_src').html(datax.n.local);
                                            $('#next_play_file').html(datax.n.name);
                                            $('#next_play_dur').html(datax.n.dur);
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
    );
</script>

</body>
</html>