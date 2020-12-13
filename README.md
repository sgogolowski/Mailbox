# Mailbox
Simple version of the SMTP  protocol. Client connects to server that can create message boxes and store messages inside each box.
Each mailbox stores a queue of messages following a FIFO structure.
Call make client to setup the client program
Call make server to setup the mailbox server
Commands include:
CREAT <message_box_name>: create an message box
OPNBX <message_box_name>: opens the specified message box
PUTMG <message>: puts message into current message box
DELBX <message_box_name>: delete the specified message box
CLOSE <message_box_name>: closes the specified open message box
NXTMG: get the next message in queue in the current message box
GDBYE: quit the program
