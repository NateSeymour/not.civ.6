# Networking Protocol

## Game Communication Header

sg_header

| Size | Type | Name | Description |
| 12 Bytes | `String` | Magic Value | "BOATSANDHOES" - Identifies the mesage as using the correct protocol |
| 2 Bytes | `Number` | Message Unique Identifier | Used for responses |
| 1 Byte | `Number` | Message Type | Contains a one-byte number defining the message type (from list below) |
| 2 Bytes | `Number` | Length | Length of the message in bytes, including the header |
| 20 Bytes | `String` | Username | Username of the user |
| 20 Bytes | `String` | User Secret | "Password" of the user to validate their messages |
| ~ | `String` | Payload | String payload of the message |

Total Size: 55 Bytes + Payload length

### Message Types

| Value | Type | Description |
|  
