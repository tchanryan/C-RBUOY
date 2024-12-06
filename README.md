# C-RBUOY
The rsync utility is a useful and popular tool which efficiently transfers files between computers. In this assignment you will be implementing rbuoy, which is a simplified version of rsync.

The rsync utility is a useful and popular tool which efficiently transfers files between computers. In this assignment you will be implementing rbuoy, which is a simplified version of rsync.

To copy a file from a sending computer to a receiver, it would theoretically be sufficient to just naïvely send over the entire contents (and possibly metadata) of the file.

However, if the receiver already has an older version of the file which is very close to the sender's version (or even an identical copy!), then a large amount of redundant data is being transmitted. With slow networks or large file sizes this can translate to a unnecessary waiting and cost.

Both the real rsync utility and the rbuoy utility that you'll be implementing in this assignment avoid unnecessary data transfer by only sending the chunks of a file which differ between sender and receiver. The rbuoy algorithm takes place over four stages:

Stage 1: the sender constructs a Type A Buoy Index (TABI) file containing a record for each file the sender wants to send. Each record contains metadata about the file, as well as a hash for each block in the file (see the subset 1 description for more information).
Stage 2: the receiver uses the TABI file to construct a Type B Buoy Index (TBBI) file containing a record for each TABI record. The TBBI file contains information about which blocks the receiver already has an up-to-date copy of (see the subset 2 description for more information).
Stage 3: the sender uses the Type B Buoy Index file to construct a Type C Buoy Index (TCBI) file containing a record for each TBBI record. The TCBI file contains the contents of the blocks which the receiver did not have an up-to-date copy of (see the subset 3 description for more information).
Stage 4: the receiver uses the TCBI file to reconstruct an up-to-date copy of the files it is receiving. (see the subset 4 description for more information).
The first four subsets of this assignment correspond to implementing each of these stages for a given list of files. The fifth subset involves adding support for directories.

The real rsync utility is able to transfer files over a network to a remote computer; where the sender would be one computer and the receiver would be a different computer. It can also transfer files locally, where the 'sender' and 'receiver' are two different directories on the same computer. In this assignment, you will only be implementing the local version of rbuoy, where the sender and receiver are two different directories on the same computer.
