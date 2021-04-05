
##Circular buffer data transmission scheme

At some generic point in time:

|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X||X|X||X|
|--- already tx'ed----|--- tx'ing ----|--tx queue --- |---- already tx`ed-----|
 0                     s               q               e                       max

 Loop invariants:
 0: start of array
 s: start of tx'ing, data is being sent out in the ongoing tx request
 q-1: end of data that is being sent out in the ongoing tx request
 q: Start of data waiting to be sent out
 e-1: End of data waiting to be sent out
 e: Start of writable area

 max: size of the array (the last index is max-1)

 array[s:q-1]: data that is tx'ing
 array[0:s-1, e:max]: new data can be written here
 array[q:e-1]: data that is in the buffer waiting to be put on the next sent request

Lets assume that at this point, there is a printf to print "cat".
Its easy, it would look like this
|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
|--- already tx'ed----|--- tx'ing ----|--tx queue --------- | already tx`ed --|
 0                     s               q                     e                 max

Now let us assume that at this point the TX Complete interrupt handler comes and
says, hurray! we've finished sending the previous block of data, so it would look
like
|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
|--- already tx'ed--------------------|--tx queue --------- | already tx`ed --|
 0                     s               q                     e                 max

For just a bit, and then we can dispatch another tx request from `q to `e, and
it would look like
|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
|--- already tx'ed--------------------|--tx'ing ----------- ||already tx`ed --|
 0                                     s                     q,e               max

Note, that the `q and `e indices are exactly the same, because there is no
data waiting in the tx-queue.
So, now, lets assume that at this point there is a printf for "Hello World!!!".
Which is a longer string than we can hold in the tailing part of the buffer

|l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
|tx queue-|--- already tx'ed----------|--tx'ing ----------- |---- tx queue ---|
 0         e                           s                     q                 max

So then how would we send out the next batch of data?
Well, we'd know that when `e < `q, then it must have wrapped itself over
we can send the data from `q:`max safely as it is all increasing indices for the
TX function. So, whenever we receive the next TX Complete Interrupt, we'd fire
off a partial TX like this

|l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
|tx queue-|--- already tx'ed------------------------------- |---- tx'ing -----|
 0,q     e                                                   s                 max
For new string of size < (max - e)

