require(library /home/comnetsii/elements/routerport.click);
require(library /home/comnetsii/elements/lossyrouterport.click);

define($dev1 veth1, $addrDev1 f6:41:bd:7d:aa:e7)
define($dev2 veth2, $addrDev2 ae:73:a6:43:b0:be)
define($dev3 veth3, $addrDev3 96:d1:b9:04:f6:47)
define($dev4 veth4, $addrDev4 66:1f:83:7f:8b:8a)
define($dev5 veth5, $addrDev5 92:f0:67:1d:e2:89)
define($dev6 veth6, $addrDev6 be:30:91:e8:25:ca)
define($dev7 veth7, $addrDev7 ae:df:f7:b1:af:be)
define($dev8 veth8, $addrDev8 6a:da:32:fe:36:12)
define($dev9 veth9, $addrDev9 26:4b:50:5b:7f:5e)
define($dev10 veth10, $addrDev10 7e:88:a3:29:9d:96)

rp :: LossyRouterPort(DEV $dev1, IN_MAC $addrDev1 , OUT_MAC $addrDev2, LOSS 1, DELAY 0.2 );

user_client::UserClient(DELAY 2, FILENAME client1_user_log.txt)
tcp_client::TCPClient(MY_ADDRESS 1, OTHER_ADDRESS 4, DELAY 2, FILENAME client1_tcp_log.txt);
ip_client::IPClient(MY_ADDRESS 1, OTHER_ADDRESS 4, FILENAME client1_ip_log.txt);
bc::BasicClassifier;

// tcp_client output 0 : to ip_client
// tcp_client output 1 : to user_client
// tcp_client input 0 : data from ip_client, tcp send ack back, send data to user_client
// tcp_client input 1 : ack from ip_client, tcp send ack
// tcp_client input 2: data from user_client

// ip_client output 0 : to router
// ip_client output 1 : to tcp_client
// ip_client input 0 : from router
// ip_client input 1 : from  tcp_client

user_client[0]->[2]tcp_client[0]->[1]ip_client[0]->rp;
rp->[0]ip_client[1]->bc;

bc[0]->[0]tcp_client[1]->[0]user_client;  // DATA
bc[1]->Discard;  // HELLO
bc[2]->[1]tcp_client;  // ACK

