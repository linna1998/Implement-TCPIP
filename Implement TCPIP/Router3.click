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

rp1 :: LossyRouterPort(DEV $dev8, IN_MAC $addrDev8 , OUT_MAC $addrDev7, LOSS 1, DELAY 0.2 );
rp2 :: LossyRouterPort(DEV $dev9, IN_MAC $addrDev9 , OUT_MAC $addrDev10, LOSS 1, DELAY 0.2 );

router::BasicRouter(ID 5, PORT_NUMBER 2,  FILENAME router3_log.txt);
rp1->[0]router[0]->rp1;
rp2->[1]router[1]->rp2;