# Boxdrop

Final assignment of Operating Systems II class (INF01151 @ UFRGS).

## Usage

To build, run:

```
make
```

To execute client, run (with valid parameters):

```
./client/dropboxClient <user> <ip> <server port>
```

To execute server, there are two possibilities:

- To run as primary server:

```
./server/dropboxServer primary
```

- To run as backup server:

```
./server/dropboxServer backup <primary process ip> <port>
```
