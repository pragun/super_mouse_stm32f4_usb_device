Trim Enabled Ring Buffer Test
---

Initial Buffer Condition
Size: 16, Empty
=> Queued(TestString,10),Empty(6)

Put 4 to Sending
Sending(Test,4), Queued(String,6), Empty(6)

Put 4 to Clear
Empty(4),Queued(String,6),Empty(6)

Write Rock
Empty(4),Queued(StringRock,10),Empty(2)

=> Write Dock,
Queued(Dock,4),Empty(0),Queued(StringRock,10) :Trimmed to 14

=> 10 to Sending
Queued(Dock,4),Empty(0),Sending(StringRock,10)

=> 10 to Clear
Queued(Dock,4), Empty(10),

=> Write Frog
Queued(DockFrog,8),Empty(8) :Untrimmed back to 16

=> 6 to Sending
Sending(DockFr,6),Queued(og,2),Empty(8)

=> 4 to Clear
Empty(4),Sending(Fr,2),Queued(og,2),Empty(8)

