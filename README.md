# Mini Kermit transfer protocol

Kermit is a computer file transfer/management protocol. The project is the implementation of the KERMIT protocol, in small format
for file transfer. For example, three files were used data as input to the sender. They were sent both if there was no corruption, loss or delay of the communication line packets, as well as when their transmission was affected.

## KSENDER:

The program uses an auxiliary function for sending again the packet until it is received by the receiver in the state good or until three errors are registered (package corruption, packet loss or packet delay).

The "send_to_receiver" function uses another auxiliary function found in lib.h, "send_package", for sending the package. After the package is
sent, the sender awaits confirmation of the package. In which the package is not confirmed, the number of errors will be increased and the package will be resubmitted.

If the sender receives the packet and it is of type "ACK" it will check if the seq of the received package is the same as in the case of the package sent. If the two differ, it means that the sent packet has not reached receiver and must be sent again with the same seq. If the two are identical then the message was sent and received successfully. If it is received as replied a message of the NACK type from the receiver, the same is sent again message but with the increased seq.

The "send_package" function in lib.h uses two structures for storage data for the mini-kermit, but also for the data of the initial package. After what the data is put in a variable of the type of structures, the data is copied into the message and the message is sent to the receiver.

For sending the initial package I used a special function which sends the packet in mini-kermit format with the data field in the specified format. The field given from the mini-kermit structure is dynamically allocated so that it does not use more space than necessary. The fields are then copied into the message, the crc is calculated and the last two fields of the structure are added in the message.

## KRECEIVER:

The program will process the initial package separately. If I pass 15 seconds after several attempts, the initial packet is not sent,
the transmission will end.

If the initial packet is successfully received, the appropriate field will be retained waiting time for the following packages.
The program will wait for you and send you messages as long as the package marks the end of the transmission is not received or until it is accumulated three errors for one of the packages.

The program is waiting for a package first. If it is not received a response will be sent with a smaller seq. than 1 expected. This thing
will signal to the sender that a connection problem has occurred and the message has not been received. Further the receiver will continue to wait for that message. If instead the message it was received but with a wrong seq, the previous message will be sent. As in the previous case, seq of the sent message will be less than 1 compared to the expected value.

If the message is received, the message crc is calculated and it is compared to the one in the message. If the two are equal then a message is sent ACK type with a sequence equal to that of the received package. This will indicate that sending and the receipt of the package was done correctly. After receiving the package in its form correct, depending on the type of the package, either a file with the formed name will open from "recv_" and the field given in the message, either the entire data field will be written to the file, either the file or transmission will be closed.

In case the message is corrupted, a NACK packet will be sent with the seq equal to that of the received package and will increase the seq used as a package indicator from the program. Thus, the receiver will know that next time to expect a package with more seq high with 1.

In case of errors of any kind, loss of the package, its corruption, or its delay, all these errors will be counted, and if it reaches the threshold of three errors, the transmission will end.
