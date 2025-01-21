# WebSocket Communication

The WebSocket protocol is an independent TCP protocol on top of exisiting infrastructure. It uses a regular or tunneled (TLS) TCP connection after a successful upgrade has been estasblished. The upgrade has in common with the HTTP protocol that its handshake is interpreted by HTTP servers. Hence, the default TCP ports are 80 and 443 (TLS tunneled). 

## Basic operation

``` mermaid
---
title: Basic operation
---
sequenceDiagram
    autonumber

    participant Client
    participant Server

    Note over Client,Server: No connection

    loop
        critical Establish connection
            alt
                Client->>Server: Connect to port (default 80)
            else
                Client->>Server: Connect to port (default 443)

                Note over Client,Server: TLS handshake
            end
        end

        Note over Client,Server: (TLS enabled) TCP established
        Note over Client,Server: HTTP upgrade request

        opt
            par
                alt
                    Note over Client,Server: Additional client authentication
                else
                    Note over Client,Server: Redirection
                end
            end
        end

        Note over Client,Server: HTTP response
    end

    Note over Client,Server: Exchange WebSocket messages

    loop
        Note over Client,Server: Close TCP
    end
```

## Detailed operation single connection

``` mermaid
---
title: Detailed operation single connection
---
sequenceDiagram
    autonumber

    participant Client
    participant Server

    critical Establish connection
        Client->>Server: Set up TCP
    end

    Note over Client,Server: Use TLS connection if selected

    critical Opening handshake
        Client->>Server: HTTP upgrade request
        Server->>Server: Validate request
        Server->>Client: HTTP response
        Client->>Client: Validate response
    end

    opt Exchange WebSocket message
        loop Frames of message
            alt
                Client->>Client: Mask frame
                Client->>Server: Frame
            else
                Server->>Client: Frame
            else Interleave control frame not being close or ping frame
                alt
                    Client-->Client: Mask control frame
                    Client->>Server: Unfragmented control frame
                else
                    Server->>Client: Unfragmented control frame
                end
            else Interleave control ping frame
                alt 
                    Client->>Client: Mask ping frame
                    Client->>Server: Unfragmented ping frame
                    Server->>Client: Unfragmented pong frame
                else
                    Server->>Client: Unfragmented ping frame
                    Client->>Client: Mask pong frame
                    Client->>Server: Unfragmented pong frame
                end
            else Interleave unsollicited control pong frame
                alt
                    Client->>Client: Mask pong frame
                    Client->>Server: Unfragmented pong frame
                else
                    Server->>Client: Unfragmented pong frame
                end
            end
        end
    end

    critical Close connection
    par
        alt
            Client->>Server: Close frame
            Server->>Client: Close frame
            Client->>Client: Close TCP
        else
            Server->>Client: Close frame
            Client->>Server: Close frame
            Server->>Server: Close TCP
        end
    end
    end
```

## Proxied operation

``` mermaid
---
title: Proxied operation
---
sequenceDiagram
    autonumber

    participant Client
    participant Proxy
    participant Server

    Note over Client,Proxy: No connection
    Note over Proxy,Server: No connection

    Client ->> Proxy: HTTP CONNECT request

    Note over Proxy,Server: Establish single connection
    Note over Proxy,Server: Use TLS connection if selected
    Note over Proxy,Server: Opening handshake

    Proxy->>Client: HTTP 2XX response

    opt Exchange WebSocket message
        alt
            Client ->> Proxy: Frame
            Proxy ->> Server: Forward frame 
        else
            Server ->> Proxy: Frame
            Proxy ->> Client: Forward frame 
        end
    end

    critical Close connection
        alt
            Client ->> Proxy: Close Frame
            Proxy ->> Server: Forward Close frame 
            Server ->> Proxy: Close Frame
            Proxy ->> Client: Forward Close frame 
            Client ->>Client: Close TCP
        else
            Server ->> Proxy: Close Frame
            Proxy ->> Client: Forward Close frame 
            Client ->> Proxy: Close Frame
            Proxy ->> Server: Forward Close frame 
            Server ->> Server: Close TCP
        end
    end
```

## HTTP Upgrade request

``` mermaid
---
title: HTTP Upgrade request, client side
---
sequenceDiagram
    autonumber

    participant Client
    participant Server

    critical Assemble HTTP GET
        Note over Client,Server: At least version 1.1

        loop All HTTP header fields
            alt
                Client->>Client: Request-URI field
            else
                Client->>Client: HOST field
            else
                Client->>Client: Upgrade field
            else
                Client->>Client: Connection field
            else
                Client->>Client: Sec-WebSocket-Key field
            else
                Client->>Client: Sec-WebSocket-Version field
            else
                opt Browsers only
                    Client->>Client: Origin field
                end
            else
                opt
                    Client->>Client: Sec-WebSocket-Protocol field
                end
            else
                opt
                    Client->>Client: Sec-WebSocket-Extentions field
                end
            else
                opt
                    Client->>Client: Not-listed HTTP header field
                end
            end
        end
    end

    Note over Client,Server: Use TLS connection if selected

    Client->>Server: HTTP request
```

- URIs do not contain unescaped character '#'; the may contain %23
- URIs have the format
    ws(s) // host [: port] path [? query]
    - host represents a hostname or IP address
    - [] represent optional elements
    - path is / if empty
    - default port 80, 443 for ws, wss respectively 
- Fragment identifier, unencoded '#', are not allowed, escaped character %23 is
- Sec-Websocket-Key appears only once, and, random 16-bit nonce base64 encoded



``` mermaid
---
title: HTTP Upgrade request, server side
---
sequenceDiagram
    autonumber

    participant Client
    participant Server

    Note over Client,Server: Use TLS connection if selected

    Client->>Server: HTTP request

    critical Validate HTTP GET
        Note over Client,Server: At least version 1.1
        
        loop All HTTP header fields
            alt
                Server->>Server: Request-URI field
            else
                Server->>Server: HOST field
            else
                Server->>Server: Upgrade field
            else
                Server->>Server: Connection field
            else
                Server->>Server: Sec-WebSocket-Key field
            else
                Server->>Server: Sec-WebSocket-Version field
            else
                opt Browsers only
                    Server->>Server: Origin field
                end
            else
                opt
                    Server->>Server: Sec-WebSocket-Protocol field
                end
            else
                opt
                    Server->>Server: Sec-WebSocket-Extentions field
                end
            else
                opt
                    Server->>Server: Not-listed HTTP header field
                end
            end
        end
    end

    Note over Client,Server: Use TLS connection if selected

    Server->>Client: HTTP response
```

``` mermaid
---
title: HTTP Upgrade response, server side
---
sequenceDiagram
    autonumber

    participant Client
    participant Server

    critical Construct HTTP response
        loop All header fields 
            alt
                Server->>Server: Status line
            else
                Server->>Server: Upgrade field
            else
                Server->>Server: Connection field
            else
                Server->>Server: Sec-WebScoket-Accept field
            else
                opt 
                    Server->>Server: Sec-WebSocket-Extensions field
                end
            else
                opt
                    Server->>Server: Sec-WebSocket-Protocol field
                end
            end
        end
    end

    Note over Client,Server: Use TLS connection if selected

    Server->>Client: HTTP response
```

- Sec-WebSocket-Accept 4base64 encoded SHA-1 of Sec-WebSocket-Key concatenated with 258EAFA5-E914-47DA-95CA-C5AB0DC85B11 padded to 20 bytes
    - Padded with = for 3base64
    - Padded with == for 2base64
- Sec-WebSocket-Key appears only once in the request
- Sec-WebSocket-Version, fixed value 13, appears only once in the request

## HTTP Upgrade response

``` mermaid
---
title: HTTP Upgrade response, client side
---
sequenceDiagram
    autonumber

    participant Client
    participant Server

    Note over Client,Server: Use TLS connection if selected

    Server->>Client: HTTP response

    critical Validate HTTP response
        loop All header fields 
            alt
                Client->>Client: Status code
            else
                Client->>Client: Upgrade field
            else
                Client->>Client: Connection field
            else
                Client->>Client: Sec-WebScoket-Accept field
            else
                opt 
                    Client->>Client: Sec-WebSocket-Extensions field
                end
            else
                opt
                    Client->>Client: Sec-WebSocket-Protocol field
                end
            end
        end
    end
```

- Sec-WebSocket-Extension appears only once in the response
- Sec-WebSocket-Accept appears only once in the response
- Sec-WebSocket-Protocol appears only once in the response

## Message (de)fragmentation and encoding

``` mermaid
---
title: Message (de)fragmentation
---
flowchart TB

    A["Input frame"] --> B{"Frame N==0 of message"}

    B -- yes --> C{"FIN set"}
    B -- no --> D{"FIN set"}

    C -- yes --> E{"Opcode == 0"}
    C -- no --> F{"Opcode in 0x1,0x2"}

    D -- yes --> G{"Opcode == 0"}
    D -- no --> H{"Opcode == 0"}

    E -- yes --> I["Error"]
    E -- no --> J{"Opcode in 0x1,0x2"}

    F -- yes --> K["Error"]
    F -- no --> L["Initial frame fragmented message"]

    G -- no --> N{"Opcode in 0x8,0x9,0xA"}
    G -- yes --> M["Termination frame fragmented message"] 

    H -- yes --> O["Continuation frame fragmented message"]
    H -- no --> P["Error"]

    J -- yes --> S["Unfragmented message"]
    J -- no --> N

    N -- yes --> U["(Interleaved) control frame"]
    N -- no --> V["Error"]
```

``` mermaid
---
title: Message encoding
---
flowchart TB

    A["Frame"] --> B{"Client to Server"} 

    B -- yes --> C["Mask = 1, Extra 32-bit Masking-Key field"]
    B -- no --> D["Mask = 0"]

    C --> E{"Control message"}
    D --> E

    E -- yes --> F["SRV = 0, Opcode in 0x8,09,0xA, Payload <= 125"]
    E -- no --> G{"Extension"}

    F --> H["Optional (masked) payload data"]

    H --> I["FIN = 1"]

    G -- yes --> J["SRV != 0 "] 
    G -- no --> K["SRV = 0"]

    J --> L{"Extended Payload"}
    K --> L

    L -- Payload = 0-125 --> M["No extra length field"]
    L -- Payload = 126 --> N["Extra 2 bytes length field "]
    L -- Payload = 127 --> O["Extra 8 bytes length field"]

    M --> P["(Masked) payload data = extension data + application data"]
    N --> P
    O --> P

    P --> Q{"Multiframe"}

    Q -- Initial frame --> R["FIN = 1, Opcode in 0x1,0x2"]
    Q -- Continuation frame --> S["FIN = 0, Opcode = 0x0"]
    Q -- Termination frame --> T["FIN = 1 , Opcode in 0x0"]
```

- Applied after regular payload data formatting
- Unique 32-bit masking key per frame
- Masked Payload data = Key (payload)
    - i is index in payload data
    - output octet j = (key octet i mod 4) XOR input octet i 
- Opcodes 0x0, 0x1, 0x2, 0x8, 0x9, 0xA are defined
- Opcodes 0x3-0x7, 0xB-0xF are reserved
- Text frame (0x1): payload data is encoded in UTF-8
- Binary frame: payload data encoded by agreement between applications
- Control Close (0x8) frame may contain optional status data
    - 2 bytes status code (networking order)
        - 1000-2999: used by this protocol
            - 1004, 1005, 1006, 1015 cannot be used by Close control frame
        - 4000-4999: private use and agreed between applications
        - 1003 applies to opcode 0x2 only
    - Followed by optional appended UTF-8 encoded value/reason
- Control messages may contain payload 
    - A pong frame contains the application data of the ping frame
- Frame boundaries do not exist unless extension defines it
- Extensions are registered
    - Opcodes 0x3-0x7 and 0xB-0xF can be used with extensions
- Suprotocols are registered

## (Data exchange) error handling

``` mermaid
---
title: (Data exchange) error handling
---

flowchart TB

    A["Prior WebSocket handshake"] --> B{"Error"}
    
    B -- yes --> C["Endpoint (may) drop(s) TCP"]
    B -- no --> D["During WebSocket handshake"]

    D --> E{"Error"}

    E -- yes --> F{"Server"}
    E -- no --> G["After successfull WebSocket handhake"]

    F -- no --> C
    F -- yes --> H["Return HTTP status code"]

    G --> I{"Error"}

    H --> C

    I -- yes --> J["Sent control Close frame"]
    I -- no --> G

    J --> K["Receive control Close frame"]

    K --> C
```

- An endpoint may try to recover from abnormal TCP closure
    - Increasing timeout for subsequent attempts

## Base64 encoding

- Input consists of octets (8 bits). 
- Convert input in 24 bits segments
- 4base64
    - 4 groups of 6 bits of input
    - Each group encoded into a character
- 3base64
    - Remainder consist of 8 bits
    - Append 12 0-bits as padding to obtain 24 bits
    - The appended 12 0-bits are represented by 2 characters =
- 2base64
    - Remainder consist of 16 bits
    - Append 8 0-bits as padding to obtain 24 bits
    - The appended 12 0-bits are represented by 1 character =
