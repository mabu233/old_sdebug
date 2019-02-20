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

- 远程服务器的<br>
暂未支持
