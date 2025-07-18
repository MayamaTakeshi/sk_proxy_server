# sk_proxy_server
This is a server app that permits to client apps to interact with Dialogic Excel CSP LLC using a JSON API.

## Building

This app is known to run on CentOS 3.9 and CentOS 4.7.

Other distros might work too but you might need to do some adjustments.

You need the have the following folder structure:

```
some_root_folder
  └── brs_stream_utils # https://github.com/MayamaTakeshi/brs_stream_utils
  └── minijson # https://github.com/MayamaTakeshi/minijson
  └── switchkitjson # https://github.com/MayamaTakeshi/switchkitjson
  └── sk_proxy_server # https://github.com/MayamaTakeshi/sk_proxy_server (this folder)
```

Then you need to have switchkit installed (you must have obtained it from Dialogic)
```
  unzip SK831b104_linux.zip # you might have another version. It is OK.
  chmod +x install.sh
  ./install.sh
  # when asked, set installation folder to be at /opt/switchkit/8.3
```

Compile sk_proxy_server
```
  cd sk_proxy_server
  make
```

## Running sk_log_parser

Before running it do some preparations to have SK logs being generated on a determined folder and removed regularly:

Create log folder:
```
  mkdir /var/log/switchkit
```

Set SK_LOG_DIR in /etc/profile:
```
  export SK_LOG_DIR=/var/log/switchkit
```

Source /etc/profile for the above to take effect:
```
  . /etc/profile
```

Create cron job to remove old log files:
```
  # add a file named del_old_sk_log_files to /etc/cron.daily with:

  #!/bin/bash
  # delete all files older than 3 days
  find /var/log/switchkit -name "*.log" -mtime +3 -exec rm {} \;

  # enable execution of that script: 
  chmod +x /etc/cron.daily/del_old_sk_log_files
```
 
Run sk_proxy_server:
```
  cd sk_proxy_server
  ./sk_proxy_server
```

Now to test connection and interaction with it you can use telnet.

Basically, after connecting to it, you should send this command:
```
skj_initialize APP_NAME APP_VERSION APP_DESCRIPTION INSTANCE_ID LLC_HOST LLC_PORT
```

Example:
```
~$ telnet 192.168.2.101 1313
Trying 192.168.2.101...
Connected to 192.168.2.101.
Escape character is '^]'.
skj_initialize tester 1.0.0 tester 12345 192.168.33.3 1312
{"_event_": "handshake_ok"}
{"_event_": "skj_initialize_ok"}
{"_sk_func_": "sendMsg", "context": 1, "tag": 0, "SpanA": 11, "ChannelA": 1, "SpanB": 27, "ChannelB": 2}   
{"_event_": "sk_func_res", "context": 1, "Success": true}
{"_event_": "sk_msg_ack", "tag": 10000, "context": 1, "status": 7427, "text": "In Service Idle"}
```

## nodejs client

You can use [sk_proxy_client](https://github.com/MayamaTakeshi/sk_proxy_client) to access the proxy from nodejs apps
