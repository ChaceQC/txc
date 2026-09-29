import time

from websockets.sync.client import connect


def main():
    checksum = 0
    start = time.perf_counter_ns()
    for _ in range(100):
        with connect("ws://127.0.0.1:19791/", compression=None,
                     open_timeout=5, close_timeout=5) as peer:
            peer.send("ok")
            checksum += int(peer.recv(timeout=5) == "ok")
    print("websocket_echo")
    print((time.perf_counter_ns() - start) // 1000)
    print(checksum)


if __name__ == "__main__":
    main()
