#!/bin/sh
#define dial_on function
dial_on()  
{  
	#test if pppd is running
	pppd_stat=`ifconfig|grep ppp|wc -l|cut -b 7-7`  
	if [ $pppd_stat -gt 0 ]  
	then  
		echo "ppp connection's already started."
	else
		#close ethernet interface
		ifconfig eth0 down  
		#ppp start
		pppd modem /dev/ttyS1 115200 nocrtscts lock connect "chat -v -f /etc/ppp/gprs-connect" user "" noauth debug defaultroute  
		# pppd配置说明：
		# ttyS1：连接GPRS模块SIM900的串口
		# 115200：GPRS的拨号速率
		# nocrtscts：无流控
		# lock：锁定设备
		# connect “chat ???? ???? /etc/ppp/gprs-connect”：GPRS连接脚本文件
		# user “”：用户名，这里是无
		# noauth：无需认证
		# debug：输出调试信息
		# defaultroute：此拨号连接作为默认路由
		echo "ppp is starting"
	fi  
}  
#dial on gprs
dial_on  
#wait for ppp's init
sleep 5  
pppd_stat=`ifconfig|grep ppp|wc -l|cut -b 7-7`  
if [ $pppd_stat -eq 0 ]  
then  
	echo "trying 2nd time to call ppp"
	dial_on  
	sleep 5  
fi  
pppd_stat=`ifconfig|grep ppp|wc -l|cut -b 7-7`  
if [ $pppd_stat -eq 0 ]  
then  
	echo "pppd error!"
	echo "please check pppd's config files"
fi  
#open ethernet interface
ifconfig eth0 up  
#end
