# smthrift
smthrift是为了实现RPC通信中TCP长连接而开发的PHP扩展
* 参考: https://github.com/wfxiang08/thrift/tree/wf/0.10.0

### 简介
PHP很多项目随着业务规模的增长(尤其是终端众多的情况下)逐渐向服务化演变，常见的一种架构模型是将相对独立或者比较耗时的业务抽象为单独的服务(如用户模块)使用c/c++、golang等更高效的语言处理，具体的业务层(如:网页端、移动端)来调用各个服务，这种架构大大降低了各业务之间的耦合度，同时最大限度的提高了模块的重用性。



业务层与后端的服务之间的通信协议中，http并不是一种高效的rpc协议。事实上php中有众多的扩展可以为我们提供很好的范例，如:mysql、memcached等等都是最常见不过的"服务"了，我们完全可以采用它们的客户端处理方式。

mysql、memcached这些扩展都是采用TCP与服务端进行通信，你肯定记得他们都有长连接的连接方式，有兴趣的同学可以去翻一下它们的源码。

如果像mysql、memcached那样将协议的处理也封装在php扩展中，那么意味着每增加一个服务我们都需要安装一个扩展，这样将很不利于维护，同时也会降低开发效率。

smthrift对socket进行了一层简单的封装，将连接放在persistent_list哈希表中，每个fastcgi进程连接后不会被释放，下次请求时直接使用。目前最大的连接数等于fastcgi进程数，当然你也可以自行修改下实现连接池的效果。

使用smthrift可以将协议相关的逻辑也使用php实现，可以大大降低开发成本，smthrift/example/memcache_client.php提供了一个简单的memcache客户端的示例。

### 安装
* 从github下载源码后解压
 * cd smthrift
 	*  For Mac OS
    	* /usr/local/Cellar/php71/7.1.5_17/bin/phpize
		* ./configure --with-php-config=/usr/local/Cellar/php71/7.1.5_17/bin/php-config  --enable-debug

	* For linux:
  	  * /usr/local/php7/bin/phpize
  	  * ./configure --with-php-config=/usr/local/php7/bin/php-config

    * 剩余步骤:
	    * ./configure
	    * make -j 3 && make install
    * 最后将extension=smthrift.so加入php.ini，重启php-fpm或者其他fastcgi
	    * php --info | grep ini 可以找到php.ini的位置

### 使用
```php
$sock = new SmSocket(string $host, string $port, bool $strict_write, bool $strict_read);

//connect
$r = $sock->pconnect([ int $timeoutms ]);  //超时时间,单位:毫秒
if(false === $r){
    exit();
}
//write
$sock->write(string $msg);//返回false时可以调用$sock->pclose()关闭再重连$sock->pconnect()

//read
$msg = $sock->read(int $len);//返回false时可以调用$sock->pclose()关闭再重连$sock->pconnect()


// 配合Thrift RPC服务:
$sock = new SmSocket('127.0.0.1', 5563, true, true);
$sock->pconnect(200);
$client = new GeoIpServiceClient($sock);

```
