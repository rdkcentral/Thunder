# Distributed

!!! danger
	Distributed mode is considered experimental

Distributed mode takes the out-of-process mode one step further by allowing plugins to run on an entirely different device than the main Thunder process. This device could even be running a different CPU with a different architecture. A COM-RPC channel is established over a TCP socket between the two devices to allow communication.

An example use case for this could be a dual-SoC platform or for communicating with peripheral devices such as cameras.

Note the COM-RPC protocol is not designed for untrusted channels (e.g. public internet), so this should be used with caution. Enabling tamer-resistant stubs can increase security & robustness but this should still not be considered completely secure.



