# Atari ST Networking and Modem Protocols

## 1. Atari ST Serial Port Architecture

### Serial Port Hardware

```
Port        Address     R/W    Function
─── ───────── ─── ── ──────── ──────────
Com1:         $FF9800   R/W    RS232 port 1 (internal ACIA)
              $FF9802   R/W    RS232 control register
              $FF9804   R/W    RS232 data register
Com2:         $FF9A00   R/W    RS232 port 2 (internal ACIA)
              $FF9A02   R/W    RS232 control register
              $FF9A04   R/W    RS232 data register
MIDI:         $FF9C00   R/W    MIDI port (ACIA 1)
              $FF9C02   R/W    MIDI control register
              $FF9C04   R/W    MIDI data register
Acia2:        $FF9A00   R/W    Modem port (ACIA)
              $FF9A02   R/W    Modem control
              $FF9A04   R/W    Modem data
```

### RS232 Configuration via XBIOS

```
XBIOS function 15 (RS232 configuration):
    D0 = port (0 = COM1, 1 = COM2, 2 = MIDI)
    D1 = baud rate code
    D2 = parity code
    D3 = stop bits code
    D4 = data bits code
    D5 = handshake code

Return: D0 = status (0 = success)

Baud rate codes:
    0 = 75 bps
    1 = 110 bps
    2 = 300 bps
    3 = 1200 bps
    4 = 2400 bps
    5 = 4800 bps
    6 = 9600 bps
    7 = 19200 bps
    8 = 31250 bps (MIDI default)
    9 = 38400 bps (some models)
    10 = 57600 bps (TT/Falcon only)
    11 = 115200 bps (TT/Falcon only)

Parity codes:
    0 = No parity
    1 = Odd
    2 = Even
    3 = Mark
    4 = Space

Stop bits codes:
    0 = 1 stop bit
    1 = 2 stop bits
    2 = 1.5 stop bits (Falcon only)
    3 = 1-2 switchable (TT)

Data bits codes:
    0 = 7 bits
    1 = 8 bits
    2 = 9 bits (TT only)

Handshake codes:
    0 = None
    1 = XON/XOFF
    2 = RTS/CTS
    3 = DTR/DSR
```

## 2. MIDI Networking Protocol

### MIDI Protocol Stack

```
MIDI Physical Layer:
    Port: 5-pin DIN or MIDI jack
    Speed: 31250 bps (7812.5 Hz baud rate, 4x oversampling)
    Voltage: 5V TTL logic
    Max cable length: 15 meters
    Daisy chain topology: max 8 devices

MIDI Message Format:
    Byte 1: Status byte (bit 7 = 1 = status)
    Byte 2: Data byte (bit 7 = 0 = data)
    Byte 3: Data byte (bit 7 = 0 = data)

Status byte breakdown:
    High nibble: Channel (0-15)
    Low nibble: Message type
    High bit: 1 = status byte

Status byte types:
    0x8x = Note Off (channel x)
    0x9x = Note On (channel x)
    0xAx = Polyphonic Pressure (channel x)
    0xBx = Control Change (channel x)
    0xCx = Program Change (channel x)
    0xDx = Channel Pressure (channel x)
    0xEx = Pitch Bend (channel x)
    0xF0 = System Exclusive
    0xF1 = Time Code Quarter Frame
    0xF2 = Song Position Pointer
    0xF3 = Song Select
    0xF4 = Undefined
    0xF5 = Undefined
    0xF6 = Tune Request
    0xF7 = End of Exclusive
```

### MIDI Networking (MIDI-to-Ethernet bridges)

```
MIDI over serial bridge:
    1. Atari ST sends MIDI bytes to $FF9C04 (MIDI ACIA)
    2. MIDI cable connects to MIDI-to-serial converter
    3. Serial data bridges to network via MIDI server software
    4. On PC: MIDI daemon (timidity, aseqnet, or rtp_midi)
    5. Network MIDI packets sent to remote machines

MIDI over TCP/IP (MIDIbox, rtp Midi):
    Protocol: IP port 1397 (MIDI over IP)
    Packet format (MIDIbox):
        Header: 4B (port ID + sequence)
        Payload: MIDI status byte + 0-2 data bytes
        Checksum: 1B CRC
```

### STinG MIDI Networking Stack

STinG (TCP/IP for Atari) provides MIDI networking:

```
STinG MIDI stack layers:
    1. MIDI driver (MIDI.PRG) - raw MIDI I/O
    2. MIDI network daemon - multiplexes channels
    3. Null modem bridge - PC <-> ST connection
    4. PPP protocol - serial point-to-point protocol

Configuration example (ROUTE.TAB):
    ; Atari ST to PC route
    PC_IP     192.168.0.1  MIDI_PGR    ; PC MIDI server
    ST_IP01   192.168.0.2  ROUTE_TAB   ; ST MIDI config
    DEST_IP   192.168.0.1  MIDI_NET    ; route to PC
    GATEWAY   192.168.0.1  ROUTE_TAB   ; default gateway

    ; ST to ST route
    ST_IP01   192.168.0.2  ROUTE_TAB   ; target ST
    DEST_IP   192.168.0.3  MIDI_NET    ; remote ST MIDI
    GATEWAY   192.168.0.1  ROUTE_TAB   ; forward through PC
```

## 3. PSS (Packet Switched Service) Protocol

### PSS Overview

PSS was Atari's packet-switched networking protocol for the Atari ST family:

```
PSS Protocol Stack:
    Physical Layer: Serial/Modem/RPC
    Data Link Layer: Frame-based packet framing
    Network Layer: PSS addressing
    Transport Layer: Connection-oriented byte stream
    Application Layer: PSS API calls (BIOS/XBIOS)

PSS Frame Format:
    Byte 0:   0x10 (start of frame)
    Byte 1:   Frame type (0x01 = data, 0x02 = control)
    Byte 2-3: Length (big-endian)
    Byte 4:   Source address
    Byte 5:   Destination address
    Byte 6-N: Payload data
    Byte N+1: Checksum
    Byte N+2: 0x10 (end of frame)

PSS Addressing:
    Byte 4: Source node ID (0x00 = broadcast)
    Byte 5: Destination node ID (0xFF = broadcast)
    Max 254 nodes per network
```

### PSS API Functions

```
PSS functions available via XBIOS/BIOS extension:
    Function    Opcode    Description
    ────        ────     ────────
    PSSOPEN     $E1       Open PSS connection
    PSSCLOSE    $E2       Close PSS connection
    PSSEND      $E3       Send PSS packet
    PSSRECV     $E4       Receive packet
    PSSSTATUS   $E5       Get connection status
    PSSSHUT     $E6       Shutdown connection
    PSSWAIT     $E7       Wait for data
    PSSSETOPR   $E8       Set operating parameters

PSSOPEN parameters:
    D0 = connection type (0 = direct, 1 = buffered)
    D1 = source address
    D2 = destination address
    D3 = buffer size
    D4 = timeout value (ms)
    D5 = retry count
```

## 4. RPC (Remote Procedure Call) Protocol

### RPC Overview

RPC was an Atari-specific remote execution protocol:

```
RPC Frame Structure:
    Byte 0-3:   Magic (0x90 0x80 0x70 0x60)
    Byte 4:     Protocol version
    Byte 5:     Message type (0 = request, 1 = reply)
    Byte 6-7:   Transaction ID
    Byte 8-11:  Server IP (4 bytes)
    Byte 12-15: Client IP (4 bytes)
    Byte 16-19: Procedure number (4 bytes)
    Byte 20-23: Parameters (variable length)
    Byte 24-27: Error code (if applicable)

RPC Address Format:
    IP address: AAA.BBB.CCC.DDD
    Where each byte represents a node on the network
    Atari ST network addresses: 192.168.0.x range
```

### RPC Programming Example

```asm
; RPC client call (send procedure to remote machine)
rpc_call:
    lea rpc_frame, a0             ; RPC frame buffer
    ; Set frame header
    move.l #$90807060,(a0)        ; Magic
    move.b #1,4(a0)               ; Protocol version = 1
    move.b #0,5(a0)               ; Message = request
    move.w #1,6(a0)               ; Transaction ID
    ; Set server address (192.168.0.10)
    move.l #$C0A8000A,8(a0)
    ; Set client address (192.168.0.1)
    move.l #$C0A80001,12(a0)
    ; Set procedure (XBIOS 34: MKFAT - format disk)
    move.l #34,16(a0)
    ; Send via ACIA2 ($FF9A00)
    move.l a0,d0                  ; frame pointer
    move.b #1,d0                  ; ACIA2 command port
    move.w #12,d1                 ; frame size
    trap #1                       ; XBIOS RPC send
    ; Wait for response
    move.l d0, rpc_xid
    move.w #5000,d1               ; 5 second timeout
rpc_wait:
    dbra d1,rpc_wait
    ; Read response from ACIA2
    move.b $FF9A04,d0             ; Read ACIA2 data
    rts

rpc_frame:
    ds.b 256                      ; RPC frame buffer
rpc_xid:
    dc.w 0
```

## 5. Atari EtherNEC Network Card

### EtherNEC Card Specifications

```
Hardware: EtherNEC (Atari ST Ethernet card)
    Chipset: AMD Am7990 LANCE (Local Area Network Controller)
    Bus: 8-bit Zorro II-compatible (Mega STE)
    Connector: DB-25 AUI (Attachment Unit Interface)
    Media: Thinnet (10BASE2) or coaxial
    Speed: 10 Mbps

Register Map (EtherNEC):
    Address    Register    R/W    Description
    ──── ──────── ────── ──── ─────── ┼ ────
    $FFA000   LANCE base    R/W    Am7990 start address
    $FFA002   AUI control   R/W    AUI interface control
    $FFA004   Status        R       Card status register
    $FFA006   Interrupt     R/W    Interrupt enable/status
    $FFA008   Memory map    R/W    Memory window control
    $FFA00A   DMA control   R/W    DMA transfer control
    $FFA00C   Bus mode      R/W    Bus mode register

    Memory window: 8 KB data buffer
    DMA channel: 3 (shared)
    IRQ: 5 (shared with MFP)
```

### EtherNEC Driver Interface

```
Driver entry points:
    init_ether()      - Initialize EtherNEC card
    send_packet()     - Send Ethernet frame (60-1500 bytes)
    poll_packet()     - Check for received frame
    get_mac()         - Get Ethernet MAC address
    set_promisc()     - Set promiscuous mode
    destroy_ether()   - Shut down driver

Packet format (Ethernet II):
    Destination MAC: 6 bytes (FF:FF:FF:FF:FF:FF for broadcast)
    Source MAC: 6 bytes (EtherNEC MAC)
    Type: 2 bytes (0x0800 = IPv4, 0x0806 = ARP)
    Payload: variable
    CRC: 4 bytes (hardware calculated by LANCE)
```

## 6. PPP (Point-to-Point Protocol) Implementation

### Atari PPP Stack

```
PPP implementation on Atari ST:
    - Serial PPP daemon (PPP.PRG)
    - Serial port configuration
    - Authentication (PAP/CHAP for PPP)
    - IP header compression (VJ header)
    - Multilink PPP (LCP negotiation)

PPP Negotiation Sequence:
    1. LCP negotiation (link control) 2. Authentication (PAP/CHAP)
    3. NCP negotiation (IPCP)
    4. Data transfer
    5. LCP termination

    PPP Frame Format:
    Byte 0-1:  Flag (0x7E)
    Byte 2:     Address (0xFF)
    Byte 3:     Control (0x03)
    Byte 4-5:   Protocol
    Byte 6-7:   Length
    Byte 8+:    Payload
    Byte N+2:   FCS (frame check sequence, CRC-CCITT)
    Byte N+4:   Flag (0x7E)
```

### Atari PPP Configuration

```
PPP configuration on Atari ST:
    Port:           COM1 or COM2
    Baud:           9600-38400 bps (modem dependent)
    Authentication: PAP (Password Authentication Protocol)
    IP range:       192.168.0.x (private)
    Mask:           255.255.255.0
    Gateway:        PC/host IP

    PPP.PRG parameters:
        SERIAL_PORT=0   ; 0=COM1, 1=COM2
        BAUD=9600
        AUTH=PAP
        PAP_USER=atari
        PAP_PASS=secret
        REMOTE_IP=192.168.0.2
        DNS=8.8.8.8
```

## 7. Atari Modem Protocols

### External Modem Support

```
Atari ST modem port specifications:
    Port: COM2 ($FF9A00) on Mega STE, ACIA2 on Falcon
    Baud: 300-38400 bps (standard)
    DTE/DTR: Full RS-232 with handshaking lines
    Voltage: +12V/-12V (RS-232 standard)
    Connector: DB-25 (male) on back panel

Modem AT command set (standard Hayes):
    ATZ       - Reset modem
    ATA       - Answer incoming call
    ATDT1234  - Dial 1234 (tone dialing)
    ATP1234   - Dial 1234 (pulse dialing)
    ATH0      - Hang up
    ATE1      - Enable echo
    AT&F      - Factory reset
    AT&W      - Save settings
    AT+VCID   - Caller ID
    ATI3      - Modem info
```

### Modem Protocol Stack

```
Atari ST BBS modem protocol stack:
    Physical:        RS-232 via COM port
    Link:            X-MODEM / Y-MODEM / Z-MODEM
    Network:         X.25 (some ST models)
    Transport:       TCP/IP (STinG or TCPstack)
    Application:     Telnet, FTP, BBS (PCboard, The Cave, Wildcat!)

X-Modem protocol:
    Block: 128 bytes payload + error check (1B CRC or 2B CRC16)
    Start: SOH (0x01) + block number (1B) + NOT(block number) (1B)
    End:   ACK (0x06) for success, NAK (0x15) for error
    Timeout: 1-3 seconds per block

Y-Modem protocol:
    Block: 1024 bytes payload (larger than X-Modem)
    Start: STX (0x02) + block number (1B) + NOT(block number) (1B)
    CRC:   2B CRC16 at end of block

Z-Modem protocol:
    Frame-based, variable length
    Start: SOH/STX + ZFRAME type + CRC16
    End:   CR (0x0D) + checksum
    Header types: ZRINIT, ZSINIT, ZACK, ZFILE, ZDATA, ZFIN, ZCRCE
```

## 8. STinG TCP/IP Stack

### STinG Overview

```
STinG (ST Internet Gamestack):
    Developer: Gerd Knopper, 1992-1994
    Version: 1.26 (latest)
    Platform: Atari ST/STE/TT/Falcon
    Protocol: TCP/IP suite

    STinG Architecture:
        Physical: Ethernet/Serial/MIDI
        Data Link: SLIP/PPP/X.25
        Network: IP, ICMP, ARP
        Transport: TCP, UDP
        Application: HTTP, FTP, SMTP, Telnet, IRC
```

### STinG Configuration

```
STinG network configuration:
    Network:         192.168.0.0/24
    ST IP:           192.168.0.25
    Gateway:         192.168.0.1
    DNS:             8.8.8.8 or 192.168.0.1 (DHCP)
    Ethernet:        EtherNEC (AMD Am7990) or serial
    Baud rate:       14400 (modem) - 10 Mbps (EtherNEC)

STinG stack layers (from bottom to top):
    1. Hardware driver (ether.drv, serio.drv)
    2. SLIP/PPP driver
    3. IP layer (ip.dll)
    4. TCP/UDP layer (tcp.dll / udp.dll)
    5. DNS resolver (dns.dll)
    6. Application layer (http.exe, ftp.exe, telnet.exe)
```

### STinG API

```
STinG socket programming:
    socket()    - Create socket
    bind()      - Bind to local address
    connect()   - Connect to remote
    listen()    - Listen for connections
    accept()    - Accept incoming
    send()      - Send data
    recv()      - Receive data
    close()     - Close socket

Example: STinG TCP client
    move.w #2,d0      ; socket type (SOCK_STREAM = TCP)
    trap #40          ; syscall (STinG specific)
    ; returns file descriptor in D0

    ; Connect to server
    ; sockaddr: IP addr, port, family
    move.w #8,d0      ; connect() syscall
    trap #40
```

## 9. Atari Network Topologies

### Network Topologies for Atari ST

```
ST family network topologies:
    1. Token Ring (PC -> ST MIDI)   ; PC token ring hub -> MIDI splitter
    2. ST -> ST MIDI chain           ; Daisy-chained MIDI ports
    3. ST -> PC PPP/SLIP             ; Serial modem connection
    4. ST EtherNEC + Ethernet card   ; Native Ethernet
    5. ST -> Internet via modem      ; BBS dial-up connection
    6. ST -> FTP server via STinG   ; TCP/IP network with STinG

    ST -> ST MIDI Network:
        Max nodes: 8 per chain
        Cable: MIDI cable (5-pin DIN)
        Speed: 31250 bps
        Max length: 15m per cable
        Topology: Daisy chain (bus)
        Protocol: MIDI System Exclusive (SysEx)
        Broadcast: All nodes receive all messages

        MIDI SysEx network frame:
            F0 (start)
            0x41 0x10 0x55 0x?? (Atari ST manufacturer ID)
            0x?? (command)
            0x?? (data)
            F7 (end)
```

### STinG MIDI Network Setup

```
STinG MIDI network setup steps:
    1. Install MIDI.PRG on each ST
    2. Configure ROUTE.TAB files
    3. Set up PPP/SLIP on PC host
    4. Connect STs via MIDI chain to PC
    5. Start STinG MIDI daemon
    6. Configure STinG PPP parameters
    7. Test connectivity with PING

    MIDI.PRG configuration file (MIDI.PRC):
        ; MIDI network settings
        MIDI_PORT=0         ; MIDI port 0 (first MIDI)
        MIDI_BAUD=31250     ; MIDI standard baud
        MIDI_ROUTE=1        ; Enable MIDI routing
        MIDI_BROADCAST=1    ; Enable broadcast mode
        MIDI_NET_IP=192.168.0.1 ; MIDI server IP
        MIDI_ST_IP=192.168.0.2     ; ST local IP
```

## 10. PPP Connection with Windows Host

### Windows PPP Server for Atari ST

```
Windows PPP server setup for Atari ST:
    1. Install PPP daemon (pppd) or PPP server software
    2. Configure COM port for modem connection
    3. Set PPP authentication (PAP/CHAP)
    4. Configure IP address pool (192.168.0.x)
    5. Enable NAT/ICS for internet sharing

    PPPD configuration on Windows:
        /DEV/COM1:115200        ; COM1, 115200 baud
        local                         ; act as PPPD server
        ms-dns 8.8.8.8             ; DNS
        proxyarp                       ; proxy ARP
        auth                              ; require authentication
        user atari:pass              ; user/pass
        ipcp-addr 192.168.0.2        ; ST IP
        ipcp-dns 8.8.8.8              ; DNS server
```

### SCSC-Link Networking

```
SCSC-Link: SCSC (SCSI card) Ethernet for Atari ST
    Card: SCSC (SCSI Ethernet card)
    Chip: NE2000-compatible (AMD Am7990)
    Speed: 10 Mbps
    Media: 10BASE2 or 10BASE-T
    Driver: scsc-link.sys (Windows) / scsc-link.dll (Atari)

    SCSC-Link setup:
        1. Install SCSC card in ST
        2. Configure jumper settings (IRQ, I/O address)
        3. Install SCSC-Link driver (SCSC-LINK.PRG)
        4. Configure IP address (192.168.0.x)
        5. Test with ping
```

## 11. Atari BBS Software Networking

### Atari ST BBS Software

```
BBS software for Atari ST:
    PCboard 3.x           ; Most popular BBS, 10+ lines
    The Cave 2.x          ; Multi-line BBS, BBSX support
    Wildcat! BBS     ; DOS-based, ported to Atari
    MARS BBS               ; Multi-line, ST-specific
    STnet                       ; Online service for Atari ST
    NetLink                   ; Network protocol suite
    TOSNet                   ; Atari ST online service
    AtariWorld              ; Atari ST dial-up portal

    Common BBS features:
        File:      Download/upload files
        Msg:       Message base (Echomail/FidoNet)
        Mail:      Private ST-to-ST mail
        Chat:      Live text chat
        Games:       Multiplayer games (Duel, Chess)
        Sysop:     Sysop commands and user management
```

### FidoNet Protocol Stack for Atari ST

```
FidoNet protocol stack:
    Physical:        RS-232 modem (300-9600 bps)
    Link:            FSC (FidoNet Standard Code)
    Network:         FidoNet addressing (2:??/??)
    Transport:       Zmodem/Xmodem
    Application:   Echo mail, File transfer

    FidoNet address format:
        Node:          2:270/700
        Where:
            2 = Net number
            270 = Zone
            700 = Node number:

        Packet format:
            Header: 13 fields (from, to, date, date, subj, flags, etc.)
            Body:    NUL-terminated text
            Trailer: Zpack checksum
```

## 12. Atari Ethernet Networking

### ST Ethernet Adapter Options

```
Atari ST Ethernet adapters:
    1. EtherNEC (by Atari)
        - AMD Am7990 LANCE chip
        - DB-25 AUI connector
        - Thinnet (10BASE2) or TAI (10BASE-T)
        - IRQ 5/7, DMA 3/4

    2. SCSC-Link (by SCSC)
        - AMD Am7990 LANCE chip
        - Zorro II compatible (Mega STE)
        - DB-25 AUI, RJ-45 (TIA)
        - IRQ 5/7, DMA 2/3

    3. SMC Elite 32 (by SMC)
        - NE2000-compatible (AMD Am7990)
        - 16-bit ISA (TT030)
        - 10BASE-T only
        - IRQ 11/12

    4. ARA (Atari Remote Access)
        - Serial modem connection to remote
        - 14400-38400 bps
        - PPP/SLIP protocol stack
```

### EtherNEC Driver Configuration

```
EtherNEC driver setup:
    1. Install EtherNEC card in ST (or Mega STE)
    2. Set jumpers: IRQ (5/7), DMA (3/4)
    3. Configure base address (usually $FFA000)
    4. Write AUI cable to Ethernet hub
    5. Install etherne.drv driver
    6. Configure with STinG or TCPstack
    7. Set IP: 192.168.0.x/24

    etherne.drv parameters:
        IRQ=5
        DMA=3
        BASE=$FFA000
        MEDIA=TPE  ; 10BASE-T (RJ-45)
        MAC=00:A0:C0:00:10:01  ; ST-specific OUI
        SPEED=10M
        DUPLEX=HALF
```

## 13. Real-World Atari Networking Examples

### Atari ST BBS Connection

```
Typical ST-to-ST BBS connection:
    Modem1 (ST1):    ATDT555-1234        ; Dial BBS
    Modem 2 (BBS):   Auto-answer           ; Answer incoming calls
    Protocol:        Zmodem                ; File transfer
    Baud:            14400 bps             ; V.32bis modem
    Lines:           8 user lines
    Max users:       8 simultaneous

    ST BBS software: PCboard 3.x or The Cave
    Upload:          XMODEM-Z                ; Standard
    Download:        YMODEM/1K                ; Large files
    Chat:            PCboard built-in chat    ; Live
    Email:           Internal mail system    ; ST -> ST
    Echomail:        FidoNet protocol        ; Net-wide
```

### Atari ST Internet Connection via Modem

```
Atari ST internet connection sequence:
    1. Initiate modem
    2. Dial ISP (Internet Service Provider)
    3. Authenticate (PAP)
    4. Negotiate IP address (from ISP)
    5. Start STinG TCP stack
    6. Connect via telnet/ftp

    ST modem connection script:
    ; Connect to ISP (e.g., 56k modem)
    ATDT1-800-555-1234   ; Dial ISP
    ; Wait for carrier
    ; Enter PPP
    PPP START
    ; Configure PPP
    PPP IP=192.168.0.25
    PPP DNS=ISP_DNS_IP
    PPP GATEWAY=ISP_GATEWAY
    ; Connect
    PPP CONNECT
    ; Now ST has internet access
    STinG HTTP fetch https://example.com
```

## 14. Networking Troubleshooting

### Common Atari Networking Issues

```
Atari networking troubleshooting guide:
    1. Check ACIA registers:
        - $FF9A02 (control) = 0x00 (reset) or 0x60 (enable)
        - $FF9A04 (data) should match ACIA TX byte
        - $FF9A2B (status) for ACIA error flags

    2. Verify modem status:
        - CTS (Clear To Send) active
        - DSR (Data Set Ready) active
        - Carrier detect (CD) active
        - Ring indicator (RI) active

    3. STinG/PPP issues:
        - Check ROUTE.TAB file for correct entries
        - Verify IP address and subnet mask
        - Ensure no IP conflicts
        - Check if PPP is started (PPP.PRG must be running)

    4. MIDI network issues:
        - All MIDI devices must be in same chain
        - Total cable length <= 15m per segment
        - No device in the chain may be offline
        - Power cycling all MIDI devices may help
```

### Network Diagnostic Commands

```
Atari ST network diagnostics:
    ; Ping remote host (via STinG)
    ping <ip_or_hostname>          ; Send ICMP echo
    arp -a                        ; Show ARP table
    ifconfig                      ; Show interface config
    netstat                       ; Show active connections
    route                         ; Show routing table

    ; ST-specific network info
    move.w #$34,d0                ; XBIOS RS232 config
    trap #1                       ; Check serial port status
    move.w #$32,d0                ; XBIOS RS232 config
    trap #1                       ; Get current config
```

## 15. STinG Configuration File Format

```
STinG configuration (STING.CON):
    ; STinG network config file

    ;; Core settings
    STING_VERSION=1.26
    STING_PORT=0                ; COM1 (0) or COM2 (1)
    STING_BAUD=14400            ; Modem baud rate
    STING_DATABITS=8            ; Data bits
    STING_STOPBITS=1            ; Stop bits
    STING_PARITY=NONE           ; No parity

    ;; Ethernet settings
    ETHERNEC=0                  ; No Ethernet (0) or Yes (1)
    ETHERNEC_IRQ=5              ; EtherNEC IRQ
    ETHERNEC_DMA=3              ; EtherNEC DMA
    ETHERNEC_BASE=$FFA000       ; EtherNEC base address
    ETHERNEC_MEDIA=TPE          ; 10BASE-T
    ETHERNEC_MAC=00:A0:C0:00:10:01

    ;; IP settings
    IP_ADDR=192.168.0.25        ; ST own IP
    IP_MASK=255.255.255.0       ; Subnet mask
    IP_GATEWAY=192.168.0.1      ; Default gateway
    IP_DNS=8.8.8.8              ; DNS server

    ;; PPP settings
    PPP_ENABLED=1               ; PPP on/off
    PPP_USER=atari              ; PPP auth username
    PPP_PASS=secret             ; PPP auth password
    PPP_AUTH=PAP                ; PAP or CHAP
    PPP_SERVER_IP=192.168.0.1   ; Remote PPP server IP
    PPP_LOCAL_IP=192.168.0.25   ; Local PPP IP
    PPP_DNS=8.8.8.8             ; DNS from PPP server

    ;; MIDI network settings
    MIDI_ENABLED=1
    MIDI_PORT=0                 ; MIDI port
    MIDI_BAUD=31250
    MIDI_NET_TYPE=TOS           ; Protocol (TOS or TCP)
    MIDI_NET_SERVER=192.168.0.1 ;; MIDI server IP
    MIDI_ST_IP=192.168.0.2      ; ST MIDI IP
```
