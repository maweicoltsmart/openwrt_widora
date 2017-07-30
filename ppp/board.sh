cp /tmp/chat /usr/sbin/
cp /tmp/pppd /usr/sbin/
cp /tmp/pppdump /usr/sbin/
cp /tmp/pppstats /usr/sbin/

cd /etc/ppp
mkdir peers
cd peers
cp /tmp/gprs ./
cd ../
cp /tmp/chat-gprs-connect ./

cp /tmp/chap-secets /etc/ppp/
cp /tmp/gprs-connect-chat /etc/ppp/
cd /sys/class/gpio
echo 17 > export
cd gpio17
echo out > direction''
echo 1 > value
