<?php
$server = new swoole_http_server('0.0.0.0', 9501);
$server->on('request', function ($request, $response) {
    go(function(){
       co::sleep(1);
       go(function(){
           co::sleep(5);
           echo 1;
       });
    });
    $response->header('Content-Type', 'application/json');
    //echo("received {$request->header['content-length']} bytes\n");
    $response->end(json_encode(array('result' => 'OK')));
});
$server->start();