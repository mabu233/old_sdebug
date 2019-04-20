# 不在更新, 目前尝试改造xdebug [https://github.com/mabu233/xdebug](https://github.com/mabu233/xdebug)

# sdebug

#### 编译安装
- git clone https://github.com/mabu233/sdebug.git
- phpize
- ./configure
- make && make install

#### 修改php.ini
zend_extension=sdebug.so

#### 使用

- 本地通过phpstorm右键debug脚本

- 同局域网不同机器(适用于windows开虚拟机的)<br>
phpstorm - settings - Languages&Frameworks - PHP 配置远程php 和 path mappings

- 远程服务器<br>
通过SSH隧道端口转发<br>
ssh工具 [OpenSSH](https://www.mls-software.com/files/setupssh-7.9p1-1.exe)<br>
执行命令: `ssh -N -f -R 9000:127.0.0.1:9000 root@193.222.222.222`<br>
其中 9000 对应的是ide监听端口号, 后面的是远程服务器地址<br>
接着phpstorm跟上面第二步一样的配置, 注意配置远程php时,找到`Configuration options` 配置 `-dxdebug.remote_host=127.0.0.1`以覆盖ide默认配置<br>
总的流程就是: phpstorm->调用远程php->php根据`xdebug.remote_host`以及
`xdebug.remote_port`连接到ide(这里连接到远程服务器本地127.0.0.1:9000再转发回本地)
