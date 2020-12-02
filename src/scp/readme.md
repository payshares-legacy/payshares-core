# PCP (Payshares Consensus Protocol)

The PCP subsystem is an abstract implementation of PCP, a protocol for federated
byzantine agreement, intended to drive a distributed system built around the
"replicated state machine" formalism. PCP is defined without reference to any
particular interpretation of the concepts of "slot" or "value", nor any
particular network communication system or replicated state machine.

This separation from the rest of the system is intended to make the
implementation of PCP easier to model, compare to the paper describing the
protocol, audit for correctness, and extract for reuse in different programs at
a later date.

The central [PCP class](PCP.h) should be subclassed by any module wishing to
implement consensus using the PCP protocol, implementing the necessary abstract
methods for handling PCP-generated events, and calling PCP base-class methods to
receive incoming messages. The messages making up the protocol are defined in
XDR, in the file [PCPXDR.x](PCPXDR.x)

The `payshares-core` program has a single subclass of PCP called
[Herder](../herder), which gives a specific interpretation to "slot" and
"value", and connects PCP up with a specific broadcast communication medium
([Overlay](../overlay)) and specific replicated state machine
([LedgerManager](../ledger)).

For details of the protocol itself, see the [paper on PCP](https://www.payshares.org/papers/payshares-consensus-protocol.pdf).
