require(library /home/comnetsii/elements/routerport.click);
require(library /home/comnetsii/elements/lossyrouterport.click);

define($dev1 veth1, $addrDev1 e2:e5:a8:a4:12:81)
define($dev2 veth2, $addrDev2 4a:26:97:74:af:2c)
define($dev3 veth3, $addrDev3 3e:65:38:79:bf:9b)
define($dev4 veth4, $addrDev4 ea:21:81:06:62:10)
define($dev5 veth5, $addrDev5 26:57:6a:f5:2d:13)
define($dev6 veth6, $addrDev6 92:9c:22:8a:f8:69)

rp :: LossyRouterPort(DEV $dev4, IN_MAC $addrDev4 , OUT_MAC $addrDev3, LOSS 0.9, DELAY 0.2 );

client::BasicClient(MY_ADDRESS 2, OTHER_ADDRESS 1, DELAY 2);
bc::BasicClassifier;
client->rp->bc
bc[0]->[0]client;
bc[1]->Discard;
bc[2]->[1]client;

