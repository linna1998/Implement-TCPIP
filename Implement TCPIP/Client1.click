require(library /home/comnetsii/elements/routerport.click);
require(library /home/comnetsii/elements/lossyrouterport.click);

define($dev1 veth1, $addrDev1 5e:04:5a:bc:e3:d0)
define($dev2 veth2, $addrDev2 2a:d1:54:47:91:62)
define($dev3 veth3, $addrDev3 ea:ee:ca:61:4f:33)
define($dev4 veth4, $addrDev4 aa:6e:09:64:27:90)
define($dev5 veth5, $addrDev5 b6:a3:3c:01:42:a9)
define($dev6 veth6, $addrDev6 22:75:ce:f2:f6:0f)
define($dev7 veth7, $addrDev7 2e:f2:8f:9c:4a:c4)
define($dev8 veth8, $addrDev8 a6:23:4c:4e:69:aa)
define($dev9 veth9, $addrDev9 ae:92:25:ce:5d:22)
define($dev10 veth10, $addrDev10 d6:e4:ec:50:09:31)

rp :: LossyRouterPort(DEV $dev1, IN_MAC $addrDev1 , OUT_MAC $addrDev2, LOSS 1, DELAY 0.2 );

client::BasicClient(MY_ADDRESS 1, OTHER_ADDRESS 4, DELAY 2);
bc::BasicClassifier;
client->rp->bc
bc[0]->[0]client;
bc[1]->Discard;
bc[2]->[1]client;

